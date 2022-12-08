// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.GUI;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows.Profiler
{
    /// <summary>
    /// The GPU Memory profiling mode.
    /// </summary>
    /// <seealso cref="FlaxEditor.Windows.Profiler.ProfilerMode" />
    internal sealed class MemoryGPU : ProfilerMode
    {
        private struct Resource
        {
            public string Name;
            public GPUResourceType Type;
            public ulong MemoryUsage;
            public Guid AssetId;
        }

        private readonly SingleChart _memoryUsageChart;
        private readonly Table _table;
        private SamplesBuffer<Resource[]> _resources;
        private List<ClickableRow> _tableRowsCache;
        private string[] _resourceTypesNames;
        private Dictionary<string, Guid> _assetPathToId;
        private Dictionary<Guid, Resource> _resourceCache;

        public MemoryGPU()
        : base("GPU Memory")
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
            _memoryUsageChart = new SingleChart
            {
                Title = "GPU Memory Usage",
                FormatSample = v => Utilities.Utils.FormatBytesCount((int)v),
                Parent = layout,
            };
            _memoryUsageChart.SelectedSampleChanged += OnSelectedSampleChanged;

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
                        Title = "Resource",
                        TitleBackgroundColor = headerColor,
                    },
                    new ColumnDefinition
                    {
                        Title = "Type",
                        CellAlignment = TextAlignment.Center,
                        TitleBackgroundColor = headerColor,
                    },
                    new ColumnDefinition
                    {
                        Title = "Memory Usage",
                        TitleBackgroundColor = headerColor,
                        FormatValue = v => Utilities.Utils.FormatBytesCount((ulong)v),
                    },
                },
                Parent = layout,
            };
            _table.Splits = new[]
            {
                0.6f,
                0.2f,
                0.2f,
            };
        }

        /// <inheritdoc />
        public override void Clear()
        {
            _memoryUsageChart.Clear();
            _resources?.Clear();
            _assetPathToId?.Clear();
            _resourceCache?.Clear();
        }

        /// <inheritdoc />
        public override void Update(ref SharedUpdateData sharedData)
        {
            _memoryUsageChart.AddSample(sharedData.Stats.MemoryGPU.Used);

            if (_resourceCache == null)
                _resourceCache = new Dictionary<Guid, Resource>();
            if (_assetPathToId == null)
                _assetPathToId = new Dictionary<string, Guid>();

            // Capture current GPU resources usage info
            var gpuResources = GPUDevice.Instance.Resources;
            var resources = new Resource[gpuResources.Length];
            for (int i = 0; i < resources.Length; i++)
            {
                var gpuResource = gpuResources[i];

                // Try to reuse cached resource info
                var gpuResourceId = gpuResource.ID;
                if (!_resourceCache.TryGetValue(gpuResourceId, out var resource))
                {
                    resource = new Resource
                    {
                        Name = gpuResource.Name,
                        Type = gpuResource.ResourceType,
                    };

                    // Detect asset path in the resource name
                    int ext = resource.Name.LastIndexOf(".flax", StringComparison.OrdinalIgnoreCase);
                    if (ext != -1)
                    {
                        var assetPath = resource.Name.Substring(0, ext + 5);
                        if (!_assetPathToId.TryGetValue(assetPath, out resource.AssetId))
                        {
                            var asset = FlaxEngine.Content.GetAsset(assetPath);
                            if (asset != null)
                                resource.AssetId = asset.ID;
                            _assetPathToId.Add(assetPath, resource.AssetId);
                        }
                    }

                    _resourceCache.Add(gpuResourceId, resource);
                }

                resource.MemoryUsage = gpuResource.MemoryUsage;
                if (resource.MemoryUsage == 1)
                    resource.MemoryUsage = 0; // Sometimes GPU backend fakes memory usage as 1 to mark as allocated but not resided in actual GPU memory
                resources[i] = resource;
            }
            Array.Sort(resources, SortResources);
            if (_resources == null)
                _resources = new SamplesBuffer<Resource[]>();
            _resources.Add(resources);
        }

        /// <inheritdoc />
        public override void UpdateView(int selectedFrame, bool showOnlyLastUpdateEvents)
        {
            _memoryUsageChart.SelectedSampleIndex = selectedFrame;

            if (_resources == null)
                return;
            if (_tableRowsCache == null)
                _tableRowsCache = new List<ClickableRow>();
            if (_resourceTypesNames == null)
                _resourceTypesNames = new string[(int)GPUResourceType.MAX]
                {
                    "Render Target",
                    "Texture",
                    "Cube Texture",
                    "Volume Texture",
                    "Buffer",
                    "Shader",
                    "Pipeline State",
                    "Descriptor",
                    "Query",
                    "Sampler",
                };
            UpdateTable();
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _resources?.Clear();
            _resourceCache?.Clear();
            _assetPathToId?.Clear();
            _tableRowsCache?.Clear();

            base.OnDestroy();
        }

        private static int SortResources(Resource a, Resource b)
        {
            return (int)(b.MemoryUsage - a.MemoryUsage);
        }

        private void UpdateTable()
        {
            _table.IsLayoutLocked = true;
            int idx = 0;
            while (_table.Children.Count > idx)
            {
                var child = _table.Children[idx];
                if (child is ClickableRow row)
                {
                    _tableRowsCache.Add(row);
                    child.Parent = null;
                }
                else
                {
                    idx++;
                }
            }
            _table.LockChildrenRecursive();

            UpdateTableInner();

            _table.UnlockChildrenRecursive();
            _table.PerformLayout();
        }

        private void UpdateTableInner()
        {
            if (_resources.Count == 0)
                return;
            var resources = _resources.Get(_memoryUsageChart.SelectedSampleIndex);
            if (resources == null || resources.Length == 0)
                return;

            // Add rows
            var rowColor2 = Style.Current.Background * 1.4f;
            var contentDatabase = Editor.Instance.ContentDatabase;
            for (int i = 0; i < resources.Length; i++)
            {
                ref var e = ref resources[i];

                ClickableRow row;
                if (_tableRowsCache.Count != 0)
                {
                    // Reuse row
                    var last = _tableRowsCache.Count - 1;
                    row = _tableRowsCache[last];
                    _tableRowsCache.RemoveAt(last);
                }
                else
                {
                    // Allocate new row
                    row = new ClickableRow { Values = new object[3] };
                }

                // Setup row data
                row.Values[0] = e.Name;
                row.Values[1] = _resourceTypesNames[(int)e.Type];
                row.Values[2] = e.MemoryUsage;

                // Setup row interactions
                row.TooltipText = null;
                row.DoubleClick = null;
                var assetItem = contentDatabase.FindAsset(e.AssetId);
                if (assetItem != null)
                {
                    row.Values[0] = assetItem.NamePath;
                    assetItem.UpdateTooltipText();
                    row.TooltipText = assetItem.TooltipText;
                    row.DoubleClick = () => { Editor.Instance.ContentEditing.Open(assetItem); };
                }

                // Add row to the table
                row.Width = _table.Width;
                row.BackgroundColor = i % 2 == 0 ? rowColor2 : Color.Transparent;
                row.Parent = _table;
            }
        }
    }
}
