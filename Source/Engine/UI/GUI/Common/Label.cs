// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System.ComponentModel;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Options for text case
    /// </summary>
    public enum TextCaseOptions
    {
        /// <summary>
        /// No text case.
        /// </summary>
        None,

        /// <summary>
        /// Uppercase.
        /// </summary>
        Uppercase,

        /// <summary>
        /// Lowercase
        /// </summary>
        Lowercase
    }

    /// <summary>
    /// The basic GUI label control.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    [ActorToolbox("GUI")]
    public class Label : ContainerControl
    {
        /// <summary>
        /// The text.
        /// </summary>
        protected LocalizedString _text = new LocalizedString();

        private bool _autoWidth;
        private bool _autoHeight;
        private bool _autoFitText;
        private Float2 _textSize;
        private Float2 _autoFitTextRange = new Float2(0.1f, 100.0f);
        private Margin _margin;

        /// <summary>
        /// The font.
        /// </summary>
        protected FontReference _font;

        /// <summary>
        /// Gets or sets the text.
        /// </summary>
        [EditorOrder(10), MultilineText, Tooltip("The label text.")]
        public LocalizedString Text
        {
            get => _text;
            set
            {
                _text = value;
                if (_autoWidth || _autoHeight || _autoFitText)
                {
                    _textSize = Float2.Zero;
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// The text case.
        /// </summary>
        [EditorDisplay("Text Style"), EditorOrder(2000), Tooltip("The case of the text.")]
        public TextCaseOptions CaseOption { get; set; } = TextCaseOptions.None;

        /// <summary>
        /// Whether to bold the text.
        /// </summary>
        [EditorDisplay("Text Style"), EditorOrder(2001), Tooltip("Bold the text.")]
        public bool Bold { get; set; } = false;

        /// <summary>
        /// Whether to italicize the text.
        /// </summary>
        [EditorDisplay("Text Style"), EditorOrder(2002), Tooltip("Italicize the text.")]
        public bool Italic { get; set; } = false;

        /// <summary>
        /// Gets or sets the color of the text.
        /// </summary>
        [EditorDisplay("Text Style"), EditorOrder(2010), Tooltip("The color of the text."), ExpandGroups]
        public Color TextColor { get; set; }

        /// <summary>
        /// Gets or sets the color of the text when it is highlighted (mouse is over).
        /// </summary>
        [EditorDisplay("Text Style"), EditorOrder(2011), Tooltip("The color of the text when it is highlighted (mouse is over).")]
        public Color TextColorHighlighted { get; set; }

        /// <summary>
        /// Gets or sets the horizontal text alignment within the control bounds.
        /// </summary>
        [EditorDisplay("Text Style"), EditorOrder(2020), Tooltip("The horizontal text alignment within the control bounds.")]
        public TextAlignment HorizontalAlignment { get; set; } = TextAlignment.Center;

        /// <summary>
        /// Gets or sets the vertical text alignment within the control bounds.
        /// </summary>
        [EditorDisplay("Text Style"), EditorOrder(2021), Tooltip("The vertical text alignment within the control bounds.")]
        public TextAlignment VerticalAlignment { get; set; } = TextAlignment.Center;

        /// <summary>
        /// Gets or sets the text wrapping within the control bounds.
        /// </summary>
        [EditorDisplay("Text Style"), EditorOrder(2022), Tooltip("The text wrapping within the control bounds.")]
        public TextWrapping Wrapping { get; set; } = TextWrapping.NoWrap;

        /// <summary>
        /// Gets or sets the text wrapping within the control bounds.
        /// </summary>
        [EditorDisplay("Text Style"), EditorOrder(2023), Tooltip("The gap between lines when wrapping and more than a single line is displayed."), Limit(0f)]
        public float BaseLinesGapScale { get; set; } = 1.0f;

        /// <summary>
        /// Gets or sets the font.
        /// </summary>
        [EditorDisplay("Text Style"), EditorOrder(2024)]
        public FontReference Font
        {
            get => _font;
            set
            {
                _font = value;
                if (_autoWidth || _autoHeight || _autoFitText)
                {
                    _textSize = Float2.Zero;
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets or sets the custom material used to render the text. It has to have domain set to GUI and have a public texture parameter named Font used to sample font atlas texture with font characters data.
        /// </summary>
        [EditorDisplay("Text Style"), EditorOrder(2025)]
        public MaterialBase Material { get; set; }

        /// <summary>
        /// Gets or sets the margin for the text within the control bounds.
        /// </summary>
        [EditorOrder(70), Tooltip("The margin for the text within the control bounds.")]
        public Margin Margin
        {
            get => _margin;
            set
            {
                _margin = value;
                if (_autoWidth || _autoHeight)
                {
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether clip text during rendering.
        /// </summary>
        [EditorOrder(80), DefaultValue(false), Tooltip("If checked, text will be clipped during rendering.")]
        public bool ClipText { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether set automatic width based on text contents. Control size is modified relative to the Pivot.
        /// </summary>
        [EditorOrder(85), DefaultValue(false), Tooltip("If checked, the control width will be based on text contents. Control size is modified relative to the Pivot.")]
        public bool AutoWidth
        {
            get => _autoWidth;
            set
            {
                if (_autoWidth != value)
                {
                    _autoWidth = value;
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether set automatic height based on text contents. Control size is modified relative to the Pivot.
        /// </summary>
        [EditorOrder(90), DefaultValue(false), Tooltip("If checked, the control height will be based on text contents. Control size is modified relative to the Pivot.")]
        public bool AutoHeight
        {
            get => _autoHeight;
            set
            {
                if (_autoHeight != value)
                {
                    _autoHeight = value;
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether scale text to fit the label control bounds. Disables using text alignment, automatic width and height.
        /// </summary>
        [EditorOrder(100), DefaultValue(false), Tooltip("If checked, enables scaling text to fit the label control bounds. Disables using text alignment, automatic width and height.")]
        public bool AutoFitText
        {
            get => _autoFitText;
            set
            {
                if (_autoFitText != value)
                {
                    _autoFitText = value;
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets or sets the text scale range (min and max) for automatic fit text option. Can be used to constraint the text scale adjustment.
        /// </summary>
        [VisibleIf(nameof(AutoFitText))]
        [EditorOrder(110), DefaultValue(typeof(Float2), "0.1, 100"), Tooltip("The text scale range (min and max) for automatic fit text option. Can be used to constraint the text scale adjustment.")]
        public Float2 AutoFitTextRange
        {
            get => _autoFitTextRange;
            set => _autoFitTextRange = value;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Label"/> class.
        /// </summary>
        public Label()
        : this(0, 0, 100, 20)
        {
        }

        /// <inheritdoc />
        public Label(float x, float y, float width, float height)
        : base(x, y, width, height)
        {
            AutoFocus = false;
            var style = Style.Current;
            if (style != null)
            {
                Font = new FontReference(style.FontMedium);
                TextColor = style.Foreground;
                TextColorHighlighted = style.Foreground;
            }
        }

        /// <inheritdoc />
        public override void DrawSelf()
        {
            base.DrawSelf();

            var margin = _margin;
            var rect = new Rectangle(margin.Location, Size - margin.Size);

            if (ClipText)
                Render2D.PushClip(ref rect);

            var color = IsMouseOver || IsNavFocused ? TextColorHighlighted : TextColor;
            if (!EnabledInHierarchy)
                color *= 0.6f;
            var scale = 1.0f;
            var hAlignment = HorizontalAlignment;
            var wAlignment = VerticalAlignment;
            if (_autoFitText && !_textSize.IsZero)
            {
                scale = (rect.Size / _textSize).MinValue;
                scale = Mathf.Clamp(scale, _autoFitTextRange.X, _autoFitTextRange.Y);
            }

            Font font = GetFont();
            var text = ConvertedText();
            Render2D.DrawText(font, Material, text, rect, color, hAlignment, wAlignment, Wrapping, BaseLinesGapScale, scale);

            if (ClipText)
                Render2D.PopClip();
        }

        private Font GetFont()
        {
            Font font;
            if (Bold)
                font = Italic ? _font.GetBold().GetItalic().GetFont() : _font.GetBold().GetFont();
            else if (Italic)
                font = _font.GetItalic().GetFont();
            else
                font = _font.GetFont();
            return font;
        }

        private LocalizedString ConvertedText()
        {
            LocalizedString text = _text;
            switch (CaseOption)
            {
            case TextCaseOptions.Uppercase:
                text = text.ToString().ToUpper();
                break;
            case TextCaseOptions.Lowercase:
                text = text.ToString().ToLower();
                break;
            }
            return text;
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            if (_autoWidth || _autoHeight || _autoFitText)
            {
                Font font = GetFont();
                var text = ConvertedText();
                if (font)
                {
                    // Calculate text size
                    var layout = TextLayoutOptions.Default;
                    layout.TextWrapping = Wrapping;
                    if (_autoHeight && !_autoWidth)
                        layout.Bounds.Size.X = Width - Margin.Width;
                    else if (_autoWidth && !_autoHeight)
                        layout.Bounds.Size.Y = Height - Margin.Height;
                    _textSize = font.MeasureText(text, ref layout);
                    _textSize.Y *= BaseLinesGapScale;

                    // Check if size is controlled via text
                    if (_autoWidth || _autoHeight)
                    {
                        var size = Size;
                        if (_autoWidth)
                            size.X = _textSize.X + Margin.Width;
                        if (_autoHeight)
                            size.Y = _textSize.Y + Margin.Height;
                        var pivotRelative = PivotRelative;
                        Size = size;
                        PivotRelative = pivotRelative;
                    }
                }
            }

            base.PerformLayoutBeforeChildren();
        }
    }
}
