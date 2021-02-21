// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEditor.GUI;
using FlaxEngine;
using FlaxEngine.GUI;

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
        private readonly SamplesBuffer<ProfilerGPU.Event[]> _events = new SamplesBuffer<ProfilerGPU.Event[]>();

        public GPU()
        : base("GPU")
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
            _drawTimeCPU = new SingleChart
            {
                Title = "Draw (CPU)",
                FormatSample = v => (Mathf.RoundToInt(v * 10.0f) / 10.0f) + " ms",
                Parent = layout,
            };
            _drawTimeCPU.SelectedSampleChanged += OnSelectedSampleChanged;
            _drawTimeGPU = new SingleChart
            {
                Title = "Draw (GPU)",
                FormatSample = v => (Mathf.RoundToInt(v * 10.0f) / 10.0f) + " ms",
                Parent = layout,
            };
            _drawTimeGPU.SelectedSampleChanged += OnSelectedSampleChanged;

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
                        FormatValue = (x) => ((float)x).ToString("0.0") + '%',
                    },
                    new ColumnDefinition
                    {
                        Title = "GPU ms",
                        TitleBackgroundColor = headerColor,
                        FormatValue = (x) => ((float)x).ToString("0.000"),
                    },
                    new ColumnDefinition
                    {
                        Title = "Draw Calls",
                        TitleBackgroundColor = headerColor,
                    },
                    new ColumnDefinition
                    {
                        Title = "Triangles",
                        TitleBackgroundColor = headerColor,
                    },
                    new ColumnDefinition
                    {
                        Title = "Vertices",
                        TitleBackgroundColor = headerColor,
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

        /// <inheritdoc />
        public override void Clear()
        {
            _drawTimeCPU.Clear();
            _drawTimeGPU.Clear();
            _events.Clear();
        }

        /// <inheritdoc />
        public override void Update(ref SharedUpdateData sharedData)
        {
            // Gather GPU events
            var data = sharedData.GetEventsGPU();
            _events.Add(data);

            // Peek draw time
            _drawTimeCPU.AddSample(sharedData.Stats.DrawCPUTimeMs);
            _drawTimeGPU.AddSample(sharedData.Stats.DrawGPUTimeMs);

            // Update timeline if using the last frame
            if (_drawTimeCPU.SelectedSampleIndex == -1)
            {
                UpdateTimeline();
                UpdateTable();
            }
        }

        /// <inheritdoc />
        public override void UpdateView(int selectedFrame, bool showOnlyLastUpdateEvents)
        {
            _drawTimeCPU.SelectedSampleIndex = selectedFrame;
            _drawTimeGPU.SelectedSampleIndex = selectedFrame;
            UpdateTimeline();
            UpdateTable();
        }

        private float AddEvent(float x, int maxDepth, int index, ProfilerGPU.Event[] events, ContainerControl parent)
        {
            ref ProfilerGPU.Event e = ref events[index];

            double scale = 100.0;
            float width = (float)(e.Time * scale);
            string name = new string(e.Name);

            new Timeline.Event(x, e.Depth, width)
            {
                Name = name,
                TooltipText = string.Format("{0}, {1} ms", name, ((int)(e.Time * 10000.0) / 10000.0f)),
                Parent = parent,
            };

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

            // Clear previous events
            container.DisposeChildren();

            container.LockChildrenRecursive();

            _timeline.Height = UpdateTimelineInner();

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
            _table.DisposeChildren();

            _table.LockChildrenRecursive();

            UpdateTableInner();

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

            float totalTimeMs = _drawTimeCPU.SelectedSample;

            // Add rows
            var rowColor2 = Style.Current.Background * 1.4f;
            for (int i = 0; i < data.Length; i++)
            {
                var e = data[i];
                string name = new string(e.Name);

                var row = new Row
                {
                    Values = new object[]
                    {
                        // Event
                        name,

                        // Total (%)
                        (int)(e.Time / totalTimeMs * 1000.0f) / 10.0f,

                        // GPU ms
                        (e.Time * 10000.0f) / 10000.0f,

                        // Draw Calls
                        e.Stats.DrawCalls,

                        // Triangles
                        e.Stats.Triangles,

                        // Vertices
                        e.Stats.Vertices,
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
