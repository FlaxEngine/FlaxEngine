// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Rich text box control which can gather text input from the user and present text in highly formatted and stylized way.
    /// </summary>
    public partial class RichTextBox : RichTextBoxBase
    {
        private TextBlockStyle _textStyle;

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
    }
}
