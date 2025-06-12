// Copyright (c) Wojciech Figat. All rights reserved.

using System.Linq;
using System.Threading.Tasks;
using FlaxEditor.Content;
using FlaxEditor.Content.Import;
using FlaxEditor.CustomEditors;
using FlaxEditor.GUI;
using FlaxEditor.Viewport.Cameras;
using FlaxEditor.Viewport.Previews;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Json;
using FlaxEngine.Tools;
using Object = FlaxEngine.Object;

namespace FlaxEditor.Windows.Assets
{
    /// <summary>
    /// Editor window to view/modify <see cref="Model"/> asset.
    /// </summary>
    /// <seealso cref="Model" />
    /// <seealso cref="FlaxEditor.Windows.Assets.AssetEditorWindow" />
    public sealed class ModelWindow : ModelBaseWindow<Model, ModelWindow>
    {
        private sealed class Preview : ModelPreview
        {
            private readonly ModelWindow _window;

            public Preview(ModelWindow window)
            : base(true)
            {
                _window = window;

                // Enable shadows
                PreviewLight.ShadowsMode = ShadowsCastingMode.All;
                PreviewLight.CascadeCount = 3;
                PreviewLight.ShadowsDistance = 2000.0f;
                Task.ViewFlags |= ViewFlags.Shadows;
            }

            public override void Draw()
            {
                base.Draw();

                var style = Style.Current;
                var asset = _window.Asset;
                if (asset == null || !asset.IsLoaded)
                {
                    Render2D.DrawText(style.FontLarge, asset != null && asset.LastLoadFailed ? "Failed to load" : "Loading...", new Rectangle(Float2.Zero, Size), style.ForegroundDisabled, TextAlignment.Center, TextAlignment.Center);
                }
            }
        }

        [CustomEditor(typeof(MeshesPropertiesProxy.ProxyEditor))]
        private sealed class MeshesPropertiesProxy : MeshesPropertiesProxyBase
        {
            private CustomEditors.Elements.IntegerValueElement _sdfModelLodIndex;
            private CustomEditorPresenter _presenter;

            protected override void OnGeneral(LayoutElementsContainer layout)
            {
                base.OnGeneral(layout);

                _presenter = layout.Presenter;

                // SDF
                {
                    var group = layout.Group("SDF");
                    var sdfOptions = Window._sdfOptions;

                    var sdf = Asset.SDF;
                    if (sdf.Texture != null)
                    {
                        var size = sdf.Texture.Size3;
                        group.Label($"SDF Texture {size.X}x{size.Y}x{size.Z} ({Utilities.Utils.FormatBytesCount(sdf.Texture.MemoryUsage)})").AddCopyContextMenu();
                    }
                    else
                    {
                        group.Label("No SDF");
                    }

                    var resolution = group.FloatValue("Resolution Scale", Window.Editor.CodeDocs.GetTooltip(typeof(ModelTool.Options), nameof(ModelImportSettings.Settings.SDFResolution)));
                    resolution.ValueBox.SlideSpeed = 0.001f;
                    resolution.ValueBox.MinValue = 0.0001f;
                    resolution.ValueBox.MaxValue = 100.0f;
                    resolution.ValueBox.Value = sdf.Texture != null ? sdf.ResolutionScale : 1.0f;
                    resolution.ValueBox.BoxValueChanged += b => { Window._importSettings.Settings.SDFResolution = b.Value; };
                    Window._importSettings.Settings.SDFResolution = sdf.ResolutionScale;

                    var gpu = group.Checkbox("Bake on GPU", "If checked, SDF generation will be calculated using GPU on Compute Shader, otherwise CPU will use Job System. GPU generation is fast but result in artifacts in various meshes (eg. foliage).");
                    gpu.CheckBox.Checked = sdfOptions.GPU;

                    var backfacesThresholdProp = group.AddPropertyItem("Backfaces Threshold", "Custom threshold (in range 0-1) for adjusting mesh internals detection based on the percentage of test rays hit triangle backfaces. Use lower value for more dense mesh.");
                    var backfacesThreshold = backfacesThresholdProp.FloatValue();
                    var backfacesThresholdLabel = backfacesThresholdProp.Labels.Last();
                    backfacesThreshold.ValueBox.MinValue = 0.001f;
                    backfacesThreshold.ValueBox.MaxValue = 1.0f;
                    backfacesThreshold.ValueBox.Value = sdfOptions.BackfacesThreshold;
                    backfacesThreshold.ValueBox.BoxValueChanged += b => { Window._sdfOptions.BackfacesThreshold = b.Value; };

                    // Toggle Backfaces Threshold visibility (CPU-only option)
                    gpu.CheckBox.StateChanged += c =>
                    {
                        Window._sdfOptions.GPU = c.Checked;
                        backfacesThresholdLabel.Visible = !c.Checked;
                        backfacesThreshold.ValueBox.Visible = !c.Checked;
                    };
                    backfacesThresholdLabel.Visible = !gpu.CheckBox.Checked;
                    backfacesThreshold.ValueBox.Visible = !gpu.CheckBox.Checked;

                    var lodIndex = group.IntegerValue("LOD Index", "Index of the model Level of Detail to use for SDF data building. By default uses the lowest quality LOD for fast building.");
                    lodIndex.IntValue.MinValue = 0;
                    lodIndex.IntValue.MaxValue = Asset.LODsCount - 1;
                    lodIndex.IntValue.Value = sdf.Texture != null ? sdf.LOD : 6;
                    _sdfModelLodIndex = lodIndex;

                    var buttons = layout.UniformGrid();
                    var gridControl = buttons.CustomControl;
                    gridControl.ClipChildren = false;
                    gridControl.Height = Button.DefaultHeight;
                    gridControl.SlotsHorizontally = 2;
                    gridControl.SlotsVertically = 1;
                    var rebuildButton = buttons.Button("Rebuild", "Rebuilds the model SDF.").Button;
                    rebuildButton.Clicked += OnRebuildSDF;
                    var removeButton = buttons.Button("Remove", "Removes the model SDF data from the asset.").Button;
                    removeButton.Enabled = sdf.Texture != null;
                    removeButton.Clicked += OnRemoveSDF;
                }
            }

            private void OnRebuildSDF()
            {
                Window.Enabled = false;
                Task.Run(() =>
                {
                    var sdfOptions = Window._sdfOptions;
                    bool failed = Asset.GenerateSDF(Window._importSettings.Settings.SDFResolution, _sdfModelLodIndex.Value, true, sdfOptions.BackfacesThreshold, sdfOptions.GPU);
                    FlaxEngine.Scripting.InvokeOnUpdate(() =>
                    {
                        Window.Enabled = true;
                        if (!failed)
                            Window.MarkAsEdited();
                        _presenter.BuildLayoutOnUpdate();

                        // Save some SDF options locally in the project cache
                        Window.Editor.ProjectCache.SetCustomData(JsonSerializer.GetStringID(Window.Item.ID) + ".SDF", JsonSerializer.Serialize(sdfOptions));
                    });
                });
            }

            private void OnRemoveSDF()
            {
                Asset.SetSDF(new ModelBase.SDFData());
                Window.MarkAsEdited();
                _presenter.BuildLayoutOnUpdate();
            }
        }

        [CustomEditor(typeof(ProxyEditor))]
        private sealed class MaterialsPropertiesProxy : MaterialsPropertiesProxyBase
        {
        }

        [CustomEditor(typeof(ProxyEditor))]
        private sealed class UVsPropertiesProxy : UVsPropertiesProxyBase
        {
        }

        [CustomEditor(typeof(ProxyEditor))]
        private sealed class ImportPropertiesProxy : ImportPropertiesProxyBase
        {
        }

        private class MeshesTab : Tab
        {
            public MeshesTab(ModelWindow window)
            : base("Meshes", window)
            {
                Proxy = new MeshesPropertiesProxy();
                Presenter.Select(Proxy);
            }
        }

        private class MaterialsTab : Tab
        {
            public MaterialsTab(ModelWindow window)
            : base("Materials", window)
            {
                Proxy = new MaterialsPropertiesProxy();
                Presenter.Select(Proxy);
            }
        }

        private class UVsTab : Tab
        {
            public UVsTab(ModelWindow window)
            : base("UVs", window, false)
            {
                Proxy = new UVsPropertiesProxy();
                Presenter.Select(Proxy);
            }
        }

        private class ImportTab : Tab
        {
            public ImportTab(ModelWindow window)
            : base("Import", window, false)
            {
                Proxy = new ImportPropertiesProxy();
                Presenter.Select(Proxy);
            }
        }

        private struct ModelSdfOptions
        {
            public bool GPU;
            public float BackfacesThreshold;
        }

        private readonly ModelPreview _preview;
        private StaticModel _highlightActor;
        private ModelSdfOptions _sdfOptions;
        private ToolStripButton _showCurrentLODButton;

        /// <inheritdoc />
        public ModelWindow(Editor editor, AssetItem item)
        : base(editor, item)
        {
            // Try to restore SDF options from project cache (not saved in the asset)
            if (Editor.ProjectCache.TryGetCustomData(JsonSerializer.GetStringID(Item.ID) + ".SDF", out string sdOptionsStr))
                _sdfOptions = JsonSerializer.Deserialize<ModelSdfOptions>(sdOptionsStr);
            else
                _sdfOptions = new ModelSdfOptions
                {
                    GPU = true,
                    BackfacesThreshold = 0.6f,
                };

            // Toolstrip
            _toolstrip.AddSeparator();
            _showCurrentLODButton = (ToolStripButton)_toolstrip.AddButton(editor.Icons.Info64, () => _preview.ShowCurrentLOD = !_preview.ShowCurrentLOD).LinkTooltip("Show LOD statistics");
            _toolstrip.AddButton(editor.Icons.CenterView64, () => _preview.ResetCamera()).LinkTooltip("Show whole model");
            _toolstrip.AddSeparator();
            _toolstrip.AddButton(editor.Icons.Docs64, () => Platform.OpenUrl(Utilities.Constants.DocsUrl + "manual/graphics/models/index.html")).LinkTooltip("See documentation to learn more");

            // Model preview
            _preview = new Preview(this)
            {
                ViewportCamera = new FPSCamera(),
                ScaleToFit = false,
                Parent = _split.Panel1
            };

            // Properties tabs
            _tabs.AddTab(new MeshesTab(this));
            _tabs.AddTab(new MaterialsTab(this));
            _tabs.AddTab(new UVsTab(this));
            _tabs.AddTab(new ImportTab(this));

            // Highlight actor (used to highlight selected material slot, see UpdateEffectsOnAsset)
            _highlightActor = new StaticModel
            {
                IsActive = false
            };
            _preview.Task.AddCustomActor(_highlightActor);
        }

        /// <inheritdoc />
        protected override void UpdateEffectsOnAsset()
        {
            var entries = _preview.PreviewActor.Entries;
            if (entries != null)
            {
                for (int i = 0; i < entries.Length; i++)
                {
                    entries[i].Visible = _isolateIndex == -1 || _isolateIndex == i;
                }
                _preview.PreviewActor.Entries = entries;
            }

            if (_highlightIndex != -1)
            {
                _highlightActor.IsActive = true;

                var highlightMaterial = EditorAssets.Cache.HighlightMaterialInstance;
                entries = _highlightActor.Entries;
                if (entries != null)
                {
                    for (int i = 0; i < entries.Length; i++)
                    {
                        entries[i].Material = highlightMaterial;
                        entries[i].Visible = _highlightIndex == i;
                    }
                    _highlightActor.Entries = entries;
                }
            }
            else
            {
                _highlightActor.IsActive = false;
            }
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            // Sync highlight actor size with actual preview model (preview scales model for better usage experience)
            if (_highlightActor && _highlightActor.IsActive)
            {
                _highlightActor.Transform = _preview.PreviewActor.Transform;
            }

            // Model is loaded but LODs data may be during streaming so refresh properties on fully loaded
            if (_refreshOnLODsLoaded && _asset && _asset.LoadedLODs == _asset.LODs.Length)
            {
                _refreshOnLODsLoaded = false;
                foreach (var child in _tabs.Children)
                {
                    if (child is Tab tab)
                    {
                        tab.Presenter.BuildLayout();
                    }
                }
            }

            _showCurrentLODButton.Checked = _preview.ShowCurrentLOD;

            base.Update(deltaTime);
        }

        /// <inheritdoc />
        public override void Save()
        {
            if (!IsEdited)
                return;
            if (_asset.WaitForLoaded())
                return;

            foreach (var child in _tabs.Children)
            {
                if (child is Tab tab && tab.Proxy.Window != null)
                    tab.Proxy.OnSave();
            }

            if (_asset.Save())
            {
                Editor.LogError("Cannot save asset.");
                return;
            }

            ClearEditedFlag();
            _item.RefreshThumbnail();
        }

        /// <inheritdoc />
        protected override void UnlinkItem()
        {
            _preview.Model = null;
            _highlightActor.Model = null;

            base.UnlinkItem();
        }

        /// <inheritdoc />
        protected override void OnAssetLinked()
        {
            _preview.Model = _asset;
            _highlightActor.Model = _asset;

            base.OnAssetLinked();
        }

        /// <inheritdoc />
        protected override void OnAssetLoaded()
        {
            _preview.ViewportCamera.SetArcBallView(Asset.GetBox());

            base.OnAssetLoaded();
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            base.OnDestroy();

            Object.Destroy(ref _highlightActor);
            _showCurrentLODButton = null;
        }
    }
}
