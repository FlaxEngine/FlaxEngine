// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Rich text box control which can gather text input from the user and present text in highly formatted and stylized way.
    /// </summary>
    public class RichTextBox : RichTextBoxBase
    {
        private TextBlockStyle _textStyle;

        /// <summary>
        /// The text style applied to the whole text.
        /// </summary>
        [EditorOrder(20), Tooltip("The watermark text to show grayed when textbox is empty.")]
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
        protected override void OnParseTextBlocks()
        {
            if (ParseTextBlocks != null)
            {
                ParseTextBlocks(_text, _textBlocks);
                return;
            }

            var font = _textStyle.Font.GetFont();
            if (!font)
                return;
            var lines = font.ProcessText(_text);
            _textBlocks.Capacity = Math.Max(_textBlocks.Capacity, lines.Length);

            for (int i = 0; i < lines.Length; i++)
            {
                ref var line = ref lines[i];

                _textBlocks.Add(new TextBlock
                {
                    Style = _textStyle,
                    Range = new TextRange
                    {
                        StartIndex = line.FirstCharIndex,
                        EndIndex = line.LastCharIndex,
                    },
                    Bounds = new Rectangle(line.Location, line.Size),
                });
            }
        }
    }
}
