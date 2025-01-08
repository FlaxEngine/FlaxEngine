// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System.IO;
using System.Xml;
using FlaxEditor.Content;
using FlaxEditor.Content.Import;
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.Viewport.Previews;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows.Assets
{
    /// <summary>
    /// Editor window to view/modify <see cref="CubeTexture"/> asset.
    /// </summary>
    /// <seealso cref="CubeTexture" />
    /// <seealso cref="FlaxEditor.Windows.Assets.AssetEditorWindow" />
    public sealed class CubeTextureWindow : AssetEditorWindowBase<CubeTexture>
    {
        /// <summary>
        /// The texture properties proxy object.
        /// </summary>
        [CustomEditor(typeof(ProxyEditor))]
        private sealed class PropertiesProxy
        {
            private CubeTextureWindow _window;

            [EditorOrder(1000), EditorDisplay("Import Settings", EditorDisplayAttribute.InlineStyle)]
            public FlaxEngine.Tools.TextureTool.Options ImportSettings = new();

            public sealed class ProxyEditor : GenericEditor
            {
                public override void Initialize(LayoutElementsContainer layout)
                {
                    var window = ((PropertiesProxy)Values[0])._window;
                    if (window == null)
                    {
                        layout.Label("Loading...", TextAlignment.Center);
                        return;
                    }

                    // Texture properties
                    {
                        var texture = window.Asset;

                        var group = layout.Group("General");
                        group.Label("Format: " + texture.Format);
                        group.Label(string.Format("Size: {0}x{1}", texture.Width, texture.Height)).AddCopyContextMenu();
                        group.Label("Mip levels: " + texture.MipLevels);
                        group.Label("Memory usage: " + Utilities.Utils.FormatBytesCount(texture.TotalMemoryUsage));
                    }

                    base.Initialize(layout);

                    // Creates the import path UI
                    Utilities.Utils.CreateImportPathUI(layout, window.Item as BinaryAssetItem);

                    layout.Space(5);
                    var reimportButton = layout.Button("Reimport");
                    reimportButton.Button.Clicked += () => ((PropertiesProxy)Values[0]).Reimport();
                }
            }

            /// <summary>
            /// Gathers parameters from the specified texture.
            /// </summary>
            /// <param name="window">The asset window.</param>
            public void OnLoad(CubeTextureWindow window)
            {
                // Link
                _window = window;

                // Try to restore target asset texture import options (useful for fast reimport)
                Editor.TryRestoreImportOptions(ref ImportSettings, window.Item.Path);

                // Prepare restore data
                PeekState();
            }

            /// <summary>
            /// Records the current state to restore it on DiscardChanges.
            /// </summary>
            public void PeekState()
            {
            }

            /// <summary>
            /// Reimports asset.
            /// </summary>
            public void Reimport()
            {
                Editor.Instance.ContentImporting.Reimport((BinaryAssetItem)_window.Item, ImportSettings);
            }

            /// <summary>
            /// On discard changes
            /// </summary>
            public void DiscardChanges()
            {
            }

            /// <summary>
            /// Clears temporary data.
            /// </summary>
            public void OnClean()
            {
                // Unlink
                _window = null;
            }
        }

        private readonly SplitPanel _split;
        private readonly CubeTexturePreview _preview;
        private readonly CustomEditorPresenter _propertiesEditor;

        private readonly PropertiesProxy _properties;
        private bool _isWaitingForLoad;

        /// <inheritdoc />
        public CubeTextureWindow(Editor editor, AssetItem item)
        : base(editor, item)
        {
            // Split Panel
            _split = new SplitPanel(Orientation.Horizontal, ScrollBars.None, ScrollBars.Vertical)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, _toolstrip.Bottom, 0),
                SplitterValue = 0.7f,
                Parent = this
            };

            // Texture preview
            _preview = new CubeTexturePreview(true)
            {
                Parent = _split.Panel1
            };

            // Texture properties editor
            _propertiesEditor = new CustomEditorPresenter(null);
            _propertiesEditor.Panel.Parent = _split.Panel2;
            _properties = new PropertiesProxy();
            _propertiesEditor.Select(_properties);

            // Toolstrip
            _toolstrip.AddButton(Editor.Icons.Import64, () => Editor.ContentImporting.Reimport((BinaryAssetItem)Item)).LinkTooltip("Reimport");
            _toolstrip.AddSeparator();
            _toolstrip.AddButton(editor.Icons.Docs64, () => Platform.OpenUrl(Utilities.Constants.DocsUrl + "manual/graphics/textures/cube-textures.html")).LinkTooltip("See documentation to learn more");
        }

        /// <inheritdoc />
        protected override void UnlinkItem()
        {
            _properties.OnClean();
            _preview.CubeTexture = null;
            _isWaitingForLoad = false;

            base.UnlinkItem();
        }

        /// <inheritdoc />
        protected override void OnAssetLinked()
        {
            _preview.CubeTexture = _asset;
            _isWaitingForLoad = true;

            base.OnAssetLinked();
        }

        /// <inheritdoc />
        public override void OnItemReimported(ContentItem item)
        {
            // Invalidate data
            _isWaitingForLoad = true;
        }

        /// <inheritdoc />
        protected override void OnClose()
        {
            // Discard unsaved changes
            _properties.DiscardChanges();

            base.OnClose();
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);

            // Check if need to load
            if (_isWaitingForLoad && _asset.IsLoaded)
            {
                // Clear flag
                _isWaitingForLoad = false;

                // Init properties and parameters proxy
                _properties.OnLoad(this);
                _propertiesEditor.BuildLayout();

                // Setup
                ClearEditedFlag();
            }
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
            _split.SplitterValue = 0.7f;
        }
    }
}
