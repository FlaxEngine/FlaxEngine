// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.Linq;
using System.Runtime.InteropServices;
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
            internal Float2 Mouselocation;
            internal int SpriteIDUnderTheMouse;
            internal int MarkedSpriteID;
            internal bool HasSelection = false;
            internal bool IsMouseOverSelection = false;


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
                    IsMouseOverSelection = false;
                    // Draw all splits
                    for (int i = 0; i < count; i++)
                    {
                        var rectangle = GetRectangle(ref rect, i);
                        if (rectangle.Contains(Mouselocation))
                        {
                            SpriteIDUnderTheMouse = i;
                            IsMouseOverSelection = true;
                        }
                        else
                        {
                            Render2D.DrawRectangle(rectangle, style.BorderNormal);
                        }
                    }
                    if (IsMouseOverSelection)
                    {
                        Render2D.DrawRectangle(GetRectangle(ref rect, SpriteIDUnderTheMouse), style.BorderSelected);
                    }
                    if (HasSelection)
                    {

                        Render2D.DrawRectangle(GetRectangle(ref rect, MarkedSpriteID), Color.Lime);
                    }
                }
            }
            /// <summary>
            /// 
            /// </summary>
            /// <returns>if mark was succesfull</returns>
            internal bool MarkSelected()
            {
                var r = TextureViewRect;
                if (IsMouseOverSelection)
                {
                    MarkedSpriteID = SpriteIDUnderTheMouse;
                    HasSelection = true;
                    return true;
                }
                return false;
            }

            private Rectangle GetRectangle(ref Rectangle rect,int Spriteindex)
            {
                var sprite = Asset.GetSprite(Spriteindex);
                var area = sprite.Area;
                var position = area.Location * rect.Size + rect.Location;
                var size = area.Size * rect.Size;
                return new Rectangle(position, size);
            }

            public override void OnMouseMove(Float2 location)
            {
                this.Mouselocation = location;
                base.OnMouseMove(location);

            }
            /// <summary>
            /// moves sprite pixels to new location
            /// </summary>
            internal void MoveSpritePixels(Float2 from, Float2 to, Float2 spriteSize)
            {
                //init
                var Duplicate = GPUDevice.Instance.CreateTexture("MoveSpritePixels.Duplicate");
                var Ereser = GPUDevice.Instance.CreateTexture("MoveSpritePixels.Ereser");
                var desc = GPUTextureDescription.New2D((int)spriteSize.X, (int)spriteSize.Y, Asset.Format);
                Duplicate.Init(ref desc);
                Ereser.Init(ref desc);

                //crop and coppy
                GPUDevice.Instance.MainContext.CopyTexture(Duplicate, 0, 0, 0, 0, Asset.Texture, 0,new Rectangle(from, spriteSize));
                unsafe
                {
                    //make a clear texture with size of the crop
                    Color[] Buffer = new Color[desc.Width * desc.Height];
                    Array.Fill(Buffer, new Color(0, 0, 0, 0));
                    fixed (void* pBuffer = Buffer)
                    {
                        GPUDevice.Instance.MainContext.UpdateTexture(Ereser, 0, 0, (nint)pBuffer, (uint)desc.Width, (uint)desc.Height);
                    }
                }
                
                
                //paste it back
                GPUDevice.Instance.MainContext.CopyTexture(Asset.Texture, 0, (uint)to.X, (uint)to.Y, 0, Duplicate, 0, new Rectangle(Float2.Zero, spriteSize));
                //erase last
                GPUDevice.Instance.MainContext.CopyTexture(Asset.Texture, 0, (uint)from.X, (uint)from.Y, 0, Ereser, 0, new Rectangle(Float2.Zero, spriteSize));

                Duplicate.ReleaseGPU();
                Ereser.ReleaseGPU();
                FlaxEngine.Object.Destroy(ref Duplicate);
                FlaxEngine.Object.Destroy(ref Ereser);
            }
            /// <summary>
            /// 
            /// </summary>
            /// <returns>Mouse location on the texture</returns>
            public Float2 GetMouseLocationOnTexture()
            {
                return Asset.Size * ((Mouselocation - TextureViewRect.Location) / TextureViewRect.Size);
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

                [EditorOrder(1), Limit(-4096, 4096)]
                public Float2 Location
                {
                    get => Sprite.Location;
                    set => Sprite.Location = value;
                }

                [EditorOrder(3), Limit(0, 4096)]
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
                    base.Initialize(layout);

                    layout.Space(10);
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
                        }
                    }
                }
                private void OnGroupPanelMouseButtonRightClicked(DropPanel groupPanel, Float2 location)
                {
                    var menu = new ContextMenu();

                    var deleteSprite = menu.AddButton("Delete sprite");
                    deleteSprite.Tag = groupPanel.Tag;
                    deleteSprite.ButtonClicked += OnDeleteSpriteClicked;

                    menu.Show(groupPanel, location);
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
                if (_window == null)
                    return;
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
        private readonly ToolStripButton _moveSprite;
        
        private readonly PropertiesProxy _properties;
        private bool _isWaitingForLoad;
        private bool LMBDown;
        private bool moveSprite;
        private DropPanel spriteUnderTheMouseDropPanel;
        private Rectangle beginDragBounds;
        private Float2 offset;
        private bool ctrlDown;
        private bool shiftDown;
        private bool gotEdit;
        private bool lockduplicate;
        private GPUTexture BackUpAtlas;

        Undo _undo = new Undo();
        /// <inheritdoc />
        public SpriteAtlasWindow(Editor editor, AssetItem item)
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
            _saveButton = (ToolStripButton)_toolstrip.AddButton(editor.Icons.Save64, Save).LinkTooltip("Save");
            _toolstrip.AddButton(editor.Icons.Import64, () => Editor.ContentImporting.Reimport((BinaryAssetItem)Item)).LinkTooltip("Reimport");
            _toolstrip.AddSeparator();
            _toolstrip.AddButton(editor.Icons.AddFile64, () =>
            {
                AddSprite(new Rectangle(Float2.Zero, Float2.One));
            }).LinkTooltip("Add a new sprite");
            _toolstrip.AddSeparator();
            _toolstrip.AddButton(editor.Icons.CenterView64, _preview.CenterView).LinkTooltip("Center view");
            _moveSprite = (ToolStripButton)_toolstrip.AddButton(editor.Icons.Paint96, ToggleMoveSprite).LinkTooltip("Enable move sprite");

            InputActions.Add(options => options.Undo, () =>
            {
                _undo.PerformUndo();
                Focus();
            });
            InputActions.Add(options => options.Redo, () =>
            {
                _undo.PerformRedo();
                Focus();
            });
            InputActions.Add(options => options.Delete, () => PuffSelectedSprite());
        }
        private void PuffSelectedSprite()
        {
            //[todo] add undo
            if (_preview.HasSelection)
            {
                Asset.RemoveSprite(_preview.MarkedSpriteID);
                MarkAsEdited();
                _properties.UpdateSprites();
                _propertiesEditor.BuildLayout();
            }
        }
        private int AddSprite(Rectangle area)
        {
            var sprite = new Sprite
            {
                Name = Utilities.Utils.IncrementNameNumber("New Sprite", name => Asset.Sprites.All(s => s.Name != name)),
                Area = area,
            };
            Asset.AddSprite(sprite);
            MarkAsEdited();
            _properties.UpdateSprites();
            _propertiesEditor.BuildLayout();

            return _properties.Sprites.Length -1;
        }
        private class UndoAction : IUndoAction
        {
            public string ActionString => "Sprite Modfyay";
            int spriteID;
            Rectangle cords;

            SpriteAtlasWindow atlas;
            //snap shot of the atlas texture
            GPUTexture snapshot;
            bool doneEditOnTexture;
            public UndoAction(int spriteID, Float2 location, SpriteAtlasWindow atlas,bool doneEditOnTexture)
            {
                this.spriteID           = spriteID;
                this.cords.Location     = location;
                this.atlas              = atlas;
                this.doneEditOnTexture  = doneEditOnTexture;
                snapshot = GPUDevice.Instance.CreateTexture("SpriteAtlasWindow.BackUpAtlas" + spriteID.ToString());
                var desc = GPUTextureDescription.New2D((int)atlas.Asset.Size.X, (int)atlas.Asset.Size.Y, atlas.Asset.Format);
                snapshot.Init(ref desc);
                GPUDevice.Instance.MainContext.CopyTexture(snapshot, 0, 0, 0, 0, atlas.Asset.Texture, 0);
            }
            public void Do()
            {
                //[noir_sc] no idea if it will work
                PreformAction();
            }
            public void Undo()
            {
                PreformAction();
            }
            private void PreformAction()
            {
                atlas._preview.MarkedSpriteID = spriteID;
                atlas._properties.Sprites[spriteID].Location = cords.Location;
                atlas._properties.UpdateSprites();
                if (!doneEditOnTexture)
                    return;
                GPUDevice.Instance.MainContext.CopyTexture(atlas.Asset.Texture, 0, 0, 0, 0, snapshot, 0);
            }
            public void Dispose()
            {
                if (!doneEditOnTexture)
                    return;
                snapshot.ReleaseGPU();
                FlaxEngine.Object.Destroy(ref snapshot);
            }
        }
        /// <inheritdoc />
        public override void Save()
        {
            if (!IsEdited)
                return;

            _undo.Clear();

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
        void ToggleMoveSprite()
        {
            moveSprite = !moveSprite;
            _moveSprite.Checked = moveSprite;

            UpdateToolstrip();
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
            _undo.Clear();
        }

        /// <inheritdoc />
        protected override void OnClose()
        {
            // Discard unsaved changes
            _properties.DiscardChanges();
            _undo.Clear();
            BackUpAtlas.ReleaseGPU();
            FlaxEngine.Object.Destroy(ref BackUpAtlas);

            base.OnClose();
        }

        /// <inheritdoc />
        protected override void OnAssetLoaded()
        {
            _properties.UpdateSprites();
            //create a backup to restore if atlas gets edited
            BackUpAtlas = GPUDevice.Instance.CreateTexture("SpriteAtlasWindow.BackUpAtlas");
            var desc = GPUTextureDescription.New2D((int)Asset.Size.X, (int)Asset.Size.Y, Asset.Format);
            BackUpAtlas.Init(ref desc);
            base.OnAssetLoaded();
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
        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            bool b = base.OnMouseDown(location, button);
            if (button == MouseButton.Left && _preview.IsMouseOver)
            {

                if (_preview.IsMouseOverSelection)
                {
                    //[Nori_SC] this will crash some day :D but not now
                    var panel = (DropPanel)(((GroupElement)_propertiesEditor.Children[0]).Children[_preview.SpriteIDUnderTheMouse].Control);
                    //let it be a auto scroll
                    var p = _propertiesEditor.Panel.Parent as Panel;
                    var parentScrollV = p?.VScrollBar;
                    if (_preview.MarkSelected())
                    {
                        LMBDown = true;
                        if (spriteUnderTheMouseDropPanel != panel)
                        {
                            if (spriteUnderTheMouseDropPanel != null)
                                spriteUnderTheMouseDropPanel.IsClosed = true;
                            panel.IsClosed = false;
                            spriteUnderTheMouseDropPanel = panel;
                        }
                    }
                    beginDragBounds.Location = _properties.Sprites[_preview.MarkedSpriteID].Location;
                    beginDragBounds.Size = _properties.Sprites[_preview.MarkedSpriteID].Size;
                    parentScrollV.Value = panel.LocalY - panel.Size.Y;
                    offset = _preview.GetMouseLocationOnTexture() - beginDragBounds.Location;
                    return true;
                }
                else
                {
                    _preview.HasSelection = false;
                    if (spriteUnderTheMouseDropPanel != null)
                        spriteUnderTheMouseDropPanel.IsClosed = true;
                    spriteUnderTheMouseDropPanel = null;
                    return true;
                }
            }
            return b;
        }
        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && LMBDown)
            {
                LMBDown = false;
                if (gotEdit)
                {
                    var sl = beginDragBounds;
                    var cl = sl.Location;
                    if (cl != _properties.Sprites[_preview.MarkedSpriteID].Location)
                    {
                        if (moveSprite)
                        {
                            _undo.AddAction(new UndoAction(_preview.MarkedSpriteID, cl, this, true));
                            _preview.MoveSpritePixels(cl, _properties.Sprites[_preview.MarkedSpriteID].Location, _properties.Sprites[_preview.MarkedSpriteID].Size);
                        }
                        else
                        {
                            _undo.AddAction(new UndoAction(_preview.MarkedSpriteID, cl, this, false));
                        }
                        IsEdited = true;
                    }
                    gotEdit = false;
                }
                return true;
            }
            
            lockduplicate = false;
            return base.OnMouseUp(location, button);
        }
        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            base.OnMouseMove(location);
            if (LMBDown && _preview.HasSelection)
            {
                location = _preview.GetMouseLocationOnTexture();
                var sl = _properties.Sprites[_preview.MarkedSpriteID];
                var cl = sl.Location;
                if (ctrlDown)
                {
                    sl.Location = Float2.Round(location + offset);
                }
                else
                {
                    if (sl.Size.X != 0 || sl.Size.Y != 0)// inf nan guard
                    {
                        sl.Location = Float2.SnapToGrid(location + offset, sl.Size);
                    }
                }
                if (cl != sl.Location)
                {
                    gotEdit = true;
                    if (shiftDown && lockduplicate == false)
                    {
                        lockduplicate = true;
                        _undo.Clear();
                        _properties.Sprites[_preview.MarkedSpriteID].Location = beginDragBounds.Location;
                        _properties.Sprites[_preview.MarkedSpriteID].Size = beginDragBounds.Size;
                        var rect = new Rectangle(sl.Location / Asset.Size, sl.Size / Asset.Size);
                        _preview.MarkedSpriteID = AddSprite(rect);
                        beginDragBounds = rect;
                    }
                    else
                    {
                        _properties.Sprites[_preview.MarkedSpriteID].Location = sl.Location;
                    }
                    offset = Vector2.Zero;
                }
            }
        }
        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            if(key == KeyboardKeys.Control)
            {
                ctrlDown = true;
            }
            if(key == KeyboardKeys.Shift)
            {
                shiftDown = true;
            }
            return base.OnKeyDown(key);
        }
        /// <inheritdoc />
        public override void OnKeyUp(KeyboardKeys key)
        {
            if (key == KeyboardKeys.Control)
            {
                ctrlDown = false;
            }
            if (key == KeyboardKeys.Shift)
            {
                shiftDown = false;
            }
            base.OnKeyUp(key);
        }
        /// <inheritdoc/>
        public override void OnLostFocus()
        {
            ctrlDown = false;
            shiftDown = false;
            LMBDown = false;
            base.OnLostFocus();
        }
    }
}
