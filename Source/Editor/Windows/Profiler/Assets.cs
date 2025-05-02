// Copyright (c) Wojciech Figat. All rights reserved.

#if USE_PROFILER
using System;
using System.Collections.Generic;
using System.Text;
using FlaxEditor.GUI;
using FlaxEngine;
using FlaxEngine.Json;
using FlaxEngine.GUI;
using FlaxEditor.GUI.ContextMenu;
using System.Linq;

namespace FlaxEditor.Windows.Profiler
{
    /// <summary>
    /// The Assets profiling mode.
    /// </summary>
    /// <seealso cref="FlaxEditor.Windows.Profiler.ProfilerMode" />
    internal sealed class Assets : ProfilerMode
    {
        private class Resource
        {
            public string Name;
            public string TypeName;
            public int ReferencesCount;
            public ulong MemoryUsage;
            public Guid AssetId;
        }

        private readonly SingleChart _memoryUsageChart;
        private readonly Table _table;
        private SamplesBuffer<Resource[]> _resources;
        private List<ClickableRow> _tableRowsCache;
        private Dictionary<Guid, Resource> _resourceCache;
        private StringBuilder _stringBuilder;

        public Assets()
        : base("Assets")
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
                Title = "Assets Memory Usage (CPU)",
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
                        Title = "References",
                        TitleBackgroundColor = headerColor,
                        TitleColor = textColor,
                    },
                    new ColumnDefinition
                    {
                        Title = "Memory Usage",
                        TitleBackgroundColor = headerColor,
                        TitleColor = textColor,
                        FormatValue = v => Utilities.Utils.FormatBytesCount((ulong)v),
                    },
                },
                Parent = layout,
            };
            _table.Splits = new[]
            {
                0.6f,
                0.2f,
                0.08f,
                0.12f,
            };
        }

        /// <inheritdoc />
        public override void Clear()
        {
            _memoryUsageChart.Clear();
            _resources?.Clear();
            _resourceCache?.Clear();
        }

        /// <inheritdoc />
        public override void Update(ref SharedUpdateData sharedData)
        {
            if (_resourceCache == null)
                _resourceCache = new Dictionary<Guid, Resource>();
            if (_stringBuilder == null)
                _stringBuilder = new StringBuilder();

            // Capture current assets usage info
            var assets = FlaxEngine.Content.Assets;
            var resources = new Resource[assets.Length];
            ulong totalMemoryUsage = 0;
            for (int i = 0; i < resources.Length; i++)
            {
                var asset = assets[i];
                ref var resource = ref resources[i];
                if (!asset)
                    continue;

                // Try to reuse cached resource info
                var assetId = asset.ID;
                if (!_resourceCache.TryGetValue(assetId, out resource))
                {
                    resource = new Resource
                    {
                        Name = asset.Path,
                        TypeName = asset.TypeName,
                        AssetId = assetId,
                    };
                    var typeNameEnding = asset.TypeName.LastIndexOf('.');
                    if (typeNameEnding != -1)
                        resource.TypeName = resource.TypeName.Substring(typeNameEnding + 1);
                    var assetItem = Editor.Instance.ContentDatabase.FindAsset(assetId);
                    if (assetItem != null)
                    {
                        resource.Name = assetItem.NamePath;
                    }
                    if (string.IsNullOrEmpty(resource.Name) && asset.IsVirtual)
                        resource.Name = "<virtual>";
                    _resourceCache.Add(assetId, resource);
                }

                resource.MemoryUsage = asset.MemoryUsage;
                resource.ReferencesCount = asset.ReferencesCount;
                totalMemoryUsage += resource.MemoryUsage;
            }
            _memoryUsageChart.AddSample((float)totalMemoryUsage);
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
            UpdateTable();
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _resources?.Clear();
            _resourceCache?.Clear();
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
                    row = new ClickableRow { Values = new object[4] };
                    row.RowDoubleClick = OnRowDoubleClick;
                    row.RowRightClick = OnRowRightClick;
                }

                // Setup row data
                row.Tag = e.AssetId;
                row.Values[0] = e.Name;
                row.Values[1] = e.TypeName;
                row.Values[2] = e.ReferencesCount;
                row.Values[3] = e.MemoryUsage;

                // Add row to the table
                row.Width = _table.Width;
                row.BackgroundColor = rowIndex % 2 == 1 ? rowColor2 : Color.Transparent;
                row.Parent = _table;
                rowIndex++;
            }
        }

        private void OnRowDoubleClick(ClickableRow row)
        {
            var assetId = (Guid)row.Tag;
            var assetItem = Editor.Instance.ContentDatabase.FindAsset(assetId);
            if (assetItem != null)
                Editor.Instance.ContentEditing.Open(assetItem);
        }

        private void OnRowRightClick(ClickableRow row)
        {
            var assetId = (Guid)row.Tag;
            var assetItem = Editor.Instance.ContentDatabase.FindAsset(assetId);
            if (assetItem != null)
            {
                var cm = new ContextMenu();
                ContextMenuButton b;
                b = cm.AddButton("Open", () => Editor.Instance.ContentEditing.Open(assetItem));
                cm.AddButton("Show in content window", () => Editor.Instance.Windows.ContentWin.Select(assetItem));
                cm.AddButton(Utilities.Constants.ShowInExplorer, () => FileSystem.ShowFileExplorer(System.IO.Path.GetDirectoryName(assetItem.Path)));
                cm.AddButton("Select actors using this asset", () => Editor.Instance.SceneEditing.SelectActorsUsingAsset(assetItem.ID));
                cm.AddButton("Show asset references graph", () => Editor.Instance.Windows.Open(new AssetReferencesGraphWindow(Editor.Instance, assetItem)));
                cm.AddButton("Copy name", () => Clipboard.Text = assetItem.NamePath);
                cm.AddButton("Copy path", () => Clipboard.Text = assetItem.Path);
                cm.AddButton("Copy asset ID", () => Clipboard.Text = JsonSerializer.GetStringID(assetItem.ID));
                cm.Show(row, row.PointFromScreen(Input.MouseScreenPosition));
            }
        }
    }
}
#endif
