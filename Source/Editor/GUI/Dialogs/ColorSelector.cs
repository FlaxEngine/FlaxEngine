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
        private const String GrayedOutParamName = "GrayedOut";

        /// <summary>
        /// Offset value applied to mouse cursor position when the user lets go of wheel or sliders.
        /// </summary>
        protected const float MouseCursorOffset = 6.0f;

        /// <summary>
        /// The color.
        /// </summary>
        protected Color _color;

        /// <summary>
        /// The wheel rectangle.
        /// </summary>
        protected Rectangle _wheelRect;

        private readonly MaterialBase _hsWheelMaterial;
        private bool _isMouseDownWheel;

        private Rectangle wheelDragRect
        {
            get
            {
                var hsv = _color.ToHSV();
                float hAngle = hsv.X * Mathf.DegreesToRadians;
                float hRadius = hsv.Y * _wheelRect.Width * 0.5f;
                var hsPos = new Float2(hRadius * Mathf.Cos(hAngle), -hRadius * Mathf.Sin(hAngle));
                float wheelBoxSize = IsSliding ? 9.0f : 5.0f;
                return new Rectangle(hsPos - (wheelBoxSize * 0.5f) + _wheelRect.Center, new Float2(wheelBoxSize));
            }
        }

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
            AutoFocus = true;

            _hsWheelMaterial = FlaxEngine.Content.LoadAsyncInternal<MaterialBase>(EditorAssets.HSWheelMaterial);
            _hsWheelMaterial = _hsWheelMaterial.CreateVirtualInstance();
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

            bool enabled = EnabledInHierarchy;

            _hsWheelMaterial.SetParameterValue(GrayedOutParamName, enabled ? 1.0f : 0.5f);
            Render2D.DrawMaterial(_hsWheelMaterial, _wheelRect, enabled ? Color.White : Color.Gray);

            // Wheel
            float boxExpand = (2.0f * 4.0f / 128.0f) * _wheelRect.Width;
            Render2D.DrawMaterial(_hsWheelMaterial, _wheelRect, enabled ? Color.White : Color.Gray);
            Float3 hsv = _color.ToHSV();
            Color hsColor = Color.FromHSV(new Float3(hsv.X, hsv.Y, 1));
            Rectangle wheelRect = wheelDragRect;
            Render2D.FillRectangle(wheelRect, hsColor);
            Render2D.DrawRectangle(wheelRect, Color.Black, _isMouseDownWheel ? 2.0f : 1.0f);
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
                    Cursor = CursorType.Hidden;
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
                // Make the cursor appear where the user expects it to be (position of selection rectangle)
                Rectangle dragRect = wheelDragRect;
                Root.MousePosition = dragRect.Center + MouseCursorOffset;
                Cursor = CursorType.Default;
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
        private Rectangle _valueSliderRect;
        private Rectangle _alphaSliderRect;
        private bool _isMouseDownValueSlider;
        private bool _isMouseDownAlphaSlider;

        /// <summary>
        /// Initializes a new instance of the <see cref="ColorSelectorWithSliders"/> class.
        /// </summary>
        /// <param name="wheelSize">Size of the wheel.</param>
        /// <param name="slidersThickness">The sliders thickness.</param>
        public ColorSelectorWithSliders(float wheelSize, float slidersThickness)
        : base(wheelSize)
        {
            // Setup dimensions
            const float slidersMargin = 10.0f;
            _valueSliderRect = new Rectangle(wheelSize + slidersMargin, 0, slidersThickness, wheelSize);
            _alphaSliderRect = new Rectangle(_valueSliderRect.Right + slidersMargin * 1.5f, _valueSliderRect.Y, slidersThickness, _valueSliderRect.Height);
            Size = new Float2(_alphaSliderRect.Right, wheelSize);
        }

        /// <inheritdoc />
        protected override void UpdateMouse(ref Float2 location)
        {
            if (_isMouseDownValueSlider)
            {
                var hsv = _color.ToHSV();
                hsv.Z = 1.0f - Mathf.Saturate((location.Y - _valueSliderRect.Y) / _valueSliderRect.Height);

                Color = Color.FromHSV(hsv, _color.A);
            }
            else if (_isMouseDownAlphaSlider)
            {
                var color = _color;
                color.A = 1.0f - Mathf.Saturate((location.Y - _alphaSliderRect.Y) / _alphaSliderRect.Height);

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

            // Value slider
            float valueKnobExpand = _isMouseDownValueSlider ? 10.0f : 4.0f;
            float valueY = _valueSliderRect.Height * (1 - hsv.Z);
            float valueKnobWidth = _valueSliderRect.Width + valueKnobExpand;
            float valueKnobHeight = _isMouseDownValueSlider ? 7.0f : 4.0f;
            float valueKnobX = _valueSliderRect.X - valueKnobExpand * 0.5f;
            float valueKnobY = _valueSliderRect.Y + valueY - valueKnobHeight * 0.5f;
            Rectangle valueKnobRect = new Rectangle(valueKnobX, valueKnobY, valueKnobWidth, valueKnobHeight);
            Render2D.FillRectangle(_valueSliderRect, hsC, hsC, Color.Black, Color.Black);
            // Draw one black and one white border to make the knob visible at any saturation level
            Render2D.DrawRectangle(valueKnobRect, Color.White, _isMouseDownValueSlider ? 3.0f : 2.0f);
            Render2D.DrawRectangle(valueKnobRect, Color.Black, _isMouseDownValueSlider ? 2.0f : 1.0f);

            // Draw checkerboard pattern as background of alpha slider
            Render2D.FillRectangle(_alphaSliderRect, Color.White);
            var smallRectSize = _alphaSliderRect.Width / 2.0f;
            var numHor = Mathf.CeilToInt(_alphaSliderRect.Width / smallRectSize);
            var numVer = Mathf.CeilToInt(_alphaSliderRect.Height / smallRectSize);
            Render2D.PushClip(_alphaSliderRect);
            for (int i = 0; i < numHor; i++)
            {
                for (int j = 0; j < numVer; j++)
                {
                    if ((i + j) % 2 == 0)
                    {
                        var rect = new Rectangle(_alphaSliderRect.X + smallRectSize * i, _alphaSliderRect.Y + smallRectSize * j, new Float2(smallRectSize));
                        Render2D.FillRectangle(rect, Color.Gray);
                    }
                }
            }
            Render2D.PopClip();

            // Alpha slider
            float alphaKnobExpand = _isMouseDownAlphaSlider ? 10.0f : 4.0f;
            float alphaY = _alphaSliderRect.Height * (1 - _color.A);
            float alphaKnobWidth = _alphaSliderRect.Width + alphaKnobExpand;
            float alphaKnobHeight = _isMouseDownAlphaSlider ? 7.0f : 4.0f;
            float alphaKnobX = _alphaSliderRect.X - alphaKnobExpand * 0.5f;
            float alphaKnobY = _alphaSliderRect.Y + alphaY - alphaKnobExpand * 0.5f;
            Rectangle alphaKnobRect = new Rectangle(alphaKnobX, alphaKnobY, alphaKnobWidth, alphaKnobHeight);
            var color = _color;
            color.A = 1; // Prevent alpha slider fill from becoming transparent
            Render2D.FillRectangle(_alphaSliderRect, color, color, Color.Transparent, Color.Transparent);
            // Draw one black and one white border to make the knob visible at any saturation level
            Render2D.DrawRectangle(alphaKnobRect, Color.White, _isMouseDownAlphaSlider ? 3.0f : 2.0f);
            Render2D.DrawRectangle(alphaKnobRect, Color.Black, _isMouseDownAlphaSlider ? 2.0f : 1.0f);
        }

        /// <inheritdoc />
        public override void OnLostFocus()
        {
            // Clear flags
            _isMouseDownValueSlider = false;
            _isMouseDownAlphaSlider = false;

            base.OnLostFocus();
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && _valueSliderRect.Contains(location))
            {
                _isMouseDownValueSlider = true;
                Cursor = CursorType.Hidden;
                StartMouseCapture();
                UpdateMouse(ref location);
            }
            if (button == MouseButton.Left && _alphaSliderRect.Contains(location))
            {
                _isMouseDownAlphaSlider = true;
                Cursor = CursorType.Hidden;
                StartMouseCapture();
                UpdateMouse(ref location);
            }

            return base.OnMouseDown(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && (_isMouseDownValueSlider || _isMouseDownAlphaSlider))
            {
                // Make the cursor appear where the user expects it to be (center of slider horizontally and slider knob position vertically)
                float sliderCenter = _isMouseDownValueSlider ? _valueSliderRect.Center.X : _alphaSliderRect.Center.X;
                // Calculate y position based on the slider knob to avoid incrementing value by a small amount when the user repeatedly clicks the slider (f.e. when moving in small steps)
                float mouseSliderPosition = _isMouseDownValueSlider ? _valueSliderRect.Y + _valueSliderRect.Height * (1 - _color.ToHSV().Z) + MouseCursorOffset : _alphaSliderRect.Y + _alphaSliderRect.Height * (1 - _color.A) + MouseCursorOffset;
                Root.MousePosition = new Float2(sliderCenter + MouseCursorOffset, Mathf.Clamp(mouseSliderPosition, _valueSliderRect.Top, _valueSliderRect.Bottom));
                Cursor = CursorType.Default;
                _isMouseDownValueSlider = false;
                _isMouseDownAlphaSlider = false;
                EndMouseCapture();
                return true;
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override void OnEndMouseCapture()
        {
            // Clear flags
            _isMouseDownValueSlider = false;
            _isMouseDownAlphaSlider = false;

            base.OnEndMouseCapture();
        }
    }
}
