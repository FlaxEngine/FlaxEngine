// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEngine.Assertions;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Text box control which can gather text input from the user.
    /// </summary>
    public class TextBox : TextBoxBase
    {
        private TextLayoutOptions _layout;

        /// <summary>
        /// Gets or sets the watermark text to show grayed when textbox is empty.
        /// </summary>
        [EditorOrder(20), Tooltip("The watermark text to show grayed when textbox is empty.")]
        public string WatermarkText { get; set; }

        /// <summary>
        /// Gets or sets the text wrapping within the control bounds.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000), Tooltip("The text wrapping within the control bounds.")]
        public TextWrapping Wrapping
        {
            get => _layout.TextWrapping;
            set => _layout.TextWrapping = value;
        }

        /// <summary>
        /// Gets or sets the font.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000)]
        public FontReference Font { get; set; }

        /// <summary>
        /// Gets or sets the custom material used to render the text. It must has domain set to GUI and have a public texture parameter named Font used to sample font atlas texture with font characters data.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000), Tooltip("Custom material used to render the text. It must has domain set to GUI and have a public texture parameter named Font used to sample font atlas texture with font characters data.")]
        public MaterialBase TextMaterial { get; set; }

        /// <summary>
        /// Gets or sets the color of the text.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000), Tooltip("The color of the text.")]
        public Color TextColor { get; set; }

        /// <summary>
        /// Gets or sets the color of the text.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000), Tooltip("The color of the watermark text.")]
        public Color WatermarkTextColor { get; set; }

        /// <summary>
        /// Gets or sets the color of the selection (Transparent if not used).
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000), Tooltip("The color of the selection (Transparent if not used).")]
        public Color SelectionColor { get; set; }

        /// <summary>
        /// Initializes a new instance of the <see cref="TextBox"/> class.
        /// </summary>
        public TextBox()
        : this(false, 0, 0)
        {
        }

        /// <summary>
        /// Init
        /// </summary>
        /// <param name="isMultiline">Enable/disable multiline text input support</param>
        /// <param name="x">Position X coordinate</param>
        /// <param name="y">Position Y coordinate</param>
        /// <param name="width">Width</param>
        public TextBox(bool isMultiline, float x, float y, float width = 120)
        : base(isMultiline, x, y, width)
        {
            _layout = TextLayoutOptions.Default;
            _layout.VerticalAlignment = IsMultiline ? TextAlignment.Near : TextAlignment.Center;
            _layout.TextWrapping = TextWrapping.NoWrap;
            _layout.Bounds = new Rectangle(DefaultMargin, 1, Width - 2 * DefaultMargin, Height - 2);

            var style = Style.Current;
            Font = new FontReference(style.FontMedium);
            TextColor = style.Foreground;
            WatermarkTextColor = style.ForegroundDisabled;
            SelectionColor = style.BackgroundSelected;
        }

        /// <inheritdoc />
        public override Vector2 GetTextSize()
        {
            var font = Font.GetFont();
            if (font == null)
            {
                return Vector2.Zero;
            }

            return font.MeasureText(_text, ref _layout);
        }

        /// <inheritdoc />
        public override Vector2 GetCharPosition(int index, out float height)
        {
            var font = Font.GetFont();
            if (font == null)
            {
                height = Height;
                return Vector2.Zero;
            }

            height = font.Height;
            return font.GetCharPosition(_text, index, ref _layout);
        }

        /// <inheritdoc />
        public override int HitTestText(Vector2 location)
        {
            var font = Font.GetFont();
            if (font == null)
            {
                return 0;
            }

            return font.HitTestText(_text, location, ref _layout);
        }

        /// <inheritdoc />
        protected override void OnIsMultilineChanged()
        {
            base.OnIsMultilineChanged();

            _layout.VerticalAlignment = IsMultiline ? TextAlignment.Near : TextAlignment.Center;
        }

        /// <inheritdoc />
        public override void Draw()
        {
            // Cache data
            var rect = new Rectangle(Vector2.Zero, Size);
            bool enabled = EnabledInHierarchy;
            var font = Font.GetFont();
            if (!font)
                return;

            // Background
            Color backColor = BackgroundColor;
            if (IsMouseOver)
                backColor = BackgroundSelectedColor;
            Render2D.FillRectangle(rect, backColor);
            Render2D.DrawRectangle(rect, IsFocused ? BorderSelectedColor : BorderColor);

            // Apply view offset and clip mask
            Render2D.PushClip(TextClipRectangle);
            bool useViewOffset = !_viewOffset.IsZero;
            if (useViewOffset)
                Render2D.PushTransform(Matrix3x3.Translation2D(-_viewOffset));

            // Check if sth is selected to draw selection
            if (HasSelection)
            {
                Vector2 leftEdge = font.GetCharPosition(_text, SelectionLeft, ref _layout);
                Vector2 rightEdge = font.GetCharPosition(_text, SelectionRight, ref _layout);
                float fontHeight = font.Height;

                // Draw selection background
                float alpha = Mathf.Min(1.0f, Mathf.Cos(_animateTime * BackgroundSelectedFlashSpeed) * 0.5f + 1.3f);
                alpha *= alpha;
                Color selectionColor = SelectionColor * alpha;
                //
                int selectedLinesCount = 1 + Mathf.FloorToInt((rightEdge.Y - leftEdge.Y) / fontHeight);
                if (selectedLinesCount == 1)
                {
                    // Selected is part of single line
                    Rectangle r1 = new Rectangle(leftEdge.X, leftEdge.Y, rightEdge.X - leftEdge.X, fontHeight);
                    Render2D.FillRectangle(r1, selectionColor);
                }
                else
                {
                    float leftMargin = _layout.Bounds.Location.X;

                    // Selected is more than one line
                    Rectangle r1 = new Rectangle(leftEdge.X, leftEdge.Y, 1000000000, fontHeight);
                    Render2D.FillRectangle(r1, selectionColor);
                    //
                    for (int i = 3; i <= selectedLinesCount; i++)
                    {
                        leftEdge.Y += fontHeight;
                        Rectangle r = new Rectangle(leftMargin, leftEdge.Y, 1000000000, fontHeight);
                        Render2D.FillRectangle(r, selectionColor);
                    }
                    //
                    Rectangle r2 = new Rectangle(leftMargin, rightEdge.Y, rightEdge.X - leftMargin, fontHeight);
                    Render2D.FillRectangle(r2, selectionColor);
                }
            }

            // Text or watermark
            if (_text.Length > 0)
            {
                var color = TextColor;
                if (!enabled)
                    color *= 0.6f;
                Render2D.DrawText(font, _text, color, ref _layout, TextMaterial);
            }
            else if (!string.IsNullOrEmpty(WatermarkText) && !IsFocused)
            {
                Render2D.DrawText(font, WatermarkText, WatermarkTextColor, ref _layout, TextMaterial);
            }

            // Caret
            if (IsFocused && CaretPosition > -1)
            {
                float alpha = Mathf.Saturate(Mathf.Cos(_animateTime * CaretFlashSpeed) * 0.5f + 0.7f);
                alpha = alpha * alpha * alpha * alpha * alpha * alpha;
                Render2D.FillRectangle(CaretBounds, CaretColor * alpha);
            }

            // Restore rendering state
            if (useViewOffset)
                Render2D.PopTransform();
            Render2D.PopClip();
        }

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Vector2 location, MouseButton button)
        {
            SelectAll();
            return base.OnMouseDoubleClick(location, button);
        }

        /// <inheritdoc />
        protected override void OnSizeChanged()
        {
            base.OnSizeChanged();

            _layout.Bounds = TextRectangle;
        }
    }
}
