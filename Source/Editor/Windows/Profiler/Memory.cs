// Copyright (c) Wojciech Figat. All rights reserved.

#if USE_PROFILER
using System;
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
        private readonly SingleChart _nativeAllocationsChart;
        private readonly SingleChart _managedAllocationsChart;

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
        }

        /// <inheritdoc />
        public override void Clear()
        {
            _nativeAllocationsChart.Clear();
            _managedAllocationsChart.Clear();
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
        }

        /// <inheritdoc />
        public override void UpdateView(int selectedFrame, bool showOnlyLastUpdateEvents)
        {
            _nativeAllocationsChart.SelectedSampleIndex = selectedFrame;
            _managedAllocationsChart.SelectedSampleIndex = selectedFrame;
        }
    }
}
#endif
