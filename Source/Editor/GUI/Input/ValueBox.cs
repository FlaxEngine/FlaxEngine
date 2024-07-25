// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Input
{
    /// <summary>
    /// Base class for text boxes for float/int value editing. Supports slider and range clamping.
    /// </summary>
    /// <typeparam name="T">The value type.</typeparam>
    /// <seealso cref="FlaxEngine.GUI.TextBox" />
    [HideInEditor]
    public abstract class ValueBox<T> : TextBox where T : struct, IComparable<T>
    {
        /// <summary>
        /// The sliding box size.
        /// </summary>
        protected const float SlidingBoxSize = 12.0f;

        /// <summary>
        /// The current value.
        /// </summary>
        protected T _value;

        /// <summary>
        /// The minimum value.
        /// </summary>
        protected T _min;

        /// <summary>
        /// The maximum value.
        /// </summary>
        protected T _max;

        /// <summary>
        /// The slider speed.
        /// </summary>
        protected float _slideSpeed;

        /// <summary>
        /// True if slider is in use.
        /// </summary>
        protected bool _isSliding;

        /// <summary>
        /// The value cached on sliding start.
        /// </summary>
        protected T _startSlideValue;

        /// <summary>
        /// The text cached on editing start. Used to compare with the end result to detect changes.
        /// </summary>
        protected string _startEditText;

        private Float2 _startSlideLocation;
        private double _clickStartTime = -1;
        private bool _cursorChanged;
        private Float2 _mouseClickedPosition;

        /// <summary>
        /// Occurs when value gets changed.
        /// </summary>
        public event Action ValueChanged;

        /// <summary>
        /// Occurs when value gets changed.
        /// </summary>
        public event Action<ValueBox<T>> BoxValueChanged;

        /// <summary>
        /// Gets or sets the value.
        /// </summary>
        public abstract T Value { get; set; }

        /// <summary>
        /// Gets or sets the minimum value.
        /// </summary>
        public abstract T MinValue { get; set; }

        /// <summary>
        /// Gets or sets the maximum value.
        /// </summary>
        public abstract T MaxValue { get; set; }

        /// <summary>
        /// Gets a value indicating whether user is using a slider.
        /// </summary>
        public bool IsSliding => _isSliding;

        /// <summary>
        /// Occurs when sliding starts.
        /// </summary>
        public event Action SlidingStart;

        /// <summary>
        /// Occurs when sliding ends.
        /// </summary>
        public event Action SlidingEnd;

        /// <summary>
        /// Gets or sets the slider speed. Use value 0 to disable and hide slider UI.
        /// </summary>
        public float SlideSpeed
        {
            get => _slideSpeed;
            set => _slideSpeed = value;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ValueBox{T}"/> class.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <param name="x">The x.</param>
        /// <param name="y">The y.</param>
        /// <param name="width">The width.</param>
        /// <param name="min">The minimum.</param>
        /// <param name="max">The maximum.</param>
        /// <param name="sliderSpeed">The slider speed.</param>
        protected ValueBox(T value, float x, float y, float width, T min, T max, float sliderSpeed)
        : base(false, x, y, width)
        {
            _value = value;
            _min = min;
            _max = max;
            _slideSpeed = sliderSpeed;
        }

        /// <summary>
        /// Updates the text of the textbox.
        /// </summary>
        protected abstract void UpdateText();

        /// <summary>
        /// Tries the get value from the textbox text.
        /// </summary>
        protected abstract void TryGetValue();

        /// <summary>
        /// Applies the sliding delta to the value.
        /// </summary>
        /// <param name="delta">The delta (scaled).</param>
        protected abstract void ApplySliding(float delta);

        /// <summary>
        /// Called when value gets changed.
        /// </summary>
        protected virtual void OnValueChanged()
        {
            ValueChanged?.Invoke();
            BoxValueChanged?.Invoke(this);
        }

        /// <summary>
        /// Gets a value indicating whether this value box can use sliding.
        /// </summary>
        protected virtual bool CanUseSliding => _slideSpeed > Mathf.Epsilon;

        /// <summary>
        /// Gets the slide rectangle.
        /// </summary>
        protected virtual Rectangle SlideRect
        {
            get
            {
                float x = Width - SlidingBoxSize - 1.0f;
                float y = (Height - SlidingBoxSize) * 0.5f;
                return new Rectangle(x, y, SlidingBoxSize, SlidingBoxSize);
            }
        }

        private void EndSliding()
        {
            _isSliding = false;
            EndEditOnClick = true;
            EndMouseCapture();
            if (_cursorChanged)
            {
                Cursor = CursorType.Default;
                _cursorChanged = false;
            }
            SlidingEnd?.Invoke();
            Defocus();
            Parent?.Focus();
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            if (CanUseSliding)
            {
                var style = Style.Current;

                // Draw sliding UI
                Render2D.DrawSprite(style.Scalar, SlideRect, EnabledInHierarchy ? style.Foreground : style.ForegroundDisabled);

                // Check if is sliding
                if (_isSliding)
                {
                    // Draw overlay
                    var bounds = new Rectangle(Float2.Zero, Size);
                    Render2D.FillRectangle(bounds, style.Selection);
                    Render2D.DrawRectangle(bounds, style.SelectionBorder);
                }
            }
        }

        /// <inheritdoc />
        public override void OnGotFocus()
        {
            base.OnGotFocus();

            SelectAll();
        }

        /// <inheritdoc />
        public override void OnLostFocus()
        {
            // Check if was sliding
            if (_isSliding)
            {
                EndSliding();

                base.OnLostFocus();
            }
            else
            {
                base.OnLostFocus();

                // Update
                UpdateText();
            }

            Cursor = CursorType.Default;

            ResetViewOffset();
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && CanUseSliding && SlideRect.Contains(location))
            {
                // Start sliding
                _isSliding = true;
                _startSlideLocation = location;
                _startSlideValue = _value;
                StartMouseCapture(true);
                EndEditOnClick = false;

                // Hide cursor and cache location
                Cursor = CursorType.Hidden;
                _mouseClickedPosition = PointToWindow(location);
                _cursorChanged = true;

                SlidingStart?.Invoke();
                return true;
            }

            if (button == MouseButton.Left && !IsFocused)
                _clickStartTime = Platform.TimeSeconds;

            return base.OnMouseDown(location, button);
        }

#if !PLATFORM_SDL
        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            if (_isSliding && !RootWindow.Window.IsMouseFlippingHorizontally)
            {
                // Update sliding
                var slideLocation = location + Root.TrackingMouseOffset;
                ApplySliding(Mathf.RoundToInt(slideLocation.X - _startSlideLocation.X) * _slideSpeed);
                return;
            }

            // Update cursor type so user knows they can slide value
            if (CanUseSliding && SlideRect.Contains(location) && !_isSliding)
            {
                Cursor = CursorType.SizeWE;
                _cursorChanged = true;
            }
            else if (_cursorChanged && !_isSliding)
            {
                Cursor = CursorType.Default;
                _cursorChanged = false;
            }

            base.OnMouseMove(location);
        }

#else

        /// <inheritdoc />
        public override void OnMouseMoveRelative(Float2 mouseMotion)
        {
            var location = Root.TrackingMouseOffset;
            if (_isSliding)
            {
                // Update sliding
                ApplySliding(Root.TrackingMouseOffset.X * _slideSpeed);
                return;
            }

            // Update cursor type so user knows they can slide value
            if (CanUseSliding && SlideRect.Contains(location) && !_isSliding)
            {
                Cursor = CursorType.SizeWE;
                _cursorChanged = true;
            }
            else if (_cursorChanged && !_isSliding)
            {
                Cursor = CursorType.Default;
                _cursorChanged = false;
            }

            base.OnMouseMoveRelative(mouseMotion);
        }

#endif

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && _isSliding)
            {
                // End sliding and return mouse to original location
                RootWindow.MousePosition = _mouseClickedPosition;
                EndSliding();
                return true;
            }

            if (button == MouseButton.Left && _clickStartTime > 0 && (Platform.TimeSeconds - _clickStartTime) < 0.2f)
            {
                _clickStartTime = -1;
                OnSelectingEnd();
                SelectAll();
                return true;
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            if (_cursorChanged)
            {
                Cursor = CursorType.Default;
                _cursorChanged = false;
            }

            base.OnMouseLeave();
        }

        /// <inheritdoc />
        protected override void OnEditBegin()
        {
            base.OnEditBegin();

            _startEditText = _text;
        }

        /// <inheritdoc />
        protected override void OnEditEnd()
        {
            if (_startEditText != _text)
            {
                // Update value
                TryGetValue();
            }
            _startEditText = null;

            base.OnEditEnd();
        }

        /// <inheritdoc />
        public override void OnEndMouseCapture()
        {
            // Check if was sliding
            if (_isSliding)
            {
                EndSliding();
            }
            else
            {
                base.OnEndMouseCapture();
            }
        }

        /// <inheritdoc />
        protected override Rectangle TextRectangle
        {
            get
            {
                var result = base.TextRectangle;
                if (CanUseSliding)
                {
                    result.Size.X -= SlidingBoxSize;
                }
                return result;
            }
        }

        /// <inheritdoc />
        protected override Rectangle TextClipRectangle
        {
            get
            {
                var result = base.TextRectangle;
                if (CanUseSliding)
                {
                    result.Size.X -= SlidingBoxSize;
                }
                return result;
            }
        }
    }
}
