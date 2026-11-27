// Copyright (c) Wojciech Figat. All rights reserved.

#if USE_PROFILER
using System;
using System.Collections.Generic;
using FlaxEditor.GUI;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEngine
{
    partial class ProfilerCPU
    {
        partial struct Event
        {
            /// <summary>
            /// Gets the event name.
            /// </summary>
            public unsafe string Name
            {
                get
                {
                    fixed (short* name = Name0)
                        return new string((char*)name);
                }
            }

            internal unsafe ulong NameHash()
            {
                ulong hash = 14695981039346656037;
                fixed (short* name = Name0)
                    for (int i = 0; i < 100 && name[i] != 0; i++)
                        hash = unchecked((hash ^ (ushort)name[i]) * 1099511628211);
                return hash;
            }

            internal unsafe bool NameEquals(string value)
            {
                if (value.Length >= 100)
                    return false;
                fixed (short* name = Name0)
                {
                    for (int i = 0; i < value.Length; i++)
                        if ((char)name[i] != value[i])
                            return false;
                    return name[value.Length] == 0;
                }
            }

            internal unsafe bool NameStartsWith(string prefix)
            {
                fixed (short* name = Name0)
                {
                    fixed (char* p = prefix)
                        return Utils.MemoryCompare(new IntPtr(name), new IntPtr(p), (ulong)(prefix.Length * 2)) == 0;
                }
            }
        }
    }
}

namespace FlaxEditor.Windows.Profiler
{
    /// <summary>
    /// The CPU performance profiling mode.
    /// </summary>
    /// <seealso cref="FlaxEditor.Windows.Profiler.ProfilerMode" />
    internal sealed unsafe class CPU : ProfilerMode
    {
        private readonly SingleChart _mainChart;
        private readonly Timeline _timeline;
        private readonly Table _table;
        private SamplesBuffer<ProfilingTools.ThreadStats[]> _events;
        private sealed class CPULabel : Timeline.TrackLabel { public ulong LastUsedCapture; }
        private sealed class CPURow : Row { public ulong LastUsedCapture, ScopeKey; }
        private List<CPULabel> _timelineLabels;
        private sealed class CPUEvent : Timeline.Event
        {
            public ulong LastUsedCapture;
        }

        private List<CPUEvent> _timelineEvents;
        private int _timelineEventCount, _timelineLabelCount, _tableRowCount;
        private readonly Dictionary<ulong, (string Raw, string Display)> _eventNames = new Dictionary<ulong, (string, string)>();
        private List<CPURow> _tableRows;
        private ulong _captureNumber;
        private bool _keepUnused;
        private readonly Dictionary<(ulong ID, string Name), int> _threadDepths = new Dictionary<(ulong, string), int>();
        private sealed class Aggregate
        {
            public string Name;
            public ulong ScopeKey;
            public int Parent, Depth, Calls;
            public double Total, Self;
            public long Memory;
            public readonly List<int> Children = new List<int>();
        }
        private readonly Dictionary<(int Parent, string Name), int> _aggregateIndices = new Dictionary<(int, string), int>();
        private readonly List<Aggregate> _aggregates = new List<Aggregate>();
        private readonly List<int> _aggregateRoots = new List<int>(), _aggregateStack = new List<int>();
        private int _aggregateCount;
        private readonly Dictionary<ulong, bool> _expandedScopes = new Dictionary<ulong, bool>();
        private bool _showOnlyLastUpdateEvents;

        public CPU()
        : base("CPU")
        {
            // Layout
            var mainPanel = new Panel(ScrollBars.None)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                Parent = this,
            };
            
            // Chart
            _mainChart = new SingleChart
            {
                Title = "Frame interval",
                TooltipText = "Elapsed wall-clock time between completed frames, including idle and present waits.",
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                Offsets = Margin.Zero,
                Height = SingleChart.DefaultHeight,
                FormatSample = v => (Mathf.RoundToInt(v * 10.0f) / 10.0f) + " ms",
                Parent = mainPanel,
            };
            _mainChart.SelectedSampleChanged += OnSelectedSampleChanged;
            
            var panel = new Panel(ScrollBars.Vertical)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, _mainChart.Height + 2, 0),
                Parent = mainPanel,
            };
            //panel.Y = _mainChart.Height + 2;
            var layout = new VerticalPanel
            {
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                Offsets = Margin.Zero,
                Pivot = Float2.Zero,
                IsScrollable = true,
                Parent = panel,
            };
            
            // Timeline
            _timeline = new Timeline
            {
                Height = 340,
                Parent = layout,
            };

            _timeline.ScaleChanged += () => UpdateView(_mainChart.SelectedSampleIndex, _showOnlyLastUpdateEvents);
            _timeline.LabelWidth = 90;

            // Table
            var style = Style.Current;
            var headerColor = style.LightBackground;
            var textColor = style.Foreground;
            _table = new Table
            {
                RowColorEven = Color.Transparent,
                RowColorOdd = style.Background * 1.4f,
                Columns = new[]
                {
                    new ColumnDefinition
                    {
                        UseExpandCollapseMode = true,
                        CellAlignment = TextAlignment.Near,
                        Title = "Event",
                        TitleBackgroundColor = headerColor,
                        TitleColor = textColor,
                    },
                    new ColumnDefinition
                    {
                        Title = "Total",
                        TitleBackgroundColor = headerColor,
                        FormatValue = FormatCellPercentage,
                        TitleColor = textColor,
                    },
                    new ColumnDefinition
                    {
                        Title = "Self",
                        TitleBackgroundColor = headerColor,
                        FormatValue = FormatCellPercentage,
                        TitleColor = textColor,
                    },
                    new ColumnDefinition
                    {
                        Title = "Time ms",
                        TitleBackgroundColor = headerColor,
                        FormatValue = FormatCellMs,
                        TitleColor = textColor,
                    },
                    new ColumnDefinition
                    {
                        Title = "Self ms",
                        TitleBackgroundColor = headerColor,
                        FormatValue = FormatCellMs,
                        TitleColor = textColor,
                    },
                    new ColumnDefinition
                    {
                        Title = "Memory",
                        TitleBackgroundColor = headerColor,
                        FormatValue = FormatCellBytes,
                        TitleColor = textColor,
                    },
                    new ColumnDefinition
                    {
                        Title = "Calls",
                        TitleBackgroundColor = headerColor,
                        TitleColor = textColor,
                    },
                },
                Parent = layout,
            };
            _table.Splits = new[]
            {
                0.4f,
                0.1f,
                0.1f,
                0.1f,
                0.1f,
                0.1f,
                0.1f,
            };
        }

        private string FormatCellPercentage(object x)
        {
            return ((float)x).ToString("0.0") + '%';
        }

        private string FormatCellMs(object x)
        {
            return ((float)x).ToString("0.00");
        }

        private string FormatCellBytes(object x)
        {
            return Utilities.Utils.FormatBytesCount(Convert.ToUInt64(x));
        }

        /// <inheritdoc />
        public override void Clear()
        {
            _mainChart.Clear();
            _events?.Clear();
            _threadDepths.Clear();
            _expandedScopes.Clear();
        }

        /// <inheritdoc />
        public override void Update(ref SharedUpdateData sharedData)
        {
            _captureNumber++;
            _mainChart.AddSample(sharedData.Stats.FrameTimeMs);

            // Gather CPU events
            var events = sharedData.GetEventsCPU();
            if (_events == null)
                _events = new SamplesBuffer<ProfilingTools.ThreadStats[]>();
            _events.Add(events);
        }

        /// <inheritdoc />
        public override void UpdateView(int selectedFrame, bool showOnlyLastUpdateEvents)
        {
            _showOnlyLastUpdateEvents = showOnlyLastUpdateEvents;
            _keepUnused = selectedFrame == -1 && !showOnlyLastUpdateEvents;
            _mainChart.SelectedSampleIndex = selectedFrame;

            if (_events == null)
                return;
            if (_timelineLabels == null)
                _timelineLabels = new List<CPULabel>();
            if (_timelineEvents == null)
                _timelineEvents = new List<CPUEvent>();
            if (_tableRows == null)
                _tableRows = new List<CPURow>();

            var viewRange = GetViewRange(_showOnlyLastUpdateEvents);
            UpdateTimeline(ref viewRange);
            UpdateTable(ref viewRange);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _timelineLabels?.Clear();
            _timelineEvents?.Clear();
            _tableRows?.Clear();
            _eventNames.Clear();

            base.OnDestroy();
        }

        private string GetEventName(ref ProfilerCPU.Event e) => GetEventNameData(ref e).Display;

        private (string Raw, string Display) GetEventNameData(ref ProfilerCPU.Event e)
        {
            ulong hash = e.NameHash();
            if (_eventNames.TryGetValue(hash, out var cached) && e.NameEquals(cached.Raw))
                return cached;
            string raw = e.Name;
            string display = raw.Replace("::", ".");
            if (_eventNames.Count >= 8192)
                _eventNames.Clear();
            _eventNames[hash] = (raw, display);
            return (raw, display);
        }

        private static void SetFloat(Row row, int index, float value)
        {
            if (!(row.Values[index] is float previous) || previous != value)
                row.Values[index] = value;
        }

        private struct ViewRange
        {
            public double Start;
            public double End;

            public ViewRange(ref ProfilerCPU.Event e)
            {
                Start = e.Start;
                End = e.End;
            }

            public bool SkipEvent(ref ProfilerCPU.Event e)
            {
                return e.End <= e.Start || e.End <= Start || e.Start >= End;
            }
        }

        private ViewRange GetViewRange(bool updateOnly)
        {
            var threads = _events?.Get(_mainChart.SelectedSampleIndex);
            if (threads == null)
                return new ViewRange();
            var range = new ViewRange { Start = double.MaxValue, End = double.MinValue };
            foreach (var thread in threads)
            {
                if (thread.Name != "Main" || thread.Events == null)
                    continue;
                // The last Update is the one selected when several updates preceded this draw.
                for (int i = thread.Events.Length - 1; i >= 0; i--)
                {
                    ref var e = ref thread.Events[i];
                    if (e.Depth != 0 || e.End <= e.Start)
                        continue;
                    if (updateOnly)
                    {
                        if (e.NameEquals("Update"))
                            return new ViewRange(ref e);
                    }
                    else
                    {
                        range.Start = Math.Min(range.Start, e.Start);
                        range.End = Math.Max(range.End, e.End);
                    }
                }
            }
            if (!updateOnly && range.End > range.Start)
                return range;
            if (updateOnly)
                return new ViewRange();
            // Support captures made without a main-thread event (for example a worker-only capture).
            foreach (var thread in threads)
                if (thread.Events != null)
                    foreach (var e in thread.Events)
                        if (e.Depth == 0 && e.End > e.Start)
                        {
                            range.Start = Math.Min(range.Start, e.Start);
                            range.End = Math.Max(range.End, e.End);
                        }
            return range.End > range.Start ? range : new ViewRange();
        }

        private void AddEvent(ViewRange range, int maxDepth, float xOffset, int depthOffset, int index, ProfilerCPU.Event[] events, ContainerControl parent)
        {
            ref ProfilerCPU.Event e = ref events[index];

            double start = Math.Max(e.Start, range.Start);
            double length = Math.Min(e.End, range.End) - start;
            if (length <= 0.0)
                return;
            double scale = _timeline.PixelsPerMillisecond;
            float x = (float)((start - range.Start) * scale);
            float width = (float)(length * scale);
            CPUEvent control;
            if (_timelineEventCount < _timelineEvents.Count)
                control = _timelineEvents[_timelineEventCount];
            else
            {
                control = new CPUEvent { Parent = parent };
                _timelineEvents.Add(control);
            }
            _timelineEventCount++;
            control.Bounds = new Rectangle(x + xOffset, (e.Depth + depthOffset) * Timeline.Event.DefaultHeight, width, Timeline.Event.DefaultHeight - 1);
            string name = GetEventName(ref e);
            if (control.Name != name) control.Name = name;
            control.DurationMilliseconds = length;
            control.LastUsedCapture = _captureNumber;
            control.Enabled = control.Visible = true;

            // Spawn sub events
            int childrenDepth = e.Depth + 1;
            if (childrenDepth <= maxDepth)
            {
                while (++index < events.Length)
                {
                    int subDepth = events[index].Depth;
                    if (subDepth <= e.Depth)
                        break;
                    if (subDepth == childrenDepth)
                    {
                        AddEvent(range, maxDepth, xOffset, depthOffset, index, events, parent);
                    }
                }
            }
        }

        private void UpdateTimeline(ref ViewRange viewRange)
        {
            var container = _timeline.EventsContainer;

            container.IsLayoutLocked = true;
            _timelineEventCount = _timelineLabelCount = 0;

            float height = UpdateTimelineInner(ref viewRange);
            for (int i = _timelineEventCount; i < _timelineEvents.Count; i++)
            {
                var item = _timelineEvents[i];
                RetainUnused(item, _keepUnused && _captureNumber - item.LastUsedCapture <= 2);
                if (item.Visible) height = Mathf.Max(height, item.Bottom);
            }
            for (int i = _timelineLabelCount; i < _timelineLabels.Count; i++)
            {
                var item = _timelineLabels[i];
                RetainUnused(item, _keepUnused && _captureNumber - item.LastUsedCapture <= 2);
                if (item.Visible) height = Mathf.Max(height, item.Bottom);
            }
            _timeline.Height = height;

            container.UnlockChildrenRecursive();
            container.PerformLayout();
        }

        private float UpdateTimelineInner(ref ViewRange viewRange)
        {
            if (_events.Count == 0)
                return 0;
            var data = _events.Get(_mainChart.SelectedSampleIndex);
            if (data == null || data.Length == 0)
                return 0;

            if (viewRange.End <= viewRange.Start)
                return 0;

            var container = _timeline.EventsContainer;

            // Create timeline track per thread
            int depthOffset = 0;
            for (int i = 0; i < data.Length; i++)
            {
                var events = data[i].Events ?? Array.Empty<ProfilerCPU.Event>();

                // Check maximum depth
                int maxDepth = -1;
                for (int j = 0; j < events.Length; j++)
                {
                    var e = events[j];
                    if (viewRange.SkipEvent(ref e))
                        continue;
                    maxDepth = Mathf.Max(maxDepth, e.Depth);
                }

                bool hasSamples = maxDepth >= 0;
                if (_keepUnused)
                {
                    // Reserve live thread lanes so missing worker samples do not move every following track.
                    if (_threadDepths.TryGetValue((data[i].ID, data[i].Name), out int previousDepth))
                        maxDepth = Math.Max(maxDepth, previousDepth);
                    maxDepth = Math.Max(maxDepth, 0);
                    _threadDepths[(data[i].ID, data[i].Name)] = maxDepth;
                }
                else if (!hasSamples)
                    continue;

                // Add thread label
                float xOffset = 90;
                CPULabel trackLabel;
                if (_timelineLabelCount < _timelineLabels.Count)
                    trackLabel = _timelineLabels[_timelineLabelCount];
                else
                {
                    trackLabel = new CPULabel { Parent = container, BackgroundColor = Style.Current.Background * 1.1f };
                    _timelineLabels.Add(trackLabel);
                }
                _timelineLabelCount++;
                trackLabel.Bounds = new Rectangle(0, depthOffset * Timeline.Event.DefaultHeight, xOffset, (maxDepth + 2) * Timeline.Event.DefaultHeight);
                trackLabel.Name = data[i].Name;
                trackLabel.LastUsedCapture = _captureNumber;
                if (!trackLabel.Enabled) trackLabel.TooltipText = null;
                trackLabel.Enabled = hasSamples;
                trackLabel.Visible = true;

                // Add events
                for (int j = 0; j < events.Length; j++)
                {
                    var e = events[j];
                    if (e.Depth == 0)
                    {
                        if (viewRange.SkipEvent(ref e))
                            continue;
                        AddEvent(viewRange, maxDepth, xOffset, depthOffset, j, events, container);
                    }
                }

                depthOffset += maxDepth + 2;
            }

            return Timeline.Event.DefaultHeight * depthOffset;
        }

        private void UpdateTable(ref ViewRange viewRange)
        {
            _table.IsLayoutLocked = true;

            _tableRowCount = 0;
            UpdateTableInner(ref viewRange);
            for (int i = _tableRowCount; i < _tableRows.Count; i++)
            {
                var item = _tableRows[i];
                RetainUnused(item, _keepUnused && _captureNumber - item.LastUsedCapture <= 2);
            }

            _table.UnlockChildrenRecursive();
            _table.PerformLayout();
        }

        private void UpdateTableInner(ref ViewRange viewRange)
        {
            if (_events.Count == 0)
                return;
            var data = _events.Get(_mainChart.SelectedSampleIndex);
            if (data == null || data.Length == 0)
                return;
            _aggregateIndices.Clear();
            _aggregateRoots.Clear();
            _aggregateCount = 0;
            for (int thread = 0; thread < data.Length; thread++)
            {
                var events = data[thread].Events;
                if (events == null) continue;
                _aggregateStack.Clear();
                ulong threadKey = 14695981039346656037 ^ data[thread].ID;
                foreach (char character in data[thread].Name) threadKey = unchecked((threadKey ^ character) * 1099511628211);
                for (int i = 0; i < events.Length; i++)
                {
                    ref var e = ref events[i];
                    int depth = Math.Max(e.Depth, 0);
                    while (_aggregateStack.Count > depth) _aggregateStack.RemoveAt(_aggregateStack.Count - 1);
                    while (_aggregateStack.Count < depth) _aggregateStack.Add(-1);
                    if (viewRange.SkipEvent(ref e))
                    {
                        _aggregateStack.Add(-1);
                        continue;
                    }
                    int parent = depth > 0 ? _aggregateStack[depth - 1] : -1;
                    var names = GetEventNameData(ref e);
                    var key = (parent >= 0 ? parent : -thread - 1, names.Raw);
                    if (!_aggregateIndices.TryGetValue(key, out int index))
                    {
                        index = _aggregateCount++;
                        if (index == _aggregates.Count) _aggregates.Add(new Aggregate());
                        var fresh = _aggregates[index];
                        fresh.Name = names.Display;
                        fresh.ScopeKey = unchecked(((parent >= 0 ? _aggregates[parent].ScopeKey : threadKey) * 1099511628211) ^ e.NameHash());
                        fresh.Parent = parent;
                        fresh.Depth = depth;
                        fresh.Calls = 0;
                        fresh.Total = fresh.Self = 0;
                        fresh.Memory = 0;
                        fresh.Children.Clear();
                        _aggregateIndices.Add(key, index);
                        if (parent >= 0) _aggregates[parent].Children.Add(index);
                        else _aggregateRoots.Add(index);
                    }
                    double duration = Math.Min(e.End, viewRange.End) - Math.Max(e.Start, viewRange.Start);
                    var node = _aggregates[index];
                    node.Calls++;
                    node.Total += duration;
                    node.Self += duration;
                    node.Memory += (long)e.NativeMemoryAllocation + e.ManagedMemoryAllocation;
                    if (parent >= 0) _aggregates[parent].Self -= duration;
                    _aggregateStack.Add(index);
                }
            }
            // Parents are allocated before their children, including repeated call stacks.
            for (int i = _aggregateCount - 1; i >= 0; i--)
                if (_aggregates[i].Parent >= 0)
                    _aggregates[_aggregates[i].Parent].Memory += _aggregates[i].Memory;
            double span = Math.Max(viewRange.End - viewRange.Start, MinEventTimeMs);
            foreach (int root in _aggregateRoots) AddAggregateRow(root, span, true);
        }

        private void OnRowExpansionChanged(Row row, bool expanded)
        {
            _expandedScopes[((CPURow)row).ScopeKey] = expanded;
        }

        private void AddAggregateRow(int index, double span, bool visible)
        {
            var node = _aggregates[index];
            CPURow row;
            if (_tableRowCount < _tableRows.Count) row = _tableRows[_tableRowCount];
            else
            {
                row = new CPURow { Values = new object[7], BackgroundColors = new Color[7], Parent = _table };
                row.ExpansionChanged += OnRowExpansionChanged;
                _tableRows.Add(row);
            }
            _tableRowCount++;
            row.LastUsedCapture = _captureNumber;
            if (!row.Enabled) row.TooltipText = null;
            row.Enabled = true;
            row.ScopeKey = node.ScopeKey;
            row.Values[0] = node.Name;
            float total = (float)(node.Total / span), self = (float)(Math.Max(node.Self, 0) / span);
            SetFloat(row, 1, (int)(total * 1000) / 10.0f);
            SetFloat(row, 2, (int)(self * 1000) / 10.0f);
            SetFloat(row, 3, (float)node.Total);
            SetFloat(row, 4, (float)Math.Max(node.Self, 0));
            if (!(row.Values[5] is long memory) || memory != node.Memory) row.Values[5] = node.Memory;
            if (!(row.Values[6] is int calls) || calls != node.Calls) row.Values[6] = node.Calls;
            row.BackgroundColors[1] = Color.Red.AlphaMultiplied(Mathf.Min(1, total) * 0.5f);
            row.BackgroundColors[2] = Color.Red.AlphaMultiplied(Mathf.Min(1, self) * 0.5f);
            row.Depth = node.Depth;
            row.Width = _table.Width;
            row.Visible = visible;
            bool expanded = _expandedScopes.TryGetValue(node.ScopeKey, out bool saved) ? saved : node.Depth == 0;
            foreach (int child in node.Children) AddAggregateRow(child, span, visible && expanded);
        }
    }
}
#endif
