// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using FlaxEngine.Utilities;

namespace FlaxEngine.GUI
{
    partial class RichTextBox
    {
        /// <summary>
        /// Rich Text parsing context.
        /// </summary>
        public struct ParsingContext
        {
            /// <summary>
            /// Text box control.
            /// </summary>
            public RichTextBox Control;

            /// <summary>
            /// HTML tags parser.
            /// </summary>
            public HtmlParser Parser;

            /// <summary>
            /// Current caret location for the new text blocks origin.
            /// </summary>
            public Float2 Caret;
            
            /// <summary>
            /// Text styles stack (new tags push modified style and pop on tag end).
            /// </summary>
            public Stack<TextBlockStyle> StyleStack;
        }
        
        /// <summary>
        /// The delegate for text blocks post-processing.
        /// </summary>
        /// <param name="context">The parsing context.</param>
        /// <param name="textBlock">The text block (not yet added to the text box - during parsing).</param>
        public delegate void PostProcessBlockDelegate(ref ParsingContext context, ref TextBlock textBlock);

        /// <summary>
        /// The custom callback for post-processing text blocks.
        /// </summary>
        [HideInEditor]
        public PostProcessBlockDelegate PostProcessBlock;
        
        /// <summary>
        /// The delegate for HTML tag processing.
        /// </summary>
        /// <param name="context">The parsing context.</param>
        /// <param name="tag">The tag.</param>
        public delegate void ProcessTagDelegate(ref ParsingContext context, ref HtmlTag tag);

        /// <summary>
        /// Collection of HTML tags processors.
        /// </summary>
        public static Dictionary<string, ProcessTagDelegate> TagProcessors = new Dictionary<string, ProcessTagDelegate>
        {
            { "br", ProcessBr },
            { "color", ProcessColor },
            { "alpha", ProcessAlpha },
            { "style", ProcessStyle },
            { "b", ProcessBold },
            { "i", ProcessItalic },
        };

        private HtmlParser _parser = new HtmlParser();
        private Stack<TextBlockStyle> _styleStack = new Stack<TextBlockStyle>();

        /// <inheritdoc />
        protected override void OnParseTextBlocks()
        {
            if (ParseTextBlocks != null)
            {
                ParseTextBlocks(_text, _textBlocks);
                return;
            }
            
            // Setup parsing
            _parser.Reset(_text);
            _styleStack.Clear();
            _styleStack.Push(_textStyle);
            var context = new ParsingContext
            {
                Control = this,
                Parser = _parser,
                StyleStack = _styleStack,
            };

            // Parse text with HTML tags
            int testStartPos = 0;
            while (_parser.ParseNext(out var tag))
            {
                // Insert text found before the tag
                OnAddTextBlock(ref context, testStartPos, tag.StartPosition);

                // Move the text pointer after the tag
                testStartPos = tag.EndPosition;

                // Process the tag
                OnParseTag(ref context, ref tag);
            }

            // Insert remaining text
            OnAddTextBlock(ref context, testStartPos, _text.Length);
        }

        /// <summary>
        /// Parses HTML tag.
        /// </summary>
        /// <param name="context">The parsing context.</param>
        /// <param name="tag">The tag.</param>
        protected virtual void OnParseTag(ref ParsingContext context, ref HtmlTag tag)
        {
            // Get tag processor matching the tag name
            if (!TagProcessors.TryGetValue(tag.Name, out var process))
                TagProcessors.TryGetValue(tag.Name.ToLower(), out process);

            if (process != null)
            {
                process(ref context, ref tag);
            }
        }

        /// <summary>
        /// Inserts parsed text block using the current style and state (eg. from tags).
        /// </summary>
        /// <param name="context">The parsing context.</param>
        /// <param name="start">Start position (character index).</param>
        /// <param name="end">End position (character index).</param>
        protected virtual void OnAddTextBlock(ref ParsingContext context, int start, int end)
        {
            if (start >= end)
                return;

            // Setup text block
            var textBlock = new TextBlock
            {
                Style = context.StyleStack.Peek(),
                Range = new TextRange
                {
                    StartIndex = start,
                    EndIndex = end,
                },
            };

            // Process text into text blocks (handle newlines etc.)
            var font = textBlock.Style.Font.GetFont();
            if (!font)
                return;
            var lines = font.ProcessText(_text, ref textBlock.Range);
            if (lines == null || lines.Length == 0)
                return;
            for (int i = 0; i < lines.Length; i++)
            {
                if (i != 0)
                    context.Caret.X = 0;
                ref var line = ref lines[i];
                textBlock.Range = new TextRange
                {
                    StartIndex = start + line.FirstCharIndex,
                    EndIndex = start + line.LastCharIndex,
                };
                textBlock.Bounds = new Rectangle(context.Caret + line.Location, line.Size);

                // Post-processing
                PostProcessBlock?.Invoke(ref context, ref textBlock);
            
                // Add to the text blocks
                _textBlocks.Add(textBlock);
            }
            
            // Update the caret location
            ref var lastLine = ref lines[lines.Length - 1];
            if (lines.Length == 1)
            {
                context.Caret.X += lastLine.Size.X;
            }
            else
            {
                context.Caret.X = lastLine.Size.X;
                context.Caret.Y += lastLine.Location.Y;
            }
        }
    }
}
