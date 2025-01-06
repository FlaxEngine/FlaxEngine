// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System.Xml;
using FlaxEditor.Content;
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.GUI;
using FlaxEditor.GUI.Tabs;
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
                Presenter.Deselect();
                Presenter = null;
                Proxy = null;

                base.OnDestroy();
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

            private void DrawMeshUVs(int meshIndex, ref MeshDataCache.MeshData meshData)
            {
                var uvScale = Size;
                if (meshData.IndexBuffer == null || meshData.VertexAccessor == null)
                    return;
                var linesColor = _highlightIndex != -1 && _highlightIndex == meshIndex ? Style.Current.BackgroundSelected : Color.White;
                var texCoordStream = meshData.VertexAccessor.TexCoord(_channel);
                if (!texCoordStream.IsValid)
                    return;
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

                var size = Size;
                if (_channel < 0 || size.MaxValue < 5.0f)
                    return;
                if (Window._meshData == null)
                    Window._meshData = new MeshDataCache();
                if (!Window._meshData.RequestMeshData(Window._asset))
                {
                    Invalidate();
                    Render2D.DrawText(Style.Current.FontMedium, "Loading...", new Rectangle(Float2.Zero, size), Color.White, TextAlignment.Center, TextAlignment.Center);
                    return;
                }

                Render2D.PushClip(new Rectangle(Float2.Zero, size));

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
                        DrawMeshUVs(meshIndex, ref lod[meshIndex]);
                    }
                }
                else
                {
                    DrawMeshUVs(mesh, ref lod[mesh]);
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

        protected readonly SplitPanel _split;
        protected readonly Tabs _tabs;
        protected readonly ToolStripButton _saveButton;

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
                if (child is Tab tab && tab.Proxy.Window != null)
                    tab.Proxy.OnClean();
            }

            base.UnlinkItem();
        }

        /// <inheritdoc />
        protected override void OnAssetLoaded()
        {
            foreach (var child in _tabs.Children)
            {
                if (child is Tab tab)
                {
                    tab.Proxy.OnLoad((TWindow)this);
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
