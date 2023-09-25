// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows.Profiler
{
    /// <summary>
    /// The network profiling mode.
    /// </summary>
    /// <seealso cref="FlaxEditor.Windows.Profiler.ProfilerMode" />
    internal sealed class Network : ProfilerMode
    {
        private readonly SingleChart _dataSentChart;
        private readonly SingleChart _dataReceivedChart;
        private FlaxEngine.Networking.NetworkDriverStats _prevStats;

        public Network()
        : base("Network")
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

            // Charts
            _dataSentChart = new SingleChart
            {
                Title = "Data Sent",
                FormatSample = FormatSampleBytes,
                Parent = layout,
            };
            _dataSentChart.SelectedSampleChanged += OnSelectedSampleChanged;
            _dataReceivedChart = new SingleChart
            {
                Title = "Data Received",
                FormatSample = FormatSampleBytes,
                Parent = layout,
            };
            _dataReceivedChart.SelectedSampleChanged += OnSelectedSampleChanged;
        }

        private static string FormatSampleBytes(float v)
        {
            return Utilities.Utils.FormatBytesCount((ulong)v);
        }

        /// <inheritdoc />
        public override void Clear()
        {
            _dataSentChart.Clear();
            _dataReceivedChart.Clear();
        }

        /// <inheritdoc />
        public override void Update(ref SharedUpdateData sharedData)
        {
            var peers = FlaxEngine.Networking.NetworkPeer.Peers;
            var stats = new FlaxEngine.Networking.NetworkDriverStats();
            foreach (var peer in peers)
            {
                var peerStats = peer.NetworkDriver.GetStats();
                stats.TotalDataSent += peerStats.TotalDataSent;
                stats.TotalDataReceived += peerStats.TotalDataReceived;
            }
            _dataSentChart.AddSample(Mathf.Max((long)stats.TotalDataSent - (long)_prevStats.TotalDataSent, 0));
            _dataReceivedChart.AddSample(Mathf.Max((long)stats.TotalDataReceived - (long)_prevStats.TotalDataReceived, 0));
            _prevStats = stats;
        }

        /// <inheritdoc />
        public override void UpdateView(int selectedFrame, bool showOnlyLastUpdateEvents)
        {
            _dataSentChart.SelectedSampleIndex = selectedFrame;
            _dataReceivedChart.SelectedSampleIndex = selectedFrame;
        }
    }
}
