// Copyright (c) Wojciech Figat. All rights reserved.

// Toggles sprite resizing widgets to be created for each sprite instead for all of them
//#define WIDGET_PER_SPRITE

using FlaxEditor.Content;
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Scripting;
using FlaxEditor.Viewport.Previews;
using FlaxEngine;
using FlaxEngine.GUI;
using System.Collections.Generic;
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
        private sealed class EditSpritesAction : IUndoAction
        {
            private SpriteAtlasWindow _window;
            private SpriteAtlas _atlas;
            private Sprite[] _before, _after;

            public string ActionString => "Edit sprite(s)";

            private EditSpritesAction(SpriteAtlasWindow window, SpriteAtlas atlas, Sprite[] before)
            {
                _window = window;
                _atlas = atlas;
                _before = before;
                _after = atlas.Sprites;
            }

            public static void Add(SpriteAtlasWindow window, SpriteAtlas atlas, Sprite[] before)
            {
                window._undo.AddAction(new EditSpritesAction(window, atlas, before));
            }

            private void Set(Sprite[] sprites)
            {
                _atlas.Sprites = sprites;
                _window.OnSpritesEdited();
            }

            public void Do()
            {
                Set(_after);
            }

            public void Undo()
            {
                Set(_before);
            }

            public void Dispose()
            {
                _window = null;
                _before = _after = null;
                _atlas = null;
            }
        }

        /// <summary>
        /// Atlas view control. Shows sprites.
        /// </summary>
        /// <seealso cref="FlaxEditor.Viewport.Previews.SpriteAtlasPreview" />
        private sealed class AtlasView : SpriteAtlasPreview
        {
            internal SpriteAtlasWindow _window;
            private Rectangle _rubberBandRectangle;
            private bool _isRubberBandSelecting, _isMovingSelection, _isMovingWidget, _isMouseOverWidget;
            private Float2 _mouseStartLocation, _moveDelta;
            private Rectangle _spritesBeforeSelectionBounds;
            private Sprite[] _spritesBefore;
            private List<UIEditorRoot.Widget> _widgets;
            private UIEditorRoot.Widget _movingWidget;

            public AtlasView()
            : base(true)
            {
            }

            private Color ProxymityHighlight(Color color, Float2 point)
            {
                var alpha = Mathf.Saturate(Float2.DistanceSquared(point, MouseLocation) / 7000.0f);
                return Color.Lerp(color, Color.Transparent, alpha);
            }

            protected override void DrawTexture(ref Rectangle rect)
            {
                base.DrawTexture(ref rect);

                if (!Asset || !Asset.IsLoaded)
                    return;
                var count = Asset.SpritesCount;
                var style = Style.Current;
                var mouse = MouseLocation;
                var options = Editor.Instance.Options.Options.Visual;

                // Draw all sprites
                for (int i = 0; i < count; i++)
                {
                    GetSpriteRect(i, ref rect, out var area);
                    var selected = _window._selection.Contains(i);
                    var highlighted = !_isMovingSelection && !_isMovingWidget && !_isMouseOverWidget && area.Contains(ref mouse);

                    if (_isMovingSelection || _isMovingWidget)
                    {
                        // Highlight nearby sprite bounds
                        var highlightColor = style.LightBackground.AlphaMultiplied(0.7f);
                        var color1 = ProxymityHighlight(highlightColor, area.UpperLeft);
                        var color2 = ProxymityHighlight(highlightColor, area.UpperRight);
                        var color3 = ProxymityHighlight(highlightColor, area.BottomLeft);
                        var color4 = ProxymityHighlight(highlightColor, area.BottomRight);
                        Render2D.DrawRectangle(area, color1, color2, color3, color4);
                    }

                    if (selected || highlighted)
                    {
                        // Selected or highlighted
                        var color = selected ? options.SelectionOutlineColor0 : style.SelectionBorder;
                        if (selected && highlighted)
                            color = color.RGBMultiplied(1.1f);
                        Render2D.DrawRectangle(area, color, options.UISelectionOutlineSize);
                    }
                }

                // Draw sizing widgets
                if (!_isMovingWidget && _widgets != null)
                {
                    // Clear widgets to collect them during drawing
                    _widgets.Clear();
                }
                if (_window._selection.Count > 0)
                {
                    if (_widgets == null)
                        _widgets = new List<UIEditorRoot.Widget>();
                    var viewScale = ViewScale;
#if WIDGET_PER_SPRITE
                    for (int i = 0; i < _window._selection.Count; i++)
                    {
                        var spriteIndex = _window._selection[i];
                        GetSpriteRect(spriteIndex, ref rect, out var area);
                        UIEditorRoot.DrawResizeWidgets(spriteIndex, 0.0f, viewScale, ref mouse, _widgets, _isMovingWidget, ref area);
                    }
#else
                    GetSpriteRect(_window._selection[0], ref rect, out var area);
                    for (int i = 1; i < _window._selection.Count; i++)
                    {
                        var spriteIndex = _window._selection[i];
                        GetSpriteRect(spriteIndex, ref rect, out var tmp);
                        Rectangle.Union(ref area, ref tmp, out area);
                    }
                    if (_window._selection.Count > 1)
                    {
                        var borderColor = style.LightBackground.AlphaMultiplied(Mathf.Saturate(Mathf.Sin((float)Platform.TimeSeconds * 5.0f) * 0.6f + 1.0f));
                        Render2D.DrawRectangle(area, borderColor, 0.7f);
                    }
                    UIEditorRoot.DrawResizeWidgets(null, 0.0f, viewScale, ref mouse, _widgets, _isMovingWidget, ref area);
#endif
                }

                // Hints
                if ((_isMovingSelection || _isMovingWidget) && _moveDelta.LengthSquared > 0.5f)
                {
                    var hintText = $"{(_isMovingSelection ? "Move" : "Resize")}: {(int)_moveDelta.X} x {(int)_moveDelta.Y}";
                    var hintRect = new Rectangle(mouse + new Float2(5, 25), style.FontMedium.MeasureText(hintText));
                    var backgroundRect = hintRect.MakeExpanded(6.0f);
                    Render2D.FillRectangle(backgroundRect, style.Background);
                    Render2D.DrawRectangle(backgroundRect, style.BackgroundSelected);
                    Render2D.DrawText(style.FontMedium, hintText, style.Foreground, hintRect.Location);
                }

                // Selection
                if (_isRubberBandSelecting)
                {
                    Render2D.FillRectangle(_rubberBandRectangle, style.Selection);
                    Render2D.DrawRectangle(_rubberBandRectangle, style.SelectionBorder);
                }
            }

            public override bool OnMouseDown(Float2 location, MouseButton button)
            {
                if (button == MouseButton.Left && GetChildAt(location) == null)
                {
                    Focus();
                    StartMouseCapture();
                    if (_widgets != null && _widgets.Count != 0)
                    {
                        foreach (var widget in _widgets)
                        {
                            if (widget.Bounds.Contains(ref location))
                            {
                                // Start moving widget
                                _isMovingWidget = true;
                                _movingWidget = widget;
                                _moveDelta = Float2.Zero;
                                _mouseStartLocation = GetAtlasPixelsLocation(location);
                                Cursor = widget.Cursor;
                                _spritesBefore = Asset.Sprites;
                                Asset.GetSpriteArea(_window._selection[0], out _spritesBeforeSelectionBounds);
                                for (int i = 1; i < _window._selection.Count; i++)
                                {
                                    Asset.GetSpriteArea(_window._selection[i], out var tmp);
                                    Rectangle.Union(ref _spritesBeforeSelectionBounds, ref tmp, out _spritesBeforeSelectionBounds);
                                }
                                return true;
                            }
                        }
                    }
                    if (HitTestSprite(ref location, out var spriteIndex))
                    {
                        // Start moving selection
                        _mouseStartLocation = GetAtlasPixelsLocation(location);
                        _isMovingSelection = true;
                        _moveDelta = Float2.Zero;
                        _spritesBefore = Asset.Sprites;
                        bool selected = _window._selection.Contains(spriteIndex);
                        if (selected)
                        {
                            if (Input.GetKey(KeyboardKeys.Control))
                                _window.Select(spriteIndex, false, true); // Toggle
                        }
                        else if (!selected || _window._selection.Count != 1)
                        {
                            // Select this sprite only
                            _window.Select(spriteIndex, Input.GetKey(KeyboardKeys.Shift), Input.GetKey(KeyboardKeys.Control));
                        }
                    }
                    else
                    {
                        // Start selecting
                        _mouseStartLocation = location;
                        _isRubberBandSelecting = true;
                        _rubberBandRectangle = new Rectangle(_mouseStartLocation, 0, 0);
                    }
                    return true;
                }

                return base.OnMouseDown(location, button);
            }

            public override void OnMouseMove(Float2 location)
            {
                // Change cursor if mouse is over active control widget
                var wasMosueOver = _isMouseOverWidget;
                _isMouseOverWidget = false;
                if (_widgets != null && _widgets.Count != 0 && !_isMovingSelection && !_isMovingWidget && !_isRubberBandSelecting)
                {
                    UIEditorRoot.MouseMoveOverWidgets(this, _widgets, ref location, out _isMouseOverWidget);
                    if (wasMosueOver && !_isMouseOverWidget)
                        Cursor = CursorType.Default;
                }

                if (_isMovingWidget || _isMovingSelection)
                {
                    // Calculate movement delta in pixels of atlas
                    _moveDelta = GetAtlasPixelsLocation(location) - _mouseStartLocation;
                    if (Input.GetKey(KeyboardKeys.Control) && _window._selection.Count != 0)
                    {
                        // Snap to the grid
                        Rectangle area;
                        if (_spritesBefore != null && _spritesBefore.Length > _window._selection[0])
                            area = _spritesBefore[_window._selection[0]].Area;
                        else
                            Asset.GetSpriteArea(_window._selection[0], out area);
                        var spriteSize = area.Size * Asset.Size;
                        var snapSize = spriteSize * 0.1f; // 10% of sprite size
                        var spriteSizeInt = new Int2(spriteSize);
                        if (Mathf.IsPowerOfTwo((int)spriteSize.X) && Mathf.IsPowerOfTwo((int)spriteSize.Y))
                            snapSize = new Float2(spriteSizeInt.X >> 3, spriteSizeInt.Y >> 3); // Power-of-two grid
                        _moveDelta = Float2.SnapToGrid(_moveDelta, snapSize);
                    }
                    _moveDelta = Float2.Floor(_moveDelta); // Snap to whole pixels
                }

                if (_isMovingWidget)
                {
                    _moveDelta *= _movingWidget.ResizeAxis.Absolute;
                    if (_movingWidget.ResizeAxis.Absolute.ValuesSum == 2 && Input.GetKey(KeyboardKeys.Shift))
                    {
                        // Resize with aspect-ratio preservation
                        _moveDelta.X = Mathf.Abs(_moveDelta.Y) * Mathf.Sign(_moveDelta.X);
                    }
                    UIEditorRoot.MouseMoveWidget(_movingWidget, ref _moveDelta, 0.0f, Float2.One, sizeDelta =>
                    {
                        var atlasSize = Asset.Size;
                        sizeDelta.Location /= atlasSize;
                        sizeDelta.Size /= atlasSize;
#if WIDGET_PER_SPRITE
                        var spriteIndex = (int)_movingWidget.Target;
                        var before = _spritesBefore[spriteIndex].Area;
                        Sprite sprite = Asset.GetSprite(spriteIndex);
                        sprite.Area.Location = before.Location + sizeDelta.Location;
                        sprite.Area.Size = before.Size + sizeDelta.Size;
                        Asset.SetSprite(spriteIndex, ref sprite);
#else
                        var selectionCount = _window._selection.Count;
                        var selectionBounds = _spritesBeforeSelectionBounds;
                        for (int i = 0; i < selectionCount; i++)
                        {
                            var spriteIndex = _window._selection[i];
                            var before = _spritesBefore[spriteIndex].Area;
                            Sprite sprite = Asset.GetSprite(spriteIndex);
                            if (selectionCount == 1)
                            {
                                // Single-sprite editing (direct)
                                sprite.Area.Location = before.Location + sizeDelta.Location;
                                sprite.Area.Size = before.Size + sizeDelta.Size;
                            }
                            else
                            {
                                // Multii-sprite editing (bounds-relative)
                                var beforePixels = before.Size * atlasSize;
                                var beforeTotalPixel = selectionBounds.Size * atlasSize;
                                var locationBias = (before.Location - selectionBounds.Location) / selectionBounds.Size;
                                var sizePart = before.Size / selectionBounds.Size;
                                sprite.Area.Location = before.Location + sizeDelta.Location + locationBias * sizeDelta.Size;
                                sprite.Area.Size = before.Size + sizeDelta.Size * sizePart;
                            }
                            Asset.SetSprite(spriteIndex, ref sprite);
                        }
#endif
                    });
                }
                else if (_isMovingSelection)
                {
                    UpdateMove();
                    Cursor = CursorType.SizeAll;
                }
                else if (_isRubberBandSelecting)
                {
                    _rubberBandRectangle.Size = location - _mouseStartLocation;
                    UpdateSelection();
                }

                base.OnMouseMove(location);
            }

            public override bool OnMouseUp(Float2 location, MouseButton button)
            {
                if (EndMovingWidget())
                    return true;
                if (EndMovingSprites())
                    return true;
                if (EndSelecting())
                    return true;

                return base.OnMouseUp(location, button);
            }

            public override void OnMouseLeave()
            {
                EndMovingSprites();
                EndMovingWidget();
                EndSelecting();
                _isMouseOverWidget = false;

                base.OnMouseLeave();
            }

            public override void OnLostFocus()
            {
                EndMovingSprites();
                EndMovingWidget();
                EndSelecting();

                base.OnLostFocus();
            }

            public override void OnDestroy()
            {
                if (IsDisposing)
                    return;
                EndMovingSprites();
                EndMovingWidget();
                EndSelecting();

                base.OnDestroy();
            }

            private void UpdateSelection()
            {
                var rect = _rubberBandRectangle;
                FlaxEngine.Gizmo.ViewportRubberBandSelector.FixRect(ref rect);
                var selected = new List<int>();
                var count = Asset.SpritesCount;
                var view = TextureViewRect;
                for (int i = 0; i < count; i++)
                {
                    GetSpriteRect(i, ref view, out var area);
                    if (rect.Intersects(area))
                        selected.Add(i);
                }
                _window.Select(selected, Input.GetKey(KeyboardKeys.Shift), Input.GetKey(KeyboardKeys.Control));
            }

            private void UpdateMove()
            {
                var count = Asset.SpritesCount;
                var moveDelta = _moveDelta / Asset.Size;
                foreach (var i in _window._selection)
                {
                    Sprite sprite = Asset.GetSprite(i);
                    sprite.Area.Location = _spritesBefore[i].Area.Location + moveDelta;
                    Asset.SetSprite(i, ref sprite);
                }
            }

            private void GetSpriteRect(int i, ref Rectangle view, out Rectangle area)
            {
                Asset.GetSpriteArea(i, out area);
                area.Location = area.Location * view.Size + view.Location;
                area.Size = area.Size * view.Size;
            }

            private Float2 GetAtlasPixelsLocation(Float2 location)
            {
                var view = TextureViewRect;
                return (location - view.Location) / view.Size * Asset.Size;
            }

            private bool HitTestSprite(ref Float2 location, out int index)
            {
                var count = Asset.SpritesCount;
                var view = TextureViewRect;
                for (int i = 0; i < count; i++)
                {
                    GetSpriteRect(i, ref view, out var area);
                    if (area.Contains(ref location))
                    {
                        index = i;
                        return true;
                    }
                }
                index = -1;
                return false;
            }

            private bool EndMovingSprites()
            {
                if (_isMovingSelection)
                {
                    _isMovingSelection = false;
                    EndMouseCapture();
                    if (_moveDelta.Length >= 1)
                        EditSpritesAction.Add(_window, Asset, _spritesBefore);
                    _spritesBefore = null;
                    return true;
                }
                return false;
            }

            private bool EndMovingWidget()
            {
                if (_isMovingWidget)
                {
                    _isMovingWidget = false;
                    _movingWidget = new UIEditorRoot.Widget();
                    EndMouseCapture();
                    Cursor = CursorType.Default;
                    if (_moveDelta.Length >= 1)
                        EditSpritesAction.Add(_window, Asset, _spritesBefore);
                    _spritesBefore = null;
                    return true;
                }
                return false;
            }

            private bool EndSelecting()
            {
                if (_isRubberBandSelecting)
                {
                    _isRubberBandSelecting = false;
                    EndMouseCapture();
                    UpdateSelection();
                    return true;
                }
                return false;
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
                    var atlas = (SpriteAtlas)proxy._window?.Asset;
                    if (atlas  == null || !atlas.IsLoaded)
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
                    if (sprites == null)
                        return;
                    var window = (SpriteAtlasWindow)((SpritePropertiesProxy)ParentEditor.Values[0])._window;
                    var atlas = (SpriteAtlas)window.Asset;
                    window._spritesListConatiner = layout;
                    var elementType = new ScriptType(typeof(SpriteEntry));
                    for (int i = 0; i < sprites.Length; i++)
                    {
                        var group = layout.Group(sprites[i].Name);
                        group.Panel.Tag = i;
                        group.Panel.Icon = new SpriteBrush(new SpriteHandle(atlas, i));
                        group.Panel.MouseButtonRightClicked += OnGroupPanelMouseButtonRightClicked;
                        group.Panel.IsClosedChanged += OnGroupPanelExpandChanged;
                        group.Object(new ListValueContainer(elementType, i, Values));

                        var stringNameElement = group.Editors[0].ChildrenEditors.Find(x => x is StringEditor).Layout.Children.Find(x => x is TextBoxElement) as TextBoxElement;
                        if (stringNameElement != null)
                        {
                            stringNameElement.TextBox.TextBoxEditEnd += (textbox) => OnNameChanged(group.Panel, (TextBox)textbox);
                        }
                    }
                    window.UpdateSpritesList();
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

                private void OnGroupPanelExpandChanged(DropPanel groupPanel)
                {
                    if (!groupPanel.IsClosed)
                    {
                        // Auto-select sprite when user expands it
                        var window = (SpriteAtlasWindow)((SpritePropertiesProxy)ParentEditor.Values[0])._window;
                        var index = (int)groupPanel.Tag;
                        window.Select(index);
                    }
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
                    var window = (SpriteAtlasWindow)((SpritePropertiesProxy)ParentEditor.Values[0])._window;
                    var atlas = (SpriteAtlas)window.Asset;
                    var index = (int)button.Tag;
                    var sprite = atlas.GetSprite(index);
                    var pasted = FlaxEngine.Json.JsonSerializer.Deserialize<Sprite>(Clipboard.Text);
                    sprite.Area = pasted.Area;
                    var spritesBefore = atlas.Sprites;
                    atlas.SetSprite(index, ref sprite);
                    EditSpritesAction.Add(window, atlas, spritesBefore);
                }

                private void OnDeleteSpriteClicked(ContextMenuButton button)
                {
                    var window = (SpriteAtlasWindow)((SpritePropertiesProxy)ParentEditor.Values[0])._window;
                    var atlas = (SpriteAtlas)window.Asset;
                    var index = (int)button.Tag;
                    var spritesBefore = atlas.Sprites;
                    atlas.RemoveSprite(index);
                    EditSpritesAction.Add(window, atlas, spritesBefore);
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
            : base("Sprites", window, true, window._undo)
            {
                Proxy = new SpritePropertiesProxy();
                Presenter.Select(Proxy);
            }
        }

        private Undo _undo;
        private ToolStripButton _undoButton, _redoButton;
        private SpritesTab _spritesTab;
        private Sprite[] _savedSprites;
        private List<int> _selection = new();
        private LayoutElementsContainer _spritesListConatiner;

        /// <inheritdoc />
        public SpriteAtlasWindow(Editor editor, AssetItem item)
        : base(editor, item, new AtlasView())
        {
            // Undo
            _undo = new Undo();
            _undo.UndoDone += OnUndoRedo;
            _undo.RedoDone += OnUndoRedo;
            _undo.ActionDone += OnUndoRedo;

            // Sprotes editing
            ((AtlasView)_preview)._window = this;
            _spritesTab = _tabs.AddTab(new SpritesTab(this));
            _spritesTab.IndexInParent -= 2;
            _tabs.SelectedTab = _spritesTab;
            _tabs.OnChildrenChanged();

            // Toolstrip
            var inputOptions = editor.Options.Options.Input;
            _undoButton = _toolstrip.AddButton(editor.Icons.Undo64, _undo.PerformUndo).LinkTooltip("Undo.", ref inputOptions.Undo);
            _redoButton = _toolstrip.AddButton(editor.Icons.Redo64, _undo.PerformRedo).LinkTooltip("Redo.", ref inputOptions.Redo);
            _toolstrip.AddSeparator();
            _toolstrip.AddButton(editor.Icons.AddFile64, AddSprite).LinkTooltip("Add a new sprite");

            // Inputs
            InputActions.Add(options => options.Undo, _undo.PerformUndo);
            InputActions.Add(options => options.Redo, _undo.PerformRedo);
            InputActions.Add(options => options.Delete, DeleteSelection);
            InputActions.Add(options => options.SelectAll, SelectAll);
            InputActions.Add(options => options.DeselectAll, Deselect);
        }

        private void OnUndoRedo(IUndoAction action)
        {
            MarkAsEdited();
            UpdateToolstrip();
        }

        private void AddSprite()
        {
            var atlas = (SpriteAtlas)Asset;
            var sprite = new Sprite
            {
                Name = Utilities.Utils.IncrementNameNumber("Sprite", name => atlas.Sprites.All(s => s.Name != name)),
                Area = new Rectangle(Float2.Zero, Float2.One),
            };
            var spritesBefore = atlas.Sprites;
            var handle = atlas.AddSprite(sprite);
            OnSpritesEdited();
            Select(handle.Index);
            EditSpritesAction.Add(this, atlas, spritesBefore);
        }

        private void DeleteSelection()
        {
            if (_selection.Count == 0)
                return;
            var atlas = (SpriteAtlas)Asset;
            var spritesBefore = atlas.Sprites;
            _selection.Sort();
            for (int i = _selection.Count - 1; i >= 0; i--)
                atlas.RemoveSprite(_selection[i]);
            Deselect();
            OnSpritesEdited();
            EditSpritesAction.Add(this, atlas, spritesBefore);
        }

        private void SelectAll()
        {
            var atlas = (SpriteAtlas)Asset;
            var sprites = new int[atlas.SpritesCount];
            for (int i = 0; i < sprites.Length; i++)
                sprites[i] = i;
            Select(sprites, false, false, false);
        }

        private void Deselect()
        {
            if (_selection.Count == 0)
                return;
            _selection.Clear();
            UpdateSpritesList();
        }

        private void Select(int spriteIndex, bool additive = false, bool toggle = false, bool autoScroll = true)
        {
            if (spriteIndex == -1)
            {
                if (!additive)
                    Deselect();
                return;
            }
            if (additive)
            {
                if (!_selection.Contains(spriteIndex))
                    _selection.Add(spriteIndex);
            }
            else if (toggle)
            {
                if (_selection.Contains(spriteIndex))
                    _selection.Remove(spriteIndex);
                else
                    _selection.Add(spriteIndex);
            }
            else
            {
                _selection.Clear();
                _selection.Add(spriteIndex);
            }
            UpdateSpritesList();
            if (autoScroll && _spritesListConatiner != null)
            {
                foreach (var child in _spritesListConatiner.Children)
                {
                    if (child is GroupElement group && group.Control.Tag is int index && index == spriteIndex)
                    {
                        // Scroll proeprties panel vertical scroll bar to show this group
                        ((Panel)group.Panel.Parent.Parent.Parent).ScrollViewTo(group.Panel);
                        break;
                    }
                }
            }
        }

        private void Select(IEnumerable<int> spriteIndices, bool additive = false, bool toggle = false, bool autoScroll = true)
        {
            // Update selection
            if (spriteIndices.Count() == 0)
            {
                if (!additive)
                    Deselect();
                return;
            }
            if (additive)
            {
                foreach (var spriteIndex in spriteIndices)
                {
                    if (!_selection.Contains(spriteIndex))
                        _selection.Add(spriteIndex);
                }
            }
            else if (toggle)
            {
                foreach (var spriteIndex in spriteIndices)
                {
                    if (_selection.Contains(spriteIndex))
                        _selection.Remove(spriteIndex);
                    else
                        _selection.Add(spriteIndex);
                }
            }
            else
            {
                    _selection.Clear();
                    _selection.AddRange(spriteIndices);
            }

            // Update UI
            UpdateSpritesList();
            if (autoScroll && _spritesListConatiner != null)
            {
                foreach (var child in _spritesListConatiner.Children)
                {
                    if (child is GroupElement group && group.Control.Tag is int index && spriteIndices.Contains(index))
                    {
                        // Scroll proeprties panel vertical scroll bar to show this group
                        ((Panel)group.Panel.Parent.Parent.Parent).ScrollViewTo(group.Panel);
                        break;
                    }
                }
            }
        }

        private void UpdateSpritesList()
        {
            if (_spritesListConatiner == null)
                return;
            var style = Style.Current;
            foreach (var child in _spritesListConatiner.Children)
            {
                if (child is GroupElement group && group.Control.Tag is int index)
                {
                    // Highlight selected sprite properties group
                    var selected = _selection.Contains(index);
                    group.Panel.HeaderColor = selected ? style.BackgroundSelected : style.BackgroundNormal;
                    group.Panel.HeaderColorMouseOver = selected ? style.BackgroundSelected.RGBMultiplied(1.1f) : style.BackgroundHighlighted;
                }
            }
        }

        private void OnSpritesEdited()
        {
            ((SpritePropertiesProxy)_spritesTab.Proxy).UpdateSprites();
            _spritesTab.Presenter.BuildLayout();
        }

        /// <inheritdoc />
        protected override void UpdateToolstrip()
        {
            _undoButton.Enabled = _undo.CanUndo;
            _redoButton.Enabled = _undo.CanRedo;

            base.UpdateToolstrip();
        }

        /// <inheritdoc />
        protected override void UnlinkItem()
        {
            _selection.Clear();
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
            _spritesListConatiner = null;
            if (Asset)
            {
                // Discard unsaved changes
                ((SpriteAtlas)Asset).Sprites = _savedSprites;
            }

            base.OnClose();
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _savedSprites = null;
            _undo.Enabled = false;
            _undo.Clear();

            base.OnDestroy();
        }
    }
}
