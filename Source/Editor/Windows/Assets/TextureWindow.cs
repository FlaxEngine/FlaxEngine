// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
    /// Texture window allows to view and edit <see cref="Texture"/> asset.
    /// </summary>
    /// <seealso cref="Texture" />
    /// <seealso cref="FlaxEditor.Windows.Assets.AssetEditorWindow" />
    public sealed class TextureWindow : AssetEditorWindowBase<Texture>
    {
        /// <summary>
        /// The texture properties proxy object.
        /// </summary>
        [CustomEditor(typeof(ProxyEditor))]
        private sealed class PropertiesProxy
        {
            private TextureWindow _window;

            [EditorOrder(1000), EditorDisplay("Import Settings", EditorDisplayAttribute.InlineStyle)]
            public TextureImportSettings ImportSettings = new TextureImportSettings();

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
                        group.Label(string.Format("Size: {0}x{1}", texture.Width, texture.Height));
                        group.Label("Mip levels: " + texture.MipLevels);
                        group.Label("Memory usage: " + Utilities.Utils.FormatBytesCount(texture.TotalMemoryUsage));
                    }

                    base.Initialize(layout);

                    layout.Space(10);
                    var reimportButton = layout.Button("Reimport");
                    reimportButton.Button.Clicked += () => ((PropertiesProxy)Values[0]).Reimport();
                }
            }

            /// <summary>
            /// Gathers parameters from the specified texture.
            /// </summary>
            /// <param name="window">The asset window.</param>
            public void OnLoad(TextureWindow window)
            {
                // Link
                _window = window;

                // Try to restore target asset texture import options (useful for fast reimport)
                TextureImportSettings.TryRestore(ref ImportSettings, window.Item.Path);

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
        private readonly TexturePreview _preview;
        private readonly CustomEditorPresenter _propertiesEditor;

        private readonly PropertiesProxy _properties;
        private bool _isWaitingForLoad;

        /// <inheritdoc />
        public TextureWindow(Editor editor, AssetItem item)
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
            _preview = new TexturePreview(true)
            {
                Parent = _split.Panel1
            };

            // Texture properties editor
            _propertiesEditor = new CustomEditorPresenter(null);
            _propertiesEditor.Panel.Parent = _split.Panel2;
            _properties = new PropertiesProxy();
            _propertiesEditor.Select(_properties);

            // Toolstrip
            _toolstrip.AddButton(Editor.Icons.Import32, () => Editor.ContentImporting.Reimport((BinaryAssetItem)Item)).LinkTooltip("Reimport");
            _toolstrip.AddSeparator();
            _toolstrip.AddButton(Editor.Icons.PageScale32, _preview.CenterView).LinkTooltip("Center view");
            _toolstrip.AddSeparator();
            _toolstrip.AddButton(editor.Icons.Docs32, () => Platform.OpenUrl(Utilities.Constants.DocsUrl + "manual/graphics/textures/index.html")).LinkTooltip("See documentation to learn more");
        }

        /// <inheritdoc />
        protected override void UnlinkItem()
        {
            _properties.OnClean();
            _preview.Asset = null;
            _isWaitingForLoad = false;

            base.UnlinkItem();
        }

        /// <inheritdoc />
        protected override void OnAssetLinked()
        {
            _preview.Asset = _asset;
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
            writer.WriteAttributeString("Split", _split.SplitterValue.ToString());
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize(XmlElement node)
        {
            if (float.TryParse(node.GetAttribute("Split"), out float value1))
                _split.SplitterValue = value1;
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize()
        {
            _split.SplitterValue = 0.7f;
        }
    }
}
