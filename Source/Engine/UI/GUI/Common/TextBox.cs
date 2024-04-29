// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Text box control which can gather text input from the user.
    /// </summary>
    [ActorToolbox("GUI")]
    public class TextBox : TextBoxBase
    {
        private TextLayoutOptions _layout;

        /// <summary>
        /// The watermark text.
        /// </summary>
        protected LocalizedString _watermarkText;

        /// <summary>
        /// Gets or sets the watermark text to show grayed when textbox is empty.
        /// </summary>
        [EditorOrder(20), Tooltip("The watermark text to show grayed when textbox is empty.")]
        public LocalizedString WatermarkText
        {
            get => _watermarkText;
            set => _watermarkText = value;
        }

        /// <summary>
        /// Gets or sets the text wrapping within the control bounds.
        /// </summary>
        [EditorDisplay("Text Style"), EditorOrder(2023), Tooltip("The text wrapping within the control bounds.")]
        public TextWrapping Wrapping
        {
            get => _layout.TextWrapping;
            set => _layout.TextWrapping = value;
        }

        /// <summary>
        /// Gets or sets the font.
        /// </summary>
        [EditorDisplay("Text Style"), EditorOrder(2024)]
        public FontReference Font { get; set; }

        /// <summary>
        /// Gets or sets the custom material used to render the text. It must has domain set to GUI and have a public texture parameter named Font used to sample font atlas texture with font characters data.
        /// </summary>
        [EditorDisplay("Text Style"), EditorOrder(2025), Tooltip("Custom material used to render the text. It must has domain set to GUI and have a public texture parameter named Font used to sample font atlas texture with font characters data.")]
        public MaterialBase TextMaterial { get; set; }

        /// <summary>
        /// Gets or sets the color of the text.
        /// </summary>
        [EditorDisplay("Text Style"), EditorOrder(2020), Tooltip("The color of the text."), ExpandGroups]
        public Color TextColor { get; set; }

        /// <summary>
        /// Gets or sets the color of the text.
        /// </summary>
        [EditorDisplay("Text Style"), EditorOrder(2021), Tooltip("The color of the watermark text.")]
        public Color WatermarkTextColor { get; set; }

        /// <summary>
        /// Gets or sets the color of the selection (Transparent if not used).
        /// </summary>
        [EditorDisplay("Text Style"), EditorOrder(2022), Tooltip("The color of the selection (Transparent if not used).")]
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
        public override Float2 GetTextSize()
        {
            var font = Font.GetFont();
            if (font == null)
            {
                return Float2.Zero;
            }

            return font.MeasureText(_text, ref _layout);
        }

        /// <inheritdoc />
        public override Float2 GetCharPosition(int index, out float height)
        {
            var font = Font.GetFont();
            if (font == null)
            {
                height = Height;
                return Float2.Zero;
            }

            height = font.Height / DpiScale;
            return font.GetCharPosition(_text, index, ref _layout);
        }

        /// <inheritdoc />
        public override int HitTestText(Float2 location)
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
        public override void DrawSelf()
        {
            // Cache data
            var rect = new Rectangle(Float2.Zero, Size);
            bool enabled = EnabledInHierarchy;
            var font = Font.GetFont();
            if (!font)
                return;

            // Background
            Color backColor = BackgroundColor;
            if (IsMouseOver || IsNavFocused)
                backColor = BackgroundSelectedColor;
            Render2D.FillRectangle(rect, backColor);
            if (HasBorder)
                Render2D.DrawRectangle(rect, IsFocused ? BorderSelectedColor : BorderColor, BorderThickness);

            // Apply view offset and clip mask
            if (ClipText)
                Render2D.PushClip(TextClipRectangle);
            bool useViewOffset = !_viewOffset.IsZero;
            if (useViewOffset)
                Render2D.PushTransform(Matrix3x3.Translation2D(-_viewOffset));

            // Check if sth is selected to draw selection
            if (HasSelection)
            {
                var leftEdge = font.GetCharPosition(_text, SelectionLeft, ref _layout);
                var rightEdge = font.GetCharPosition(_text, SelectionRight, ref _layout);
                var fontHeight = font.Height;
                var textHeight = fontHeight / DpiScale;

                // Draw selection background
                float alpha = Mathf.Min(1.0f, Mathf.Cos(_animateTime * BackgroundSelectedFlashSpeed) * 0.5f + 1.3f);
                alpha *= alpha;
                Color selectionColor = SelectionColor * alpha;
                //
                int selectedLinesCount = 1 + Mathf.FloorToInt((rightEdge.Y - leftEdge.Y) / textHeight);
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
                        leftEdge.Y += textHeight;
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
            else if (!string.IsNullOrEmpty(_watermarkText))
            {
                Render2D.DrawText(font, _watermarkText, WatermarkTextColor, ref _layout, TextMaterial);
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
            if (ClipText)
                Render2D.PopClip();
        }

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            if (IsSelectable)
            {
                SelectAll();
            }
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
