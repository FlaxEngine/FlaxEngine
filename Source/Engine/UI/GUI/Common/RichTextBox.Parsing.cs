// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.Runtime.InteropServices;
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
            /// Index of the current line start character.
            /// </summary>
            public int LineStartCharacterIndex;

            /// <summary>
            /// Index of the current line start text block.
            /// </summary>
            public int LineStartTextBlockIndex;

            /// <summary>
            /// Text styles stack (new tags push modified style and pop on tag end).
            /// </summary>
            public Stack<TextBlockStyle> StyleStack;

            /// <summary>
            /// Adds the text block to the control
            /// </summary>
            /// <param name="textBlock">The text block to add.</param>
            public void AddTextBlock(ref TextBlock textBlock)
            {
                // Post-processing
                Control.PostProcessBlock?.Invoke(ref this, ref textBlock);

                // Add to the text blocks
                Control._textBlocks.Add(textBlock);
            }
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
            { "font", ProcessFont },
            { "b", ProcessBold },
            { "i", ProcessItalic },
            { "size", ProcessSize },
            { "img", ProcessImage },
            { "valign", ProcessVAlign },
            { "align", ProcessAlign },
            { "center", ProcessCenter },
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
                Caret = Float2.Zero,
                LineStartCharacterIndex = 0,
                LineStartTextBlockIndex = 0,
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

            // Insert remaining text (and line)
            OnAddTextBlock(ref context, testStartPos, _text.Length);
            if (context.LineStartTextBlockIndex != _textBlocks.Count)
            {
                context.Caret.X = 0;
                OnLineAdded(ref context, _text.Length - 1);
            }

            // Organize lines vertically
            if (_textBlocks.Count != 0)
            {
                var lastBlock = _textBlocks[_textBlocks.Count - 1];

                // Get style (global or leftover from style stack or the last lime)
                var verticalAlignments = _textStyle.Alignment;
                if (context.StyleStack.Count > 1)
                    verticalAlignments = context.StyleStack.Peek().Alignment;
                else if ((lastBlock.Style.Alignment & TextBlockStyle.Alignments.VerticalMask) != TextBlockStyle.Alignments.Baseline)
                    verticalAlignments = lastBlock.Style.Alignment;

                var totalSize = lastBlock.Bounds.BottomRight;
                var sizeOffset = Size - totalSize;
                var textBlocks = CollectionsMarshal.AsSpan(_textBlocks);

                switch (verticalAlignments)
                {
                    case TextBlockStyle.Alignments.Middle:
                    {
                        sizeOffset.Y *= 0.5f;
                        for (int i = 0; i < _textBlocks.Count; i++)
                        {
                            ref TextBlock textBlock = ref textBlocks[i];
                            textBlock.Bounds.Location.Y += sizeOffset.Y;
                        }
                        break;
                    }

                    case TextBlockStyle.Alignments.Bottom:
                    {
                        for (int i = 0; i < _textBlocks.Count; i++)
                        {
                            ref TextBlock textBlock = ref textBlocks[i];
                            textBlock.Bounds.Location.Y += sizeOffset.Y;
                        }
                        break;
                    }
                }
            }
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
            var lines = font.ProcessText(_text, ref textBlock.Range);
            
            if (!font || (lines == null || lines.Length == 0))
                return;

            var wrapWidth = Wrapping != TextWrapping.NoWrap ? Width : -1.0f;
            
            for (int i = 0; i < lines.Length; i++)
            {
                ref var line = ref lines[i];

                var lineRange = new TextRange
                {
                    StartIndex = start + line.FirstCharIndex,
                    EndIndex = start + line.LastCharIndex + 1,
                };

                if (i != 0)
                {
                    context.Caret.X = 0;
                    OnLineAdded(ref context, lineRange.StartIndex - 1);
                }

                if (wrapWidth > 0 && context.Caret.X + line.Size.X > wrapWidth)
                {
                    // Line overflows the available width - split it into multiple wrapped lines
                    AddWrappedTextBlocks(ref context, ref textBlock, font, lineRange, wrapWidth, Wrapping);
                }
                else
                {
                    textBlock.Range = lineRange;
                    textBlock.Bounds = new Rectangle(context.Caret, line.Size);
                    textBlock.Bounds.X += line.Location.X;
                    context.AddTextBlock(ref textBlock);
                    context.Caret.X += line.Size.X;
                }
            }
        }

        /// <summary>
        /// Splits the given text range into multiple text blocks (lines) so it fits within the available wrapping width, correctly continuing from the current caret position (eg. when a differently-styled text run follows on the same line).
        /// </summary>
        /// <param name="context">The parsing context.</param>
        /// <param name="textBlock">The template text block (style is reused, range and bounds get overriden per produced block).</param>
        /// <param name="font">The font used to measure and render the text.</param>
        /// <param name="range">The text range to wrap (single logical line - no explicit newlines inside).</param>
        /// <param name="wrapWidth">The maximum available width (in control-space) that a single visual line can use.</param>
        /// <param name="wrapping">The wrapping mode - either whole-word (breaks only at whitespace) or per-character (can break in the middle of a word).</param>
        private void AddWrappedTextBlocks(ref ParsingContext context, ref TextBlock textBlock, Font font, TextRange range, float wrapWidth, TextWrapping wrapping)
        {
            int segmentStart = range.StartIndex;
            float segmentWidth = 0.0f;
            int pos = range.StartIndex;
            while (pos < range.EndIndex)
            {
                // Consume the next "chunk" - a single character for WrapChars, or a whole word (plus trailing whitespace) for WrapWords
                int chunkStart = pos;
                if (wrapping == TextWrapping.WrapChars)
                {
                    pos++;
                }
                else
                {
                    while (pos < range.EndIndex && !char.IsWhiteSpace(_text[pos]))
                        pos++;
                    while (pos < range.EndIndex && char.IsWhiteSpace(_text[pos]))
                        pos++;
                }

                var chunkRange = new TextRange { StartIndex = chunkStart, EndIndex = pos };
                var chunkWidth = font.MeasureText(_text, ref chunkRange).X;

                if ((segmentWidth > 0.0f || context.Caret.X > 0.0f) && context.Caret.X + segmentWidth + chunkWidth > wrapWidth)
                {
                    // The next chunk no longer fits - emit the accumulated segment (if any) and start a new line
                    if (segmentStart < chunkStart)
                        AddWrappedTextBlock(ref context, ref textBlock, font, segmentStart, chunkStart);
                    context.Caret.X = 0;
                    OnLineAdded(ref context, chunkStart - 1);
                    segmentStart = chunkStart;
                    segmentWidth = 0.0f;
                }
                segmentWidth += chunkWidth;

                // For Wrap Words mode: A single word wider than the whole available width can't be split further, so force it onto its own line
                if (wrapping == TextWrapping.WrapWords && context.Caret.X <= 0.0f && segmentStart == chunkStart && segmentWidth > wrapWidth && pos < range.EndIndex)
                {
                    AddWrappedTextBlock(ref context, ref textBlock, font, segmentStart, pos);
                    context.Caret.X = 0;
                    OnLineAdded(ref context, pos - 1);
                    segmentStart = pos;
                    segmentWidth = 0.0f;
                }
            }
            if (segmentStart < range.EndIndex)
                AddWrappedTextBlock(ref context, ref textBlock, font, segmentStart, range.EndIndex);
        }

        /// <summary>
        /// Adds a single text block to the control, using the current caret position as the origin and moving the caret forward by the width of the text block.
        /// </summary>
        /// <param name="context">The parsing context.</param>
        /// <param name="textBlock">The text block to add.</param>
        /// <param name="font">The font to use for measurement.</param>
        /// <param name="start">The start index of the text range.</param>
        /// <param name="end">The end index of the text range.</param>
        private void AddWrappedTextBlock(ref ParsingContext context, ref TextBlock textBlock, Font font, int start, int end)
        {
            var range = new TextRange { StartIndex = start, EndIndex = end };
            var size = font.MeasureText(_text, ref range);
            textBlock.Range = range;
            textBlock.Bounds = new Rectangle(context.Caret, size);
            context.AddTextBlock(ref textBlock);
            context.Caret.X += size.X;
        }

        /// <summary>
        /// Called when a new line is added (eg. after a newline character or when the text overflows the available width). 
        /// It organizes the text blocks within the line and moves the caret to the next line.
        /// </summary>
        /// <param name="context">The parsing context.</param>
        /// <param name="lineEnd">The index of the last character in the line.</param>
        private void OnLineAdded(ref ParsingContext context, int lineEnd)
        {
            // Calculate size of the line
            var textBlocks = CollectionsMarshal.AsSpan(_textBlocks);
            var lineOrigin = textBlocks[context.LineStartTextBlockIndex].Bounds.Location;
            var lineSize = Float2.Zero;
            var lineAscender = 0.0f;

            for (int i = context.LineStartTextBlockIndex; i < _textBlocks.Count; i++)
            {
                ref TextBlock textBlock = ref textBlocks[i];
                var textBlockSize = textBlock.Bounds.BottomRight - lineOrigin;
                var ascender = textBlock.GetAscender();
                lineAscender = Mathf.Max(lineAscender, ascender);
                lineSize = Float2.Max(lineSize, textBlockSize);
            }

            // Organize text blocks within line
            var lineAlignments = TextBlockStyle.Alignments.Baseline;
            for (int i = context.LineStartTextBlockIndex; i < _textBlocks.Count; i++)
            {
                ref TextBlock textBlock = ref textBlocks[i];
                var vOffset = lineSize.Y - textBlock.Bounds.Height;

                lineAlignments = (i == context.LineStartTextBlockIndex) ? textBlock.Style.Alignment :
                    lineAlignments & textBlock.Style.Alignment;
                
                switch (textBlock.Style.Alignment & TextBlockStyle.Alignments.VerticalMask)
                {
                    case TextBlockStyle.Alignments.Baseline:
                    {
                        // Match the baseline of the line (use ascender)
                        var ascender = textBlock.GetAscender();
                        vOffset = lineAscender - ascender;
                        textBlock.Bounds.Location.Y += vOffset;
                        break;
                    }

                    case TextBlockStyle.Alignments.Top:
                    {
                        textBlock.Bounds.Location.Y = lineOrigin.Y;
                        break;
                    }

                    case TextBlockStyle.Alignments.Middle:
                    {
                        textBlock.Bounds.Location.Y = lineOrigin.Y + vOffset * 0.5f;
                        break;
                    }

                    case TextBlockStyle.Alignments.Bottom:
                    {
                        textBlock.Bounds.Location.Y = lineOrigin.Y + vOffset;
                        break;
                    }
                }
            }

            // Organize whole line horizontally
            var sizeOffset = Size - lineSize;

            switch (lineAlignments & TextBlockStyle.Alignments.HorizontalMask)
            {
                case TextBlockStyle.Alignments.Center:
                {
                    sizeOffset.X *= 0.5f;
                    for (int i = context.LineStartTextBlockIndex; i < _textBlocks.Count; i++)
                    {
                        ref TextBlock textBlock = ref textBlocks[i];
                        textBlock.Bounds.Location.X += sizeOffset.X;
                    }
                    break;
                }

                case TextBlockStyle.Alignments.Right:
                {
                    for (int i = context.LineStartTextBlockIndex; i < _textBlocks.Count; i++)
                    {
                        ref TextBlock textBlock = ref textBlocks[i];
                        textBlock.Bounds.Location.X += sizeOffset.X;
                    }
                    break;
                }
            }

            // Move to the next line
            context.LineStartCharacterIndex = lineEnd + 1;
            context.LineStartTextBlockIndex = _textBlocks.Count;
            context.Caret.Y += lineSize.Y * BaseLinesGapScale;
        }
    }
}
