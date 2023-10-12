// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.GUI;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEngine
{
    partial class ProfilingTools
    {
        partial struct NetworkEventStat
        {
            /// <summary>
            /// Gets the event name.
            /// </summary>
            public unsafe string Name
            {
                get
                {
                    fixed (byte* name = Name0)
                        return System.Runtime.InteropServices.Marshal.PtrToStringAnsi(new IntPtr(name));
                }
            }
        }
    }
}

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
        private readonly Table _tableRpc;
        private readonly Table _tableRep;
        private SamplesBuffer<ProfilingTools.NetworkEventStat[]> _events;
        private List<Row> _tableRowsCache;
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

            // Tables
            _tableRpc = InitTable(layout, "RPC Name");
            _tableRep = InitTable(layout, "Replication Name");
        }

        /// <inheritdoc />
        public override void Clear()
        {
            _dataSentChart.Clear();
            _dataReceivedChart.Clear();
            _events?.Clear();
        }

        /// <inheritdoc />
        public override void Update(ref SharedUpdateData sharedData)
        {
            // Gather peer stats
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

            // Gather network events
            var events = ProfilingTools.EventsNetwork;
            if (_events == null)
                _events = new SamplesBuffer<ProfilingTools.NetworkEventStat[]>();
            _events.Add(events);
        }

        /// <inheritdoc />
        public override void UpdateView(int selectedFrame, bool showOnlyLastUpdateEvents)
        {
            _dataSentChart.SelectedSampleIndex = selectedFrame;
            _dataReceivedChart.SelectedSampleIndex = selectedFrame;

            // Update events tables
            if (_events != null)
            {
                if (_tableRowsCache == null)
                    _tableRowsCache = new List<Row>();
                _tableRpc.IsLayoutLocked = true;
                _tableRep.IsLayoutLocked = true;
                RecycleTableRows(_tableRpc, _tableRowsCache);
                RecycleTableRows(_tableRep, _tableRowsCache);

                var events = _events.Get(selectedFrame);
                var rowCount = Int2.Zero;
                if (events != null && events.Length != 0)
                {
                    var rowColor2 = Style.Current.Background * 1.4f;
                    for (int i = 0; i < events.Length; i++)
                    {
                        var e = events[i];
                        var name = e.Name;
                        var isRpc = name.Contains("::", StringComparison.Ordinal);

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
                                Values = new object[5],
                            };
                        }
                        {
                            // Name
                            row.Values[0] = name;

                            // Count
                            row.Values[1] = (int)e.Count;

                            // Data Size
                            row.Values[2] = (int)e.DataSize;

                            // Message Size
                            row.Values[3] = (int)e.MessageSize;

                            // Receivers
                            row.Values[4] = (float)e.Receivers / (float)e.Count;
                        }

                        var table = isRpc ? _tableRpc : _tableRep;
                        row.Width = table.Width;
                        row.BackgroundColor = rowCount[isRpc ? 0 : 1] % 2 == 0 ? rowColor2 : Color.Transparent;
                        row.Parent = table;
                        if (isRpc)
                            rowCount.X++;
                        else
                            rowCount.Y++;
                    }
                }

                _tableRpc.Visible = rowCount.X != 0;
                _tableRep.Visible = rowCount.Y != 0;
                _tableRpc.Children.Sort(SortRows);
                _tableRep.Children.Sort(SortRows);

                _tableRpc.UnlockChildrenRecursive();
                _tableRpc.PerformLayout();
                _tableRep.UnlockChildrenRecursive();
                _tableRep.PerformLayout();
            }
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _tableRowsCache?.Clear();

            base.OnDestroy();
        }

        private static Table InitTable(ContainerControl parent, string name)
        {
            var headerColor = Style.Current.LightBackground;
            var table = new Table
            {
                Columns = new[]
                {
                    new ColumnDefinition
                    {
                        UseExpandCollapseMode = true,
                        CellAlignment = TextAlignment.Near,
                        Title = name,
                        TitleBackgroundColor = headerColor,
                    },
                    new ColumnDefinition
                    {
                        Title = "Count",
                        TitleBackgroundColor = headerColor,
                    },
                    new ColumnDefinition
                    {
                        Title = "Data Size",
                        TitleBackgroundColor = headerColor,
                        FormatValue = FormatCellBytes,
                    },
                    new ColumnDefinition
                    {
                        Title = "Message Size",
                        TitleBackgroundColor = headerColor,
                        FormatValue = FormatCellBytes,
                    },
                    new ColumnDefinition
                    {
                        Title = "Receivers",
                        TitleBackgroundColor = headerColor,
                    },
                },
                Splits = new[]
                {
                    0.40f,
                    0.15f,
                    0.15f,
                    0.15f,
                    0.15f,
                },
                Parent = parent,
            };
            return table;
        }

        private static string FormatSampleBytes(float v)
        {
            return Utilities.Utils.FormatBytesCount((ulong)v);
        }

        private static string FormatCellBytes(object x)
        {
            return Utilities.Utils.FormatBytesCount((int)x);
        }

        private static int SortRows(Control x, Control y)
        {
            if (x is Row xRow && y is Row yRow)
            {
                var xDataSize = (int)xRow.Values[2];
                var yDataSize = (int)yRow.Values[2];
                return yDataSize - xDataSize;
            }
            return 0;
        }
    }
}
