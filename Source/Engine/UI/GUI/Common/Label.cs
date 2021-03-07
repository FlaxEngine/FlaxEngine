// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.ComponentModel;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// The basic GUI label control.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    public class Label : ContainerControl
    {
        private string _text;
        private bool _autoWidth;
        private bool _autoHeight;
        private bool _autoFitText;
        private Vector2 _textSize;
        private Vector2 _autoFitTextRange = new Vector2(0.1f, 100.0f);

        /// <summary>
        /// The font.
        /// </summary>
        protected FontReference _font;

        /// <summary>
        /// Gets or sets the text.
        /// </summary>
        [EditorOrder(10), MultilineText, Tooltip("The label text.")]
        public string Text
        {
            get => _text;
            set
            {
                if (_text != value)
                {
                    _text = value;
                    _textSize = Vector2.Zero;
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets or sets the color of the text.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000), Tooltip("The color of the text.")]
        public Color TextColor { get; set; }

        /// <summary>
        /// Gets or sets the color of the text when it is highlighted (mouse is over).
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2000), Tooltip("The color of the text when it is highlighted (mouse is over).")]
        public Color TextColorHighlighted { get; set; }

        /// <summary>
        /// Gets or sets the horizontal text alignment within the control bounds.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2010), Tooltip("The horizontal text alignment within the control bounds.")]
        public TextAlignment HorizontalAlignment { get; set; } = TextAlignment.Center;

        /// <summary>
        /// Gets or sets the vertical text alignment within the control bounds.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2020), Tooltip("The vertical text alignment within the control bounds.")]
        public TextAlignment VerticalAlignment { get; set; } = TextAlignment.Center;

        /// <summary>
        /// Gets or sets the text wrapping within the control bounds.
        /// </summary>
        [EditorDisplay("Style"), EditorOrder(2030), Tooltip("The text wrapping within the control bounds.")]
        public TextWrapping Wrapping { get; set; } = TextWrapping.NoWrap;

        /// <summary>
        /// Gets or sets the font.
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
        [EditorDisplay("Style"), EditorOrder(2000)]
        public MaterialBase Material { get; set; }

        /// <summary>
        /// Gets or sets the margin for the text within the control bounds.
        /// </summary>
        [EditorOrder(70), Tooltip("The margin for the text within the control bounds.")]
        public Margin Margin { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether clip text during rendering.
        /// </summary>
        [EditorOrder(80), DefaultValue(false), Tooltip("If checked, text will be clipped during rendering.")]
        public bool ClipText { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether set automatic width based on text contents.
        /// </summary>
        [EditorOrder(85), DefaultValue(false), Tooltip("If checked, the control width will be based on text contents.")]
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
        /// Gets or sets a value indicating whether set automatic height based on text contents.
        /// </summary>
        [EditorOrder(90), DefaultValue(false), Tooltip("If checked, the control height will be based on text contents.")]
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
        [EditorOrder(110), DefaultValue(typeof(Vector2), "0.1, 100"), Tooltip("The text scale range (min and max) for automatic fit text option. Can be used to constraint the text scale adjustment.")]
        public Vector2 AutoFitTextRange
        {
            get => _autoFitTextRange;
            set => _autoFitTextRange = value;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Label"/> class.
        /// </summary>
        public Label()
        : base(0, 0, 100, 20)
        {
            AutoFocus = false;
            var style = Style.Current;
            Font = new FontReference(style.FontMedium);
            TextColor = Style.Current.Foreground;
            TextColorHighlighted = Style.Current.Foreground;
        }

        /// <inheritdoc />
        public Label(float x, float y, float width, float height)
        : base(x, y, width, height)
        {
            AutoFocus = false;
            var style = Style.Current;
            Font = new FontReference(style.FontMedium);
            TextColor = Style.Current.Foreground;
            TextColorHighlighted = Style.Current.Foreground;
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            var rect = new Rectangle(new Vector2(Margin.Left, Margin.Top), Size - Margin.Size);

            if (ClipText)
                Render2D.PushClip(new Rectangle(Vector2.Zero, Size));

            var color = IsMouseOver ? TextColorHighlighted : TextColor;

            if (!EnabledInHierarchy)
                color *= 0.6f;

            var scale = 1.0f;
            var hAlignment = _autoWidth ? TextAlignment.Near : HorizontalAlignment;
            var wAlignment = _autoHeight ? TextAlignment.Near : VerticalAlignment;
            if (_autoFitText)
            {
                hAlignment = TextAlignment.Center;
                wAlignment = TextAlignment.Center;
                if (!_textSize.IsZero)
                {
                    scale = (rect.Size / _textSize).MinValue;
                    scale = Mathf.Clamp(scale, _autoFitTextRange.X, _autoFitTextRange.Y);
                }
            }

            Render2D.DrawText(
                _font.GetFont(),
                Material,
                Text,
                rect,
                color,
                hAlignment,
                wAlignment,
                Wrapping,
                1.0f,
                scale
            );

            if (ClipText)
                Render2D.PopClip();
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            if (_autoWidth || _autoHeight || _autoFitText)
            {
                var font = _font.GetFont();
                if (font)
                {
                    // Calculate text size
                    _textSize = font.MeasureText(_text);

                    // Check if size is controlled via text
                    if (_autoWidth || _autoHeight)
                    {
                        var size = Size;
                        if (_autoWidth)
                            size.X = _textSize.X + Margin.Width;
                        if (_autoHeight)
                            size.Y = _textSize.Y + Margin.Height;
                        Size = size;
                    }
                }
            }

            base.PerformLayoutBeforeChildren();
        }
    }
}
