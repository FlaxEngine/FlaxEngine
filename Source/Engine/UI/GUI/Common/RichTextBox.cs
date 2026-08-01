// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Rich text box control which can gather text input from the user and present text in highly formatted and stylized way.
    /// </summary>
    [ActorToolbox("GUI")]
    public partial class RichTextBox : RichTextBoxBase
    {
        private TextBlockStyle _textStyle;
        private TextWrapping _wrapping = TextWrapping.NoWrap;
        private float _baseLinesGapScale = 1.0f;

        /// <summary>
        /// The default text style applied to the whole text.
        /// </summary>
        [EditorOrder(20)]
        public TextBlockStyle TextStyle
        {
            get => _textStyle;
            set
            {
                _textStyle = value;
                UpdateTextBlocks();
            }
        }

        /// <summary>
        /// The collection of custom text styles to apply (named).
        /// </summary>
        [EditorOrder(30)]
        public Dictionary<string, TextBlockStyle> Styles = new Dictionary<string, TextBlockStyle>();

        /// <summary>
        /// The collection of custom images/sprites that can be inlined in text (named).
        /// </summary>
        [EditorOrder(40)]
        public Dictionary<string, IBrush> Images = new Dictionary<string, IBrush>();

        /// <summary>
        /// Gets or sets the text wrapping within the control bounds.
        /// </summary>
        [EditorOrder(50), Tooltip("The text wrapping within the control bounds.")]
        public TextWrapping Wrapping
        {
            get => _wrapping;
            set
            {
                if (_wrapping == value)
                    return;
                _wrapping = value;
                UpdateTextBlocks();
            }
        }

        /// <summary>
        /// Gets or sets the gap between lines when wrapping and more than a single line is displayed.
        /// </summary>
        [EditorOrder(60), Tooltip("The gap between lines when wrapping and more than a single line is displayed."), Limit(0f)]
        public float BaseLinesGapScale
        {
            get => _baseLinesGapScale;
            set
            {
                if (Mathf.NearEqual(_baseLinesGapScale, value))
                    return;
                _baseLinesGapScale = value;
                UpdateTextBlocks();
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="RichTextBox"/> class.
        /// </summary>
        public RichTextBox()
        {
            var style = Style.Current;
            _textStyle = new TextBlockStyle
            {
                Font = new FontReference(style.FontMedium),
                Color = style.Foreground,
                BackgroundSelectedBrush = new SolidColorBrush(style.BackgroundSelected),
            };
        }

        /// <inheritdoc />
        protected override void OnSizeChanged()
        {
            base.OnSizeChanged();

            // Refresh textblocks since those might depend on control size (eg. align right)
            UpdateTextBlocks();
        }
    }
}
