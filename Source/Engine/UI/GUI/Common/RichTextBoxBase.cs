// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Base class for all rich text box controls which can gather text input from the user and present text in highly formatted and stylized way.
    /// </summary>
    public abstract class RichTextBoxBase : TextBoxBase
    {
        /// <summary>
        /// The delegate for text blocks processing.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <param name="textBlocks">The output text blocks. Given list is not-null and cleared before.</param>
        public delegate void ParseTextBlocksDelegate(string text, List<TextBlock> textBlocks);

        /// <summary>
        /// The text blocks.
        /// </summary>
        protected List<TextBlock> _textBlocks = new List<TextBlock>();

        /// <summary>
        /// The custom callback for parsing text blocks.
        /// </summary>
        [HideInEditor]
        public ParseTextBlocksDelegate ParseTextBlocks;

        /// <summary>
        /// Initializes a new instance of the <see cref="RichTextBoxBase"/> class.
        /// </summary>
        protected RichTextBoxBase()
        {
            IsMultiline = true;
        }

        /// <summary>
        /// Gets the text block of the character at the given index.
        /// </summary>
        /// <param name="index">The character index.</param>
        /// <param name="result">The result text block descriptor.</param>
        /// <returns>True if got text block, otherwise false.</returns>
        public bool GetTextBlock(int index, out TextBlock result)
        {
            var textBlocks = CollectionsMarshal.AsSpan(_textBlocks);
            var count = _textBlocks.Count;
            for (int i = 0; i < count; i++)
            {
                ref TextBlock textBlock = ref textBlocks[i];
                if (textBlock.Range.Contains(index))
                {
                    result = textBlock;
                    return true;
                }
            }
            result = new TextBlock();
            return false;
        }

        /// <summary>
        /// Gets the text block or the nearest text block of the character at the given index.
        /// </summary>
        /// <param name="index">The character index.</param>
        /// <param name="result">The result text block descriptor.</param>
        /// <param name="snapToNext">If true, the when the index is between two text blocks, it will return the next block.</param>
        /// <returns>True if a text block is found, otherwise false.</returns>
        public bool GetNearestTextBlock(int index, out TextBlock result, bool snapToNext = false)
        {
            var textBlocksSpan = CollectionsMarshal.AsSpan(_textBlocks);
            int blockCount = _textBlocks.Count;

            for (int i = 0; i < blockCount; i++)
            {
                ref TextBlock currentBlock = ref textBlocksSpan[i];

                if (currentBlock.Range.Contains(index))
                {
                    result = currentBlock;
                    return true;
                }

                if (i < blockCount - 1)
                {
                    ref TextBlock nextBlock = ref textBlocksSpan[i + 1];
                    if (index >= currentBlock.Range.EndIndex && index < nextBlock.Range.StartIndex)
                    {
                        result = snapToNext ? nextBlock : currentBlock;
                        return true;
                    }
                }
            }

            // Handle case when index is outside all text ranges
            if (index >= 0 && blockCount > 0)
            {
                result = (index <= textBlocksSpan[0].Range.StartIndex) ? textBlocksSpan[0] : textBlocksSpan[blockCount - 1];
                return true;
            }

            // If no text block is found
            result = new TextBlock();
            return false;
        }

        /// <summary>
        /// Updates the text blocks.
        /// </summary>
        public virtual void UpdateTextBlocks()
        {
            Profiler.BeginEvent("RichTextBoxBase.UpdateTextBlocks");

            _textBlocks.Clear();
            if (_text.Length != 0)
            {
                OnParseTextBlocks();
            }

            Profiler.EndEvent();
        }

        /// <summary>
        /// Called when text blocks needs to be updated from the current text.
        /// </summary>
        protected virtual void OnParseTextBlocks()
        {
            ParseTextBlocks?.Invoke(_text, _textBlocks);
        }

        /// <inheritdoc />
        protected override void OnTextChanged()
        {
            UpdateTextBlocks();

            base.OnTextChanged();
        }

        /// <inheritdoc />
        public override Float2 GetTextSize()
        {
            var count = _textBlocks.Count;
            var textBlocks = CollectionsMarshal.AsSpan(_textBlocks);
            var max = Float2.Zero;
            for (int i = 0; i < count; i++)
            {
                ref TextBlock textBlock = ref textBlocks[i];
                max = Float2.Max(max, textBlock.Bounds.BottomRight);
            }
            return max;
        }

        /// <inheritdoc />
        public override Float2 GetCharPosition(int index, out float height)
        {
            var count = _textBlocks.Count;
            var textBlocks = CollectionsMarshal.AsSpan(_textBlocks);

            // Check if text is empty
            if (count == 0)
            {
                height = 0;
                return Float2.Zero;
            }

            // Check if get first character position
            if (index <= 0)
            {
                ref TextBlock textBlock = ref textBlocks[0];
                var font = textBlock.Style.Font.GetFont();
                if (font)
                {
                    height = font.Height / DpiScale;
                    return textBlock.Bounds.UpperLeft;
                }
            }

            // Check if get last character position
            if (index >= _text.Length)
            {
                ref TextBlock textBlock = ref textBlocks[count - 1];
                var font = textBlock.Style.Font.GetFont();
                if (font)
                {
                    height = font.Height / DpiScale;
                    return textBlock.Bounds.UpperRight;
                }
            }

            // Test any text block
            for (int i = 0; i < count; i++)
            {
                ref TextBlock textBlock = ref textBlocks[i];

                if (textBlock.Range.Contains(index))
                {
                    var font = textBlock.Style.Font.GetFont();
                    if (!font)
                        break;
                    height = font.Height / DpiScale;
                    return textBlock.Bounds.Location + font.GetCharPosition(_text, ref textBlock.Range, index - textBlock.Range.StartIndex);
                }
            }

            // Test any character between block that was not included in any of them (eg. newline character)
            for (int i = count - 1; i >= 0; i--)
            {
                ref TextBlock textBlock = ref textBlocks[i];

                if (index >= textBlock.Range.EndIndex)
                {
                    var font = textBlock.Style.Font.GetFont();
                    if (!font)
                        break;
                    height = font.Height / DpiScale;
                    return textBlock.Bounds.UpperRight;
                }
            }

            height = 0;
            return Float2.Zero;
        }

        /// <inheritdoc />
        public override int HitTestText(Float2 location)
        {
            location = Float2.Clamp(location, Float2.Zero, _textSize);

            var textBlocks = CollectionsMarshal.AsSpan(_textBlocks);
            var count = _textBlocks.Count;
            for (int i = 0; i < count; i++)
            {
                ref TextBlock textBlock = ref textBlocks[i];

                var containsX = location.X >= textBlock.Bounds.Location.X && location.X < textBlock.Bounds.Location.X + textBlock.Bounds.Size.X;
                var containsY = location.Y >= textBlock.Bounds.Location.Y && location.Y < textBlock.Bounds.Location.Y + textBlock.Bounds.Size.Y;

                if (containsY && (containsX || (i + 1 < count && textBlocks[i + 1].Bounds.Location.Y > textBlock.Bounds.Location.Y + 1.0f)))
                {
                    var font = textBlock.Style.Font.GetFont();
                    if (!font && textBlock.Range.Length > 0)
                        break;
                    return font.HitTestText(_text, ref textBlock.Range, location - textBlock.Bounds.Location) + textBlock.Range.StartIndex;
                }
            }

            return _text.Length;
        }

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            // Select the word under the mouse
            int textLength = TextLength;
            if (textLength != 0 && IsSelectable)
            {
                var hitPos = CharIndexAtPoint(ref location);
                int spaceLoc = _text.LastIndexOfAny(Separators, hitPos - 2);
                var left = spaceLoc == -1 ? 0 : spaceLoc + 1;
                spaceLoc = _text.IndexOfAny(Separators, Math.Min(hitPos + 1, _text.Length));
                var right = spaceLoc == -1 ? textLength : spaceLoc;
                SetSelection(left, right);
            }

            return base.OnMouseDoubleClick(location, button);
        }

        /// <inheritdoc />
        protected override void SetSelection(int start, int end, bool withScroll = true)
        {
            int snappedStart = start;
            int snappedEnd = end;

            if (start != -1 && end != -1)
            {
                bool movingBack = (_selectionStart != -1 && _selectionEnd != -1) && (end < _selectionEnd || start < _selectionStart);

                GetNearestTextBlock(start, out TextBlock startTextBlock, !movingBack);
                GetNearestTextBlock(end, out TextBlock endTextBlock, !movingBack);

                snappedStart = startTextBlock.Range.Contains(start) ? start : (movingBack ? startTextBlock.Range.EndIndex - 1 : startTextBlock.Range.StartIndex);
                snappedEnd = endTextBlock.Range.Contains(end) ? end : (movingBack ? endTextBlock.Range.EndIndex - 1 : endTextBlock.Range.StartIndex);

                snappedStart = movingBack ? Math.Min(start, snappedStart) : Math.Max(start, snappedStart);
                snappedEnd = movingBack ? Math.Min(end, snappedEnd) : Math.Max(end, snappedEnd);

                // Don't snap if selection is right in the end of the text
                if (start == _text.Length)
                    snappedStart = _text.Length;
                if (end == _text.Length)
                    snappedEnd = _text.Length;
            }

            base.SetSelection(snappedStart, snappedEnd, withScroll);
        }

        /// <inheritdoc />
        public override void DrawSelf()
        {
            // Cache data
            var rect = new Rectangle(Float2.Zero, Size);
            bool enabled = EnabledInHierarchy;

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

            // Calculate text blocks for drawing
            var textBlocks = CollectionsMarshal.AsSpan(_textBlocks);
            var textBlocksCount = _textBlocks?.Count ?? 0;
            var hasSelection = HasSelection;
            var selection = new TextRange(SelectionLeft, SelectionRight);
            var viewRect = new Rectangle(_viewOffset, Size).MakeExpanded(10.0f);
            var firstTextBlock = textBlocksCount;
            for (int i = 0; i < textBlocksCount; i++)
            {
                ref TextBlock textBlock = ref textBlocks[i];
                if (textBlock.Bounds.Intersects(ref viewRect))
                {
                    firstTextBlock = i;
                    break;
                }
            }
            var endTextBlock = Mathf.Min(firstTextBlock + 1, textBlocksCount);
            for (int i = textBlocksCount - 1; i > firstTextBlock; i--)
            {
                ref TextBlock textBlock = ref textBlocks[i];
                if (textBlock.Bounds.Intersects(ref viewRect))
                {
                    endTextBlock = i + 1;
                    break;
                }
            }

            // Draw background
            for (int i = firstTextBlock; i < endTextBlock; i++)
            {
                ref TextBlock textBlock = ref textBlocks[i];

                // Background
                if (textBlock.Style.BackgroundBrush != null)
                {
                    textBlock.Style.BackgroundBrush.Draw(textBlock.Bounds, textBlock.Style.Color);
                }

                // Pick font
                var font = textBlock.Style.Font.GetFont();
                if (!font)
                    continue;

                // Selection
                if (hasSelection && textBlock.Style.BackgroundSelectedBrush != null && textBlock.Range.Intersect(ref selection))
                {
                    var leftEdge = selection.StartIndex <= textBlock.Range.StartIndex ? textBlock.Bounds.UpperLeft : GetCharPosition(selection.StartIndex, out _);
                    var rightEdge = selection.EndIndex >= textBlock.Range.EndIndex ? textBlock.Bounds.UpperRight : GetCharPosition(selection.EndIndex, out _);
                    float height = font.Height;
                    float alpha = Mathf.Min(1.0f, Mathf.Cos(_animateTime * BackgroundSelectedFlashSpeed) * 0.5f + 1.3f);
                    alpha *= alpha;
                    Color selectionColor = Color.White * alpha;
                    Rectangle selectionRect = new Rectangle(leftEdge.X, leftEdge.Y, rightEdge.X - leftEdge.X, height);
                    textBlock.Style.BackgroundSelectedBrush.Draw(selectionRect, selectionColor);
                }
            }

            // Draw text
            for (int i = firstTextBlock; i < endTextBlock; i++)
            {
                ref TextBlock textBlock = ref textBlocks[i];

                // Pick font
                var font = textBlock.Style.Font.GetFont();
                if (!font)
                    continue;

                // Shadow
                Color color;
                if (!textBlock.Style.ShadowOffset.IsZero && textBlock.Style.ShadowColor != Color.Transparent)
                {
                    color = textBlock.Style.ShadowColor;
                    if (!enabled)
                        color *= 0.6f;
                    Render2D.DrawText(font, _text, ref textBlock.Range, color, textBlock.Bounds.Location + textBlock.Style.ShadowOffset, textBlock.Style.CustomMaterial);
                }

                // Text
                color = textBlock.Style.Color;
                if (!enabled)
                    color *= 0.6f;
                Render2D.DrawText(font, _text, ref textBlock.Range, color, textBlock.Bounds.Location, textBlock.Style.CustomMaterial);
            }

            // Draw underline
            for (int i = firstTextBlock; i < endTextBlock; i++)
            {
                ref TextBlock textBlock = ref textBlocks[i];

                // Pick font
                var font = textBlock.Style.Font.GetFont();
                if (!font)
                    continue;

                // Underline
                if (textBlock.Style.UnderlineBrush != null)
                {
                    var underLineHeight = 2.0f;
                    var height = font.Height;
                    var underlineRect = new Rectangle(textBlock.Bounds.Location.X, textBlock.Bounds.Location.Y + height - underLineHeight * 0.5f, textBlock.Bounds.Width, underLineHeight);
                    textBlock.Style.UnderlineBrush.Draw(underlineRect, textBlock.Style.Color);
                }
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
        public override void OnDestroy()
        {
            _textBlocks.Clear();
            _textBlocks = null;

            base.OnDestroy();
        }
    }
}
