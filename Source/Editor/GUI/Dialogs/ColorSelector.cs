// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Dialogs
{
    /// <summary>
    /// Color selecting control.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    public class ColorSelector : ContainerControl
    {
        /// <summary>
        /// The color.
        /// </summary>
        protected Color _color;

        /// <summary>
        /// The wheel rectangle.
        /// </summary>
        protected Rectangle _wheelRect;

        private readonly SpriteHandle _colorWheelSprite;
        private bool _isMouseDownWheel;

        /// <summary>
        /// Occurs when selected color gets changed.
        /// </summary>
        public event Action<Color> ColorChanged;

        /// <summary>
        /// Gets or sets the color.
        /// </summary>
        public Color Color
        {
            get => _color;
            set
            {
                if (_color != value)
                {
                    _color = value;
                    ColorChanged?.Invoke(_color);
                }
            }
        }

        /// <summary>
        /// Gets a value indicating whether user is using a wheel.
        /// </summary>
        public bool IsSliding => _isMouseDownWheel;

        /// <summary>
        /// Occurs when sliding starts.
        /// </summary>
        public event Action SlidingStart;

        /// <summary>
        /// Occurs when sliding ends.
        /// </summary>
        public event Action SlidingEnd;

        /// <summary>
        /// Initializes a new instance of the <see cref="ColorSelector"/> class.
        /// </summary>
        public ColorSelector()
        : this(64)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ColorSelector"/> class.
        /// </summary>
        /// <param name="wheelSize">Size of the wheel.</param>
        public ColorSelector(float wheelSize)
        : base(0, 0, wheelSize, wheelSize)
        {
            _colorWheelSprite = Editor.Instance.Icons.ColorWheel128;
            _wheelRect = new Rectangle(0, 0, wheelSize, wheelSize);
        }

        /// <summary>
        /// Updates the color selected by the mouse.
        /// </summary>
        /// <param name="location">The location.</param>
        protected virtual void UpdateMouse(ref Float2 location)
        {
            if (_isMouseDownWheel)
            {
                var delta = location - _wheelRect.Center;
                float distance = delta.Length;

                float degrees;
                if (Mathf.IsZero(delta.X))
                {
                    // The point is on the y-axis. Determine whether 
                    // it's above or below the x-axis, and return the 
                    // corresponding angle. Note that the orientation of the
                    // y-coordinate is backwards. That is, A positive Y value 
                    // indicates a point BELOW the x-axis.
                    if (delta.Y > 0)
                    {
                        degrees = 270;
                    }
                    else
                    {
                        degrees = 90;
                    }
                }
                else
                {
                    // This value needs to be multiplied
                    // by -1 because the y-coordinate
                    // is opposite from the normal direction here.
                    // That is, a y-coordinate that's "higher" on 
                    // the form has a lower y-value, in this coordinate
                    // system. So everything's off by a factor of -1 when
                    // performing the ratio calculations.
                    degrees = -Mathf.Atan(delta.Y / delta.X) * Mathf.RadiansToDegrees;

                    // If the x-coordinate of the selected point
                    // is to the left of the center of the circle, you 
                    // need to add 180 degrees to the angle. ArcTan only
                    // gives you a value on the right-hand side of the circle.
                    if (delta.X < 0)
                    {
                        degrees += 180;
                    }

                    // Ensure that the return value is between 0 and 360
                    while (degrees > 360)
                        degrees -= 360;
                    while (degrees < 360)
                        degrees += 360;
                }

                var hsv = _color.ToHSV();
                hsv.X = degrees;
                hsv.Y = Mathf.Saturate(distance / (_wheelRect.Width * 0.5f));

                // Auto set Value to 1 when color is black. Makes editing easier.
                if ((_color == Color.Black || _color == Color.Transparent) && hsv.Z <= 0.001f)
                    hsv.Z = 1.0f;

                var color = Color.FromHSV(hsv);
                color.A = _color.A;
                Color = color;
            }
        }

        private void EndSliding()
        {
            if (_isMouseDownWheel)
            {
                _isMouseDownWheel = false;
                SlidingEnd?.Invoke();
            }
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            var hsv = _color.ToHSV();
            bool enabled = EnabledInHierarchy;

            // Wheel
            float boxExpand = (2.0f * 4.0f / 128.0f) * _wheelRect.Width;
            Render2D.DrawSprite(_colorWheelSprite, _wheelRect.MakeExpanded(boxExpand), enabled ? Color.White : Color.Gray);
            float hAngle = hsv.X * Mathf.DegreesToRadians;
            float hRadius = hsv.Y * _wheelRect.Width * 0.5f;
            var hsPos = new Float2(hRadius * Mathf.Cos(hAngle), -hRadius * Mathf.Sin(hAngle));
            const float wheelBoxSize = 4.0f;
            Render2D.DrawRectangle(new Rectangle(hsPos - (wheelBoxSize * 0.5f) + _wheelRect.Center, new Float2(wheelBoxSize)), _isMouseDownWheel ? Color.Gray : Color.Black);
        }

        /// <inheritdoc />
        public override void OnLostFocus()
        {
            EndSliding();

            base.OnLostFocus();
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            UpdateMouse(ref location);

            base.OnMouseMove(location);
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && _wheelRect.Contains(location))
            {
                if (!_isMouseDownWheel)
                {
                    _isMouseDownWheel = true;
                    StartMouseCapture();
                    SlidingStart?.Invoke();
                }
                UpdateMouse(ref location);
            }

            Focus();
            return true;
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && _isMouseDownWheel)
            {
                EndMouseCapture();
                EndSliding();
                return true;
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override void OnEndMouseCapture()
        {
            EndSliding();
        }

        /// <inheritdoc />
        protected override void OnSizeChanged()
        {
            base.OnSizeChanged();

            _wheelRect = new Rectangle(0, 0, Size.Y, Size.Y);
        }
    }

    /// <summary>
    /// Color selecting control with additional sliders.
    /// </summary>
    /// <seealso cref="ColorSelector" />
    public class ColorSelectorWithSliders : ColorSelector
    {
        private Rectangle _slider1Rect;
        private Rectangle _slider2Rect;
        private bool _isMouseDownSlider1;
        private bool _isMouseDownSlider2;

        /// <summary>
        /// Initializes a new instance of the <see cref="ColorSelectorWithSliders"/> class.
        /// </summary>
        /// <param name="wheelSize">Size of the wheel.</param>
        /// <param name="slidersThickness">The sliders thickness.</param>
        public ColorSelectorWithSliders(float wheelSize, float slidersThickness)
        : base(wheelSize)
        {
            // Setup dimensions
            const float slidersMargin = 8.0f;
            _slider1Rect = new Rectangle(wheelSize + slidersMargin, 0, slidersThickness, wheelSize);
            _slider2Rect = new Rectangle(_slider1Rect.Right + slidersMargin, _slider1Rect.Y, slidersThickness, _slider1Rect.Height);
            Size = new Float2(_slider2Rect.Right, wheelSize);
        }

        /// <inheritdoc />
        protected override void UpdateMouse(ref Float2 location)
        {
            if (_isMouseDownSlider1)
            {
                var hsv = _color.ToHSV();
                hsv.Z = 1.0f - Mathf.Saturate((location.Y - _slider1Rect.Y) / _slider1Rect.Height);

                Color = Color.FromHSV(hsv, _color.A);
            }
            else if (_isMouseDownSlider2)
            {
                var color = _color;
                color.A = 1.0f - Mathf.Saturate((location.Y - _slider2Rect.Y) / _slider2Rect.Height);

                Color = color;
            }
            else
            {
                base.UpdateMouse(ref location);
            }
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            // Cache data
            var style = Style.Current;
            var hsv = _color.ToHSV();
            var hs = hsv;
            hs.Z = 1.0f;
            Color hsC = Color.FromHSV(hs);
            const float slidersOffset = 3.0f;
            const float slidersThickness = 4.0f;

            // Value
            float valueY = _slider2Rect.Height * (1 - hsv.Z);
            var valueR = new Rectangle(_slider1Rect.X - slidersOffset, _slider1Rect.Y + valueY - slidersThickness / 2, _slider1Rect.Width + slidersOffset * 2, slidersThickness);
            Render2D.FillRectangle(_slider1Rect, hsC, hsC, Color.Black, Color.Black);
            Render2D.DrawRectangle(_slider1Rect, _isMouseDownSlider1 ? style.BackgroundSelected : Color.Black);
            Render2D.DrawRectangle(valueR, _isMouseDownSlider1 ? Color.White : Color.Gray);

            // Alpha
            float alphaY = _slider2Rect.Height * (1 - _color.A);
            var alphaR = new Rectangle(_slider2Rect.X - slidersOffset, _slider2Rect.Y + alphaY - slidersThickness / 2, _slider2Rect.Width + slidersOffset * 2, slidersThickness);
            var color = _color;
            color.A = 1; // Keep slider 2 fill rect from changing color alpha while selecting.
            Render2D.FillRectangle(_slider2Rect, color, color, Color.Transparent, Color.Transparent);
            Render2D.DrawRectangle(_slider2Rect, _isMouseDownSlider2 ? style.BackgroundSelected : Color.Black);
            Render2D.DrawRectangle(alphaR, _isMouseDownSlider2 ? Color.White : Color.Gray);
        }

        /// <inheritdoc />
        public override void OnLostFocus()
        {
            // Clear flags
            _isMouseDownSlider1 = false;
            _isMouseDownSlider2 = false;

            base.OnLostFocus();
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && _slider1Rect.Contains(location))
            {
                _isMouseDownSlider1 = true;
                StartMouseCapture();
                UpdateMouse(ref location);
            }
            if (button == MouseButton.Left && _slider2Rect.Contains(location))
            {
                _isMouseDownSlider2 = true;
                StartMouseCapture();
                UpdateMouse(ref location);
            }

            return base.OnMouseDown(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && (_isMouseDownSlider1 || _isMouseDownSlider2))
            {
                _isMouseDownSlider1 = false;
                _isMouseDownSlider2 = false;
                EndMouseCapture();
                return true;
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override void OnEndMouseCapture()
        {
            // Clear flags
            _isMouseDownSlider1 = false;
            _isMouseDownSlider2 = false;

            base.OnEndMouseCapture();
        }
    }
}
