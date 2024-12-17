// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Input;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;

namespace FlaxEditor.Surface
{
    /// <summary>
    /// Visject Surface comment control.
    /// </summary>
    /// <seealso cref="SurfaceNode" />
    [HideInEditor]
    public class SurfaceComment : SurfaceNode
    {
        private Rectangle _colorButtonRect;
        private Rectangle _resizeButtonRect;
        private Float2 _startResizingSize;
        private readonly TextBox _renameTextBox;

        /// <summary>
        /// True if sizing tool is in use.
        /// </summary>
        protected bool _isResizing;

        /// <summary>
        /// True if rename textbox is active in order to rename comment
        /// </summary>
        protected bool _isRenaming;

        /// <summary>
        /// Gets or sets the color of the comment.
        /// </summary>
        public Color Color
        {
            get => BackgroundColor;
            set => BackgroundColor = value;
        }

        private string TitleValue
        {
            get => (string)Values[0];
            set => SetValue(0, value, false);
        }

        private Color ColorValue
        {
            get => (Color)Values[1];
            set => SetValue(1, value, false);
        }

        private Float2 SizeValue
        {
            get => (Float2)Values[2];
            set => SetValue(2, value, false);
        }

        private int OrderValue
        {
            get => (int)Values[3];
            set => SetValue(3, value, false);
        }

        /// <inheritdoc />
        public SurfaceComment(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
        : base(id, context, nodeArch, groupArch)
        {
            _renameTextBox = new TextBox(false, 0, 0, Width)
            {
                Height = Constants.NodeHeaderSize,
                Visible = false,
                Parent = this,
                EndEditOnClick = false, // We have to handle this ourselves, otherwise the textbox instantly loses focus when double-clicking the header
                HorizontalAlignment = TextAlignment.Center,
            };
        }

        /// <inheritdoc />
        public override void OnSurfaceLoaded(SurfaceNodeActions action)
        {
            base.OnSurfaceLoaded(action);

            // Read node data
            Title = TitleValue;
            Color = ColorValue;
            var size = SizeValue;
            if (Surface.GridSnappingEnabled)
                size = Surface.SnapToGrid(size, true);
            Size = size;

            // Order
            // Backwards compatibility - When opening with an older version send the old comments to the back
            if (Values.Length < 4)
            {
                if (IndexInParent > 0)
                    IndexInParent = 0;
                OrderValue = IndexInParent;
            }
            else if (OrderValue != -1)
            {
                IndexInParent = OrderValue;
            }
        }

        /// <inheritdoc />
        public override void OnSpawned(SurfaceNodeActions action)
        {
            base.OnSpawned(action);

            // Randomize color
            Color = ColorValue = Color.FromHSV(new Random().NextFloat(0, 360), 0.7f, 0.25f, 0.8f);

            if (OrderValue == -1)
                OrderValue = Context.CommentCount - 1;
            IndexInParent = OrderValue;
        }

        /// <inheritdoc />
        public override void OnValuesChanged()
        {
            base.OnValuesChanged();

            // Read node data
            Title = TitleValue;
            Color = ColorValue;
            Size = SizeValue;
        }

        private void EndResizing()
        {
            // Clear state
            _isResizing = false;

            if (_startResizingSize != Size)
            {
                SizeValue = Size;
                Surface.MarkAsEdited(false);
            }

            EndMouseCapture();
        }

        /// <inheritdoc />
        public override bool CanSelect(ref Float2 location)
        {
            return _headerRect.MakeOffsetted(Location).Contains(ref location) && !_resizeButtonRect.MakeOffsetted(Location).Contains(ref location);
        }

        /// <inheritdoc />
        public override bool IsSelectionIntersecting(ref Rectangle selectionRect)
        {
            return _headerRect.MakeOffsetted(Location).Intersects(ref selectionRect);
        }

        /// <inheritdoc />
        protected override void UpdateRectangles()
        {
            const float headerSize = Constants.NodeHeaderSize;
            const float buttonMargin = Constants.NodeCloseButtonMargin;
            const float buttonSize = Constants.NodeCloseButtonSize;
            _headerRect = new Rectangle(0, 0, Width, headerSize);
            _closeButtonRect = new Rectangle(Width - buttonSize - buttonMargin, buttonMargin, buttonSize, buttonSize);
            _colorButtonRect = new Rectangle(_closeButtonRect.Left - buttonSize - buttonMargin, buttonMargin, buttonSize, buttonSize);
            _resizeButtonRect = new Rectangle(_closeButtonRect.Left, Height - buttonSize - buttonMargin, buttonSize, buttonSize);
            _renameTextBox.Width = Width;
            _renameTextBox.Height = headerSize;
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            if (_isRenaming)
            {
                // Stop renaming when clicking anywhere else
                if (!_renameTextBox.IsFocused || !RootWindow.IsFocused)
                {
                    Rename(_renameTextBox.Text);
                    StopRenaming();
                }
            }
            else
            {
                // Rename on F2
                if (IsSelected && Editor.Instance.Options.Options.Input.Rename.Process(this))
                {
                    StartRenaming();
                }
            }

            base.Update(deltaTime);
        }

        /// <inheritdoc />
        public override void Draw()
        {
            var style = Style.Current;
            var color = Color;
            var backgroundRect = new Rectangle(Float2.Zero, Size);
            var headerColor = new Color(Mathf.Clamp(color.R, 0.1f, 0.3f), Mathf.Clamp(color.G, 0.1f, 0.3f), Mathf.Clamp(color.B, 0.1f, 0.3f), 0.4f);
            if (IsSelected && !_isRenaming)
                headerColor *= 2.0f;

            // Paint background
            Render2D.FillRectangle(new Rectangle(Float2.Zero, Size), BackgroundColor);

            // Draw child controls
            DrawChildren();

            // Header
            Render2D.FillRectangle(_headerRect, headerColor);
            if (!_isRenaming)
                Render2D.DrawText(style.FontLarge, Title, _headerRect, style.Foreground, TextAlignment.Center, TextAlignment.Center);

            // Close button
            Render2D.DrawSprite(style.Cross, _closeButtonRect, _closeButtonRect.Contains(_mousePosition) && Surface.CanEdit ? style.Foreground : style.ForegroundGrey);

            // Color button
            Render2D.DrawSprite(style.Settings, _colorButtonRect, _colorButtonRect.Contains(_mousePosition) && Surface.CanEdit ? style.Foreground : style.ForegroundGrey);

            // Check if is resizing
            if (_isResizing)
            {
                // Draw overlay
                Render2D.FillRectangle(_resizeButtonRect, style.Selection);
                Render2D.DrawRectangle(_resizeButtonRect, style.SelectionBorder);
            }

            // Resize button
            Render2D.DrawSprite(style.Scale, _resizeButtonRect, _resizeButtonRect.Contains(_mousePosition) && Surface.CanEdit ? style.Foreground : style.ForegroundGrey);

            // Selection outline
            if (_isSelected)
            {
                backgroundRect.Expand(1.5f);
                var colorTop = Color.Orange;
                var colorBottom = Color.OrangeRed;
                Render2D.DrawRectangle(backgroundRect, colorTop, colorTop, colorBottom, colorBottom);
            }
        }

        /// <inheritdoc />
        protected override Float2 CalculateNodeSize(float width, float height)
        {
            return Size;
        }

        /// <inheritdoc />
        public override void OnLostFocus()
        {
            // Check if was resizing
            if (_isResizing)
            {
                EndResizing();
            }

            // Check if was renaming
            if (_isRenaming)
            {
                Rename(_renameTextBox.Text);
                StopRenaming();
            }

            // Base
            base.OnLostFocus();
        }

        /// <inheritdoc />
        public override void OnEndMouseCapture()
        {
            // Check if was resizing
            if (_isResizing)
            {
                EndResizing();
            }
            else
            {
                base.OnEndMouseCapture();
            }
        }

        /// <inheritdoc />
        public override bool ContainsPoint(ref Float2 location, bool precise)
        {
            return _headerRect.Contains(ref location) || _resizeButtonRect.Contains(ref location);
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (base.OnMouseDown(location, button))
                return true;

            // Check if can start resizing
            if (button == MouseButton.Left && _resizeButtonRect.Contains(ref location) && Surface.CanEdit)
            {
                // Start sliding
                _isResizing = true;
                _startResizingSize = Size;
                StartMouseCapture();

                return true;
            }

            return false;
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            // Check if is resizing
            if (_isResizing)
            {
                // Update size
                var size = Float2.Max(location, new Float2(140.0f, _headerRect.Bottom));
                if (Surface.GridSnappingEnabled)
                    size = Surface.SnapToGrid(size, true);
                Size = size;
            }
            else
            {
                // Base
                base.OnMouseMove(location);
            }
        }

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            if (base.OnMouseDoubleClick(location, button))
                return true;

            // Rename
            if (_headerRect.Contains(ref location) && Surface.CanEdit)
            {
                StartRenaming();
                return true;
            }

            return false;
        }

        /// <summary>
        /// Starts the renaming of the comment. Shows the UI for the user.
        /// </summary>
        public void StartRenaming()
        {
            _isRenaming = true;
            _renameTextBox.Visible = true;
            _renameTextBox.SetText(Title);
            _renameTextBox.Focus();
            _renameTextBox.SelectAll();
        }

        private void StopRenaming()
        {
            _isRenaming = false;
            _renameTextBox.Visible = false;
        }

        private void Rename(string newTitle)
        {
            if (string.Equals(Title, newTitle, StringComparison.Ordinal))
                return;

            Title = TitleValue = newTitle;
            Surface.MarkAsEdited(false);
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (key == KeyboardKeys.Return)
            {
                Rename(_renameTextBox.Text);
                StopRenaming();
                return true;
            }

            if (key == KeyboardKeys.Escape)
            {
                StopRenaming();
                return true;
            }

            return base.OnKeyDown(key);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && _isResizing)
            {
                EndResizing();
                return true;
            }

            if (base.OnMouseUp(location, button))
                return true;

            // Close
            if (_closeButtonRect.Contains(ref location) && Surface.CanEdit)
            {
                Surface.Delete(this);
                return true;
            }

            // Color
            if (_colorButtonRect.Contains(ref location) && Surface.CanEdit)
            {
                ColorValueBox.ShowPickColorDialog?.Invoke(this, Color, OnColorChanged);
                return true;
            }

            return false;
        }

        private void OnColorChanged(Color color, bool sliding)
        {
            Color = ColorValue = color;
            Surface.MarkAsEdited(false);
        }

        /// <inheritdoc />
        public override void OnShowSecondaryContextMenu(FlaxEditor.GUI.ContextMenu.ContextMenu menu, Float2 location)
        {
            base.OnShowSecondaryContextMenu(menu, location);

            menu.AddSeparator();
            menu.AddButton("Rename", StartRenaming);
            ContextMenuChildMenu cmOrder = menu.AddChildMenu("Order");
            {
                cmOrder.ContextMenu.AddButton("Bring Forward", () =>
                {
                    if (IndexInParent < Context.CommentCount - 1)
                        IndexInParent++;
                    OrderValue = IndexInParent;
                });
                cmOrder.ContextMenu.AddButton("Bring to Front", () =>
                {
                    IndexInParent = Context.CommentCount - 1;
                    OrderValue = IndexInParent;
                });
                cmOrder.ContextMenu.AddButton("Send Backward", () =>
                {
                    if (IndexInParent > 0)
                        IndexInParent--;
                    OrderValue = IndexInParent;
                });
                cmOrder.ContextMenu.AddButton("Send to Back", () =>
                {
                    IndexInParent = 0;
                    OrderValue = IndexInParent;
                });
            }
        }
    }
}
