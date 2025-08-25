// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.GUI.ContextMenu;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI
{
    /// <summary>
    /// Popup menu useful for renaming objects via UI. Displays text box for renaming.
    /// </summary>
    /// <seealso cref="ContextMenuBase" />
    public class RenamePopup : ContextMenuBase
    {
        private string _startValue;
        private TextBox _inputField;

        /// <summary>
        /// Occurs when renaming is done.
        /// </summary>
        public event Action<RenamePopup> Renamed;

        /// <summary>
        /// Occurs when popup is closing (after renaming done or not).
        /// </summary>
        public event Action<RenamePopup> Closed;

        /// <summary>
        /// Input value validation delegate.
        /// </summary>
        /// <param name="popup">The popup reference.</param>
        /// <param name="value">The input text value.</param>
        /// <returns>True if text is valid, otherwise false.</returns>
        public delegate bool ValidateDelegate(RenamePopup popup, string value);

        /// <summary>
        /// Occurs when input text validation should be performed.
        /// </summary>
        public ValidateDelegate Validate;

        /// <summary>
        /// Gets or sets the initial value.
        /// </summary>
        public string InitialValue
        {
            get => _startValue;
            set => _startValue = value;
        }

        /// <summary>
        /// Gets or sets the input field text.
        /// </summary>
        public string Text
        {
            get => _inputField.Text;
            set => _inputField.Text = value;
        }

        /// <summary>
        /// Gets the text input field control.
        /// </summary>
        public TextBox InputField => _inputField;

        /// <summary>
        /// Initializes a new instance of the <see cref="RenamePopup"/> class.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <param name="size">The size.</param>
        /// <param name="isMultiline">Enable/disable multiline text input support</param>
        public RenamePopup(string value, Float2 size, bool isMultiline)
        {
            if (!isMultiline)
                size.Y = TextBox.DefaultHeight;
            Size = size;

            _startValue = value;

            _inputField = new TextBox(isMultiline, 0, 0, size.Y);
            _inputField.TextChanged += OnTextChanged;
            _inputField.AnchorPreset = AnchorPresets.StretchAll;
            _inputField.Offsets = Margin.Zero;
            _inputField.Text = _startValue;
            _inputField.Parent = this;
        }

        private bool IsInputValid => !string.IsNullOrWhiteSpace(_inputField.Text) && (_inputField.Text == _startValue || Validate == null || Validate(this, _inputField.Text));

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            var mouseLocation = Root.MousePosition;
            if (!ContainsPoint(ref mouseLocation) && RootWindow.ContainsFocus && Text != _startValue)
            {
                // rename item before closing if left mouse button in clicked
                if (FlaxEngine.Input.GetMouseButtonDown(MouseButton.Left))
                    OnEnd();
            }

            base.Update(deltaTime);
        }

        private void OnTextChanged()
        {
            if (Validate == null)
                return;

            var valid = IsInputValid;
            var style = Style.Current;
            if (valid)
            {
                _inputField.BorderColor = Color.Transparent;
                _inputField.BorderSelectedColor = style.BackgroundSelected;
            }
            else
            {
                var color = new Color(1.0f, 0.0f, 0.02745f, 1.0f);
                _inputField.BorderColor = Color.Lerp(color, style.TextBoxBackground, 0.6f);
                _inputField.BorderSelectedColor = color;
            }
        }

        /// <summary>
        /// Shows the rename popup.
        /// </summary>
        /// <param name="control">The target control.</param>
        /// <param name="area">The target control area to cover.</param>
        /// <param name="value">The initial value.</param>
        /// <param name="isMultiline">Enable/disable multiline text input support</param>
        /// <returns>Created popup.</returns>
        public static RenamePopup Show(Control control, Rectangle area, string value, bool isMultiline)
        {
            // hardcoded flushing layout for tree controls
            if (control is Tree.TreeNode treeNode && treeNode.ParentTree != null)
                treeNode.ParentTree.FlushPendingPerformLayout();

            // Calculate the control size in the window space to handle scaled controls
            var upperLeft = control.PointToWindow(area.UpperLeft);
            var bottomRight = control.PointToWindow(area.BottomRight);
            var size = bottomRight - upperLeft;

            var rename = new RenamePopup(value, size, isMultiline);
            rename.Show(control, area.Location + new Float2(0, (size.Y - rename.Height) * 0.5f));
            return rename;
        }

        private void OnEnd()
        {
            var text = Text;
            if (text != _startValue && IsInputValid)
            {
                Renamed?.Invoke(this);
            }

            Hide();
        }

        /// <inheritdoc />
        protected override bool UseAutomaticDirectionFix => false;

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            // Enter
            if (key == KeyboardKeys.Return)
            {
                OnEnd();
                return true;
            }
            // Esc
            if (key == KeyboardKeys.Escape)
            {
                Hide();
                return true;
            }

            // Base
            return base.OnKeyDown(key);
        }

        /// <inheritdoc />
        protected override void OnShow()
        {
            _inputField.EndEditOnClick = false; // Ending edit is handled through popup
            _inputField.Focus();
            _inputField.SelectAll();

            base.OnShow();
        }

        /// <inheritdoc />
        protected override void OnHide()
        {
            Closed?.Invoke(this);
            Closed = null;

            base.OnHide();

            // Remove itself
            Dispose();
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            Renamed = null;
            Closed = null;
            Validate = null;
            _inputField = null;

            base.OnDestroy();
        }
    }
}
