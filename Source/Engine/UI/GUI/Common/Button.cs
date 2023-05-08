// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Button control
    /// </summary>
    public class Button : ContainerControl
    {
        /// <summary>
        /// The default height fro the buttons.
        /// </summary>
        public const float DefaultHeight = 24.0f;

        /// <summary>
        /// True if button is being pressed (by mouse or touch).
        /// </summary>
        protected bool _isPressed;

        /// <summary>
        /// The font.
        /// </summary>
        protected FontReference _font;

        /// <summary>
        /// The text.
        /// </summary>
        protected LocalizedString _text = new LocalizedString();

        /// <summary>
        /// Button text property.
        /// </summary>
        [EditorOrder(10), Tooltip("The button label text.")]
        public LocalizedString Text
        {
            get => _text;
            set => _text = value;
        }

        /// <summary>
        /// Gets or sets the font used to draw button text.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000)]
        public FontReference Font
        {
            get => _font;
            set => _font = value;
        }

        /// <summary>
        /// Gets or sets the custom material used to render the text. It must has domain set to GUI and have a public texture parameter named Font used to sample font atlas texture with font characters data.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000), Tooltip("Custom material used to render the text. It must has domain set to GUI and have a public texture parameter named Font used to sample font atlas texture with font characters data.")]
        public MaterialBase TextMaterial { get; set; }

        /// <summary>
        /// Gets or sets the color used to draw button text.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000)]
        public Color TextColor;

        /// <summary>
        /// Event fired when user clicks on the button.
        /// </summary>
        public event Action Clicked;

        /// <summary>
        /// Event fired when user clicks on the button.
        /// </summary>
        public event Action<Button> ButtonClicked;

        /// <summary>
        /// Event fired when users mouse enters the control.
        /// </summary>
        public event Action HoverBegin;

        /// <summary>
        /// Event fired when users mouse leaves the control.
        /// </summary>
        public event Action HoverEnd;

        /// <summary>
        /// Gets or sets the brush used for background drawing.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000), Tooltip("The brush used for background drawing.")]
        public IBrush BackgroundBrush { get; set; }

        /// <summary>
        /// Gets or sets the color of the border.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000)]
        public Color BorderColor { get; set; }

        /// <summary>
        /// Gets or sets the background color when button is selected.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2010)]
        public Color BackgroundColorSelected { get; set; }

        /// <summary>
        /// Gets or sets the border color when button is selected.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2020)]
        public Color BorderColorSelected { get; set; }

        /// <summary>
        /// Gets or sets the background color when button is highlighted.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000)]
        public Color BackgroundColorHighlighted { get; set; }

        /// <summary>
        /// Gets or sets the border color when button is highlighted.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000)]
        public Color BorderColorHighlighted { get; set; }

        /// <summary>
        /// Gets a value indicating whether this button is being pressed (by mouse or touch).
        /// </summary>
        public bool IsPressed => _isPressed;

        /// <summary>
        /// Initializes a new instance of the <see cref="Button"/> class.
        /// </summary>
        public Button()
        : this(0, 0)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Button"/> class.
        /// </summary>
        /// <param name="x">Position X coordinate</param>
        /// <param name="y">Position Y coordinate</param>
        /// <param name="width">Width</param>
        /// <param name="height">Height</param>
        public Button(float x, float y, float width = 120, float height = DefaultHeight)
        : base(x, y, width, height)
        {
            var style = Style.Current;
            if (style != null)
            {
                _font = new FontReference(style.FontMedium);
                TextColor = style.Foreground;
                BackgroundColor = style.BackgroundNormal;
                BorderColor = style.BorderNormal;
                BackgroundColorSelected = style.BackgroundSelected;
                BorderColorSelected = style.BorderSelected;
                BackgroundColorHighlighted = style.BackgroundHighlighted;
                BorderColorHighlighted = style.BorderHighlighted;
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Button"/> class.
        /// </summary>
        /// <param name="location">Position</param>
        /// <param name="size">Size</param>
        public Button(Float2 location, Float2 size)
        : this(location.X, location.Y, size.X, size.Y)
        {
        }

        /// <summary>
        /// Called when mouse or touch clicks the button.
        /// </summary>
        protected virtual void OnClick()
        {
            Clicked?.Invoke();
            ButtonClicked?.Invoke(this);
        }

        /// <summary>
        /// Called when button starts to be pressed by the used (via mouse or touch).
        /// </summary>
        protected virtual void OnPressBegin()
        {
            _isPressed = true;
            if (AutoFocus)
                Focus();
        }

        /// <summary>
        /// Called when button ends to be pressed by the used (via mouse or touch).
        /// </summary>
        protected virtual void OnPressEnd()
        {
            _isPressed = false;
        }

        /// <summary>
        /// Sets the button colors palette based on a given main color.
        /// </summary>
        /// <param name="color">The main color.</param>
        public virtual void SetColors(Color color)
        {
            BackgroundColor = color;
            BorderColor = color.RGBMultiplied(0.5f);
            BackgroundColorSelected = color.RGBMultiplied(0.8f);
            BorderColorSelected = BorderColor;
            BackgroundColorHighlighted = color.RGBMultiplied(1.2f);
            BorderColorHighlighted = BorderColor;
        }

        /// <inheritdoc />
        public override void ClearState()
        {
            base.ClearState();
            
            if (_isPressed)
                OnPressEnd();
        }

        /// <inheritdoc />
        public override void DrawSelf()
        {
            // Cache data
            Rectangle clientRect = new Rectangle(Float2.Zero, Size);
            bool enabled = EnabledInHierarchy;
            Color backgroundColor = BackgroundColor;
            Color borderColor = BorderColor;
            Color textColor = TextColor;
            if (!enabled)
            {
                backgroundColor *= 0.5f;
                borderColor *= 0.5f;
                textColor *= 0.6f;
            }
            else if (_isPressed)
            {
                backgroundColor = BackgroundColorSelected;
                borderColor = BorderColorSelected;
            }
            else if (IsMouseOver || IsNavFocused)
            {
                backgroundColor = BackgroundColorHighlighted;
                borderColor = BorderColorHighlighted;
            }

            // Draw background
            if (BackgroundBrush != null)
                BackgroundBrush.Draw(clientRect, backgroundColor);
            else
                Render2D.FillRectangle(clientRect, backgroundColor);
            Render2D.DrawRectangle(clientRect, borderColor);

            // Draw text
            Render2D.DrawText(_font?.GetFont(), TextMaterial, _text, clientRect, textColor, TextAlignment.Center, TextAlignment.Center);
        }

        /// <inheritdoc />
        public override void OnMouseEnter(Float2 location)
        {
            base.OnMouseEnter(location);

            HoverBegin?.Invoke();
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            if (_isPressed)
            {
                OnPressEnd();
            }

            HoverEnd?.Invoke();

            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (base.OnMouseDown(location, button))
                return true;

            if (button == MouseButton.Left && !_isPressed)
            {
                OnPressBegin();
                return true;
            }
            return false;
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (base.OnMouseUp(location, button))
                return true;

            if (button == MouseButton.Left && _isPressed)
            {
                OnPressEnd();
                OnClick();
                return true;
            }
            return false;
        }

        /// <inheritdoc />
        public override bool OnTouchDown(Float2 location, int pointerId)
        {
            if (base.OnTouchDown(location, pointerId))
                return true;

            if (!_isPressed)
            {
                OnPressBegin();
                return true;
            }
            return false;
        }

        /// <inheritdoc />
        public override bool OnTouchUp(Float2 location, int pointerId)
        {
            if (base.OnTouchUp(location, pointerId))
                return true;

            if (_isPressed)
            {
                OnPressEnd();
                OnClick();
                return true;
            }
            return false;
        }

        /// <inheritdoc />
        public override void OnTouchLeave()
        {
            if (_isPressed)
            {
                OnPressEnd();
            }

            base.OnTouchLeave();
        }

        /// <inheritdoc />
        public override void OnLostFocus()
        {
            if (_isPressed)
            {
                OnPressEnd();
            }

            base.OnLostFocus();
        }

        /// <inheritdoc />
        public override void OnSubmit()
        {
            OnClick();

            base.OnSubmit();
        }
    }
}
