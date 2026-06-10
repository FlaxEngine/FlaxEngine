// Copyright (c) Wojciech Figat. All rights reserved.

#if USE_PROFILER
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using FlaxEditor.CustomEditors;
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
        private enum ResourceTypes
        {
            Texture,
            Buffer,
            RenderTarget,
            DepthBuffer,
            VolumeTexture,
            CubeTexture,
            TextureArray,
            VertexBuffer,
            IndexBuffer,
            MAX
        }

        private class Resource
        {
            public string Name;
            public string Tooltip;
            public ResourceTypes Type;
            public ulong MemoryUsage;
            public Guid AssetId;
            public bool IsAssetItem;
            public GPUResource Reference;
        }

        [CustomEditor(typeof(Resource))]
        private sealed class ResourceEditor : CustomEditor
        {
            public override void Initialize(LayoutElementsContainer layout)
            {
                var resource = (Resource)Values[0];

                // Resource name
                var style = FlaxEngine.GUI.Style.Current;
                var nameSplit = resource.Name.LastIndexOf('/');
                var label = layout.Label(nameSplit == -1 ? resource.Name : resource.Name.Substring(nameSplit + 1), TextAlignment.Center).Label;
                label.AutoFitText = true;
                label.Font = new FontReference(style.FontLarge);

                var memoryUsage = Utilities.Utils.FormatBytesCount(resource.MemoryUsage);
                if (resource.Reference is GPUTexture texture && texture)
                {
                    // Texture preview
                    var desc = texture.Description;
                    var canSave = false;
                    // TODO: custom viewers for non-2d textures
                    if (desc.IsShaderResource && !desc.IsArray && !desc.IsVolume && !desc.IsCubeMap)
                    {
                        var image = layout.Image(texture);
                        image.Image.Size = new Float2(layout.Presenter.Panel.Width - Utilities.Constants.UIMargin * 2);
                        canSave = true;
                    }

                    // Texture info
                    layout.AddPropertyItem("Format").Label(desc.Format.ToString());
                    string size;
                    if (desc.IsVolume)
                        size = $"{desc.Width}x{desc.Height}x{desc.Depth}";
                    else
                        size = $"{desc.Width}x{desc.Height}";
                    if (desc.IsArray)
                        size += $"[{desc.ArraySize}]";
                    layout.AddPropertyItem("Size").Label(size);
                    var residentMipLevels = texture.ResidentMipLevels;
                    layout.AddPropertyItem("Mips").Label(residentMipLevels == desc.MipLevels ? desc.MipLevels.ToString() : $"{residentMipLevels} / {desc.MipLevels}");
                    if (desc.IsMultiSample)
                        layout.AddPropertyItem("MSAA").Label(desc.MultiSampleLevel.ToString());
                    layout.AddPropertyItem("Memory Size").Label(memoryUsage);
                    var asset = FlaxEngine.Content.LoadAsync(resource.AssetId) as TextureBase;
                    if (asset)
                    {
                        // Texture asset info
                        var path = asset.Path;
                        if (!asset.IsVirtual && File.Exists(path))
                            layout.AddPropertyItem("Disk Size").Label(Utilities.Utils.FormatBytesCount((ulong)new FileInfo(path).Length));
                        var textureGroup = asset.TextureGroup;
                        if (textureGroup >= 0)
                        {
                            var textureGroups = Streaming.TextureGroups;
                            if (textureGroup < textureGroups.Length)
                                layout.AddPropertyItem("Texture Group").Label(textureGroups[textureGroup].Name);
                        }
                        layout.AddPropertyItem("Refs").Label(asset.ReferencesCount.ToString());
                    }

                    if (canSave)
                    {
                        var buttonPanel = layout.HorizontalPanel();
                        buttonPanel.Panel.Size = new Float2(0, Button.DefaultHeight);
                        buttonPanel.Panel.Margin = Margin.Zero;
                        buttonPanel.Panel.Spacing = Utilities.Constants.UIMargin;
                        var button = buttonPanel.Button("Save", "Downloads the texture from the GPU and saves it to file inside project Screenshots folder");
                        button.Button.Width = 100;
                        button.Button.Clicked += OnSave;
                    }
                }
                else if (resource.Reference is GPUBuffer buffer && buffer)
                {
                    var desc = buffer.Description;

                    // Buffer info
                    layout.AddPropertyItem("Memory Usage").Label(memoryUsage);
                    layout.AddPropertyItem("Stride").Label($"{desc.Stride} bytes");
                    layout.AddPropertyItem("Elements").Label(desc.ElementsCount.ToString("###,###,###"));
                    if (desc.Format != PixelFormat.Unknown)
                        layout.AddPropertyItem("Format").Label(desc.Format.ToString());
                    layout.AddPropertyItem("Usage").Label(desc.Usage.ToString());
                    var asset = FlaxEngine.Content.LoadAsync(resource.AssetId) as ModelBase;
                    if (asset)
                    {
                        // Model asset info
                        layout.AddPropertyItem("Refs").Label(asset.ReferencesCount.ToString());
                    }
                    if (desc.VertexLayout)
                    {
                        var group = layout.Group("Vertex Layout");
                        var elements = desc.VertexLayout.Elements;
                        foreach (var e in elements)
                            group.Label($" > {e.Type}, {e.Format} ({PixelFormatExtensions.SizeInBytes(e.Format)} bytes), offset {e.Offset}").Label.Height = 14;
                    }
                }
                else
                {
                    // Unknown resource or broken reference (eg. object deleted)
                    layout.Label("Memory Usage: " + memoryUsage);
                    label = layout.Label(resource.Tooltip).Label;
                    label.AutoHeight = true;
                }
            }

            private void OnSave()
            {
                var resource = (Resource)Values[0];
                if (resource.Reference is GPUTexture texture && texture)
                {
                    Screenshot.Capture(texture);
                }
            }
        }

        private readonly SingleChart _memoryUsageChart;
        private readonly Table _table;
        private readonly Panel _tablePanel;
        private readonly Panel _resourcePanel;
        private readonly CustomEditorPresenter _resourceProperties;
        private SamplesBuffer<Resource[]> _resources;
        private List<ClickableRow> _tableRowsCache;
        private string[] _resourceTypesNames;
        private Dictionary<string, Guid> _assetPathToId;
        private Dictionary<Guid, Resource> _resourceCache;
        private List<Resource> _resourceList;
        private StringBuilder _stringBuilder;
        private GPUResource[] _gpuResourcesCached;

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
            var chartsBottom = _memoryUsageChart.Height + Utilities.Constants.UIMargin;

            // Selected resource info
            _resourcePanel = new Panel(ScrollBars.Vertical)
            {
                AnchorPreset = AnchorPresets.VerticalStretchRight,
                Offsets = new Margin(0, 0, chartsBottom, 0),
                Visible = false,
                Parent = mainPanel,
            };
            _resourceProperties = new CustomEditorPresenter();
            _resourceProperties.Panel.Parent = _resourcePanel;

            // Table panel
            _tablePanel = new Panel(ScrollBars.Vertical)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, chartsBottom, 0),
                Parent = mainPanel,
            };
            var layout = new VerticalPanel
            {
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                Offsets = Margin.Zero,
                Pivot = Float2.Zero,
                IsScrollable = true,
                Parent = _tablePanel,
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
            _resourceList?.Clear();
        }

        /// <inheritdoc />
        public override void Update(ref SharedUpdateData sharedData)
        {
            _memoryUsageChart.AddSample(sharedData.Stats.MemoryGPU.Used);

            // Lazy-init cache data
            if (_resourceCache == null)
                _resourceCache = new Dictionary<Guid, Resource>();
            if (_resourceList == null)
                _resourceList = new List<Resource>();
            if (_assetPathToId == null)
                _assetPathToId = new Dictionary<string, Guid>();
            if (_stringBuilder == null)
                _stringBuilder = new StringBuilder();

            // Capture current GPU resources usage info
            var contentDatabase = Editor.Instance.ContentDatabase;
            GPUDevice.Instance.GetResources(ref _gpuResourcesCached, out var count);
            var sb = _stringBuilder;
            _resourceList.Clear();
            _resourceList.EnsureCapacity(count);
            for (int i = 0; i < count; i++)
            {
                var gpuResource = _gpuResourcesCached[i];
                if (!gpuResource || gpuResource.MemoryUsage < 100) // Skip invalid, unallocated or very small resources
                    continue;

                // Try to reuse cached resource info
                var gpuResourceId = gpuResource.ID;
                if (!_resourceCache.TryGetValue(gpuResourceId, out var resource))
                {
                    // Create tooltip
                    sb.Clear();
                    ResourceTypes type;
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
                        if (desc.Flags.HasFlag(GPUTextureFlags.RenderTarget))
                            type = ResourceTypes.RenderTarget;
                        else if (desc.Flags.HasFlag(GPUTextureFlags.DepthStencil))
                            type = ResourceTypes.DepthBuffer;
                        else if (desc.IsVolume)
                            type = ResourceTypes.VolumeTexture;
                        else if (desc.IsCubeMap)
                            type = ResourceTypes.CubeTexture;
                        else if (desc.IsArray)
                            type = ResourceTypes.TextureArray;
                        else
                            type = ResourceTypes.Texture;
                    }
                    else if (gpuResource is GPUBuffer gpuBuffer)
                    {
                        var desc = gpuBuffer.Description;
                        sb.Append("Format: ").Append(desc.Format).AppendLine();
                        sb.Append("Stride: ").Append(desc.Stride).AppendLine();
                        sb.Append("Elements: ").Append(desc.ElementsCount).AppendLine();
                        sb.Append("Flags: ").Append(desc.Flags).AppendLine();
                        sb.Append("Usage: ").Append(desc.Usage);
                        if (desc.Flags.HasFlag(GPUBufferFlags.VertexBuffer))
                            type = ResourceTypes.VertexBuffer;
                        else if (desc.Flags.HasFlag(GPUBufferFlags.IndexBuffer))
                            type = ResourceTypes.IndexBuffer;
                        else
                            type = ResourceTypes.Buffer;
                    }
                    else
                    {
                        // Ignore internal resources (not useful for user)
                        continue;
                    }
                    resource = new Resource
                    {
#if !BUILD_RELEASE
                        Name = gpuResource.Name ?? string.Empty,
#else
                        Name = string.Empty,
#endif
                        Tooltip = _stringBuilder.ToString(),
                        Type = type,
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
                resource.Reference = gpuResource;
                _resourceList.Add(resource);
            }
            if (_resources == null)
                _resources = new SamplesBuffer<Resource[]>();
            _resources.Add(_resourceList.ToArray());
            Array.Clear(_gpuResourcesCached);
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (base.OnKeyDown(key))
                return true;

            // Input control over resource table
            if (_table.ContainsFocus)
            {
                var selectedRow = GetSelectedRow();
                switch (key)
                {
                case KeyboardKeys.Return:
                    // Open selected resource
                    if (selectedRow != null && selectedRow.RowDoubleClick != null)
                    {
                        selectedRow.RowDoubleClick(selectedRow);
                    }
                    break;
                case KeyboardKeys.Escape:
                    // Deselect all rows
                    if (selectedRow != null)
                    {
                        selectedRow.IsSelected = false;
                        ShowResourcePanel(false);
                        _resourceProperties.Deselect();
                    }
                    break;
                case KeyboardKeys.ArrowUp:
                    // Select the previous row
                    if (selectedRow != null)
                    {
                        var prevIndex = _table.Children.IndexOf(selectedRow) - 1;
                        if (prevIndex >= 0 && _table.Children[prevIndex] is ClickableRow prevRow)
                        {
                            selectedRow.IsSelected = false;
                            selectedRow = prevRow;
                            selectedRow.IsSelected = true;
                            selectedRow.Focus();
                            _tablePanel.ScrollViewTo(selectedRow);
                            var e = (Resource)selectedRow.Tag;
                            _resourceProperties.Select(e);
                        }
                    }
                    break;
                case KeyboardKeys.ArrowDown:
                    // Select the next row
                    if (selectedRow != null)
                    {
                        var nextIndex = _table.Children.IndexOf(selectedRow) + 1;
                        if (nextIndex < _table.Children.Count && _table.Children[nextIndex] is ClickableRow nextRow)
                        {
                            selectedRow.IsSelected = false;
                            selectedRow = nextRow;
                            selectedRow.IsSelected = true;
                            selectedRow.Focus();
                            _tablePanel.ScrollViewTo(selectedRow);
                            var e = (Resource)selectedRow.Tag;
                            _resourceProperties.Select(e);
                        }
                    }
                    break;
                }
            }

            return false;
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
            {
                _resourceTypesNames = new string[(int)ResourceTypes.MAX];
                for (int i = 0; i < _resourceTypesNames.Length; i++)
                    _resourceTypesNames[i] = ((ResourceTypes)i).ToString();
            }
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
            _gpuResourcesCached = null;

            base.OnDestroy();
        }

        private ClickableRow GetSelectedRow()
        {
            foreach (var child in _table.Children)
            {
                if (child is ClickableRow row && row.IsSelected)
                    return row;
            }
            return null;
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
                    if (row.IsSelected)
                    {
                        row.IsSelected = false;
                        ShowResourcePanel(false);
                        _resourceProperties.Deselect();
                    }
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
            var resourcesOrdered = resources.OrderByDescending(x => x?.MemoryUsage ?? 0);

            // Add rows
            var rowColor2 = Style.Current.Background * 1.4f;
            int rowIndex = 0;
            foreach (var e in resourcesOrdered)
            {
                if (e == null)
                    continue;
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
                    row = new ClickableRow
                    {
                        Values = new object[3],
                        RowLeftClick = OnRowLeftClick
                    };
                }

                // Setup row data
                row.Values[0] = e.Name;
                row.Values[1] = _resourceTypesNames[(int)e.Type];
                row.Values[2] = e.MemoryUsage;

                // Setup row interactions
                row.Tag = e;
                row.TooltipText = e.Tooltip;
                row.RowDoubleClick = e.IsAssetItem ? OnRowDoubleClickAsset : null;

                // Add row to the table
                row.Width = _table.Width;
                row.BackgroundColor = rowIndex % 2 == 1 ? rowColor2 : Color.Transparent;
                row.Parent = _table;
                rowIndex++;
            }
        }

        private void OnRowLeftClick(ClickableRow row)
        {
            if (!row.IsSelected)
            {
                // Deselect all other rows
                foreach (var child in _table.Children)
                {
                    if (child is ClickableRow r)
                        r.IsSelected = false;
                }
            }
            row.IsSelected = !row.IsSelected;
            ShowResourcePanel(row.IsSelected);
            row.Focus();
            var e = (Resource)row.Tag;
            _resourceProperties.Select(e);
        }

        private void OnRowDoubleClickAsset(ClickableRow row)
        {
            var e = (Resource)row.Tag;
            var assetItem = Editor.Instance.ContentDatabase.FindAsset(e.AssetId);
            if (assetItem != null)
                Editor.Instance.ContentEditing.Open(assetItem);
        }

        private void ShowResourcePanel(bool visible = true)
        {
            _resourcePanel.Visible = visible;
            var parentSize = _resourcePanel.Parent.Size;
            var split = visible ? parentSize.X * 0.3f : 0;
            var chartsBottom = _memoryUsageChart.Height + Utilities.Constants.UIMargin;
            _resourcePanel.Bounds = new Rectangle(parentSize.X - split, chartsBottom, split, parentSize.Y - chartsBottom);
            var offsets = _tablePanel.Offsets;
            offsets.Right = visible ? split + Utilities.Constants.UIMargin : 0;
            _tablePanel.Offsets = offsets;
        }
    }
}
#endif
