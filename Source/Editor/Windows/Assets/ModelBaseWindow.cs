// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Xml;
using FlaxEditor.Content;
using FlaxEditor.Content.Import;
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.GUI;
using FlaxEditor.GUI.Tabs;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;

#pragma warning disable 1591

namespace FlaxEditor.Windows.Assets
{
    /// <summary>
    /// Editor window to view/modify <see cref="ModelBase"/> asset.
    /// </summary>
    /// <seealso cref="Model" />
    /// <seealso cref="FlaxEditor.Windows.Assets.AssetEditorWindow" />
    public abstract class ModelBaseWindow<TAsset, TWindow> : AssetEditorWindowBase<TAsset>
    where TAsset : ModelBase
    where TWindow : ModelBaseWindow<TAsset, TWindow>
    {
        protected abstract class PropertiesProxyBase
        {
            [HideInEditor]
            public TWindow Window;

            [HideInEditor]
            public TAsset Asset;

            public virtual void OnLoad(TWindow window)
            {
                Window = window;
                Asset = window.Asset;
            }

            public virtual void OnSave()
            {
            }

            public virtual void OnClean()
            {
                Window = null;
                Asset = null;
            }
        }

        protected abstract class ProxyEditorBase : GenericEditor
        {
            internal override void RefreshInternal()
            {
                // Skip updates when model is not loaded
                var proxy = (PropertiesProxyBase)Values[0];
                if (proxy.Asset == null || !proxy.Asset.IsLoaded)
                    return;

                base.RefreshInternal();
            }
        }

        protected class Tab : GUI.Tabs.Tab
        {
            public CustomEditorPresenter Presenter;
            public PropertiesProxyBase Proxy;

            public Tab(string text, TWindow window, bool modifiesAsset = true)
            : base(text)
            {
                var scrollPanel = new Panel(ScrollBars.Vertical)
                {
                    AnchorPreset = AnchorPresets.StretchAll,
                    Offsets = Margin.Zero,
                    Parent = this
                };

                Presenter = new CustomEditorPresenter(null);
                Presenter.Panel.Parent = scrollPanel;
                if (modifiesAsset)
                    Presenter.Modified += window.MarkAsEdited;
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                if (IsDisposing)
                    return;
                base.OnDestroy();

                Presenter.Deselect();
                Presenter = null;
                Proxy = null;
            }
        }

        protected class MeshesPropertiesProxyBase : PropertiesProxyBase
        {
            private readonly List<ComboBox> _materialSlotComboBoxes = new List<ComboBox>();
            private readonly List<CheckBox> _isolateCheckBoxes = new List<CheckBox>();
            private readonly List<CheckBox> _highlightCheckBoxes = new List<CheckBox>();

            public override void OnLoad(TWindow window)
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

            /// <summary>
            /// Updates the highlight/isolate effects on UI.
            /// </summary>
            public void UpdateEffectsOnUI()
            {
                Window._skipEffectsGuiEvents = true;

                for (int i = 0; i < _isolateCheckBoxes.Count; i++)
                {
                    var checkBox = _isolateCheckBoxes[i];
                    checkBox.Checked = Window._isolateIndex == ((MeshBase)checkBox.Tag).MaterialSlotIndex;
                }

                for (int i = 0; i < _highlightCheckBoxes.Count; i++)
                {
                    var checkBox = _highlightCheckBoxes[i];
                    checkBox.Checked = Window._highlightIndex == ((MeshBase)checkBox.Tag).MaterialSlotIndex;
                }

                Window._skipEffectsGuiEvents = false;
            }

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
                    comboBox.SelectedIndex = ((MeshBase)comboBox.Tag).MaterialSlotIndex;
                }

                Window._skipEffectsGuiEvents = false;
            }

            /// <summary>
            /// Sets the material slot index to the mesh.
            /// </summary>
            /// <param name="mesh">The mesh.</param>
            /// <param name="newSlotIndex">New index of the material slot to use.</param>
            public void SetMaterialSlot(MeshBase mesh, int newSlotIndex)
            {
                if (Window._skipEffectsGuiEvents)
                    return;

                mesh.MaterialSlotIndex = newSlotIndex == -1 ? 0 : newSlotIndex;
                Window.UpdateEffectsOnAsset();
                UpdateEffectsOnUI();
                Window.MarkAsEdited();
            }

            /// <summary>
            /// Sets the material slot to isolate.
            /// </summary>
            /// <param name="mesh">The mesh.</param>
            public void SetIsolate(MeshBase mesh)
            {
                if (Window._skipEffectsGuiEvents)
                    return;

                Window._isolateIndex = mesh != null ? mesh.MaterialSlotIndex : -1;
                Window.UpdateEffectsOnAsset();
                UpdateEffectsOnUI();
            }

            /// <summary>
            /// Sets the material slot index to highlight.
            /// </summary>
            /// <param name="mesh">The mesh.</param>
            public void SetHighlight(MeshBase mesh)
            {
                if (Window._skipEffectsGuiEvents)
                    return;

                Window._highlightIndex = mesh != null ? mesh.MaterialSlotIndex : -1;
                Window.UpdateEffectsOnAsset();
                UpdateEffectsOnUI();
            }

            protected virtual void OnGeneral(LayoutElementsContainer layout)
            {
            }

            protected class ProxyEditor : ProxyEditorBase
            {
                public override void Initialize(LayoutElementsContainer layout)
                {
                    var proxy = (MeshesPropertiesProxyBase)Values[0];
                    if (Utilities.Utils.OnAssetProperties(layout, proxy.Asset))
                        return;
                    proxy._materialSlotComboBoxes.Clear();
                    proxy._isolateCheckBoxes.Clear();
                    proxy._highlightCheckBoxes.Clear();
                    var countLODs = proxy.Asset.LODsCount;
                    var loadedLODs = proxy.Asset.LoadedLODs;

                    // General properties
                    {
                        var group = layout.Group("General");

                        var minScreenSize = group.FloatValue("Min Screen Size", "The minimum screen size to draw model (the bottom limit). Used to cull small models. Set to 0 to disable this feature.");
                        minScreenSize.ValueBox.SlideSpeed = 0.005f;
                        minScreenSize.ValueBox.MinValue = 0.0f;
                        minScreenSize.ValueBox.MaxValue = 1.0f;
                        minScreenSize.ValueBox.Value = proxy.Asset.MinScreenSize;
                        minScreenSize.ValueBox.ValueChanged += () =>
                        {
                            proxy.Asset.MinScreenSize = minScreenSize.ValueBox.Value;
                            proxy.Window.MarkAsEdited();
                        };
                    }
                    {
                        var group = layout.Group("Vertex Layout");
                        GPUVertexLayout vertexLayout = null;
                        for (int lodIndex = 0; lodIndex < countLODs; lodIndex++)
                        {
                            var lod = proxy.Asset.GetLOD(lodIndex);
                            if (lodIndex >= countLODs - loadedLODs)
                            {
                                var mesh = lod.GetMesh(0);
                                if (mesh != null)
                                    vertexLayout = mesh.VertexLayout;
                                if (vertexLayout != null && vertexLayout.Elements.Length != 0)
                                    break;
                                vertexLayout = null;
                            }
                        }
                        if (vertexLayout != null)
                        {
                            var elements = vertexLayout.Elements;
                            for (uint slot = 0; slot < 4u; slot++)
                            {
                                var slotElements = elements.Where(x => x.Slot == slot && x.PerInstance == 0).ToArray();
                                if (slotElements.Length == 0)
                                    continue;
                                var height = 14;
                                group.Label($"   Vertex Buffer {slot}:").Label.Height = height;
                                foreach (var e in slotElements)
                                    group.Label($"      {e.Type}, {e.Format} ({PixelFormatExtensions.SizeInBytes(e.Format)} bytes), offset {e.Offset}").Label.Height = height;
                            }
                            group.Label($"{vertexLayout.Stride} bytes per-vertex");
                        }
                    }
                    proxy.OnGeneral(layout);

                    // Group per LOD
                    for (int lodIndex = 0; lodIndex < countLODs; lodIndex++)
                    {
                        var group = layout.Group("LOD " + lodIndex);
                        if (lodIndex < countLODs - loadedLODs)
                        {
                            group.Label("Loading LOD...");
                            continue;
                        }
                        var lod = proxy.Asset.GetLOD(lodIndex);
                        proxy.Asset.GetMeshes(out var meshes, lodIndex);

                        int triangleCount = 0, vertexCount = 0;
                        ulong[] meshMemory = new ulong[meshes.Length];
                        ulong lodMemory = 0;
                        for (int meshIndex = 0; meshIndex < meshes.Length; meshIndex++)
                        {
                            var mesh = meshes[meshIndex];
                            triangleCount += mesh.TriangleCount;
                            vertexCount += mesh.VertexCount;
                            ulong memory = 0;
                            for (int vbIndex = 0; vbIndex < 4; vbIndex++)
                            {
                                var vb = mesh.GetVertexBuffer(vbIndex);
                                if (vb != null)
                                    memory += vb.Size;
                            }
                            {
                                var ib = mesh.GetIndexBuffer();
                                if (ib != null)
                                    memory += ib.Size;
                            }
                            meshMemory[meshIndex] = memory;
                            lodMemory += memory;
                        }

                        group.Label(string.Format($"Triangles: {{0:N0}}   Vertices: {{1:N0}}   Memory: {Utilities.Utils.FormatBytesCount(lodMemory)}", triangleCount, vertexCount)).AddCopyContextMenu();
                        group.Label("Size: " + lod.Box.Size).AddCopyContextMenu();
                        var screenSize = group.FloatValue("Screen Size", "The screen size to switch LODs. Bottom limit of the model screen size to render this LOD.");
                        screenSize.ValueBox.MinValue = 0.0f;
                        screenSize.ValueBox.MaxValue = 10.0f;
                        screenSize.ValueBox.Value = lod.ScreenSize;
                        screenSize.ValueBox.ValueChanged += () =>
                        {
                            lod.ScreenSize = screenSize.ValueBox.Value;
                            proxy.Window.MarkAsEdited();
                        };

                        // Every mesh properties
                        for (int meshIndex = 0; meshIndex < meshes.Length; meshIndex++)
                        {
                            var mesh = meshes[meshIndex];
                            group.Label($"Mesh {meshIndex} (tris: {mesh.TriangleCount:N0}, vert: {mesh.VertexCount:N0}, mem: {Utilities.Utils.FormatBytesCount(meshMemory[meshIndex])})").AddCopyContextMenu();

                            // Material Slot
                            var materialSlot = group.ComboBox("Material Slot", "Material slot used by this mesh during rendering");
                            materialSlot.ComboBox.Tag = mesh;
                            materialSlot.ComboBox.SelectedIndexChanged += comboBox => proxy.SetMaterialSlot((MeshBase)comboBox.Tag, comboBox.SelectedIndex);
                            proxy._materialSlotComboBoxes.Add(materialSlot.ComboBox);

                            // Isolate
                            var isolate = group.Checkbox("Isolate", "Shows only this mesh (and meshes using the same material slot)");
                            isolate.CheckBox.Tag = mesh;
                            isolate.CheckBox.StateChanged += (box) => proxy.SetIsolate(box.Checked ? (MeshBase)box.Tag : null);
                            proxy._isolateCheckBoxes.Add(isolate.CheckBox);

                            // Highlight
                            var highlight = group.Checkbox("Highlight", "Highlights this mesh with a tint color (and meshes using the same material slot)");
                            highlight.CheckBox.Tag = mesh;
                            highlight.CheckBox.StateChanged += (box) => proxy.SetHighlight(box.Checked ? (MeshBase)box.Tag : null);
                            proxy._highlightCheckBoxes.Add(highlight.CheckBox);
                        }
                    }

                    // Refresh UI
                    proxy.UpdateMaterialSlotsUI();
                }
            }
        }

        protected class MaterialsPropertiesProxyBase : PropertiesProxyBase
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

            protected class ProxyEditor : ProxyEditorBase
            {
                public override void Initialize(LayoutElementsContainer layout)
                {
                    var proxy = (MaterialsPropertiesProxyBase)Values[0];
                    if (Utilities.Utils.OnAssetProperties(layout, proxy.Asset))
                        return;

                    base.Initialize(layout);
                }
            }
        }

        protected class UVsPropertiesProxyBase : PropertiesProxyBase
        {
            public enum UVChannel
            {
                None,
                TexCoord0,
                TexCoord1,
                TexCoord2,
                TexCoord3,
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

            [EditorOrder(1), EditorDisplay(null, "LOD"), Limit(0, Model.MaxLODs, 0.01f), VisibleIf("ShowUVs")]
            [Tooltip("Level Of Detail index to preview UVs layout at.")]
            public int LOD = 0;

            [EditorOrder(2), EditorDisplay(null, "Mesh"), Limit(-1, 1000000, 0.01f), VisibleIf("ShowUVs")]
            [Tooltip("Mesh index to show UVs layout for. Use -1 to display all UVs of all meshes")]
            public int Mesh = -1;

            private bool ShowUVs => _uvChannel != UVChannel.None;

            /// <inheritdoc />
            public override void OnClean()
            {
                Channel = UVChannel.None;

                base.OnClean();
            }

            protected class ProxyEditor : ProxyEditorBase
            {
                private UVsLayoutPreviewControl _uvsPreview;

                public override void Initialize(LayoutElementsContainer layout)
                {
                    var proxy = (UVsPropertiesProxyBase)Values[0];
                    if (Utilities.Utils.OnAssetProperties(layout, proxy.Asset))
                        return;

                    base.Initialize(layout);

                    _uvsPreview = layout.Custom<UVsLayoutPreviewControl>().CustomControl;
                    _uvsPreview.Window = proxy.Window;
                }

                /// <inheritdoc />
                public override void Refresh()
                {
                    base.Refresh();

                    if (_uvsPreview != null)
                    {
                        var proxy = (UVsPropertiesProxyBase)Values[0];
                        switch (proxy._uvChannel)
                        {
                        case UVChannel.TexCoord0: _uvsPreview.Channel = 0; break;
                        case UVChannel.TexCoord1: _uvsPreview.Channel = 1; break;
                        case UVChannel.TexCoord2: _uvsPreview.Channel = 2; break;
                        case UVChannel.TexCoord3: _uvsPreview.Channel = 3; break;
                        case UVChannel.LightmapUVs:
                        {
                            _uvsPreview.Channel = -1;
                            if (proxy.Window.Asset && proxy.Window.Asset.IsLoaded)
                            {
                                // Pick UVs channel index from the first mesh
                                proxy.Window.Asset.GetMeshes(out var meshes);
                                foreach (var mesh in meshes)
                                {
                                    if (mesh is Mesh m && m.HasLightmapUVs)
                                    {
                                        _uvsPreview.Channel = m.LightmapUVsIndex;
                                        break;
                                    }
                                }
                            }
                            break;
                        }
                        default: _uvsPreview.Channel = -1; break;
                        }
                        _uvsPreview.LOD = proxy.LOD;
                        _uvsPreview.Mesh = proxy.Mesh;
                        _uvsPreview.HighlightIndex = proxy.Window._highlightIndex;
                        _uvsPreview.IsolateIndex = proxy.Window._isolateIndex;
                    }
                }

                protected override void Deinitialize()
                {
                    _uvsPreview = null;

                    base.Deinitialize();
                }
            }
        }

        protected sealed class UVsLayoutPreviewControl : RenderToTextureControl
        {
            private int _channel = -1;
            private int _lod, _mesh = -1;
            private int _highlightIndex = -1;
            private int _isolateIndex = -1;
            public ModelBaseWindow<TAsset, TWindow> Window;

            public UVsLayoutPreviewControl()
            {
                Offsets = new Margin(4);
                AutomaticInvalidate = false;
            }

            public int Channel
            {
                set
                {
                    if (_channel == value)
                        return;
                    _channel = value;
                    Visible = _channel != -1;
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

            private void DrawMeshUVs(int meshIndex, ref MeshDataCache.MeshData meshData, ref Rectangle bounds)
            {
                if (meshData.IndexBuffer == null || meshData.VertexAccessor == null)
                {
                    Render2D.DrawText(Style.Current.FontMedium, "Missing mesh data", bounds, Color.Red, TextAlignment.Center, TextAlignment.Center);
                    return;
                }
                var linesColor = _highlightIndex != -1 && _highlightIndex == meshIndex ? Style.Current.BackgroundSelected : Color.White;
                var texCoordStream = meshData.VertexAccessor.TexCoord(_channel);
                if (!texCoordStream.IsValid)
                {
                    Render2D.DrawText(Style.Current.FontMedium, "Missing texcoords channel", bounds, Color.Yellow, TextAlignment.Center, TextAlignment.Center);
                    return;
                }
                var uvScale = bounds.Size;
                for (int i = 0; i < meshData.IndexBuffer.Length; i += 3)
                {
                    // Cache triangle indices
                    uint i0 = meshData.IndexBuffer[i + 0];
                    uint i1 = meshData.IndexBuffer[i + 1];
                    uint i2 = meshData.IndexBuffer[i + 2];

                    // Cache triangle uvs positions and transform positions to output target
                    Float2 uv0 = texCoordStream.GetFloat2((int)i0) * uvScale;
                    Float2 uv1 = texCoordStream.GetFloat2((int)i1) * uvScale;
                    Float2 uv2 = texCoordStream.GetFloat2((int)i2) * uvScale;

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
            }

            /// <inheritdoc />
            public override void DrawSelf()
            {
                base.DrawSelf();

                var bounds = new Rectangle(Float2.Zero, Size);
                if (_channel < 0 || bounds.Size.MaxValue < 5.0f)
                    return;
                if (Window._meshData == null)
                    Window._meshData = new MeshDataCache();
                if (!Window._meshData.RequestMeshData(Window._asset))
                {
                    Invalidate();
                    Render2D.DrawText(Style.Current.FontMedium, "Loading...", bounds, Color.White, TextAlignment.Center, TextAlignment.Center);
                    return;
                }

                Render2D.PushClip(bounds);

                var meshDatas = Window._meshData.MeshDatas;
                var lodIndex = Mathf.Clamp(_lod, 0, meshDatas.Length - 1);
                var lod = meshDatas[lodIndex];
                var mesh = Mathf.Clamp(_mesh, -1, lod.Length - 1);
                if (mesh == -1)
                {
                    for (int meshIndex = 0; meshIndex < lod.Length; meshIndex++)
                    {
                        if (_isolateIndex != -1 && _isolateIndex != meshIndex)
                            continue;
                        DrawMeshUVs(meshIndex, ref lod[meshIndex], ref bounds);
                    }
                }
                else
                {
                    DrawMeshUVs(mesh, ref lod[mesh], ref bounds);
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

        protected class ImportPropertiesProxyBase : PropertiesProxyBase
        {
            private ModelImportSettings ImportSettings;

            /// <inheritdoc />
            public override void OnLoad(TWindow window)
            {
                base.OnLoad(window);

                ImportSettings = window._importSettings;
            }

            public void Reimport()
            {
                Editor.Instance.ContentImporting.Reimport((BinaryAssetItem)Window.Item, ImportSettings, true);
            }

            protected class ProxyEditor : ProxyEditorBase
            {
                public override void Initialize(LayoutElementsContainer layout)
                {
                    var proxy = (ImportPropertiesProxyBase)Values[0];
                    if (Utilities.Utils.OnAssetProperties(layout, proxy.Asset))
                        return;

                    var importSettingsGroup = layout.Group("Import Settings");

                    var importSettingsField = typeof(ImportPropertiesProxyBase).GetField(nameof(ImportSettings), BindingFlags.NonPublic | BindingFlags.Instance);
                    var importSettingsValues = new ValueContainer(new ScriptMemberInfo(importSettingsField)) { proxy.ImportSettings };
                    importSettingsGroup.Object(importSettingsValues);

                    // Creates the import path UI
                    var group = layout.Group("Import Path");
                    Utilities.Utils.CreateImportPathUI(group, proxy.Window.Item as BinaryAssetItem);

                    var reimportButton = importSettingsGroup.Button("Reimport");
                    reimportButton.Button.Clicked += () => ((ImportPropertiesProxyBase)Values[0]).Reimport();
                }
            }
        }

        protected readonly SplitPanel _split;
        protected readonly Tabs _tabs;
        protected readonly ToolStripButton _saveButton;

        protected ModelImportSettings _importSettings = new ModelImportSettings();
        protected bool _refreshOnLODsLoaded;
        protected bool _skipEffectsGuiEvents;
        protected int _isolateIndex = -1;
        protected int _highlightIndex = -1;

        protected MeshDataCache _meshData;

        /// <inheritdoc />
        protected ModelBaseWindow(Editor editor, AssetItem item)
        : base(editor, item)
        {
            var inputOptions = Editor.Options.Options.Input;

            // Toolstrip
            _saveButton = _toolstrip.AddButton(editor.Icons.Save64, Save).LinkTooltip("Save", ref inputOptions.Save);

            // Split Panel
            _split = new SplitPanel(Orientation.Horizontal, ScrollBars.None, ScrollBars.None)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, _toolstrip.Bottom, 0),
                SplitterValue = 0.59f,
                Parent = this
            };

            // Properties tabs
            _tabs = new Tabs
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                TabsSize = new Float2(60, 20),
                TabsTextHorizontalAlignment = TextAlignment.Center,
                UseScroll = true,
                Parent = _split.Panel2
            };
        }

        /// <summary>
        /// Updates the highlight/isolate effects on a model asset.
        /// </summary>
        protected abstract void UpdateEffectsOnAsset();

        /// <inheritdoc />
        protected override void UpdateToolstrip()
        {
            _saveButton.Enabled = IsEdited;

            base.UpdateToolstrip();
        }

        /// <inheritdoc />
        protected override void UnlinkItem()
        {
            _meshData?.WaitForMeshDataRequestEnd();
            foreach (var child in _tabs.Children)
            {
                if (child is Tab tab && tab.Proxy?.Window != null)
                    tab.Proxy?.OnClean();
            }

            base.UnlinkItem();
        }

        /// <inheritdoc />
        protected override void OnAssetLoaded()
        {
            _refreshOnLODsLoaded = true;
            Editor.TryRestoreImportOptions(ref _importSettings.Settings, Item.Path);
            UpdateEffectsOnAsset();
            foreach (var child in _tabs.Children)
            {
                if (child is Tab tab)
                {
                    tab.Proxy?.OnLoad((TWindow)this);
                    tab.Presenter.BuildLayout();
                }
            }
            ClearEditedFlag();

            base.OnAssetLoaded();
        }

        /// <inheritdoc />
        public override void OnItemReimported(ContentItem item)
        {
            // Discard any old mesh data cache
            _meshData?.Dispose();

            // Refresh the properties (will get new data in OnAssetLoaded)
            foreach (var child in _tabs.Children)
            {
                if (child is Tab tab)
                {
                    tab.Proxy.OnClean();
                    tab.Presenter.BuildLayout();
                }
            }
            ClearEditedFlag();
            _refreshOnLODsLoaded = true;

            base.OnItemReimported(item);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            // Free mesh memory
            _meshData?.Dispose();
            _meshData = null;

            base.OnDestroy();
        }

        /// <inheritdoc />
        public override bool UseLayoutData => true;

        /// <inheritdoc />
        public override void OnLayoutSerialize(XmlWriter writer)
        {
            LayoutSerializeSplitter(writer, "Split", _split);
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize(XmlElement node)
        {
            LayoutDeserializeSplitter(node, "Split", _split);
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize()
        {
            _split.SplitterValue = 0.59f;
        }
    }
}
