// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
                        degrees = 270;
                    else
                        degrees = 90;
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
                        degrees += 180;

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

                // Clamp mouse position to circle
                const float distanceOffset = 15f;

                float centerMouseDistance = Float2.Distance(Root.MousePosition, _wheelRect.Center);
                float wheelRadius = _wheelRect.Height * 0.5f + distanceOffset;
                if (wheelRadius < centerMouseDistance)
                {
                    Float2 normalizedCenterMouseDirection = (Root.MousePosition - _wheelRect.Center).Normalized;
                    Root.MousePosition = _wheelRect.Center + normalizedCenterMouseDirection * wheelRadius;
                }
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

            // Wheel selection box
            const float wheelSelectionBoxEdgeLenght = 5f;

            Float2 selectionBoxSize = new Float2(_isMouseDownWheel ? wheelSelectionBoxEdgeLenght * 2 : wheelSelectionBoxEdgeLenght);
            Rectangle selectionBoxFill = new Rectangle(hsPos - (selectionBoxSize * 0.5f) + _wheelRect.Center, selectionBoxSize);
            Rectangle selectionSecondOutline = new Rectangle(hsPos - ((selectionBoxSize - Float2.One) * 0.5f) + _wheelRect.Center, selectionBoxSize - Float2.One);

            Color rawWheelColor = Color.FromHSV(new Float3(Color.ToHSV().X, Color.ToHSV().Y, 1));

            Render2D.FillRectangle(selectionBoxFill, rawWheelColor);
            Render2D.DrawRectangle(selectionBoxFill, Color.Black);
            Render2D.DrawRectangle(selectionSecondOutline, Color.White, 0.5f);
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

                    Cursor = CursorType.Hidden;
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
                Cursor = CursorType.Default;

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
        private Rectangle _valueSliderRect;
        private Rectangle _valueSliderHitbox;
        private Rectangle _alphaSliderRect;
        private Rectangle _alphaSliderHitbox;
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
            const float slidersMargin = 16.0f;
            const float SliderHitboxExpansion = 12.0f;
            
            _valueSliderRect = new Rectangle(wheelSize + slidersMargin, 0, slidersThickness, wheelSize);
            _valueSliderHitbox = new Rectangle(_valueSliderRect.Location - SliderHitboxExpansion * 0.5f, _valueSliderRect.Size + SliderHitboxExpansion);
            _alphaSliderRect = new Rectangle(_valueSliderRect.Right + slidersMargin, _valueSliderRect.Y, slidersThickness, _valueSliderRect.Height);
            _alphaSliderHitbox = new Rectangle(_alphaSliderRect.Location - SliderHitboxExpansion * 0.5f, _alphaSliderRect.Size + SliderHitboxExpansion);
            
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

                // Clamp mouse cursor to stay in bounds of the slider
                Root.MousePosition = ClampFloat2WithOffset(Root.MousePosition, _valueSliderRect.UpperLeft, _valueSliderRect.BottomRight);
            }
            else if (_isMouseDownAlphaSlider)
            {
                var color = _color;
                color.A = 1.0f - Mathf.Saturate((location.Y - _alphaSliderRect.Y) / _alphaSliderRect.Height);

                Color = color;

                // Clamp mouse cursor to stay in bounds of the slider
                Root.MousePosition = ClampFloat2WithOffset(Root.MousePosition, _alphaSliderRect.UpperLeft, _alphaSliderRect.BottomRight);
            }
            else
                base.UpdateMouse(ref location);
        }

        private static Float2 ClampFloat2WithOffset(Float2 value, Float2 min, Float2 max)
        {
            Float2 clampedValue;

            // Add a small offset to prevent "value flickering" from clamping
            Float2 clampMinMaxOffset = new Float2(3, 12);
            min -= clampMinMaxOffset;
            max += clampMinMaxOffset;

            Float2.Clamp(ref value, ref min, ref max, out clampedValue);

            return clampedValue;
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

            const float knobWidthNormalExpansion = 5;
            const float knobWidthDragExpansion = 8;

            // Generate value slider and knob
            float valueKnobWidth = _isMouseDownValueSlider ? _valueSliderRect.Size.X + knobWidthDragExpansion : _valueSliderRect.Size.X + knobWidthNormalExpansion;
            float valueKnobHeight = _isMouseDownValueSlider ? 7 : 4;
            float valueKnobX = _isMouseDownValueSlider ? _valueSliderRect.X - knobWidthDragExpansion * 0.5f : _valueSliderRect.X - knobWidthNormalExpansion * 0.5f;
            float valueKnobY = _valueSliderRect.Height * (1 - hsv.Z);
            valueKnobY = Mathf.Clamp(valueKnobY, 0.0f, _valueSliderRect.Height) - valueKnobHeight * 0.5f;

            // TODO: Make this lerp the outline color to white instead of just abruptly showing it
            Color valueSliderTopOutlineColor = hsv.X > 205 && hsv.Y > 0.65f ? Color.White : Color.Black;
            Color valueKnobColor = Color.FromHSV(new Float3(0, 0, Mathf.Clamp(1f - hsv.Z, 0.45f, 1)));
            
            Rectangle valueKnob = new Rectangle(valueKnobX, valueKnobY, valueKnobWidth, valueKnobHeight);

            // Generate alpha slider and knob
            float alphaKnobWidth = _isMouseDownAlphaSlider ? _alphaSliderRect.Size.X + knobWidthDragExpansion : _alphaSliderRect.Size.X + knobWidthNormalExpansion;
            float alphaKnobHeight = _isMouseDownAlphaSlider ? 7 : 4;
            float alphaKnobX = _isMouseDownAlphaSlider ? _alphaSliderRect.X - knobWidthDragExpansion * 0.5f : _alphaSliderRect.X - knobWidthNormalExpansion * 0.5f;
            float alphaKnobY = _alphaSliderRect.Height * (1 - _color.A);
            alphaKnobY = Mathf.Clamp(alphaKnobY, 0.0f, _alphaSliderRect.Height) - alphaKnobHeight * 0.5f;

            // Prevent alpha slider fill from being affected by alpha
            var opaqueColor = _color;
            opaqueColor.A = 1; 
            
            Color alphaSliderTopOutlineColor = hsv.X > 205 && hsv.Y > 0.65f || hsv.Z < 0.6f ? Color.White : Color.Black;
            Color alphaKnobColor = Color.FromHSV(new Float3(0, 0, Mathf.Clamp(1f - hsv.Z, 0.35f, 1)));

            Rectangle alphaKnob = new Rectangle(alphaKnobX, alphaKnobY, alphaKnobWidth, alphaKnobHeight);

            // Draw value slider and knob
            Render2D.FillRectangle(_valueSliderRect, hsC, hsC, Color.Black, Color.Black);
            Render2D.DrawRectangle(_valueSliderRect, valueSliderTopOutlineColor, valueSliderTopOutlineColor, Color.White, Color.White);
            Render2D.DrawRectangle(valueKnob, valueKnobColor, _isMouseDownValueSlider ? 3 : 2);

            // Draw alpha slider, grid and knob
            DrawAlphaGrid(_alphaSliderRect.Width / 2, ref _alphaSliderRect.Location, _alphaSliderRect.Width, _alphaSliderRect.Height);
            Render2D.FillRectangle(_alphaSliderRect, opaqueColor, opaqueColor, Color.Transparent, Color.Transparent);
            Render2D.DrawRectangle(_alphaSliderRect, alphaSliderTopOutlineColor, alphaSliderTopOutlineColor, Color.Transparent, Color.Transparent);
            Render2D.DrawRectangle(alphaKnob, alphaKnobColor, _isMouseDownAlphaSlider ? 3 : 2);

            // Sliders hitbox debug
            //Render2D.DrawRectangle(_valueSliderHitbox, Color.Green);
            //Render2D.DrawRectangle(_alphaSliderHitbox, Color.Red);
        }

        private static void DrawAlphaGrid(float gridElementSize, ref Float2 startingPosition, float width, float height)
        {
            Rectangle backgroundRectangle = new Rectangle(startingPosition, new Float2(width, height));
            Render2D.FillRectangle(backgroundRectangle, Color.White);

            var numHor = Mathf.FloorToInt(width / gridElementSize);
            var numVer = Mathf.FloorToInt(height / gridElementSize);
            for (int i = 0; i < numHor; i++)
            {
                for (int j = 0; j < numVer; j++)
                {
                    if ((i + j) % 2 == 0)
                    {
                        var rect = new Rectangle(startingPosition.X + gridElementSize * i, startingPosition.Y + gridElementSize * j, new Float2(gridElementSize));
                        Render2D.FillRectangle(rect, Color.Gray);
                    }
                }
            }
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
            if (button == MouseButton.Left && _valueSliderHitbox.Contains(location))
            {
                _isMouseDownValueSlider = true;

                Cursor = CursorType.Hidden;

                StartMouseCapture();
                UpdateMouse(ref location);
            }
            if (button == MouseButton.Left && _alphaSliderHitbox.Contains(location))
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
                _isMouseDownValueSlider = false;
                _isMouseDownAlphaSlider = false;

                Cursor = CursorType.Default;

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
