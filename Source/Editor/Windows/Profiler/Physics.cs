// Copyright (c) Wojciech Figat. All rights reserved.

#if USE_PROFILER
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows.Profiler
{
    /// <summary>
    /// The physics simulation profiling mode.
    /// </summary>
    /// <seealso cref="FlaxEditor.Windows.Profiler.ProfilerMode" />
    internal sealed class Physics : ProfilerMode
    {
        private readonly SingleChart _activeBodiesChart;
        private readonly SingleChart _activeJointsChart;
        private readonly SingleChart _dynamicBodiesChart;
        private readonly SingleChart _staticBodiesChart;
        private readonly SingleChart _newPairsChart;
        private readonly SingleChart _newTouchesChart;

        public Physics()
        : base("Physics")
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
            _activeBodiesChart = new SingleChart
            {
                Title = "Active Bodies",
                Parent = layout,
            };
            _activeBodiesChart.SelectedSampleChanged += OnSelectedSampleChanged;
            _activeJointsChart = new SingleChart
            {
                Title = "Active Joints",
                Parent = layout,
            };
            _activeJointsChart.SelectedSampleChanged += OnSelectedSampleChanged;
            _dynamicBodiesChart = new SingleChart
            {
                Title = "Dynamic Bodies",
                Parent = layout,
            };
            _dynamicBodiesChart.SelectedSampleChanged += OnSelectedSampleChanged;
            _staticBodiesChart = new SingleChart
            {
                Title = "Static Bodies",
                Parent = layout,
            };
            _staticBodiesChart.SelectedSampleChanged += OnSelectedSampleChanged;
            _newPairsChart = new SingleChart
            {
                Title = "New Pairs",
                Parent = layout,
            };
            _newPairsChart.SelectedSampleChanged += OnSelectedSampleChanged;
            _newTouchesChart = new SingleChart
            {
                Title = "New Touches",
                Parent = layout,
            };
            _newTouchesChart.SelectedSampleChanged += OnSelectedSampleChanged;
        }

        /// <inheritdoc />
        public override void Clear()
        {
            _activeBodiesChart.Clear();
            _activeJointsChart.Clear();
            _dynamicBodiesChart.Clear();
            _staticBodiesChart.Clear();
            _newPairsChart.Clear();
            _newTouchesChart.Clear();
        }

        /// <inheritdoc />
        public override void Update(ref SharedUpdateData sharedData)
        {
            PhysicsStatistics statistics = FlaxEngine.Physics.DefaultScene.Statistics;

            _activeBodiesChart.AddSample(statistics.ActiveDynamicBodies + statistics.ActiveKinematicBodies);
            _activeJointsChart.AddSample(statistics.ActiveJoints);
            _dynamicBodiesChart.AddSample(statistics.DynamicBodies + statistics.KinematicBodies);
            _staticBodiesChart.AddSample(statistics.StaticBodies);
            _newPairsChart.AddSample(statistics.NewPairs);
            _newTouchesChart.AddSample(statistics.NewTouches);
        }

        /// <inheritdoc />
        public override void UpdateView(int selectedFrame, bool showOnlyLastUpdateEvents)
        {
            _activeBodiesChart.SelectedSampleIndex = selectedFrame;
            _activeJointsChart.SelectedSampleIndex = selectedFrame;
            _dynamicBodiesChart.SelectedSampleIndex = selectedFrame;
            _staticBodiesChart.SelectedSampleIndex = selectedFrame;
            _newPairsChart.SelectedSampleIndex = selectedFrame;
            _newTouchesChart.SelectedSampleIndex = selectedFrame;
        }
    }
}
#endif
