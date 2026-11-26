// Copyright (c) Wojciech Figat. All rights reserved.

#if USE_PROFILER
using System.Collections.Generic;
using FlaxEditor.GUI;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEngine
{
    partial class ProfilerGPU
    {
        /// <summary>
        /// Delegate for profiler event click callback. Can be used by external profiling tools integration inside Editor.
        /// </summary>
        /// <param name="e">Event data.</param>
        public delegate void EventDelegate(ref Event e);

        /// <summary>
        /// Called when the user double-clicks a GPU profiler event in Editor to open it. Can be used by external profiling tools integration inside Editor.
        /// </summary>
        public static EventDelegate OpenEventEditor;
    }
}

namespace FlaxEditor.Windows.Profiler
{
    /// <summary>
    /// The GPU performance profiling mode.
    /// </summary>
    /// <seealso cref="FlaxEditor.Windows.Profiler.ProfilerMode" />
    internal sealed unsafe class GPU : ProfilerMode
    {
        private readonly SingleChart _drawTimeCPU;
        private readonly SingleChart _drawTimeGPU;
        private readonly Timeline _timeline;
        private readonly Table _table;
        private SamplesBuffer<ProfilerGPU.Event[]> _events;
        private sealed class GPUEvent : Timeline.Event
        {
            public ProfilerGPU.Event Capture;
            public ulong LastUsedCapture;
        }
        private sealed class GPURow : ClickableRow
        {
            public ProfilerGPU.Event Capture;
            public ulong LastUsedCapture, ScopeKey;
        }
        private List<GPUEvent> _timelineEvents;
        private List<GPURow> _tableRows;
        private int _timelineEventCount, _tableRowCount;
        private ulong _captureNumber;
        private bool _keepUnused;
        private readonly List<ulong> _scopeStack = new List<ulong>();
        private readonly List<bool> _visibleStack = new List<bool>();
        private readonly Dictionary<ulong, bool> _expandedScopes = new Dictionary<ulong, bool>();

        public GPU()
        : base("GPU")
        {
            // Layout
            var mainPanel = new Panel(ScrollBars.None)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                Parent = this,
            };

            // Charts
            _drawTimeCPU = new SingleChart
            {
                Title = "Draw (CPU)",
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                Offsets = Margin.Zero,
                Height = SingleChart.DefaultHeight,
                FormatSample = v => (Mathf.RoundToInt(v * 10.0f) / 10.0f) + " ms",
                Parent = mainPanel,
            };
            _drawTimeCPU.SelectedSampleChanged += OnSelectedSampleChanged;
            _drawTimeGPU = new SingleChart
            {
                Title = "Draw (GPU)",
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                Offsets = new Margin(0, 0, _drawTimeCPU.Height + 2, SingleChart.DefaultHeight),
                FormatSample = v => (Mathf.RoundToInt(v * 10.0f) / 10.0f) + " ms",
                Parent = mainPanel,
            };
            _drawTimeGPU.SelectedSampleChanged += OnSelectedSampleChanged;

            var panel = new Panel(ScrollBars.Vertical)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, _drawTimeCPU.Height + _drawTimeGPU.Height + 4, 0),
                Parent = mainPanel,
            };
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

            _timeline.ScaleChanged += () => UpdateView(_drawTimeCPU.SelectedSampleIndex, false);

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
                        TitleColor = textColor,
                        FormatValue = (x) => ((float)x).ToString("0.0") + '%',
                    },
                    new ColumnDefinition
                    {
                        Title = "GPU ms",
                        TitleBackgroundColor = headerColor,
                        TitleColor = textColor,
                        FormatValue = (x) => ((float)x).ToString("0.000"),
                    },
                    new ColumnDefinition
                    {
                        Title = "Draw Calls",
                        TitleBackgroundColor = headerColor,
                        TitleColor = textColor,
                        FormatValue = FormatCountLong,
                    },
                    new ColumnDefinition
                    {
                        Title = "Triangles",
                        TitleBackgroundColor = headerColor,
                        TitleColor = textColor,
                        FormatValue = FormatCountLong,
                    },
                    new ColumnDefinition
                    {
                        Title = "Vertices",
                        TitleBackgroundColor = headerColor,
                        TitleColor = textColor,
                        FormatValue = FormatCountLong,
                    },
                },
                Parent = layout,
            };
            _table.Splits = new[]
            {
                0.5f,
                0.1f,
                0.1f,
                0.1f,
                0.1f,
                0.1f,
            };
        }

        private static string FormatCountLong(object x)
        {
            return ((long)x).ToString("###,###,###");
        }

        /// <inheritdoc />
        public override void Clear()
        {
            _drawTimeCPU.Clear();
            _drawTimeGPU.Clear();
            _events?.Clear();
            _expandedScopes.Clear();
        }

        /// <inheritdoc />
        public override void Update(ref SharedUpdateData sharedData)
        {
            _captureNumber++;
            // Gather GPU events
            var data = sharedData.GetEventsGPU();
            if (_events == null)
                _events = new SamplesBuffer<ProfilerGPU.Event[]>();
            _events.Add(data);

            // Peek draw time
            _drawTimeCPU.AddSample(sharedData.Stats.DrawCPUTimeMs);
            _drawTimeGPU.AddSample(sharedData.Stats.DrawGPUTimeMs);
        }

        /// <inheritdoc />
        public override void UpdateView(int selectedFrame, bool showOnlyLastUpdateEvents)
        {
            _keepUnused = selectedFrame == -1;
            _drawTimeCPU.SelectedSampleIndex = selectedFrame;
            _drawTimeGPU.SelectedSampleIndex = selectedFrame;

            if (_events == null)
                return;
            if (_timelineEvents == null)
                _timelineEvents = new List<GPUEvent>();
            if (_tableRows == null)
                _tableRows = new List<GPURow>();

            UpdateTimeline();
            UpdateTable();
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _timelineEvents?.Clear();
            _tableRows?.Clear();

            base.OnDestroy();
        }

        private float AddEvent(float x, int maxDepth, int index, ProfilerGPU.Event[] events, ContainerControl parent)
        {
            ref ProfilerGPU.Event e = ref events[index];

            double scale = _timeline.PixelsPerMillisecond;
            float width = (float)(e.Time * scale);
            string name = e.Name ?? string.Empty;

            GPUEvent control;
            if (_timelineEventCount < _timelineEvents.Count)
                control = _timelineEvents[_timelineEventCount];
            else
            {
                control = new GPUEvent { Parent = parent, EventDoubleClick = OnEventDoubleClick };
                _timelineEvents.Add(control);
            }
            _timelineEventCount++;
            control.Bounds = new Rectangle(x, e.Depth * Timeline.Event.DefaultHeight, width, Timeline.Event.DefaultHeight - 1);
            if (control.Name != name) control.Name = name;
            control.DurationMilliseconds = e.Time;
            control.Capture = e;
            control.LastUsedCapture = _captureNumber;
            control.Enabled = control.Visible = true;

            // Spawn sub events
            int childrenDepth = e.Depth + 1;
            if (childrenDepth <= maxDepth)
            {
                // Count sub events total duration
                double subEventsDuration = 0;
                int tmpIndex = index;
                while (++tmpIndex < events.Length)
                {
                    int subDepth = events[tmpIndex].Depth;

                    if (subDepth <= e.Depth)
                        break;
                    if (subDepth == childrenDepth)
                        subEventsDuration += events[tmpIndex].Time;
                }

                // Skip if has no sub events
                if (subEventsDuration > 0)
                {
                    // Apply some offset to sub-events (center them within this event)
                    x += (float)((e.Time - subEventsDuration) * scale) * 0.5f;

                    while (++index < events.Length)
                    {
                        int subDepth = events[index].Depth;

                        if (subDepth <= e.Depth)
                            break;
                        if (subDepth == childrenDepth)
                        {
                            x += AddEvent(x, maxDepth, index, events, parent);
                        }
                    }
                }
            }

            return width;
        }

        private void UpdateTimeline()
        {
            var container = _timeline.EventsContainer;

            container.IsLayoutLocked = true;
            _timelineEventCount = 0;

            container.LockChildrenRecursive();

            float height = UpdateTimelineInner();
            for (int i = _timelineEventCount; i < _timelineEvents.Count; i++)
            {
                var item = _timelineEvents[i];
                RetainUnused(item, _keepUnused && _captureNumber - item.LastUsedCapture <= 2);
                if (item.Visible) height = Mathf.Max(height, item.Bottom);
            }
            _timeline.Height = height;

            container.UnlockChildrenRecursive();
            container.PerformLayout();
        }

        private float UpdateTimelineInner()
        {
            if (_events.Count == 0)
                return 0;
            var data = _events.Get(_drawTimeCPU.SelectedSampleIndex);
            if (data == null || data.Length == 0)
                return 0;

            var container = _timeline.EventsContainer;
            var events = data;

            // Check maximum depth
            int maxDepth = 0;
            for (int j = 0; j < events.Length; j++)
            {
                maxDepth = Mathf.Max(maxDepth, events[j].Depth);
            }

            // Add events
            float x = 0;
            for (int j = 0; j < events.Length; j++)
            {
                if (events[j].Depth == 0)
                {
                    x += AddEvent(x, maxDepth, j, events, container);
                }
            }

            return Timeline.Event.DefaultHeight * (maxDepth + 2);
        }

        private void UpdateTable()
        {
            _table.IsLayoutLocked = true;
            _tableRowCount = 0;

            UpdateTableInner();
            for (int i = _tableRowCount; i < _tableRows.Count; i++)
            {
                var item = _tableRows[i];
                RetainUnused(item, _keepUnused && _captureNumber - item.LastUsedCapture <= 2);
            }

            _table.UnlockChildrenRecursive();
            _table.PerformLayout();
        }

        private void UpdateTableInner()
        {
            if (_events.Count == 0)
                return;
            var data = _events.Get(_drawTimeCPU.SelectedSampleIndex);
            if (data == null || data.Length == 0)
                return;
            float totalTimeMs = _drawTimeGPU.SelectedSample;

            // Add rows
            _scopeStack.Clear();
            _visibleStack.Clear();
            for (int i = 0; i < data.Length; i++)
            {
                var e = data[i];
                string name = e.Name ?? string.Empty;
                while (_scopeStack.Count > e.Depth) { _scopeStack.RemoveAt(_scopeStack.Count - 1); _visibleStack.RemoveAt(_visibleStack.Count - 1); }
                while (_scopeStack.Count < e.Depth) { _scopeStack.Add(0); _visibleStack.Add(true); }
                ulong key = e.Depth > 0 ? _scopeStack[e.Depth - 1] : 14695981039346656037;
                foreach (char character in name) key = unchecked((key ^ character) * 1099511628211);
                bool visible = e.Depth == 0 || _visibleStack[e.Depth - 1];
                bool expanded = _expandedScopes.TryGetValue(key, out bool saved) ? saved : e.Depth < 2;
                _scopeStack.Add(key);
                _visibleStack.Add(visible && expanded);

                GPURow row;
                if (_tableRowCount < _tableRows.Count)
                {
                    row = _tableRows[_tableRowCount];
                }
                else
                {
                    row = new GPURow
                    {
                        Values = new object[6],
                        BackgroundColors = new Color[6],
                        RowDoubleClick = OnRowDoubleClick,
                    };
                    for (int k = 0; k < row.BackgroundColors.Length; k++)
                        row.BackgroundColors[k] = Color.Transparent;
                    row.Parent = _table;
                    row.ExpansionChanged += OnRowExpansionChanged;
                    _tableRows.Add(row);
                }
                _tableRowCount++;
                row.LastUsedCapture = _captureNumber;
                row.ScopeKey = key;
                if (!row.Enabled) row.TooltipText = null;
                row.Enabled = true;
                {
                    // Event
                    row.Values[0] = name;

                    // Total (%)
                    float rowTimePerc = totalTimeMs > 0 ? e.Time / totalTimeMs : 0;
                    SetFloat(row, 1, (int)(rowTimePerc * 1000.0f) / 10.0f);
                    row.BackgroundColors[1] = Color.Red.AlphaMultiplied(Mathf.Min(1, rowTimePerc) * 0.5f);

                    // GPU ms
                    SetFloat(row, 2, e.Time);

                    // Draw Calls
                    SetLong(row, 3, e.Stats.DrawCalls + e.Stats.DispatchCalls);

                    // Triangles
                    SetLong(row, 4, e.Stats.Triangles);

                    // Vertices
                    SetLong(row, 5, e.Stats.Vertices);
                }
                row.Depth = e.Depth;
                row.Width = _table.Width;
                row.Visible = visible;
                row.Capture = e;
            }
        }

        private static void SetFloat(Row row, int index, float value)
        {
            if (!(row.Values[index] is float previous) || previous != value)
                row.Values[index] = value;
        }

        private static void SetLong(Row row, int index, long value)
        {
            if (!(row.Values[index] is long previous) || previous != value)
                row.Values[index] = value;
        }

        private void OnRowExpansionChanged(Row row, bool expanded)
        {
            _expandedScopes[((GPURow)row).ScopeKey] = expanded;
        }

        private void OnEventDoubleClick(Timeline.Event control)
        {
            if (ProfilerGPU.OpenEventEditor != null)
            {
                var e = ((GPUEvent)control).Capture;
                ProfilerGPU.OpenEventEditor(ref e);
            }
        }

        private void OnRowDoubleClick(ClickableRow row)
        {
            if (ProfilerGPU.OpenEventEditor != null)
            {
                var e = ((GPURow)row).Capture;
                ProfilerGPU.OpenEventEditor(ref e);
            }
        }
    }
}
#endif
