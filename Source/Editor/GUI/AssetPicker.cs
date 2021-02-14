// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Content;
using FlaxEditor.GUI.Drag;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI
{
    /// <summary>
    /// Assets picking control.
    /// </summary>
    /// <seealso cref="Control" />
    /// <seealso cref="IContentItemOwner" />
    [HideInEditor]
    public class AssetPicker : Control, IContentItemOwner
    {
        private const float DefaultIconSize = 64;
        private const float ButtonsOffset = 2;
        private const float ButtonsSize = 12;

        private Asset _selected;
        private AssetItem _selectedItem;
        private ScriptType _type;

        private bool _isMouseDown;
        private Vector2 _mouseDownPos;
        private Vector2 _mousePos;
        private DragAssets _dragOverElement;

        /// <summary>
        /// Gets or sets the selected item.
        /// </summary>
        public AssetItem SelectedItem
        {
            get => _selectedItem;
            set
            {
                if (value == null)
                {
                    if (_selected == null && _selectedItem is SceneItem)
                    {
                        // Deselect scene reference
                        _selectedItem.RemoveReference(this);
                        _selectedItem = null;
                        _selected = null;
                        TooltipText = string.Empty;
                        OnSelectedItemChanged();
                        return;
                    }

                    // Deselect
                    SelectedAsset = null;
                }
                else if (value is SceneItem item)
                {
                    if (_selectedItem == item)
                        return;
                    if (!IsValid(item))
                        throw new ArgumentException("Invalid asset type.");

                    // Change value to scene reference (cannot load asset because scene can be already loaded - duplicated ID issue)
                    _selectedItem?.RemoveReference(this);
                    _selectedItem = item;
                    _selected = null;
                    _selectedItem?.AddReference(this);

                    // Update tooltip
                    TooltipText = _selectedItem?.NamePath;

                    OnSelectedItemChanged();
                }
                else
                {
                    SelectedAsset = FlaxEngine.Content.LoadAsync(value.ID);
                }
            }
        }

        /// <summary>
        /// Gets or sets the selected asset identifier.
        /// </summary>
        public Guid SelectedID
        {
            get
            {
                if (_selected != null)
                    return _selected.ID;
                if (_selectedItem != null)
                    return _selectedItem.ID;
                return Guid.Empty;
            }
            set => SelectedAsset = FlaxEngine.Content.LoadAsync(value);
        }

        /// <summary>
        /// Gets or sets the selected asset object.
        /// </summary>
        public Asset SelectedAsset
        {
            get => _selected;
            set
            {
                // Check if value won't change
                if (value == _selected)
                    return;

                // Find item from content database and check it
                var item = value ? Editor.Instance.ContentDatabase.FindAsset(value.ID) : null;
                if (item != null && !IsValid(item))
                    throw new ArgumentException("Invalid asset type.");

                // Change value
                _selectedItem?.RemoveReference(this);
                _selectedItem = item;
                _selected = value;
                _selectedItem?.AddReference(this);

                // Update tooltip
                TooltipText = _selectedItem?.NamePath;

                OnSelectedItemChanged();
            }
        }

        /// <summary>
        /// Gets or sets the assets types that this picker accepts (it supports types derived from the given type).
        /// </summary>
        public ScriptType AssetType
        {
            get => _type;
            set
            {
                if (_type != value)
                {
                    _type = value;

                    // Auto deselect if the current value is invalid
                    if (_selectedItem != null && !IsValid(_selectedItem))
                        SelectedItem = null;
                }
            }
        }

        /// <summary>
        /// Occurs when selected item gets changed.
        /// </summary>
        public event Action SelectedItemChanged;

        private bool IsValid(AssetItem item)
        {
            // Faster path for binary items (in-build)
            if (item is BinaryAssetItem binaryItem)
                return _type.IsAssignableFrom(new ScriptType(binaryItem.Type));

            // Type filter
            var type = TypeUtils.GetType(item.TypeName);
            if (_type.IsAssignableFrom(type))
                return true;

            // Json assets can contain any type of the object defined by the C# type (data oriented design)
            if (item is JsonAssetItem && (_type.Type == typeof(JsonAsset) || _type.Type == typeof(Asset)))
                return true;

            // Special case for scene asset references
            if (_type.Type == typeof(SceneReference) && item is SceneItem)
                return true;

            return false;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AssetPicker"/> class.
        /// </summary>
        public AssetPicker()
        : this(new ScriptType(typeof(Asset)), Vector2.Zero)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AssetPicker"/> class.
        /// </summary>
        /// <param name="assetType">The assets types that this picker accepts.</param>
        /// <param name="location">The control location.</param>
        public AssetPicker(ScriptType assetType, Vector2 location)
        : base(location, new Vector2(DefaultIconSize + ButtonsOffset + ButtonsSize, DefaultIconSize))
        {
            _type = assetType;
            _mousePos = Vector2.Minimum;
        }

        /// <summary>
        /// Called when selected item gets changed.
        /// </summary>
        protected virtual void OnSelectedItemChanged()
        {
            SelectedItemChanged?.Invoke();
        }

        private void DoDrag()
        {
            // Do the drag drop operation if has selected element
            if (_selected != null && new Rectangle(Vector2.Zero, Size).Contains(ref _mouseDownPos))
            {
                DoDragDrop(DragAssets.GetDragData(_selected));
            }
        }

        /// <inheritdoc />
        public void OnItemDeleted(ContentItem item)
        {
            // Deselect item
            SelectedItem = null;
        }

        /// <inheritdoc />
        public void OnItemRenamed(ContentItem item)
        {
        }

        /// <inheritdoc />
        public void OnItemReimported(ContentItem item)
        {
        }

        /// <inheritdoc />
        public void OnItemDispose(ContentItem item)
        {
            // Deselect item
            SelectedItem = null;
        }

        private Rectangle IconRect => new Rectangle(0, 0, Height, Height);

        private Rectangle Button1Rect => new Rectangle(Height + ButtonsOffset, 0, ButtonsSize, ButtonsSize);

        private Rectangle Button2Rect => new Rectangle(Height + ButtonsOffset, ButtonsSize, ButtonsSize, ButtonsSize);

        private Rectangle Button3Rect => new Rectangle(Height + ButtonsOffset, ButtonsSize * 2, ButtonsSize, ButtonsSize);

        /// <inheritdoc />
        public override void Draw()
        {
            // Cache data
            var style = Style.Current;
            var iconRect = IconRect;
            var button1Rect = Button1Rect;
            var button2Rect = Button2Rect;
            var button3Rect = Button3Rect;

            // Draw asset picker button
            Render2D.DrawSprite(style.ArrowDown, button1Rect, button1Rect.Contains(_mousePos) ? style.Foreground : style.ForegroundGrey);

            // Check if has item selected
            if (_selectedItem != null)
            {
                // Draw item preview
                _selectedItem.DrawThumbnail(ref iconRect);

                // Draw find button
                Render2D.DrawSprite(style.Search, button2Rect, button2Rect.Contains(_mousePos) ? style.Foreground : style.ForegroundGrey);

                // Draw remove button
                Render2D.DrawSprite(style.Cross, button3Rect, button3Rect.Contains(_mousePos) ? style.Foreground : style.ForegroundGrey);

                // Draw name
                float sizeForTextLeft = Width - button1Rect.Right;
                if (sizeForTextLeft > 30)
                {
                    Render2D.DrawText(
                                      style.FontSmall,
                                      _selectedItem.ShortName,
                                      new Rectangle(button1Rect.Right + 2, 0, sizeForTextLeft, ButtonsSize),
                                      style.Foreground,
                                      TextAlignment.Near,
                                      TextAlignment.Center);
                }
            }
            // Check if has no item but has an asset (eg. virtual asset)
            else if (_selected)
            {
                // Draw remove button
                Render2D.DrawSprite(style.Cross, button3Rect, button3Rect.Contains(_mousePos) ? style.Foreground : style.ForegroundGrey);

                // Draw name
                float sizeForTextLeft = Width - button1Rect.Right;
                if (sizeForTextLeft > 30)
                {
                    var name = _selected.GetType().Name;
                    if (_selected.IsVirtual)
                        name += " (virtual)";
                    Render2D.DrawText(
                                      style.FontSmall,
                                      name,
                                      new Rectangle(button1Rect.Right + 2, 0, sizeForTextLeft, ButtonsSize),
                                      style.Foreground,
                                      TextAlignment.Near,
                                      TextAlignment.Center);
                }
            }
            else
            {
                // No element selected
                Render2D.FillRectangle(iconRect, new Color(0.2f));
                Render2D.DrawText(style.FontMedium, "No asset\nselected", iconRect, Color.Wheat, TextAlignment.Center, TextAlignment.Center, TextWrapping.NoWrap, 1.0f, Height / DefaultIconSize);
            }

            // Check if drag is over
            if (IsDragOver && _dragOverElement != null && _dragOverElement.HasValidDrag)
                Render2D.FillRectangle(iconRect, style.BackgroundSelected * 0.4f);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _selectedItem?.RemoveReference(this);
            _selectedItem = null;
            _selected = null;

            base.OnDestroy();
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            _mousePos = Vector2.Minimum;

            // Check if start drag drop
            if (_isMouseDown)
            {
                // Clear flag
                _isMouseDown = false;

                // Do the drag
                DoDrag();
            }

            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override void OnMouseEnter(Vector2 location)
        {
            _mousePos = location;
            _mouseDownPos = Vector2.Minimum;

            base.OnMouseEnter(location);
        }

        /// <inheritdoc />
        public override void OnMouseMove(Vector2 location)
        {
            _mousePos = location;

            // Check if start drag drop
            if (_isMouseDown && Vector2.Distance(location, _mouseDownPos) > 10.0f && IconRect.Contains(_mouseDownPos))
            {
                // Do the drag
                _isMouseDown = false;
                DoDrag();
            }

            base.OnMouseMove(location);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Vector2 location, MouseButton button)
        {
            if (button == MouseButton.Left && _isMouseDown)
            {
                _isMouseDown = false;

                // Buttons logic
                if (Button1Rect.Contains(location))
                {
                    // Show asset picker popup
                    Focus();
                    AssetSearchPopup.Show(this, Button1Rect.BottomLeft, IsValid, assetItem =>
                    {
                        SelectedItem = assetItem;
                        RootWindow.Focus();
                        Focus();
                    });
                }
                else if (_selected != null || _selectedItem != null)
                {
                    if (Button2Rect.Contains(location) && _selectedItem != null)
                    {
                        // Select asset
                        Editor.Instance.Windows.ContentWin.Select(_selectedItem);
                    }
                    else if (Button3Rect.Contains(location))
                    {
                        // Deselect asset
                        Focus();
                        SelectedItem = null;
                    }
                }
            }

            // Handled
            return true;
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Vector2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                _isMouseDown = true;
                _mouseDownPos = location;
            }

            // Handled
            return true;
        }

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Vector2 location, MouseButton button)
        {
            // Focus
            Focus();

            // Check if has element selected
            if (_selectedItem != null && IconRect.Contains(location))
            {
                // Open it
                Editor.Instance.ContentEditing.Open(_selectedItem);
            }

            // Handled
            return true;
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragEnter(ref Vector2 location, DragData data)
        {
            base.OnDragEnter(ref location, data);

            // Check if drop asset
            if (_dragOverElement == null)
                _dragOverElement = new DragAssets(IsValid);
            if (_dragOverElement.OnDragEnter(data))
            {
            }

            return _dragOverElement.Effect;
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragMove(ref Vector2 location, DragData data)
        {
            base.OnDragMove(ref location, data);

            return _dragOverElement.Effect;
        }

        /// <inheritdoc />
        public override void OnDragLeave()
        {
            // Clear cache
            _dragOverElement.OnDragLeave();

            base.OnDragLeave();
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragDrop(ref Vector2 location, DragData data)
        {
            base.OnDragDrop(ref location, data);

            if (_dragOverElement.HasValidDrag)
            {
                // Select element
                SelectedItem = _dragOverElement.Objects[0];
            }

            // Clear cache
            _dragOverElement.OnDragDrop();

            return DragDropEffect.Move;
        }
    }
}
