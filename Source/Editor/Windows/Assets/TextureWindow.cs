// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System.IO;
using System.Xml;
using FlaxEditor.Content;
using FlaxEditor.Content.Import;
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Dedicated;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.GUI;
using FlaxEditor.Scripting;
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
        /// Properties base class.
        /// </summary>
        public class PropertiesProxyBase
        {
            internal TextureWindow _window;

            /// <summary>
            /// Gathers parameters from the specified texture.
            /// </summary>
            /// <param name="window">The asset window.</param>
            public virtual void OnLoad(TextureWindow window)
            {
                // Link
                _window = window;
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

        [CustomEditor(typeof(ProxyEditor))]
        private sealed class TexturePropertiesProxy : PropertiesProxyBase
        {
            private sealed class ProxyEditor : GenericEditor
            {
                public override void Initialize(LayoutElementsContainer layout)
                {
                    var window = ((TexturePropertiesProxy)Values[0])._window;
                    var texture = window?.Asset;
                    if (texture == null || !texture.IsLoaded)
                    {
                        layout.Label("Loading...", TextAlignment.Center);
                        return;
                    }

                    // Texture info
                    var general = layout.Group("General");
                    general.Label("Format: " + texture.Format);
                    general.Label(string.Format("Size: {0}x{1}", texture.Width, texture.Height)).AddCopyContextMenu();
                    general.Label("Mip levels: " + texture.MipLevels);
                    general.Label("Memory usage: " + Utilities.Utils.FormatBytesCount(texture.TotalMemoryUsage)).AddCopyContextMenu();

                    // Texture properties
                    var properties = layout.Group("Properties");
                    var textureGroup = new CustomValueContainer(new ScriptType(typeof(int)), texture.TextureGroup,
                                                                (instance, index) => texture.TextureGroup,
                                                                (instance, index, value) =>
                                                                {
                                                                    texture.TextureGroup = (int)value;
                                                                    window.MarkAsEdited();
                                                                });
                    properties.Property("Texture Group", textureGroup, new TextureGroupEditor(), "The texture group used by this texture.");
                }
            }
        }

        /// <summary>
        /// The texture import properties proxy object.
        /// </summary>
        [CustomEditor(typeof(ProxyEditor))]
        private sealed class ImportPropertiesProxy : PropertiesProxyBase
        {
            [EditorOrder(1000), EditorDisplay("Import Settings", EditorDisplayAttribute.InlineStyle)]
            public FlaxEngine.Tools.TextureTool.Options ImportSettings = new();

            /// <summary>
            /// Gathers parameters from the specified texture.
            /// </summary>
            /// <param name="window">The asset window.</param>
            public override void OnLoad(TextureWindow window)
            {
                base.OnLoad(window);

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

            private sealed class ProxyEditor : GenericEditor
            {
                public override void Initialize(LayoutElementsContainer layout)
                {
                    var proxy = (ImportPropertiesProxy)Values[0];
                    if (proxy._window == null)
                    {
                        layout.Label("Loading...", TextAlignment.Center);
                        return;
                    }

                    // Import settings
                    base.Initialize(layout);

                    // Creates the import path UI
                    Utilities.Utils.CreateImportPathUI(layout, proxy._window.Item as BinaryAssetItem);

                    // Reimport
                    layout.Space(5);
                    var reimportButton = layout.Button("Reimport");
                    reimportButton.Button.Clicked += () => ((ImportPropertiesProxy)Values[0]).Reimport();
                }
            }
        }

        private class Tab : GUI.Tabs.Tab
        {
            /// <summary>
            /// The presenter to use in the tab.
            /// </summary>
            public CustomEditorPresenter Presenter;

            /// <summary>
            /// The proxy to use in the tab.
            /// </summary>
            public PropertiesProxyBase Proxy;

            public Tab(string text, TextureWindow window, bool modifiesAsset = true)
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

        private class TextureTab : Tab
        {
            public TextureTab(TextureWindow window)
            : base("Texture", window)
            {
                Proxy = new TexturePropertiesProxy();
                Presenter.Select(Proxy);
            }
        }

        private class ImportTab : Tab
        {
            public ImportTab(TextureWindow window)
            : base("Import", window)
            {
                Proxy = new ImportPropertiesProxy();
                Presenter.Select(Proxy);
            }
        }

        private readonly GUI.Tabs.Tabs _tabs;
        private readonly SplitPanel _split;
        private readonly TexturePreview _preview;
        private readonly ToolStripButton _saveButton;
        private bool _isWaitingForLoad;

        /// <inheritdoc />
        public TextureWindow(Editor editor, AssetItem item)
        : base(editor, item)
        {
            var inputOptions = Editor.Options.Options.Input;

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

            // Properties tabs
            _tabs = new()
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                TabsSize = new Float2(60, 20),
                TabsTextHorizontalAlignment = TextAlignment.Center,
                UseScroll = true,
                Parent = _split.Panel2
            };

            _tabs.AddTab(new TextureTab(this));
            _tabs.AddTab(new ImportTab(this));

            // Toolstrip
            _saveButton = (ToolStripButton)_toolstrip.AddButton(Editor.Icons.Save64, Save).LinkTooltip("Save", ref inputOptions.Save);
            _toolstrip.AddButton(Editor.Icons.Import64, () => Editor.ContentImporting.Reimport((BinaryAssetItem)Item)).LinkTooltip("Reimport");
            _toolstrip.AddSeparator();
            _toolstrip.AddButton(Editor.Icons.CenterView64, _preview.CenterView).LinkTooltip("Center view");
            _toolstrip.AddSeparator();
            _toolstrip.AddButton(editor.Icons.Docs64, () => Platform.OpenUrl(Utilities.Constants.DocsUrl + "manual/graphics/textures/index.html")).LinkTooltip("See documentation to learn more");
        }

        /// <inheritdoc />
        protected override void UnlinkItem()
        {
            foreach (var child in _tabs.Children)
            {
                if (child is Tab tab && tab.Proxy != null)
                    tab.Proxy.OnClean();
            }
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
        protected override void UpdateToolstrip()
        {
            _saveButton.Enabled = IsEdited;

            base.UpdateToolstrip();
        }

        /// <inheritdoc />
        public override void Save()
        {
            if (!IsEdited)
                return;

            if (Asset.Save())
            {
                Editor.LogError("Cannot save asset.");
                return;
            }

            ClearEditedFlag();
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
                foreach (var child in _tabs.Children)
                {
                    if (child is Tab tab && tab.Proxy != null)
                    {
                        tab.Proxy.OnLoad(this);
                        tab.Presenter.BuildLayout();
                    }
                }

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
