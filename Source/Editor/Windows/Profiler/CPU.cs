// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
        private List<Timeline.TrackLabel> _timelineLabelsCache;
        private List<Timeline.Event> _timelineEventsCache;
        private List<Row> _tableRowsCache;
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
                Title = "Update",
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
                IsScrollable = true,
                Parent = panel,
            };
            
            // Timeline
            _timeline = new Timeline
            {
                Height = 340,
                Parent = layout,
            };

            // Table
            var style = Style.Current;
            var headerColor = style.LightBackground;
            var textColor = style.Foreground;
            _table = new Table
            {
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
        }

        /// <inheritdoc />
        public override void Update(ref SharedUpdateData sharedData)
        {
            _mainChart.AddSample(sharedData.Stats.UpdateTimeMs);

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
            _mainChart.SelectedSampleIndex = selectedFrame;

            if (_events == null)
                return;
            if (_timelineLabelsCache == null)
                _timelineLabelsCache = new List<Timeline.TrackLabel>();
            if (_timelineEventsCache == null)
                _timelineEventsCache = new List<Timeline.Event>();
            if (_tableRowsCache == null)
                _tableRowsCache = new List<Row>();

            var viewRange = _showOnlyLastUpdateEvents ? GetMainThreadUpdateRange() : ViewRange.Full;
            UpdateTimeline(ref viewRange);
            UpdateTable(ref viewRange);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _timelineLabelsCache?.Clear();
            _timelineEventsCache?.Clear();
            _tableRowsCache?.Clear();

            base.OnDestroy();
        }

        private struct ViewRange
        {
            public double Start;
            public double End;

            public static ViewRange Full = new ViewRange
            {
                Start = float.MinValue,
                End = float.MaxValue
            };

            public ViewRange(ref ProfilerCPU.Event e)
            {
                Start = e.Start - MinEventTimeMs;
                End = e.End + MinEventTimeMs;
            }

            public bool SkipEvent(ref ProfilerCPU.Event e)
            {
                return e.Start < Start || e.Start > End;
            }
        }

        private ViewRange GetMainThreadUpdateRange()
        {
            if (_events != null && _events.Count != 0)
            {
                var threads = _events.Get(_mainChart.SelectedSampleIndex);
                if (threads != null)
                {
                    for (int j = 0; j < threads.Length; j++)
                    {
                        var thread = threads[j];
                        if (thread.Name != "Main" || thread.Events == null)
                            continue;
                        for (int i = 0; i < thread.Events.Length; i++)
                        {
                            ref var e = ref thread.Events[i];
                            if (e.Depth == 0 && e.Name == "Update")
                                return new ViewRange(ref e);
                        }
                    }
                }
            }
            return ViewRange.Full;
        }

        private void AddEvent(double startTime, int maxDepth, float xOffset, int depthOffset, int index, ProfilerCPU.Event[] events, ContainerControl parent)
        {
            ref ProfilerCPU.Event e = ref events[index];

            double length = e.End - e.Start;
            if (length <= 0.0)
                return;
            double scale = 100.0;
            float x = (float)((e.Start - startTime) * scale);
            float width = (float)(length * scale);
            Timeline.Event control;
            if (_timelineEventsCache.Count != 0)
            {
                var last = _timelineEventsCache.Count - 1;
                control = _timelineEventsCache[last];
                _timelineEventsCache.RemoveAt(last);
            }
            else
            {
                control = new Timeline.Event();
            }
            control.Bounds = new Rectangle(x + xOffset, (e.Depth + depthOffset) * Timeline.Event.DefaultHeight, width, Timeline.Event.DefaultHeight - 1);
            control.Name = e.Name.Replace("::", ".");
            control.TooltipText = string.Format("{0}, {1} ms", control.Name, ((int)(length * 1000.0) / 1000.0f));
            control.Parent = parent;

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
                        AddEvent(startTime, maxDepth, xOffset, depthOffset, index, events, parent);
                    }
                }
            }
        }

        private void UpdateTimeline(ref ViewRange viewRange)
        {
            var container = _timeline.EventsContainer;

            container.IsLayoutLocked = true;
            int idx = 0;
            while (container.Children.Count > idx)
            {
                var child = container.Children[idx];
                if (child is Timeline.Event e)
                {
                    _timelineEventsCache.Add(e);
                    child.Parent = null;
                }
                else if (child is Timeline.TrackLabel l)
                {
                    _timelineLabelsCache.Add(l);
                    child.Parent = null;
                }
                else
                {
                    idx++;
                }
            }

            _timeline.Height = UpdateTimelineInner(ref viewRange);

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

            // Find the first event start time (for the timeline start time)
            double startTime = double.MaxValue;
            if (viewRange.Start > 0)
            {
                startTime = viewRange.Start;
            }
            else
            {
                var r = GetMainThreadUpdateRange();
                if (r.Start > 0)
                {
                    startTime = r.Start;
                }
                else
                {
                    for (int i = 0; i < data.Length; i++)
                    {
                        if (data[i].Events != null && data[i].Events.Length != 0)
                            startTime = Math.Min(startTime, data[i].Events[0].Start);
                    }
                    if (startTime >= double.MaxValue)
                        return 0;
                }
            }

            var container = _timeline.EventsContainer;

            // Create timeline track per thread
            int depthOffset = 0;
            for (int i = 0; i < data.Length; i++)
            {
                var events = data[i].Events;
                if (events == null)
                    continue;

                // Check maximum depth
                int maxDepth = -1;
                for (int j = 0; j < events.Length; j++)
                {
                    var e = events[j];
                    if (viewRange.SkipEvent(ref e))
                        continue;
                    maxDepth = Mathf.Max(maxDepth, e.Depth);
                }

                // Skip empty tracks
                if (maxDepth == -1)
                    continue;

                // Add thread label
                float xOffset = 90;
                Timeline.TrackLabel trackLabel;
                if (_timelineLabelsCache.Count != 0)
                {
                    var last = _timelineLabelsCache.Count - 1;
                    trackLabel = _timelineLabelsCache[last];
                    _timelineLabelsCache.RemoveAt(last);
                }
                else
                {
                    trackLabel = new Timeline.TrackLabel();
                }
                trackLabel.Bounds = new Rectangle(0, depthOffset * Timeline.Event.DefaultHeight, xOffset, (maxDepth + 2) * Timeline.Event.DefaultHeight);
                trackLabel.Name = data[i].Name;
                trackLabel.BackgroundColor = Style.Current.Background * 1.1f;
                trackLabel.Parent = container;

                // Add events
                for (int j = 0; j < events.Length; j++)
                {
                    var e = events[j];
                    if (e.Depth == 0)
                    {
                        if (viewRange.SkipEvent(ref e))
                            continue;
                        AddEvent(startTime, maxDepth, xOffset, depthOffset, j, events, container);
                    }
                }

                depthOffset += maxDepth + 2;
            }

            return Timeline.Event.DefaultHeight * depthOffset;
        }

        private void UpdateTable(ref ViewRange viewRange)
        {
            _table.IsLayoutLocked = true;

            RecycleTableRows(_table, _tableRowsCache);
            UpdateTableInner(ref viewRange);

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
            float totalTimeMs = _mainChart.SelectedSample;

            // Add rows
            var rowColor2 = Style.Current.Background * 1.4f;
            for (int j = 0; j < data.Length; j++)
            {
                var events = data[j].Events;
                if (events == null)
                    continue;

                for (int i = 0; i < events.Length; i++)
                {
                    var e = events[i];
                    var time = Math.Max(e.End - e.Start, MinEventTimeMs);
                    if (e.End <= 0.0f || viewRange.SkipEvent(ref e))
                        continue;

                    // Count sub-events time
                    double subEventsTimeTotal = 0;
                    int subEventsMemoryTotal = e.ManagedMemoryAllocation + e.NativeMemoryAllocation;
                    for (int k = i + 1; k < events.Length; k++)
                    {
                        var sub = events[k];
                        if (sub.Depth == e.Depth + 1 && e.End > 0.0f)
                        {
                            subEventsTimeTotal += Math.Max(sub.End - sub.Start, MinEventTimeMs);
                        }
                        else if (sub.Depth <= e.Depth)
                        {
                            break;
                        }
                        subEventsMemoryTotal += sub.ManagedMemoryAllocation + e.NativeMemoryAllocation;
                    }

                    string name = e.Name.Replace("::", ".");

                    Row row;
                    if (_tableRowsCache.Count != 0)
                    {
                        var last = _tableRowsCache.Count - 1;
                        row = _tableRowsCache[last];
                        _tableRowsCache.RemoveAt(last);
                    }
                    else
                    {
                        row = new Row
                        {
                            Values = new object[6],
                            BackgroundColors = new Color[6],
                        };
                        for (int k = 0; k < row.BackgroundColors.Length; k++)
                            row.BackgroundColors[k] = Color.Transparent;
                    }
                    {
                        // Event
                        row.Values[0] = name;

                        // Total (%)
                        float rowTotalTimePerc = (float)(time / totalTimeMs);
                        row.Values[1] = (int)(rowTotalTimePerc * 1000.0f) / 10.0f;
                        row.BackgroundColors[1] = Color.Red.AlphaMultiplied(Mathf.Min(1, rowTotalTimePerc) * 0.5f);

                        // Self (%)
                        float rowSelfTimePerc = (float)((time - subEventsTimeTotal) / totalTimeMs);
                        row.Values[2] = (int)(rowSelfTimePerc * 1000.0f) / 10.0f;
                        row.BackgroundColors[2] = Color.Red.AlphaMultiplied(Mathf.Min(1, rowSelfTimePerc) * 0.5f);

                        // Time ms
                        row.Values[3] = (float)((time * 10000.0f) / 10000.0f);

                        // Self ms
                        row.Values[4] = (float)(((time - subEventsTimeTotal) * 10000.0f) / 10000.0f);

                        // Memory Alloc
                        row.Values[5] = subEventsMemoryTotal;
                    }
                    row.Depth = e.Depth;
                    row.Width = _table.Width;
                    row.Visible = e.Depth < 2;
                    row.BackgroundColor = i % 2 == 0 ? rowColor2 : Color.Transparent;
                    row.Parent = _table;
                }
            }
        }
    }
}
