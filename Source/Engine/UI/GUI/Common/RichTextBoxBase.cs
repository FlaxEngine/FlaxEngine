// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.Collections.Generic;

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
        /// <param name="textBlocks">The output text blocks. Given list is not-nul and cleared before.</param>
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
            var textBlocks = Utils.ExtractArrayFromList(_textBlocks);
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
        /// Updates the text blocks.
        /// </summary>
        protected virtual void UpdateTextBlocks()
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
        public override Vector2 GetTextSize()
        {
            var count = _textBlocks.Count;
            var textBlocks = Utils.ExtractArrayFromList(_textBlocks);
            var max = Vector2.Zero;
            for (int i = 0; i < count; i++)
            {
                ref TextBlock textBlock = ref textBlocks[i];
                max = Vector2.Max(max, textBlock.Bounds.BottomRight);
            }
            return max;
        }

        /// <inheritdoc />
        public override Vector2 GetCharPosition(int index, out float height)
        {
            var count = _textBlocks.Count;
            var textBlocks = Utils.ExtractArrayFromList(_textBlocks);

            // Check if text is empty
            if (count == 0)
            {
                height = 0;
                return Vector2.Zero;
            }

            // Check if get first character position
            if (index <= 0)
            {
                ref TextBlock textBlock = ref textBlocks[0];
                var font = textBlock.Style.Font.GetFont();
                if (font)
                {
                    height = font.Height / Platform.DpiScale;
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
                    height = font.Height / Platform.DpiScale;
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
                    height = font.Height / Platform.DpiScale;
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
                    height = font.Height / Platform.DpiScale;
                    return textBlock.Bounds.UpperRight;
                }
            }

            height = 0;
            return Vector2.Zero;
        }

        /// <inheritdoc />
        public override int HitTestText(Vector2 location)
        {
            location = Vector2.Clamp(location, Vector2.Zero, _textSize);

            var textBlocks = Utils.ExtractArrayFromList(_textBlocks);
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
        public override bool OnMouseDoubleClick(Vector2 location, MouseButton button)
        {
            // Select the word under the mouse
            int textLength = TextLength;
            if (textLength != 0)
            {
                var hitPos = CharIndexAtPoint(ref location);
                int spaceLoc = _text.LastIndexOfAny(Separators, hitPos - 2);
                var left = spaceLoc == -1 ? 0 : spaceLoc + 1;
                spaceLoc = _text.IndexOfAny(Separators, hitPos + 1);
                var right = spaceLoc == -1 ? textLength : spaceLoc;
                SetSelection(left, right);
            }

            return base.OnMouseDoubleClick(location, button);
        }

        /// <inheritdoc />
        public override void Draw()
        {
            // Cache data
            var rect = new Rectangle(Vector2.Zero, Size);
            bool enabled = EnabledInHierarchy;

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

            // Calculate text blocks for drawing
            var textBlocks = Utils.ExtractArrayFromList(_textBlocks);
            var hasSelection = HasSelection;
            var selection = new TextRange(SelectionLeft, SelectionRight);
            var viewRect = new Rectangle(_viewOffset, Size).MakeExpanded(10.0f);
            var firstTextBlock = _textBlocks.Count;
            for (int i = 0; i < _textBlocks.Count; i++)
            {
                ref TextBlock textBlock = ref textBlocks[i];
                if (textBlock.Bounds.Intersects(ref viewRect))
                {
                    firstTextBlock = i;
                    break;
                }
            }
            var endTextBlock = Mathf.Min(firstTextBlock + 1, textBlocks.Length);
            for (int i = _textBlocks.Count - 1; i > firstTextBlock; i--)
            {
                ref TextBlock textBlock = ref textBlocks[i];
                if (textBlock.Bounds.Intersects(ref viewRect))
                {
                    endTextBlock = i + 1;
                    break;
                }
            }

            // Draw selection background
            for (int i = firstTextBlock; i < endTextBlock; i++)
            {
                ref TextBlock textBlock = ref textBlocks[i];

                // Pick font
                var font = textBlock.Style.Font.GetFont();
                if (!font)
                    continue;

                // Selection
                if (hasSelection && textBlock.Style.BackgroundSelectedBrush != null && textBlock.Range.Intersect(ref selection))
                {
                    Vector2 leftEdge = selection.StartIndex <= textBlock.Range.StartIndex ? textBlock.Bounds.UpperLeft : font.GetCharPosition(_text, selection.StartIndex);
                    Vector2 rightEdge = selection.EndIndex >= textBlock.Range.EndIndex ? textBlock.Bounds.UpperRight : font.GetCharPosition(_text, selection.EndIndex);
                    float height = font.Height / Platform.DpiScale;
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
                    var height = font.Height / Platform.DpiScale;
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
