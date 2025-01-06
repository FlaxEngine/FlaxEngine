// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Threading.Tasks;
using FlaxEditor.Content;
using FlaxEditor.Content.Import;
using FlaxEditor.CustomEditors;
using FlaxEditor.GUI;
using FlaxEditor.Scripting;
using FlaxEditor.Viewport.Cameras;
using FlaxEditor.Viewport.Previews;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Json;
using FlaxEngine.Tools;
using FlaxEngine.Utilities;
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
                    Render2D.DrawText(style.FontLarge, "Loading...", new Rectangle(Float2.Zero, Size), style.ForegroundDisabled, TextAlignment.Center, TextAlignment.Center);
                }
            }
        }

        [CustomEditor(typeof(ProxyEditor))]
        private sealed class MeshesPropertiesProxy : PropertiesProxyBase
        {
            private readonly List<ComboBox> _materialSlotComboBoxes = new List<ComboBox>();
            private readonly List<CheckBox> _isolateCheckBoxes = new List<CheckBox>();
            private readonly List<CheckBox> _highlightCheckBoxes = new List<CheckBox>();

            public override void OnLoad(ModelWindow window)
            {
                base.OnLoad(window);

                Window._isolateIndex = -1;
                Window._highlightIndex = -1;
            }

            public override void OnClean()
            {
                Window._isolateIndex = -1;
                Window._highlightIndex = -1;

                base.OnClean();
            }

            private void UpdateEffectsOnUI()
            {
                Window._skipEffectsGuiEvents = true;

                for (int i = 0; i < _isolateCheckBoxes.Count; i++)
                {
                    var checkBox = _isolateCheckBoxes[i];
                    checkBox.Checked = Window._isolateIndex == ((Mesh)checkBox.Tag).MaterialSlotIndex;
                }

                for (int i = 0; i < _highlightCheckBoxes.Count; i++)
                {
                    var checkBox = _highlightCheckBoxes[i];
                    checkBox.Checked = Window._highlightIndex == ((Mesh)checkBox.Tag).MaterialSlotIndex;
                }

                Window._skipEffectsGuiEvents = false;
            }

            private void UpdateMaterialSlotsUI()
            {
                Window._skipEffectsGuiEvents = true;

                // Generate material slots labels (with index prefix)
                var slots = Asset.MaterialSlots;
                var slotsLabels = new string[slots.Length];
                for (int i = 0; i < slots.Length; i++)
                {
                    slotsLabels[i] = string.Format("[{0}] {1}", i, slots[i].Name);
                }

                // Update comboboxes
                for (int i = 0; i < _materialSlotComboBoxes.Count; i++)
                {
                    var comboBox = _materialSlotComboBoxes[i];
                    comboBox.SetItems(slotsLabels);
                    comboBox.SelectedIndex = ((Mesh)comboBox.Tag).MaterialSlotIndex;
                }

                Window._skipEffectsGuiEvents = false;
            }

            private void SetMaterialSlot(Mesh mesh, int newSlotIndex)
            {
                if (Window._skipEffectsGuiEvents)
                    return;

                mesh.MaterialSlotIndex = newSlotIndex == -1 ? 0 : newSlotIndex;
                Window.UpdateEffectsOnAsset();
                UpdateEffectsOnUI();
                Window.MarkAsEdited();
            }

            private void SetIsolate(Mesh mesh)
            {
                if (Window._skipEffectsGuiEvents)
                    return;

                Window._isolateIndex = mesh != null ? mesh.MaterialSlotIndex : -1;
                Window.UpdateEffectsOnAsset();
                UpdateEffectsOnUI();
            }

            private void SetHighlight(Mesh mesh)
            {
                if (Window._skipEffectsGuiEvents)
                    return;

                Window._highlightIndex = mesh != null ? mesh.MaterialSlotIndex : -1;
                Window.UpdateEffectsOnAsset();
                UpdateEffectsOnUI();
            }

            private class ProxyEditor : ProxyEditorBase
            {
                private CustomEditors.Elements.IntegerValueElement _sdfModelLodIndex;

                public override void Initialize(LayoutElementsContainer layout)
                {
                    var proxy = (MeshesPropertiesProxy)Values[0];
                    if (proxy.Asset == null || !proxy.Asset.IsLoaded)
                    {
                        layout.Label("Loading...", TextAlignment.Center);
                        return;
                    }
                    proxy._materialSlotComboBoxes.Clear();
                    proxy._isolateCheckBoxes.Clear();
                    proxy._highlightCheckBoxes.Clear();
                    var lods = proxy.Asset.LODs;
                    var loadedLODs = proxy.Asset.LoadedLODs;

                    // General properties
                    {
                        var group = layout.Group("General");

                        var minScreenSize = group.FloatValue("Min Screen Size", "The minimum screen size to draw model (the bottom limit). Used to cull small models. Set to 0 to disable this feature.");
                        minScreenSize.ValueBox.MinValue = 0.0f;
                        minScreenSize.ValueBox.MaxValue = 1.0f;
                        minScreenSize.ValueBox.Value = proxy.Asset.MinScreenSize;
                        minScreenSize.ValueBox.BoxValueChanged += b =>
                        {
                            proxy.Asset.MinScreenSize = b.Value;
                            proxy.Window.MarkAsEdited();
                        };
                    }

                    // SDF
                    {
                        var group = layout.Group("SDF");
                        var sdfOptions = proxy.Window._sdfOptions;

                        var sdf = proxy.Asset.SDF;
                        if (sdf.Texture != null)
                        {
                            var size = sdf.Texture.Size3;
                            group.Label($"SDF Texture {size.X}x{size.Y}x{size.Z} ({Utilities.Utils.FormatBytesCount(sdf.Texture.MemoryUsage)})").AddCopyContextMenu();
                        }
                        else
                        {
                            group.Label("No SDF");
                        }

                        var resolution = group.FloatValue("Resolution Scale", proxy.Window.Editor.CodeDocs.GetTooltip(typeof(ModelTool.Options), nameof(ModelImportSettings.Settings.SDFResolution)));
                        resolution.ValueBox.MinValue = 0.0001f;
                        resolution.ValueBox.MaxValue = 100.0f;
                        resolution.ValueBox.Value = sdf.Texture != null ? sdf.ResolutionScale : 1.0f;
                        resolution.ValueBox.BoxValueChanged += b => { proxy.Window._importSettings.Settings.SDFResolution = b.Value; };
                        proxy.Window._importSettings.Settings.SDFResolution = sdf.ResolutionScale;

                        var gpu = group.Checkbox("Bake on GPU", "If checked, SDF generation will be calculated using GPU on Compute Shader, otherwise CPU will use Job System. GPU generation is fast but result in artifacts in various meshes (eg. foliage).");
                        gpu.CheckBox.Checked = sdfOptions.GPU;

                        var backfacesThresholdProp = group.AddPropertyItem("Backfaces Threshold", "Custom threshold (in range 0-1) for adjusting mesh internals detection based on the percentage of test rays hit triangle backfaces. Use lower value for more dense mesh.");
                        var backfacesThreshold = backfacesThresholdProp.FloatValue();
                        var backfacesThresholdLabel = backfacesThresholdProp.Labels.Last();
                        backfacesThreshold.ValueBox.MinValue = 0.001f;
                        backfacesThreshold.ValueBox.MaxValue = 1.0f;
                        backfacesThreshold.ValueBox.Value = sdfOptions.BackfacesThreshold;
                        backfacesThreshold.ValueBox.BoxValueChanged += b => { proxy.Window._sdfOptions.BackfacesThreshold = b.Value; };

                        // Toggle Backfaces Threshold visibility (CPU-only option)
                        gpu.CheckBox.StateChanged += c =>
                        {
                            proxy.Window._sdfOptions.GPU = c.Checked;
                            backfacesThresholdLabel.Visible = !c.Checked;
                            backfacesThreshold.ValueBox.Visible = !c.Checked;
                        };
                        backfacesThresholdLabel.Visible = !gpu.CheckBox.Checked;
                        backfacesThreshold.ValueBox.Visible = !gpu.CheckBox.Checked;

                        var lodIndex = group.IntegerValue("LOD Index", "Index of the model Level of Detail to use for SDF data building. By default uses the lowest quality LOD for fast building.");
                        lodIndex.IntValue.MinValue = 0;
                        lodIndex.IntValue.MaxValue = lods.Length - 1;
                        lodIndex.IntValue.Value = sdf.Texture != null ? sdf.LOD : 6;
                        _sdfModelLodIndex = lodIndex;

                        var buttons = group.CustomContainer<UniformGridPanel>();
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

                    // Group per LOD
                    for (int lodIndex = 0; lodIndex < lods.Length; lodIndex++)
                    {
                        var group = layout.Group("LOD " + lodIndex);
                        if (lodIndex < lods.Length - loadedLODs)
                        {
                            group.Label("Loading LOD...");
                            continue;
                        }
                        var lod = lods[lodIndex];
                        var meshes = lod.Meshes;

                        int triangleCount = 0, vertexCount = 0;
                        for (int meshIndex = 0; meshIndex < meshes.Length; meshIndex++)
                        {
                            var mesh = meshes[meshIndex];
                            triangleCount += mesh.TriangleCount;
                            vertexCount += mesh.VertexCount;
                        }

                        group.Label(string.Format("Triangles: {0:N0}   Vertices: {1:N0}", triangleCount, vertexCount)).AddCopyContextMenu();
                        group.Label("Size: " + lod.Box.Size).AddCopyContextMenu();
                        var screenSize = group.FloatValue("Screen Size", "The screen size to switch LODs. Bottom limit of the model screen size to render this LOD.");
                        screenSize.ValueBox.MinValue = 0.0f;
                        screenSize.ValueBox.MaxValue = 10.0f;
                        screenSize.ValueBox.Value = lod.ScreenSize;
                        screenSize.ValueBox.BoxValueChanged += b =>
                        {
                            lod.ScreenSize = b.Value;
                            proxy.Window.MarkAsEdited();
                        };

                        // Every mesh properties
                        for (int meshIndex = 0; meshIndex < meshes.Length; meshIndex++)
                        {
                            var mesh = meshes[meshIndex];
                            group.Label($"Mesh {meshIndex} (tris: {mesh.TriangleCount:N0}, verts: {mesh.VertexCount:N0})").AddCopyContextMenu();

                            // Material Slot
                            var materialSlot = group.ComboBox("Material Slot", "Material slot used by this mesh during rendering");
                            materialSlot.ComboBox.Tag = mesh;
                            materialSlot.ComboBox.SelectedIndexChanged += comboBox => proxy.SetMaterialSlot((Mesh)comboBox.Tag, comboBox.SelectedIndex);
                            proxy._materialSlotComboBoxes.Add(materialSlot.ComboBox);

                            // Isolate
                            var isolate = group.Checkbox("Isolate", "Shows only this mesh (and meshes using the same material slot)");
                            isolate.CheckBox.Tag = mesh;
                            isolate.CheckBox.StateChanged += (box) => proxy.SetIsolate(box.Checked ? (Mesh)box.Tag : null);
                            proxy._isolateCheckBoxes.Add(isolate.CheckBox);

                            // Highlight
                            var highlight = group.Checkbox("Highlight", "Highlights this mesh with a tint color (and meshes using the same material slot)");
                            highlight.CheckBox.Tag = mesh;
                            highlight.CheckBox.StateChanged += (box) => proxy.SetHighlight(box.Checked ? (Mesh)box.Tag : null);
                            proxy._highlightCheckBoxes.Add(highlight.CheckBox);
                        }
                    }

                    // Refresh UI
                    proxy.UpdateMaterialSlotsUI();
                }

                private void OnRebuildSDF()
                {
                    var proxy = (MeshesPropertiesProxy)Values[0];
                    proxy.Window.Enabled = false;
                    Task.Run(() =>
                    {
                        var sdfOptions = proxy.Window._sdfOptions;
                        bool failed = proxy.Asset.GenerateSDF(proxy.Window._importSettings.Settings.SDFResolution, _sdfModelLodIndex.Value, true, sdfOptions.BackfacesThreshold, sdfOptions.GPU);
                        FlaxEngine.Scripting.InvokeOnUpdate(() =>
                        {
                            proxy.Window.Enabled = true;
                            if (!failed)
                                proxy.Window.MarkAsEdited();
                            Presenter.BuildLayoutOnUpdate();

                            // Save some SDF options locally in the project cache
                            proxy.Window.Editor.ProjectCache.SetCustomData(JsonSerializer.GetStringID(proxy.Window.Item.ID) + ".SDF", JsonSerializer.Serialize(sdfOptions));
                        });
                    });
                }

                private void OnRemoveSDF()
                {
                    var proxy = (MeshesPropertiesProxy)Values[0];
                    proxy.Asset.SetSDF(new ModelBase.SDFData());
                    proxy.Window.MarkAsEdited();
                    Presenter.BuildLayoutOnUpdate();
                }

                internal override void RefreshInternal()
                {
                    // Skip updates when model is not loaded
                    var proxy = (MeshesPropertiesProxy)Values[0];
                    if (proxy.Asset == null || !proxy.Asset.IsLoaded)
                        return;

                    base.RefreshInternal();
                }
            }
        }

        [CustomEditor(typeof(ProxyEditor))]
        private sealed class MaterialsPropertiesProxy : PropertiesProxyBase
        {
            [Collection(CanReorderItems = true, NotNullItems = true, OverrideEditorTypeName = "FlaxEditor.CustomEditors.Editors.GenericEditor", Spacing = 10)]
            [EditorOrder(10), EditorDisplay("Materials", EditorDisplayAttribute.InlineStyle)]
            public MaterialSlot[] MaterialSlots
            {
                get => Asset != null ? Asset.MaterialSlots : null;
                set
                {
                    if (Asset != null)
                    {
                        if (Asset.MaterialSlots.Length != value.Length)
                        {
                            MaterialBase[] materials = new MaterialBase[value.Length];
                            string[] names = new string[value.Length];
                            ShadowsCastingMode[] shadowsModes = new ShadowsCastingMode[value.Length];
                            for (int i = 0; i < value.Length; i++)
                            {
                                if (value[i] != null)
                                {
                                    materials[i] = value[i].Material;
                                    names[i] = value[i].Name;
                                    shadowsModes[i] = value[i].ShadowsMode;
                                }
                                else
                                {
                                    materials[i] = null;
                                    names[i] = "Material " + i;
                                    shadowsModes[i] = ShadowsCastingMode.All;
                                }
                            }

                            Asset.SetupMaterialSlots(value.Length);

                            var slots = Asset.MaterialSlots;
                            for (int i = 0; i < slots.Length; i++)
                            {
                                slots[i].Material = materials[i];
                                slots[i].Name = names[i];
                                slots[i].ShadowsMode = shadowsModes[i];
                            }

                            UpdateMaterialSlotsUI();
                        }
                    }
                }
            }

            private readonly List<ComboBox> _materialSlotComboBoxes = new List<ComboBox>();

            /// <summary>
            /// Updates the material slots UI parts. Should be called after material slot rename.
            /// </summary>
            public void UpdateMaterialSlotsUI()
            {
                Window._skipEffectsGuiEvents = true;

                // Generate material slots labels (with index prefix)
                var slots = Asset.MaterialSlots;
                var slotsLabels = new string[slots.Length];
                for (int i = 0; i < slots.Length; i++)
                {
                    slotsLabels[i] = string.Format("[{0}] {1}", i, slots[i].Name);
                }

                // Update comboboxes
                for (int i = 0; i < _materialSlotComboBoxes.Count; i++)
                {
                    var comboBox = _materialSlotComboBoxes[i];
                    comboBox.SetItems(slotsLabels);
                    comboBox.SelectedIndex = ((Mesh)comboBox.Tag).MaterialSlotIndex;
                }

                Window._skipEffectsGuiEvents = false;
            }

            private class ProxyEditor : ProxyEditorBase
            {
                public override void Initialize(LayoutElementsContainer layout)
                {
                    var proxy = (MaterialsPropertiesProxy)Values[0];
                    if (proxy.Asset == null || !proxy.Asset.IsLoaded)
                    {
                        layout.Label("Loading...", TextAlignment.Center);
                        return;
                    }

                    base.Initialize(layout);
                }
            }
        }

        [CustomEditor(typeof(ProxyEditor))]
        private sealed class UVsPropertiesProxy : PropertiesProxyBase
        {
            public enum UVChannel
            {
                None,
                TexCoord,
                LightmapUVs,
            };

            private UVChannel _uvChannel = UVChannel.None;

            [EditorOrder(0), EditorDisplay(null, "Preview UV Channel"), EnumDisplay(EnumDisplayAttribute.FormatMode.None)]
            [Tooltip("Set UV channel to preview.")]
            public UVChannel Channel
            {
                get => _uvChannel;
                set
                {
                    if (_uvChannel == value)
                        return;
                    _uvChannel = value;
                    Window._meshData?.RequestMeshData(Window._asset);
                }
            }

            [EditorOrder(1), EditorDisplay(null, "LOD"), Limit(0, Model.MaxLODs), VisibleIf("ShowUVs")]
            [Tooltip("Level Of Detail index to preview UVs layout.")]
            public int LOD = 0;

            [EditorOrder(2), EditorDisplay(null, "Mesh"), Limit(-1, 1000000), VisibleIf("ShowUVs")]
            [Tooltip("Mesh index to preview UVs layout. Use -1 for all meshes")]
            public int Mesh = -1;

            private bool ShowUVs => _uvChannel != UVChannel.None;

            /// <inheritdoc />
            public override void OnClean()
            {
                Channel = UVChannel.None;

                base.OnClean();
            }

            private class ProxyEditor : ProxyEditorBase
            {
                private UVsLayoutPreviewControl _uvsPreview;

                public override void Initialize(LayoutElementsContainer layout)
                {
                    var proxy = (UVsPropertiesProxy)Values[0];
                    if (proxy.Asset == null || !proxy.Asset.IsLoaded)
                    {
                        layout.Label("Loading...", TextAlignment.Center);
                        return;
                    }

                    base.Initialize(layout);

                    _uvsPreview = layout.Custom<UVsLayoutPreviewControl>().CustomControl;
                    _uvsPreview.Proxy = proxy;
                }

                /// <inheritdoc />
                public override void Refresh()
                {
                    base.Refresh();

                    if (_uvsPreview != null)
                    {
                        _uvsPreview.Channel = _uvsPreview.Proxy._uvChannel;
                        _uvsPreview.LOD = _uvsPreview.Proxy.LOD;
                        _uvsPreview.Mesh = _uvsPreview.Proxy.Mesh;
                        _uvsPreview.HighlightIndex = _uvsPreview.Proxy.Window._highlightIndex;
                        _uvsPreview.IsolateIndex = _uvsPreview.Proxy.Window._isolateIndex;
                    }
                }

                protected override void Deinitialize()
                {
                    _uvsPreview = null;

                    base.Deinitialize();
                }
            }

            private sealed class UVsLayoutPreviewControl : RenderToTextureControl
            {
                private UVChannel _channel;
                private int _lod, _mesh = -1;
                private int _highlightIndex = -1;
                private int _isolateIndex = -1;
                public UVsPropertiesProxy Proxy;

                public UVsLayoutPreviewControl()
                {
                    Offsets = new Margin(4);
                    AutomaticInvalidate = false;
                }

                public UVChannel Channel
                {
                    set
                    {
                        if (_channel == value)
                            return;
                        _channel = value;
                        Visible = _channel != UVChannel.None;
                        if (Visible)
                            Invalidate();
                    }
                }

                public int LOD
                {
                    set
                    {
                        if (_lod != value)
                        {
                            _lod = value;
                            Invalidate();
                        }
                    }
                }

                public int Mesh
                {
                    set
                    {
                        if (_mesh != value)
                        {
                            _mesh = value;
                            Invalidate();
                        }
                    }
                }

                public int HighlightIndex
                {
                    set
                    {
                        if (_highlightIndex != value)
                        {
                            _highlightIndex = value;
                            Invalidate();
                        }
                    }
                }

                public int IsolateIndex
                {
                    set
                    {
                        if (_isolateIndex != value)
                        {
                            _isolateIndex = value;
                            Invalidate();
                        }
                    }
                }

                private void DrawMeshUVs(int meshIndex, MeshDataCache.MeshData meshData)
                {
                    var uvScale = Size;
                    if (meshData.IndexBuffer == null || meshData.VertexBuffer == null)
                        return;
                    var linesColor = _highlightIndex != -1 && _highlightIndex == meshIndex ? Style.Current.BackgroundSelected : Color.White;
                    switch (_channel)
                    {
                    case UVChannel.TexCoord:
                        for (int i = 0; i < meshData.IndexBuffer.Length; i += 3)
                        {
                            // Cache triangle indices
                            uint i0 = meshData.IndexBuffer[i + 0];
                            uint i1 = meshData.IndexBuffer[i + 1];
                            uint i2 = meshData.IndexBuffer[i + 2];

                            // Cache triangle uvs positions and transform positions to output target
                            Float2 uv0 = meshData.VertexBuffer[i0].TexCoord * uvScale;
                            Float2 uv1 = meshData.VertexBuffer[i1].TexCoord * uvScale;
                            Float2 uv2 = meshData.VertexBuffer[i2].TexCoord * uvScale;

                            // Don't draw too small triangles
                            float area = Float2.TriangleArea(ref uv0, ref uv1, ref uv2);
                            if (area > 10.0f)
                            {
                                // Draw triangle
                                Render2D.DrawLine(uv0, uv1, linesColor);
                                Render2D.DrawLine(uv1, uv2, linesColor);
                                Render2D.DrawLine(uv2, uv0, linesColor);
                            }
                        }
                        break;
                    case UVChannel.LightmapUVs:
                        for (int i = 0; i < meshData.IndexBuffer.Length; i += 3)
                        {
                            // Cache triangle indices
                            uint i0 = meshData.IndexBuffer[i + 0];
                            uint i1 = meshData.IndexBuffer[i + 1];
                            uint i2 = meshData.IndexBuffer[i + 2];

                            // Cache triangle uvs positions and transform positions to output target
                            Float2 uv0 = meshData.VertexBuffer[i0].LightmapUVs * uvScale;
                            Float2 uv1 = meshData.VertexBuffer[i1].LightmapUVs * uvScale;
                            Float2 uv2 = meshData.VertexBuffer[i2].LightmapUVs * uvScale;

                            // Don't draw too small triangles
                            float area = Float2.TriangleArea(ref uv0, ref uv1, ref uv2);
                            if (area > 3.0f)
                            {
                                // Draw triangle
                                Render2D.DrawLine(uv0, uv1, linesColor);
                                Render2D.DrawLine(uv1, uv2, linesColor);
                                Render2D.DrawLine(uv2, uv0, linesColor);
                            }
                        }
                        break;
                    }
                }

                /// <inheritdoc />
                public override void DrawSelf()
                {
                    base.DrawSelf();

                    var size = Size;
                    if (_channel == UVChannel.None || size.MaxValue < 5.0f)
                        return;
                    if (Proxy.Window._meshData == null)
                        Proxy.Window._meshData = new MeshDataCache();
                    if (!Proxy.Window._meshData.RequestMeshData(Proxy.Window._asset))
                    {
                        Invalidate();
                        Render2D.DrawText(Style.Current.FontMedium, "Loading...", new Rectangle(Float2.Zero, size), Color.White, TextAlignment.Center, TextAlignment.Center);
                        return;
                    }

                    Render2D.PushClip(new Rectangle(Float2.Zero, size));

                    var meshDatas = Proxy.Window._meshData.MeshDatas;
                    var lodIndex = Mathf.Clamp(_lod, 0, meshDatas.Length - 1);
                    var lod = meshDatas[lodIndex];
                    var mesh = Mathf.Clamp(_mesh, -1, lod.Length - 1);
                    if (mesh == -1)
                    {
                        for (int meshIndex = 0; meshIndex < lod.Length; meshIndex++)
                        {
                            if (_isolateIndex != -1 && _isolateIndex != meshIndex)
                                continue;
                            DrawMeshUVs(meshIndex, lod[meshIndex]);
                        }
                    }
                    else
                    {
                        DrawMeshUVs(mesh, lod[mesh]);
                    }

                    Render2D.PopClip();
                }

                protected override void OnSizeChanged()
                {
                    Height = Width;

                    base.OnSizeChanged();
                }

                protected override void OnVisibleChanged()
                {
                    base.OnVisibleChanged();

                    Parent.PerformLayout();
                    Height = Width;
                }
            }
        }

        [CustomEditor(typeof(ProxyEditor))]
        private sealed class ImportPropertiesProxy : PropertiesProxyBase
        {
            private ModelImportSettings ImportSettings;

            /// <inheritdoc />
            public override void OnLoad(ModelWindow window)
            {
                base.OnLoad(window);

                ImportSettings = window._importSettings;
            }

            public void Reimport()
            {
                Editor.Instance.ContentImporting.Reimport((BinaryAssetItem)Window.Item, ImportSettings, true);
            }

            private class ProxyEditor : ProxyEditorBase
            {
                public override void Initialize(LayoutElementsContainer layout)
                {
                    var proxy = (ImportPropertiesProxy)Values[0];
                    if (proxy.Asset == null || !proxy.Asset.IsLoaded)
                    {
                        layout.Label("Loading...", TextAlignment.Center);
                        return;
                    }

                    // Import Settings
                    {
                        var group = layout.Group("Import Settings");

                        var importSettingsField = typeof(ImportPropertiesProxy).GetField(nameof(ImportSettings), BindingFlags.NonPublic | BindingFlags.Instance);
                        var importSettingsValues = new ValueContainer(new ScriptMemberInfo(importSettingsField)) { proxy.ImportSettings };
                        group.Object(importSettingsValues);

                        // Creates the import path UI
                        Utilities.Utils.CreateImportPathUI(layout, proxy.Window.Item as BinaryAssetItem);

                        layout.Space(5);
                        var reimportButton = layout.Button("Reimport");
                        reimportButton.Button.Clicked += () => ((ImportPropertiesProxy)Values[0]).Reimport();
                    }
                }
            }
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
        private MeshDataCache _meshData;
        private ModelImportSettings _importSettings = new ModelImportSettings();
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

        /// <summary>
        /// Updates the highlight/isolate effects on a model asset.
        /// </summary>
        private void UpdateEffectsOnAsset()
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
            _meshData?.WaitForMeshDataRequestEnd();
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
            _refreshOnLODsLoaded = true;
            _preview.ViewportCamera.SetArcBallView(Asset.GetBox());
            Editor.TryRestoreImportOptions(ref _importSettings.Settings, Item.Path);
            UpdateEffectsOnAsset();

            // TODO: disable streaming for this model

            base.OnAssetLoaded();
        }

        /// <inheritdoc />
        public override void OnItemReimported(ContentItem item)
        {
            // Discard any old mesh data cache
            _meshData?.Dispose();

            base.OnItemReimported(item);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _meshData?.Dispose();
            _meshData = null;

            base.OnDestroy();

            Object.Destroy(ref _highlightActor);
            _showCurrentLODButton = null;
        }
    }
}
