// Copyright (c) Wojciech Figat. All rights reserved.

#if USE_PROFILER
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
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
        private class Resource
        {
            public string Name;
            public string Tooltip;
            public GPUResourceType Type;
            public ulong MemoryUsage;
            public Guid AssetId;
            public bool IsAssetItem;
        }

        private readonly SingleChart _memoryUsageChart;
        private readonly Table _table;
        private SamplesBuffer<Resource[]> _resources;
        private List<ClickableRow> _tableRowsCache;
        private string[] _resourceTypesNames;
        private Dictionary<string, Guid> _assetPathToId;
        private Dictionary<Guid, Resource> _resourceCache;
        private StringBuilder _stringBuilder;

        public MemoryGPU()
        : base("GPU Memory")
        {
            // Layout
            var mainPanel = new Panel(ScrollBars.None)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                Parent = this,
            };
            
            // Chart
            _memoryUsageChart = new SingleChart
            {
                Title = "GPU Memory Usage",
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                Offsets = Margin.Zero,
                Height = SingleChart.DefaultHeight,
                FormatSample = v => Utilities.Utils.FormatBytesCount((ulong)v),
                Parent = mainPanel,
            };
            _memoryUsageChart.SelectedSampleChanged += OnSelectedSampleChanged;

            var panel = new Panel(ScrollBars.Vertical)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, _memoryUsageChart.Height + 2, 0),
                Parent = mainPanel,
            };
            var layout = new VerticalPanel
            {
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                Offsets = Margin.Zero,
                Pivot = Float2.Zero,
                IsScrollable = true,
                Parent = panel,
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
                        Title = "Resource",
                        TitleBackgroundColor = headerColor,
                        TitleColor = textColor,
                    },
                    new ColumnDefinition
                    {
                        Title = "Type",
                        CellAlignment = TextAlignment.Center,
                        TitleBackgroundColor = headerColor,
                        TitleColor = textColor,
                    },
                    new ColumnDefinition
                    {
                        Title = "Memory Usage",
                        TitleBackgroundColor = headerColor,
                        FormatValue = v => Utilities.Utils.FormatBytesCount((ulong)v),
                        TitleColor = textColor,
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
            if (_stringBuilder == null)
                _stringBuilder = new StringBuilder();

            // Capture current GPU resources usage info
            var contentDatabase = Editor.Instance.ContentDatabase;
            var gpuResources = GPUDevice.Instance.Resources;
            var resources = new Resource[gpuResources.Length];
            var sb = _stringBuilder;
            for (int i = 0; i < resources.Length; i++)
            {
                var gpuResource = gpuResources[i];
                ref var resource = ref resources[i];

                // Try to reuse cached resource info
                var gpuResourceId = gpuResource.ID;
                if (!_resourceCache.TryGetValue(gpuResourceId, out resource))
                {
                    resource = new Resource
                    {
#if !BUILD_RELEASE
                        Name = gpuResource.Name,
#endif
                        Type = gpuResource.ResourceType,
                    };
                    if (resource.Name == null)
                        resource.Name = string.Empty;

                    // Create tooltip
                    sb.Clear();
                    if (gpuResource is GPUTexture gpuTexture)
                    {
                        var desc = gpuTexture.Description;
                        sb.Append("Format: ").Append(desc.Format).AppendLine();
                        sb.Append("Size: ").Append(desc.Width).Append('x').Append(desc.Height);
                        if (desc.Depth != 1)
                            sb.Append('x').Append(desc.Depth);
                        if (desc.ArraySize != 1)
                            sb.Append('[').Append(desc.ArraySize).Append(']');
                        sb.AppendLine();
                        sb.Append("Mip Levels: ").Append(desc.MipLevels).AppendLine();
                        if (desc.IsMultiSample)
                            sb.Append("MSAA: ").Append('x').Append((int)desc.MultiSampleLevel).AppendLine();
                        sb.Append("Flags: ").Append(desc.Flags).AppendLine();
                        sb.Append("Usage: ").Append(desc.Usage);
                    }
                    else if (gpuResource is GPUBuffer gpuBuffer)
                    {
                        var desc = gpuBuffer.Description;
                        sb.Append("Format: ").Append(desc.Format).AppendLine();
                        sb.Append("Stride: ").Append(desc.Stride).AppendLine();
                        sb.Append("Elements: ").Append(desc.ElementsCount).AppendLine();
                        sb.Append("Flags: ").Append(desc.Flags).AppendLine();
                        sb.Append("Usage: ").Append(desc.Usage);
                    }
                    resource.Tooltip = _stringBuilder.ToString();

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
                        var assetItem = contentDatabase.FindAsset(resource.AssetId);
                        if (assetItem != null)
                        {
                            resource.IsAssetItem = true;
                            resource.Name = assetItem.NamePath + resource.Name.Substring(ext + 5); // Use text after asset path to display (eg. subobject)
                        }
                    }

                    _resourceCache.Add(gpuResourceId, resource);
                }

                resource.MemoryUsage = gpuResource.MemoryUsage;
                if (resource.MemoryUsage == 1)
                    resource.MemoryUsage = 0; // Sometimes GPU backend fakes memory usage as 1 to mark as allocated but not resided in actual GPU memory
            }
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
            _stringBuilder?.Clear();

            base.OnDestroy();
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
            var resourcesOrdered = resources.OrderByDescending(x => x.MemoryUsage);

            // Add rows
            var rowColor2 = Style.Current.Background * 1.4f;
            int rowIndex = 0;
            foreach (var e in resourcesOrdered)
            {
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
                row.Tag = e;
                row.TooltipText = e.Tooltip;
                row.RowDoubleClick = null;
                if (e.IsAssetItem)
                {
                    row.RowDoubleClick = OnRowDoubleClickAsset;
                }

                // Add row to the table
                row.Width = _table.Width;
                row.BackgroundColor = rowIndex % 2 == 1 ? rowColor2 : Color.Transparent;
                row.Parent = _table;
                rowIndex++;
            }
        }

        private void OnRowDoubleClickAsset(ClickableRow row)
        {
            var e = (Resource)row.Tag;
            var assetItem = Editor.Instance.ContentDatabase.FindAsset(e.AssetId);
            Editor.Instance.ContentEditing.Open(assetItem);
        }
    }
}
#endif
