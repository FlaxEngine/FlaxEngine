// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.GUI;
using FlaxEngine;
using FlaxEngine.GUI;

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
        private readonly SamplesBuffer<ProfilingTools.ThreadStats[]> _events = new SamplesBuffer<ProfilingTools.ThreadStats[]>();
        private bool _showOnlyLastUpdateEvents;

        public CPU()
        : base("CPU")
        {
            // Layout
            var panel = new Panel(ScrollBars.Vertical)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                Parent = this,
            };
            var layout = new VerticalPanel
            {
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                Offsets = Margin.Zero,
                IsScrollable = true,
                Parent = panel,
            };

            // Chart
            _mainChart = new SingleChart
            {
                Title = "Update",
                FormatSample = v => (Mathf.RoundToInt(v * 10.0f) / 10.0f) + " ms",
                Parent = layout,
            };
            _mainChart.SelectedSampleChanged += OnSelectedSampleChanged;

            // Timeline
            _timeline = new Timeline
            {
                Height = 340,
                Parent = layout,
            };

            // Table
            var headerColor = Style.Current.LightBackground;
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
                    },
                    new ColumnDefinition
                    {
                        Title = "Total",
                        TitleBackgroundColor = headerColor,
                        FormatValue = FormatCellPercentage,
                    },
                    new ColumnDefinition
                    {
                        Title = "Self",
                        TitleBackgroundColor = headerColor,
                        FormatValue = FormatCellPercentage,
                    },
                    new ColumnDefinition
                    {
                        Title = "Time ms",
                        TitleBackgroundColor = headerColor,
                        FormatValue = FormatCellMs,
                    },
                    new ColumnDefinition
                    {
                        Title = "Self ms",
                        TitleBackgroundColor = headerColor,
                        FormatValue = FormatCellMs,
                    },
                    new ColumnDefinition
                    {
                        Title = "Memory",
                        TitleBackgroundColor = headerColor,
                        FormatValue = FormatCellBytes,
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
            return Utilities.Utils.FormatBytesCount((int)x);
        }

        /// <inheritdoc />
        public override void Clear()
        {
            _mainChart.Clear();
            _events.Clear();
        }

        /// <inheritdoc />
        public override void Update(ref SharedUpdateData sharedData)
        {
            _mainChart.AddSample(sharedData.Stats.UpdateTimeMs);

            // Gather CPU events
            var events = sharedData.GetEventsCPU();
            _events.Add(events);

            // Update timeline if using the last frame
            if (_mainChart.SelectedSampleIndex == -1)
            {
                var viewRange = GetEventsViewRange();
                UpdateTimeline(ref viewRange);
                UpdateTable(ref viewRange);
            }
        }

        /// <inheritdoc />
        public override void UpdateView(int selectedFrame, bool showOnlyLastUpdateEvents)
        {
            _showOnlyLastUpdateEvents = showOnlyLastUpdateEvents;
            _mainChart.SelectedSampleIndex = selectedFrame;

            var viewRange = GetEventsViewRange();
            UpdateTimeline(ref viewRange);
            UpdateTable(ref viewRange);
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

        private ViewRange GetEventsViewRange()
        {
            if (_showOnlyLastUpdateEvents)
            {
                // Find root event named 'Update' and use it as a view range
                if (_events.Count != 0)
                {
                    var data = _events.Get(_mainChart.SelectedSampleIndex);
                    if (data != null)
                    {
                        for (int j = 0; j < data.Length; j++)
                        {
                            var events = data[j].Events;
                            if (events == null)
                                continue;

                            for (int i = 0; i < events.Length; i++)
                            {
                                var e = events[i];

                                if (e.Depth == 0 && new string(e.Name) == "Update")
                                {
                                    return new ViewRange(ref e);
                                }
                            }
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
            double scale = 100.0;
            float x = (float)((e.Start - startTime) * scale);
            float width = (float)(length * scale);
            string name = new string(e.Name).Replace("::", ".");

            var control = new Timeline.Event(x + xOffset, e.Depth + depthOffset, width)
            {
                Name = name,
                TooltipText = string.Format("{0}, {1} ms", name, ((int)(length * 1000.0) / 1000.0f)),
                Parent = parent,
            };

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

            // Clear previous events
            container.DisposeChildren();

            container.LockChildrenRecursive();

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
            for (int i = 0; i < data.Length; i++)
            {
                if (data[i].Events != null && data[i].Events.Length != 0)
                    startTime = Math.Min(startTime, data[i].Events[0].Start);
            }
            if (startTime >= double.MaxValue)
                return 0;

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

                    // Reject events outside the view range
                    if (viewRange.SkipEvent(ref e))
                        continue;

                    maxDepth = Mathf.Max(maxDepth, e.Depth);
                }

                // Skip empty tracks
                if (maxDepth == -1)
                    continue;

                // Add thread label
                float xOffset = 90;
                var label = new Timeline.TrackLabel
                {
                    Bounds = new Rectangle(0, depthOffset * Timeline.Event.DefaultHeight, xOffset, (maxDepth + 2) * Timeline.Event.DefaultHeight),
                    Name = data[i].Name,
                    BackgroundColor = Style.Current.Background * 1.1f,
                    Parent = container,
                };

                // Add events
                for (int j = 0; j < events.Length; j++)
                {
                    var e = events[j];
                    if (e.Depth == 0)
                    {
                        // Reject events outside the view range
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
            _table.DisposeChildren();

            _table.LockChildrenRecursive();

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

                    // Reject events outside the view range
                    if (viewRange.SkipEvent(ref e))
                        continue;

                    // Count sub-events time
                    double subEventsTimeTotal = 0;
                    int subEventsMemoryTotal = e.ManagedMemoryAllocation + e.NativeMemoryAllocation;
                    for (int k = i + 1; k < events.Length; k++)
                    {
                        var sub = events[k];
                        if (sub.Depth == e.Depth + 1)
                        {
                            subEventsTimeTotal += Math.Max(sub.End - sub.Start, MinEventTimeMs);
                        }
                        else if (sub.Depth <= e.Depth)
                        {
                            break;
                        }

                        subEventsMemoryTotal += sub.ManagedMemoryAllocation + e.NativeMemoryAllocation;
                    }

                    string name = new string(e.Name).Replace("::", ".");

                    var row = new Row
                    {
                        Values = new object[]
                        {
                            // Event
                            name,

                            // Total (%)
                            (int)(time / totalTimeMs * 1000.0f) / 10.0f,

                            // Self (%)
                            (int)((time - subEventsTimeTotal) / time * 1000.0f) / 10.0f,

                            // Time ms
                            (float)((time * 10000.0f) / 10000.0f),

                            // Self ms
                            (float)(((time - subEventsTimeTotal) * 10000.0f) / 10000.0f),

                            // Memory Alloc
                            subEventsMemoryTotal,
                        },
                        Depth = e.Depth,
                        Width = _table.Width,
                        Parent = _table,
                    };

                    if (i % 2 == 0)
                        row.BackgroundColor = rowColor2;
                    row.Visible = e.Depth < 3;
                }
            }
        }
    }
}
