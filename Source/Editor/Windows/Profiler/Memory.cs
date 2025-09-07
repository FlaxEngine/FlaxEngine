// Copyright (c) Wojciech Figat. All rights reserved.

#if USE_PROFILER
using System;
using System.Collections.Generic;
using FlaxEditor.GUI;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows.Profiler
{
    /// <summary>
    /// The memory profiling mode focused on system memory allocations breakdown.
    /// </summary>
    /// <seealso cref="FlaxEditor.Windows.Profiler.ProfilerMode" />
    internal sealed class Memory : ProfilerMode
    {
        private struct FrameData
        {
            public ProfilerMemory.GroupsArray Usage;
            public ProfilerMemory.GroupsArray Peek;
            public ProfilerMemory.GroupsArray Count;
        }

        private readonly SingleChart _nativeAllocationsChart;
        private readonly SingleChart _managedAllocationsChart;
        private readonly Table _table;
        private SamplesBuffer<FrameData> _frames;
        private List<Row> _tableRowsCache;
        private string[] _groupNames;
        private int[] _groupOrder;
        private Label _warningText;
        
        public Memory()
        : base("Memory")
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
                Pivot = Float2.Zero,
                IsScrollable = true,
                Parent = panel,
            };

            // Chart
            _nativeAllocationsChart = new SingleChart
            {
                Title = "Native Memory Allocation",
                FormatSample = v => Utilities.Utils.FormatBytesCount((ulong)v),
                Parent = layout,
            };
            _nativeAllocationsChart.SelectedSampleChanged += OnSelectedSampleChanged;
            _managedAllocationsChart = new SingleChart
            {
                Title = "Managed Memory Allocation",
                FormatSample = v => Utilities.Utils.FormatBytesCount((ulong)v),
                Parent = layout,
            };
            _managedAllocationsChart.SelectedSampleChanged += OnSelectedSampleChanged;

            // Warning text
            if (!ProfilerMemory.Enabled)
            {
                _warningText = new Label
                {
                    Text = "Detailed memory profiling is disabled. Run with command line '-mem'",
                    TextColor = Color.Red,
                    Visible = false,
                    Parent = layout,
                };
            }

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
                        Title = "Group",
                        TitleBackgroundColor = headerColor,
                        TitleColor = textColor,
                    },
                    new ColumnDefinition
                    {
                        Title = "Usage",
                        TitleBackgroundColor = headerColor,
                        FormatValue = FormatCellBytes,
                        TitleColor = textColor,
                    },
                    new ColumnDefinition
                    {
                        Title = "Peek",
                        TitleBackgroundColor = headerColor,
                        FormatValue = FormatCellBytes,
                        TitleColor = textColor,
                    },
                    new ColumnDefinition
                    {
                        Title = "Count",
                        TitleBackgroundColor = headerColor,
                        TitleColor = textColor,
                    },
                },
                Parent = layout,
            };
            _table.Splits = new[]
            {
                0.5f,
                0.2f,
                0.2f,
                0.1f,
            };
        }

        private string FormatCellBytes(object x)
        {
            return Utilities.Utils.FormatBytesCount(Convert.ToUInt64(x));
        }

        /// <inheritdoc />
        public override void Clear()
        {
            _nativeAllocationsChart.Clear();
            _managedAllocationsChart.Clear();
            _frames?.Clear();
        }

        /// <inheritdoc />
        public override void Update(ref SharedUpdateData sharedData)
        {
            // Count memory allocated during last frame
            int nativeMemoryAllocation = 0;
            int managedMemoryAllocation = sharedData.ManagedMemoryAllocation;
            var events = sharedData.GetEventsCPU();
            var length = events?.Length ?? 0;
            for (int i = 0; i < length; i++)
            {
                var ee = events[i].Events;
                if (ee == null)
                    continue;
                for (int j = 0; j < ee.Length; j++)
                {
                    ref var e = ref ee[j];
                    if (e.NameStartsWith("ProfilerWindow"))
                        continue;
                    nativeMemoryAllocation += e.NativeMemoryAllocation;
                    managedMemoryAllocation += e.ManagedMemoryAllocation;
                }
            }

            _nativeAllocationsChart.AddSample(nativeMemoryAllocation);
            _managedAllocationsChart.AddSample(managedMemoryAllocation);

            // Gather memory profiler stats for groups
            var frame = new FrameData
            {
                Usage = ProfilerMemory.GetGroups(0),
                Peek = ProfilerMemory.GetGroups(1),
                Count = ProfilerMemory.GetGroups(2),
            };
            if (_frames == null)
                _frames = new SamplesBuffer<FrameData>();
            if (_groupNames == null)
                _groupNames = ProfilerMemory.GetGroupNames();
            _frames.Add(frame);
        }

        /// <inheritdoc />
        public override void UpdateView(int selectedFrame, bool showOnlyLastUpdateEvents)
        {
            _nativeAllocationsChart.SelectedSampleIndex = selectedFrame;
            _managedAllocationsChart.SelectedSampleIndex = selectedFrame;

            UpdateTable(selectedFrame);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _tableRowsCache?.Clear();
            _groupNames = null;
            _groupOrder = null;

            base.OnDestroy();
        }

        private void UpdateTable(int selectedFrame)
        {
            if (_frames == null)
                return;
            if (_tableRowsCache == null)
                _tableRowsCache = new List<Row>();
            _table.IsLayoutLocked = true;

            RecycleTableRows(_table, _tableRowsCache);
            UpdateTableInner(selectedFrame);

            _table.UnlockChildrenRecursive();
            _table.PerformLayout();
        }

        private unsafe void UpdateTableInner(int selectedFrame)
        {
            if (_frames.Count == 0)
                return;
            if (_warningText != null)
                _warningText.Visible = true;
            var frame = _frames.Get(selectedFrame);
            var totalUage = frame.Usage.Values0[(int)ProfilerMemory.Groups.TotalTracked];
            var totalPeek = frame.Peek.Values0[(int)ProfilerMemory.Groups.TotalTracked];
            var totalCount = frame.Count.Values0[(int)ProfilerMemory.Groups.TotalTracked];

            // Sort by memory size
            if (_groupOrder == null)
                _groupOrder = new int[(int)ProfilerMemory.Groups.MAX];
            for (int i = 0; i < (int)ProfilerMemory.Groups.MAX; i++)
                _groupOrder[i] = i;
            Array.Sort(_groupOrder, (x, y) =>
            {
                var tmp = _frames.Get(selectedFrame);
                return tmp.Usage.Values0[y].CompareTo(tmp.Usage.Values0[x]);
            });

            // Add rows
            var rowColor2 = Style.Current.Background * 1.4f;
            for (int i = 0; i < (int)ProfilerMemory.Groups.MAX; i++)
            {
                var group = _groupOrder[i];
                var groupUsage = frame.Usage.Values0[group];
                if (groupUsage <= 0)
                    continue;
                var groupPeek = frame.Peek.Values0[group];
                var groupCount = frame.Count.Values0[group];

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
                        Values = new object[4],
                        BackgroundColors = new Color[4],
                    };
                }
                {
                    // Group
                    row.Values[0] = _groupNames[group];

                    // Usage
                    row.Values[1] = groupUsage;
                    row.BackgroundColors[1] = Color.Red.AlphaMultiplied(Mathf.Min(1, (float)groupUsage / totalUage) * 0.5f);

                    // Peek
                    row.Values[2] = groupPeek;
                    row.BackgroundColors[2] = Color.Red.AlphaMultiplied(Mathf.Min(1, (float)groupPeek / totalPeek) * 0.5f);

                    // Count
                    row.Values[3] = groupCount;
                    row.BackgroundColors[3] = Color.Red.AlphaMultiplied(Mathf.Min(1, (float)groupCount / totalCount) * 0.5f);
                }
                row.Width = _table.Width;
                row.BackgroundColor = i % 2 == 1 ? rowColor2 : Color.Transparent;
                row.Parent = _table;

                var useBackground = group != (int)ProfilerMemory.Groups.Total &&
                                    group != (int)ProfilerMemory.Groups.TotalTracked &&
                                    group != (int)ProfilerMemory.Groups.Malloc;
                if (!useBackground)
                {
                    for (int k = 1; k < row.BackgroundColors.Length; k++)
                        row.BackgroundColors[k] = Color.Transparent;
                }
            }
        }
    }
}
#endif
