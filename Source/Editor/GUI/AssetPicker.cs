// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.IO;
using FlaxEditor.Content;
using FlaxEditor.GUI.Drag;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;

namespace FlaxEditor.GUI
{
    /// <summary>
    /// Assets picking control.
    /// </summary>
    /// <seealso cref="Control" />
    /// <seealso cref="IContentItemOwner" />
    [HideInEditor]
    public class AssetPicker : Control
    {
        private const float DefaultIconSize = 64;
        private const float ButtonsOffset = 2;
        private const float ButtonsSize = 12;

        private bool _isMouseDown;
        private Float2 _mouseDownPos;
        private Float2 _mousePos;
        private DragItems _dragOverElement;

        /// <summary>
        /// The asset validator. Used to ensure only appropriate items can be picked.
        /// </summary>
        public AssetPickerValidator Validator { get; }

        /// <summary>
        /// Occurs when selected item gets changed.
        /// </summary>
        public event Action SelectedItemChanged;

        /// <summary>
        /// The custom callback for assets validation. Cane be used to implement a rule for assets to pick.
        /// </summary>
        public Func<ContentItem, bool> CheckValid;

        /// <summary>
        /// False if changing selected item is disabled.
        /// </summary>
        public bool CanEdit = true;

        /// <summary>
        /// Utility flag used to indicate that there are different values assigned to this reference editor and user should be informed about it.
        /// </summary>
        public bool DifferentValues;

        /// <summary>
        /// Initializes a new instance of the <see cref="AssetPicker"/> class.
        /// </summary>
        public AssetPicker()
        : this(new ScriptType(typeof(Asset)), Float2.Zero)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AssetPicker"/> class.
        /// </summary>
        /// <param name="assetType">The asset types that this picker accepts.</param>
        /// <param name="location">The control location.</param>
        public AssetPicker(ScriptType assetType, Float2 location)
        : base(location, new Float2(DefaultIconSize + ButtonsOffset + ButtonsSize, DefaultIconSize))
        {
            Validator = new AssetPickerValidator(assetType);
            Validator.SelectedItemChanged += OnSelectedItemChanged;
            _mousePos = Float2.Minimum;
        }

        /// <summary>
        /// Called when selected item gets changed.
        /// </summary>
        protected virtual void OnSelectedItemChanged()
        {
            if (IsDisposing)
                return;

            // Update tooltip
            string tooltip;
            if (Validator.SelectedItem is AssetItem assetItem)
                tooltip = assetItem.NamePath;
            else
                tooltip = Validator.SelectedPath;
            TooltipText = tooltip;

            SelectedItemChanged?.Invoke();
        }

        private void DoDrag()
        {
            // Do the drag drop operation if has selected element
            if (new Rectangle(Float2.Zero, Size).Contains(ref _mouseDownPos))
            {
                if (Validator.SelectedAsset != null)
                    DoDragDrop(DragAssets.GetDragData(Validator.SelectedAsset));
                else if (Validator.SelectedItem != null)
                    DoDragDrop(DragItems.GetDragData(Validator.SelectedItem));
            }
        }

        private Rectangle IconRect => new Rectangle(0, 0, Height, Height);

        private Rectangle Button1Rect => new Rectangle(Height + ButtonsOffset, 0, ButtonsSize, ButtonsSize);

        private Rectangle Button2Rect => new Rectangle(Height + ButtonsOffset, ButtonsSize + 2, ButtonsSize, ButtonsSize);

        private Rectangle Button3Rect => new Rectangle(Height + ButtonsOffset, (ButtonsSize + 2) * 2, ButtonsSize, ButtonsSize);

        /// <inheritdoc />
        public override void Draw()
        {
            var style = Style.Current;
            var iconRect = IconRect;
            var button1Rect = Button1Rect;
            var button2Rect = Button2Rect;
            var button3Rect = Button3Rect;

            // Draw asset picker button
            if (CanEdit)
                Render2D.DrawSprite(style.ArrowDown, button1Rect, button1Rect.Contains(_mousePos) ? style.Foreground : style.ForegroundGrey);

            if (DifferentValues)
            {
                // No element selected
                Render2D.FillRectangle(iconRect, style.BackgroundNormal);
                Render2D.DrawText(style.FontMedium, "Multiple\nValues", iconRect, style.Foreground, TextAlignment.Center, TextAlignment.Center, TextWrapping.NoWrap, 1.0f, Height / DefaultIconSize);
            }
            else if (Validator.SelectedItem != null)
            {
                // Draw item preview
                Validator.SelectedItem.DrawThumbnail(ref iconRect);

                // Draw buttons
                if (CanEdit)
                {
                    Render2D.DrawSprite(style.Search, button2Rect, button2Rect.Contains(_mousePos) ? style.Foreground : style.ForegroundGrey);
                    Render2D.DrawSprite(style.Cross, button3Rect, button3Rect.Contains(_mousePos) ? style.Foreground : style.ForegroundGrey);
                }
                else
                {
                    Render2D.DrawSprite(style.Search, button1Rect, button1Rect.Contains(_mousePos) ? style.Foreground : style.ForegroundGrey);
                }

                // Draw name
                float sizeForTextLeft = Width - button1Rect.Right;
                if (sizeForTextLeft > 30)
                {
                    Render2D.DrawText(
                                      style.FontSmall,
                                      Validator.SelectedItem.ShortName,
                                      new Rectangle(button1Rect.Right + 2, 0, sizeForTextLeft, ButtonsSize),
                                      style.Foreground,
                                      TextAlignment.Near,
                                      TextAlignment.Center);
                    Render2D.DrawText(
                                      style.FontSmall,
                                      $"{Validator.AssetType.Type.GetTypeDisplayName()}",
                                      new Rectangle(button1Rect.Right + 2, ButtonsSize + 2, sizeForTextLeft, ButtonsSize),
                                      style.ForegroundGrey,
                                      TextAlignment.Near,
                                      TextAlignment.Center);
                }
            }
            // Check if has no item but has an asset (eg. virtual asset)
            else if (Validator.SelectedAsset)
            {
                // Draw remove button
                Render2D.DrawSprite(style.Cross, button3Rect, button3Rect.Contains(_mousePos) ? style.Foreground : style.ForegroundGrey);

                // Draw name
                float sizeForTextLeft = Width - button1Rect.Right;
                if (sizeForTextLeft > 30)
                {
                    var name = Validator.SelectedAsset.GetType().Name;
                    if (Validator.SelectedAsset.IsVirtual)
                        name += " (virtual)";
                    Render2D.DrawText(
                                      style.FontSmall,
                                      name,
                                      new Rectangle(button1Rect.Right + 2, 0, sizeForTextLeft, ButtonsSize),
                                      style.Foreground,
                                      TextAlignment.Near,
                                      TextAlignment.Center);
                    Render2D.DrawText(
                                      style.FontSmall,
                                      $"{Validator.AssetType.Type.GetTypeDisplayName()}",
                                      new Rectangle(button1Rect.Right + 2, ButtonsSize + 2, sizeForTextLeft, ButtonsSize),
                                      style.ForegroundGrey,
                                      TextAlignment.Near,
                                      TextAlignment.Center);
                }
            }
            else
            {
                // No element selected
                Render2D.FillRectangle(iconRect, style.BackgroundNormal);
                Render2D.DrawText(style.FontMedium, "No asset\nselected", iconRect, Color.Orange, TextAlignment.Center, TextAlignment.Center, TextWrapping.NoWrap, 1.0f, Height / DefaultIconSize);
                float sizeForTextLeft = Width - button1Rect.Right;
                if (sizeForTextLeft > 30)
                {
                    Render2D.DrawText(
                                      style.FontSmall,
                                      $"None",
                                      new Rectangle(button1Rect.Right + 2, 0, sizeForTextLeft, ButtonsSize),
                                      style.Foreground,
                                      TextAlignment.Near,
                                      TextAlignment.Center);
                    Render2D.DrawText(
                                      style.FontSmall,
                                      $"{Validator.AssetType.Type.GetTypeDisplayName()}",
                                      new Rectangle(button1Rect.Right + 2, ButtonsSize + 2, sizeForTextLeft, ButtonsSize),
                                      style.ForegroundGrey,
                                      TextAlignment.Near,
                                      TextAlignment.Center);
                }
            }

            // Check if drag is over
            if (IsDragOver && _dragOverElement != null && _dragOverElement.HasValidDrag)
            {
                var bounds = new Rectangle(Float2.Zero, Size);
                Render2D.FillRectangle(bounds, style.Selection);
                Render2D.DrawRectangle(bounds, style.SelectionBorder);
            }

            // Navigation focus highlight
            if (IsNavFocused)
            {
                var bounds = new Rectangle(Float2.Zero, Size);
                Render2D.DrawRectangle(bounds, style.BackgroundSelected);
            }
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            Validator.OnDestroy();

            base.OnDestroy();
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            _mousePos = Float2.Minimum;

            // Check if start drag drop
            if (_isMouseDown)
            {
                _isMouseDown = false;
                DoDrag();
            }

            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override void OnMouseEnter(Float2 location)
        {
            _mousePos = location;
            _mouseDownPos = Float2.Minimum;

            base.OnMouseEnter(location);
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            _mousePos = location;

            // Check if start drag drop
            if (_isMouseDown && Float2.Distance(location, _mouseDownPos) > 10.0f && IconRect.Contains(_mouseDownPos))
            {
                _isMouseDown = false;
                DoDrag();
            }

            base.OnMouseMove(location);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && _isMouseDown)
            {
                _isMouseDown = false;

                // Buttons logic
                if (!CanEdit)
                {
                    if (Button1Rect.Contains(location) && Validator.SelectedItem != null)
                    {
                        // Select asset
                        Editor.Instance.Windows.ContentWin.Select(Validator.SelectedItem);
                    }
                }
                else if (Button1Rect.Contains(location))
                {
                    Focus();
                    OnSubmit();
                }
                else if (Validator.SelectedAsset != null || Validator.SelectedItem != null)
                {
                    if (Button2Rect.Contains(location) && Validator.SelectedItem != null)
                    {
                        // Select asset
                        Editor.Instance.Windows.ContentWin.Select(Validator.SelectedItem);
                    }
                    else if (Button3Rect.Contains(location))
                    {
                        // Deselect asset
                        Focus();
                        Validator.SelectedItem = null;
                    }
                }
            }

            // Handled
            return true;
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
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
        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            Focus();

            if (Validator.SelectedItem != null && IconRect.Contains(location))
            {
                // Open it
                Editor.Instance.ContentEditing.Open(Validator.SelectedItem);
            }

            // Handled
            return true;
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragEnter(ref Float2 location, DragData data)
        {
            base.OnDragEnter(ref location, data);

            // Check if drop asset
            if (_dragOverElement == null)
                _dragOverElement = new DragItems(Validator.IsValid);
            if (CanEdit && _dragOverElement.OnDragEnter(data))
            {
            }

            return _dragOverElement.Effect;
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragMove(ref Float2 location, DragData data)
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
        public override DragDropEffect OnDragDrop(ref Float2 location, DragData data)
        {
            base.OnDragDrop(ref location, data);

            if (CanEdit && _dragOverElement.HasValidDrag)
            {
                // Select element
                Validator.SelectedItem = _dragOverElement.Objects[0];
            }

            // Clear cache
            _dragOverElement.OnDragDrop();

            return DragDropEffect.Move;
        }

        /// <inheritdoc />
        public override void OnSubmit()
        {
            base.OnSubmit();

            if (Validator.AssetType != ScriptType.Null)
            {
                // Show asset picker popup
                var popup = AssetSearchPopup.Show(this, Button1Rect.BottomLeft, Validator.IsValid, item =>
                {
                    Validator.SelectedItem = item;
                    RootWindow.Focus();
                    Focus();
                });
                if (Validator.SelectedAsset != null)
                {
                    var selectedAssetName = Path.GetFileNameWithoutExtension(Validator.SelectedAsset.Path);
                    popup.ScrollToAndHighlightItemByName(selectedAssetName);
                }
            }
            else
            {
                // Show content item picker popup
                var popup = ContentSearchPopup.Show(this, Button1Rect.BottomLeft, Validator.IsValid, item =>
                {
                    Validator.SelectedItem = item;
                    RootWindow.Focus();
                    Focus();
                });
                if (Validator.SelectedItem != null)
                {
                    popup.ScrollToAndHighlightItemByName(Validator.SelectedItem.ShortName);
                }
            }
        }
    }
}
