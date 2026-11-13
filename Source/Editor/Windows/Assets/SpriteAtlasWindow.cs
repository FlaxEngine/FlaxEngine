// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.Content;
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Scripting;
using FlaxEditor.Viewport.Previews;
using FlaxEngine;
using FlaxEngine.GUI;
using System.Linq;

namespace FlaxEditor.Windows.Assets
{
    /// <summary>
    /// Sprite Atlas window allows to view and edit <see cref="SpriteAtlas"/> asset.
    /// </summary>
    /// <seealso cref="SpriteAtlas" />
    /// <seealso cref="FlaxEditor.Windows.Assets.AssetEditorWindow" />
    public sealed class SpriteAtlasWindow : TextureBaseWindow
    {
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
        private sealed class SpritePropertiesProxy : PropertiesProxyBase
        {
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

            private SpriteEntry[] _savedSprites;

            [EditorOrder(0), EditorDisplay("Sprites")]
            [CustomEditor(typeof(SpritesCollectionEditor))]
            public SpriteEntry[] Sprites;

            public sealed class ProxyEditor : GenericEditor
            {
                public override void Initialize(LayoutElementsContainer layout)
                {
                    var proxy = (SpritePropertiesProxy)Values[0];
                    if (proxy._window == null)
                    {
                        layout.Label("Loading...", TextAlignment.Center);
                        return;
                    }
                    
                    base.Initialize(layout);
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
                    var window = ((SpritePropertiesProxy)ParentEditor.Values[0])._window;
                    var atlas = (SpriteAtlas)window.Asset;
                    var index = (int)button.Tag;
                    var sprite = atlas.GetSprite(index);
                    Clipboard.Text = FlaxEngine.Json.JsonSerializer.Serialize(sprite, typeof(Sprite));
                }

                private void OnPasteSpriteClicked(ContextMenuButton button)
                {
                    var window = ((SpritePropertiesProxy)ParentEditor.Values[0])._window;
                    var atlas = (SpriteAtlas)window.Asset;
                    var index = (int)button.Tag;
                    var sprite = atlas.GetSprite(index);
                    var pasted = FlaxEngine.Json.JsonSerializer.Deserialize<Sprite>(Clipboard.Text);
                    sprite.Area = pasted.Area;
                    atlas.SetSprite(index, ref sprite);
                }

                private void OnDeleteSpriteClicked(ContextMenuButton button)
                {
                    var window = ((SpritePropertiesProxy)ParentEditor.Values[0])._window;
                    var atlas = (SpriteAtlas)window.Asset;
                    var index = (int)button.Tag;
                    atlas.RemoveSprite(index);
                    window.MarkAsEdited();
                    //window._properties.UpdateSprites();
                    //window._propertiesEditor.BuildLayout();
                }
            }

            /// <summary>
            /// Updates the sprites collection.
            /// </summary>
            public void UpdateSprites()
            {
                var atlas = (SpriteAtlas)_window.Asset;
                Sprites = new SpriteEntry[atlas.SpritesCount];
                for (int i = 0; i < Sprites.Length; i++)
                {
                    Sprites[i] = new SpriteEntry(atlas, i);
                }
            }

            /// <summary>
            /// On discard changes
            /// </summary>
            public void DiscardChanges()
            {
            }

            public override void OnLoad(TextureBaseWindow window)
            {
                base.OnLoad(window);

                UpdateSprites();
                _savedSprites = (SpriteEntry[])Sprites.Clone();
            }

            public override void OnClean()
            {
                Sprites = null;
                _savedSprites = null;
                base.OnClean();
            }
        }

        private class SpritesTab : Tab
        {
            public SpritesTab(SpriteAtlasWindow window)
            : base("Sprites", window)
            {
                Proxy = new SpritePropertiesProxy();
                Presenter.Select(Proxy);
            }
        }

        private SpritesTab _spritesTab;
        private Sprite[] _savedSprites;

        /// <inheritdoc />
        public SpriteAtlasWindow(Editor editor, AssetItem item)
        : base(editor, item, new AtlasView(true))
        {
            _spritesTab = _tabs.AddTab(new SpritesTab(this));
            _spritesTab.IndexInParent -= 2;
            _tabs.SelectedTab = _spritesTab;
            _tabs.OnChildrenChanged();
            _toolstrip.AddButton(editor.Icons.AddFile64, AddSprite).LinkTooltip("Add a new sprite");
        }

        private void AddSprite()
        {
            var atlas = (SpriteAtlas)Asset;
            var sprite = new Sprite
            {
                Name = Utilities.Utils.IncrementNameNumber("New Sprite", name => atlas.Sprites.All(s => s.Name != name)),
                Area = new Rectangle(Float2.Zero, Float2.One),
            };
            atlas.AddSprite(sprite);
            MarkAsEdited();
            ((SpritePropertiesProxy)_spritesTab.Proxy).UpdateSprites();
            _spritesTab.Presenter.BuildLayout();
        }

        /// <inheritdoc />
        protected override void UnlinkItem()
        {
            ((AtlasView)_preview).Asset = null;

            base.UnlinkItem();
        }

        /// <inheritdoc />
        protected override void OnAssetLinked()
        {
            ((AtlasView)_preview).Asset = (SpriteAtlas)_asset;

            base.OnAssetLinked();
        }

        /// <inheritdoc />
        protected override void OnAssetLoaded()
        {
            // Cache state before editing
            _savedSprites = ((SpriteAtlas)Asset).Sprites;

            base.OnAssetLoaded();
        }

        /// <inheritdoc />
        protected override void OnClose()
        {
            if (Asset)
            {
                // Discard unsaved changes
                ((SpriteAtlas)Asset).Sprites = _savedSprites;
            }
            _savedSprites = null;

            base.OnClose();
        }
    }
}
