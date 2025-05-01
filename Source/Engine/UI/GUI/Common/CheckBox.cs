// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// The checkbox control states.
    /// </summary>
    public enum CheckBoxState
    {
        /// <summary>
        /// The default state.
        /// </summary>
        Default,

        /// <summary>
        /// The checked state.
        /// </summary>
        Checked,

        /// <summary>
        /// The intermediate state.
        /// </summary>
        Intermediate,
    }

    /// <summary>
    /// Check box control.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.Control" />
    [ActorToolbox("GUI")]
    public class CheckBox : Control
    {
        /// <summary>
        /// True if checked is being pressed (by mouse or touch).
        /// </summary>
        protected bool _isPressed;

        /// <summary>
        /// The current state.
        /// </summary>
        protected CheckBoxState _state;

        /// <summary>
        /// The mouse over box state.
        /// </summary>
        protected bool _mouseOverBox;

        /// <summary>
        /// The box size.
        /// </summary>
        protected float _boxSize;

        /// <summary>
        /// The box rectangle.
        /// </summary>
        protected Rectangle _box;

        /// <summary>
        /// Gets or sets the state of the checkbox.
        /// </summary>
        [EditorOrder(10)]
        public CheckBoxState State
        {
            get => _state;
            set
            {
                if (_state != value)
                {
                    _state = value;

                    StateChanged?.Invoke(this);
                }
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether this <see cref="CheckBox"/> is checked.
        /// </summary>
        [NoSerialize, HideInEditor]
        public bool Checked
        {
            get => _state == CheckBoxState.Checked;
            set => State = value ? CheckBoxState.Checked : CheckBoxState.Default;
        }

        /// <summary>
        /// Gets or sets a value indicating whether this <see cref="CheckBox"/> is in the intermediate state.
        /// </summary>
        [NoSerialize, HideInEditor]
        public bool Intermediate
        {
            get => _state == CheckBoxState.Intermediate;
            set => State = value ? CheckBoxState.Intermediate : CheckBoxState.Default;
        }

        /// <summary>
        /// Gets or sets the size of the box.
        /// </summary>
        [EditorOrder(20)]
        public float BoxSize
        {
            get => _boxSize;
            set
            {
                _boxSize = value;
                CacheBox();
            }
        }
        
        /// <summary>
        /// Gets or sets whether to have a border.
        /// </summary>
        [EditorDisplay("Border Style"), EditorOrder(2010), Tooltip("Whether to have a border."), ExpandGroups]
        public bool HasBorder { get; set; } = true;
        
        /// <summary>
        /// Gets or sets the border thickness.
        /// </summary>
        [EditorDisplay("Border Style"), EditorOrder(2011), Tooltip("The thickness of the border."), Limit(0)]
        public float BorderThickness { get; set; } = 1.0f;

        /// <summary>
        /// Gets or sets the color of the border.
        /// </summary>
        [EditorDisplay("Border Style"), EditorOrder(2012)]
        public Color BorderColor { get; set; }

        /// <summary>
        /// Gets or sets the border color when checkbox is hovered.
        /// </summary>
        [EditorDisplay("Border Style"), EditorOrder(2013)]
        public Color BorderColorHighlighted { get; set; }

        /// <summary>
        /// Gets or sets the color of the checkbox icon.
        /// </summary>
        [EditorDisplay("Image Style"), EditorOrder(2020), ExpandGroups]
        public Color ImageColor { get; set; }

        /// <summary>
        /// Gets or sets the image used to render checkbox checked state.
        /// </summary>
        [EditorDisplay("Image Style"), EditorOrder(2021), Tooltip("The image used to render checkbox checked state.")]
        public IBrush CheckedImage { get; set; }

        /// <summary>
        /// Gets or sets the image used to render checkbox intermediate state.
        /// </summary>
        [EditorDisplay("Image Style"), EditorOrder(2022), Tooltip("The image used to render checkbox intermediate state.")]
        public IBrush IntermediateImage { get; set; }

        /// <summary>
        /// Event fired when 'checked' state gets changed.
        /// </summary>
        public event Action<CheckBox> StateChanged;

        /// <summary>
        /// Initializes a new instance of the <see cref="CheckBox"/> class.
        /// </summary>
        public CheckBox()
        : this(0, 0)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="CheckBox"/> class.
        /// </summary>
        /// <param name="x">The x.</param>
        /// <param name="y">The y.</param>
        /// <param name="isChecked">if set to <c>true</c> set checked on start.</param>
        /// <param name="size">The checkbox size.</param>
        public CheckBox(float x, float y, bool isChecked = false, float size = 18)
        : base(x, y, size, size)
        {
            _state = isChecked ? CheckBoxState.Checked : CheckBoxState.Default;
            _boxSize = Mathf.Min(16.0f, size);

            var style = Style.Current;
            ImageColor = style.BorderSelected * 1.2f;
            BorderColor = style.BorderNormal;
            BorderColorHighlighted = style.BorderSelected;
            CheckedImage = new SpriteBrush(style.CheckBoxTick);
            IntermediateImage = new SpriteBrush(style.CheckBoxIntermediate);

            CacheBox();
        }

        /// <summary>
        /// Toggles the checked state.
        /// </summary>
        public void Toggle()
        {
            Checked = !Checked;
        }

        private void CacheBox()
        {
            _box = new Rectangle(0, (Height - _boxSize) * 0.5f, _boxSize, _boxSize);
        }

        /// <summary>
        /// Called when mouse or touch clicks the checkbox.
        /// </summary>
        protected virtual void OnClick()
        {
            Toggle();
        }

        /// <summary>
        /// Called when checkbox starts to be pressed by the used (via mouse or touch).
        /// </summary>
        protected virtual void OnPressBegin()
        {
            _isPressed = true;
            if (AutoFocus)
                Focus();
        }

        /// <summary>
        /// Called when checkbox ends to be pressed by the used (via mouse or touch).
        /// </summary>
        protected virtual void OnPressEnd()
        {
            _isPressed = false;
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            bool enabled = EnabledInHierarchy;

            // Border
            if (HasBorder)
            {
                Color borderColor = BorderColor;
                if (!enabled)
                    borderColor *= 0.5f;
                else if (_isPressed || _mouseOverBox || IsNavFocused)
                    borderColor = BorderColorHighlighted;
                Render2D.DrawRectangle(_box.MakeExpanded(-2.0f), borderColor, BorderThickness);
            }

            // Icon
            if (_state != CheckBoxState.Default)
            {
                var color = ImageColor;
                if (!enabled)
                    color *= 0.6f;

                if (_state == CheckBoxState.Checked)
                    CheckedImage?.Draw(_box, color);
                else
                    IntermediateImage?.Draw(_box, color);
            }
        }

        /// <inheritdoc />
        public override bool ContainsPoint(ref Float2 location, bool precise = false)
        {
            if (precise) // Precise check for checkbox element
                return _box.Contains(ref location);
            return base.ContainsPoint(ref location, precise);
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            base.OnMouseMove(location);

            _mouseOverBox = _box.Contains(ref location);
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && !_isPressed)
            {
                OnPressBegin();
                return true;
            }

            return base.OnMouseDown(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && !_isPressed)
            {
                OnPressBegin();
                return true;
            }
            
            if (button == MouseButton.Left && _isPressed)
            {
                OnPressEnd();
                if (_box.Contains(ref location))
                {
                    OnClick();
                    return true;
                }
            }
            return base.OnMouseDoubleClick(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && _isPressed)
            {
                OnPressEnd();
                if (_box.Contains(ref location))
                {
                    OnClick();
                    return true;
                }
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            if (_isPressed)
                OnPressEnd();
            _mouseOverBox = false;

            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override bool OnTouchDown(Float2 location, int pointerId)
        {
            if (!_isPressed)
            {
                OnPressBegin();
                return true;
            }

            return base.OnTouchDown(location, pointerId);
        }

        /// <inheritdoc />
        public override bool OnTouchUp(Float2 location, int pointerId)
        {
            if (_isPressed)
            {
                OnPressEnd();
                if (_box.Contains(ref location))
                {
                    OnClick();
                    return true;
                }
            }

            return base.OnTouchUp(location, pointerId);
        }

        /// <inheritdoc />
        public override void OnTouchLeave()
        {
            if (_isPressed)
                OnPressEnd();

            base.OnTouchLeave();
        }

        /// <inheritdoc />
        public override void OnLostFocus()
        {
            if (_isPressed)
                OnPressEnd();

            base.OnLostFocus();
        }

        /// <inheritdoc />
        protected override void OnSizeChanged()
        {
            base.OnSizeChanged();

            CacheBox();
        }

        /// <inheritdoc />
        public override void OnSubmit()
        {
            OnClick();

            base.OnSubmit();
        }
    }
}
