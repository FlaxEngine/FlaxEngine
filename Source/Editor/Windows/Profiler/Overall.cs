// Copyright (c) Wojciech Figat. All rights reserved.

#if USE_PROFILER
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows.Profiler
{
    /// <summary>
    /// The general profiling mode with major game performance charts and stats.
    /// </summary>
    /// <seealso cref="FlaxEditor.Windows.Profiler.ProfilerMode" />
    internal sealed class Overall : ProfilerMode
    {
        private readonly SingleChart _fpsChart;
        private readonly SingleChart _updateTimeChart;
        private readonly SingleChart _drawTimeCPUChart;
        private readonly SingleChart _drawTimeGPUChart;
        private readonly SingleChart _cpuMemChart;
        private readonly SingleChart _gpuMemChart;

        public Overall()
        : base("Overall")
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

            // Charts
            _fpsChart = new SingleChart
            {
                Title = "FPS",
                Parent = layout,
            };
            _fpsChart.SelectedSampleChanged += OnSelectedSampleChanged;
            _updateTimeChart = new SingleChart
            {
                Title = "Update Time",
                FormatSample = v => (Mathf.RoundToInt(v * 10.0f) / 10.0f) + " ms",
                Parent = layout,
            };
            _updateTimeChart.SelectedSampleChanged += OnSelectedSampleChanged;
            _drawTimeCPUChart = new SingleChart
            {
                Title = "Draw Time (CPU)",
                FormatSample = v => (Mathf.RoundToInt(v * 10.0f) / 10.0f) + " ms",
                Parent = layout,
            };
            _drawTimeCPUChart.SelectedSampleChanged += OnSelectedSampleChanged;
            _drawTimeGPUChart = new SingleChart
            {
                Title = "Draw Time (GPU)",
                FormatSample = v => (Mathf.RoundToInt(v * 10.0f) / 10.0f) + " ms",
                Parent = layout,
            };
            _drawTimeGPUChart.SelectedSampleChanged += OnSelectedSampleChanged;
            _cpuMemChart = new SingleChart
            {
                Title = "CPU Memory",
                FormatSample = v => ((int)v) + " MB",
                Parent = layout,
            };
            _cpuMemChart.SelectedSampleChanged += OnSelectedSampleChanged;
            _gpuMemChart = new SingleChart
            {
                Title = "GPU Memory",
                FormatSample = v => ((int)v) + " MB",
                Parent = layout,
            };
            _gpuMemChart.SelectedSampleChanged += OnSelectedSampleChanged;
        }

        /// <inheritdoc />
        public override void Clear()
        {
            _fpsChart.Clear();
            _updateTimeChart.Clear();
            _drawTimeCPUChart.Clear();
            _drawTimeGPUChart.Clear();
            _cpuMemChart.Clear();
            _gpuMemChart.Clear();
        }

        /// <inheritdoc />
        public override void Update(ref SharedUpdateData sharedData)
        {
            _fpsChart.AddSample(sharedData.Stats.FPS);
            _updateTimeChart.AddSample(sharedData.Stats.UpdateTimeMs);
            _drawTimeCPUChart.AddSample(sharedData.Stats.DrawCPUTimeMs);
            _drawTimeGPUChart.AddSample(sharedData.Stats.DrawGPUTimeMs);
            _cpuMemChart.AddSample(sharedData.Stats.ProcessMemory.UsedPhysicalMemory / 1024 / 1024);
            _gpuMemChart.AddSample(sharedData.Stats.MemoryGPU.Used / 1024 / 1024);
        }

        /// <inheritdoc />
        public override void UpdateView(int selectedFrame, bool showOnlyLastUpdateEvents)
        {
            _fpsChart.SelectedSampleIndex = selectedFrame;
            _updateTimeChart.SelectedSampleIndex = selectedFrame;
            _drawTimeCPUChart.SelectedSampleIndex = selectedFrame;
            _drawTimeGPUChart.SelectedSampleIndex = selectedFrame;
            _cpuMemChart.SelectedSampleIndex = selectedFrame;
            _gpuMemChart.SelectedSampleIndex = selectedFrame;
        }
    }
}
#endif
