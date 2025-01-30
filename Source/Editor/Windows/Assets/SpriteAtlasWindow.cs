// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System.IO;
using System.Linq;
using System.Xml;
using FlaxEditor.Content;
using FlaxEditor.Content.Import;
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Scripting;
using FlaxEditor.Viewport.Previews;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows.Assets
{
    /// <summary>
    /// Sprite Atlas window allows to view and edit <see cref="SpriteAtlas"/> asset.
    /// </summary>
    /// <seealso cref="SpriteAtlas" />
    /// <seealso cref="FlaxEditor.Windows.Assets.AssetEditorWindow" />
    public sealed class SpriteAtlasWindow : AssetEditorWindowBase<SpriteAtlas>
    {
        // TODO: allow to select and move sprites
        // TODO: restore changes on win close without a changes

        /// <summary>
        /// Atlas view control. Shows sprites.
        /// </summary>
        /// <seealso cref="FlaxEditor.Viewport.Previews.SpriteAtlasPreview" />
        private sealed class AtlasView : SpriteAtlasPreview
        {
            public AtlasView(bool useWidgets)
            : base(useWidgets)
            {
            }

            protected override void DrawTexture(ref Rectangle rect)
            {
                base.DrawTexture(ref rect);

                if (Asset && Asset.IsLoaded)
                {
                    var count = Asset.SpritesCount;
                    var style = Style.Current;

                    // Draw all splits
                    for (int i = 0; i < count; i++)
                    {
                        var sprite = Asset.GetSprite(i);
                        var area = sprite.Area;
                        var position = area.Location * rect.Size + rect.Location;
                        var size = area.Size * rect.Size;
                        Render2D.DrawRectangle(new Rectangle(position, size), style.BackgroundSelected);
                    }
                }
            }
        }

        /// <summary>
        /// The texture properties proxy object.
        /// </summary>
        [CustomEditor(typeof(ProxyEditor))]
        private sealed class PropertiesProxy
        {
            private SpriteAtlasWindow _window;

            public class SpriteEntry
            {
                [HideInEditor]
                public SpriteHandle Sprite;

                public SpriteEntry(SpriteAtlas atlas, int index)
                {
                    Sprite = new SpriteHandle(atlas, index);
                }

                [EditorOrder(0)]
                public string Name
                {
                    get => Sprite.Name;
                    set => Sprite.Name = value;
                }

                [EditorOrder(1)]
                public Float2 Location
                {
                    get => Sprite.Location;
                    set => Sprite.Location = value;
                }

                [EditorOrder(3), Limit(0)]
                public Float2 Size
                {
                    get => Sprite.Size;
                    set => Sprite.Size = value;
                }
            }

            [EditorOrder(0), EditorDisplay("Sprites")]
            [CustomEditor(typeof(SpritesCollectionEditor))]
            public SpriteEntry[] Sprites;

            [EditorOrder(1000), EditorDisplay("Import Settings", EditorDisplayAttribute.InlineStyle)]
            public FlaxEngine.Tools.TextureTool.Options ImportSettings = new();

            public sealed class ProxyEditor : GenericEditor
            {
                public override void Initialize(LayoutElementsContainer layout)
                {
                    var proxy = (PropertiesProxy)Values[0];
                    if (proxy._window == null)
                    {
                        layout.Label("Loading...", TextAlignment.Center);
                        return;
                    }
                    
                    base.Initialize(layout);
                    
                    // Creates the import path UI
                    Utilities.Utils.CreateImportPathUI(layout, proxy._window.Item as BinaryAssetItem);

                    layout.Space(5);
                    var reimportButton = layout.Button("Reimport");
                    reimportButton.Button.Clicked += () => ((PropertiesProxy)Values[0]).Reimport();
                }
            }

            public sealed class SpritesCollectionEditor : CustomEditor
            {
                public override DisplayStyle Style => DisplayStyle.InlineIntoParent;

                public override void Initialize(LayoutElementsContainer layout)
                {
                    var sprites = (SpriteEntry[])Values[0];
                    if (sprites != null)
                    {
                        var elementType = new ScriptType(typeof(SpriteEntry));
                        for (int i = 0; i < sprites.Length; i++)
                        {
                            var group = layout.Group(sprites[i].Name);
                            group.Panel.Tag = i;
                            group.Panel.MouseButtonRightClicked += OnGroupPanelMouseButtonRightClicked;
                            group.Object(new ListValueContainer(elementType, i, Values));

                            var stringNameElement = group.Editors[0].ChildrenEditors.Find(x => x is StringEditor).Layout.Children.Find(x => x is TextBoxElement) as TextBoxElement;
                            if (stringNameElement != null)
                            {
                                stringNameElement.TextBox.TextBoxEditEnd += (textbox) => OnNameChanged(group.Panel, (TextBox)textbox);
                            }
                        }
                    }
                }

                private void OnNameChanged(DropPanel panel, TextBox textbox)
                {
                    panel.HeaderText = textbox.Text;
                }

                private void OnGroupPanelMouseButtonRightClicked(DropPanel groupPanel, Float2 location)
                {
                    var menu = new ContextMenu();

                    var copySprite = menu.AddButton("Copy sprite");
                    copySprite.Tag = groupPanel.Tag;
                    copySprite.ButtonClicked += OnCopySpriteClicked;

                    var pasteSprite = menu.AddButton("Paste sprite");
                    pasteSprite.Tag = groupPanel.Tag;
                    pasteSprite.ButtonClicked += OnPasteSpriteClicked;

                    var deleteSprite = menu.AddButton("Delete sprite");
                    deleteSprite.Tag = groupPanel.Tag;
                    deleteSprite.ButtonClicked += OnDeleteSpriteClicked;

                    menu.Show(groupPanel, location);
                }

                private void OnCopySpriteClicked(ContextMenuButton button)
                {
                    var window = ((PropertiesProxy)ParentEditor.Values[0])._window;
                    var index = (int)button.Tag;
                    var sprite = window.Asset.GetSprite(index);
                    Clipboard.Text = FlaxEngine.Json.JsonSerializer.Serialize(sprite, typeof(Sprite));
                }

                private void OnPasteSpriteClicked(ContextMenuButton button)
                {
                    var window = ((PropertiesProxy)ParentEditor.Values[0])._window;
                    var index = (int)button.Tag;
                    var sprite = window.Asset.GetSprite(index);
                    var pasted = FlaxEngine.Json.JsonSerializer.Deserialize<Sprite>(Clipboard.Text);
                    sprite.Area = pasted.Area;
                    window.Asset.SetSprite(index, ref sprite);
                }

                private void OnDeleteSpriteClicked(ContextMenuButton button)
                {
                    var window = ((PropertiesProxy)ParentEditor.Values[0])._window;
                    var index = (int)button.Tag;
                    window.Asset.RemoveSprite(index);
                    window.MarkAsEdited();
                    window._properties.UpdateSprites();
                    window._propertiesEditor.BuildLayout();
                }
            }

            /// <summary>
            /// Updates the sprites collection.
            /// </summary>
            public void UpdateSprites()
            {
                var atlas = _window.Asset;
                Sprites = new SpriteEntry[atlas.SpritesCount];
                for (int i = 0; i < Sprites.Length; i++)
                {
                    Sprites[i] = new SpriteEntry(atlas, i);
                }
            }

            /// <summary>
            /// Gathers parameters from the specified texture.
            /// </summary>
            /// <param name="win">The texture window.</param>
            public void OnLoad(SpriteAtlasWindow win)
            {
                // Link
                _window = win;
                UpdateSprites();

                // Try to restore target asset texture import options (useful for fast reimport)
                Editor.TryRestoreImportOptions(ref ImportSettings, win.Item.Path);

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
                ImportSettings.Sprites = null; // Don't override sprites (use sprites from asset)
                Editor.Instance.ContentImporting.Reimport((BinaryAssetItem)_window.Item, ImportSettings, true);
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
                Sprites = null;
            }
        }

        private readonly SplitPanel _split;
        private readonly AtlasView _preview;
        private readonly CustomEditorPresenter _propertiesEditor;
        private readonly ToolStripButton _saveButton;

        private readonly PropertiesProxy _properties;
        private bool _isWaitingForLoad;

        /// <inheritdoc />
        public SpriteAtlasWindow(Editor editor, AssetItem item)
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

            // Sprite atlas preview
            _preview = new AtlasView(true)
            {
                Parent = _split.Panel1
            };

            // Sprite atlas properties editor
            _propertiesEditor = new CustomEditorPresenter(null);
            _propertiesEditor.Panel.Parent = _split.Panel2;
            _properties = new PropertiesProxy();
            _propertiesEditor.Select(_properties);
            _propertiesEditor.Modified += MarkAsEdited;

            // Toolstrip
            _saveButton = _toolstrip.AddButton(editor.Icons.Save64, Save).LinkTooltip("Save", ref inputOptions.Save);
            _toolstrip.AddButton(editor.Icons.Import64, () => Editor.ContentImporting.Reimport((BinaryAssetItem)Item)).LinkTooltip("Reimport");
            _toolstrip.AddSeparator();
            _toolstrip.AddButton(editor.Icons.AddFile64, () =>
            {
                var sprite = new Sprite
                {
                    Name = Utilities.Utils.IncrementNameNumber("New Sprite", name => Asset.Sprites.All(s => s.Name != name)),
                    Area = new Rectangle(Float2.Zero, Float2.One),
                };
                Asset.AddSprite(sprite);
                MarkAsEdited();
                _properties.UpdateSprites();
                _propertiesEditor.BuildLayout();
            }).LinkTooltip("Add a new sprite");
            _toolstrip.AddSeparator();
            _toolstrip.AddButton(editor.Icons.CenterView64, _preview.CenterView).LinkTooltip("Center view");
        }

        /// <inheritdoc />
        public override void Save()
        {
            if (!IsEdited)
                return;

            if (Asset.SaveSprites())
            {
                Editor.LogError("Cannot save asset.");
                return;
            }

            ClearEditedFlag();
            _item.RefreshThumbnail();
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
