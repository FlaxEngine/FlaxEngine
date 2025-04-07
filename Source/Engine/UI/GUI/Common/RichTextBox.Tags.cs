// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine.Utilities;

namespace FlaxEngine.GUI
{
    partial class RichTextBox
    {
        private static void ProcessBr(ref ParsingContext context, ref HtmlTag tag)
        {
            context.Caret.X = 0;
            var style = context.StyleStack.Peek();
            var font = style.Font.GetFont();
            if (font)
                context.Caret.Y += font.Height;
        }

        private static void ProcessColor(ref ParsingContext context, ref HtmlTag tag)
        {
            if (tag.IsSlash)
            {
                context.StyleStack.Pop();
            }
            else
            {
                var style = context.StyleStack.Peek();
                if (tag.Attributes.TryGetValue(string.Empty, out var colorText))
                {
                    if (Color.TryParse(colorText, out var color))
                    {
                        style.Color = color;
                    }
                }
                context.StyleStack.Push(style);
            }
        }

        private static void ProcessAlpha(ref ParsingContext context, ref HtmlTag tag)
        {
            if (tag.IsSlash)
            {
                context.StyleStack.Pop();
            }
            else
            {
                var style = context.StyleStack.Peek();
                if (tag.Attributes.TryGetValue(string.Empty, out var alphaText))
                {
                    if (alphaText.Length == 3 && alphaText[0] == '#')
                        style.Color.A = ((StringUtils.HexDigit(alphaText[1]) << 4) + StringUtils.HexDigit(alphaText[2])) / 255.0f;
                    else if (alphaText.Length > 1 && alphaText[alphaText.Length - 1] == '%' && float.TryParse(alphaText.Substring(0, alphaText.Length - 1), out var alpha))
                        style.Color.A = alpha / 100.0f;
                }
                context.StyleStack.Push(style);
            }
        }

        private static void ProcessStyle(ref ParsingContext context, ref HtmlTag tag)
        {
            if (tag.IsSlash)
            {
                context.StyleStack.Pop();
            }
            else
            {
                var style = context.StyleStack.Peek();
                if (tag.Attributes.TryGetValue(string.Empty, out var styleName))
                {
                    if (context.Control.Styles.TryGetValue(styleName, out var customStyle))
                    {
                        if (customStyle.Font == null)
                            customStyle.Font = style.Font;
                        style = customStyle;
                    }
                }
                context.StyleStack.Push(style);
            }
        }

        private static void ProcessFont(ref ParsingContext context, ref HtmlTag tag)
        {
            if (tag.IsSlash)
            {
                context.StyleStack.Pop();
            }
            else
            {
                var style = context.StyleStack.Peek();
                style.Font = new FontReference(style.Font);
                if (tag.Attributes.TryGetValue(string.Empty, out var fontName))
                {
                    var font = (FontAsset)FindAsset(fontName, typeof(FontAsset));
                    if (font)
                        style.Font.Font = font;
                }
                if (tag.Attributes.TryGetValue("size", out var sizeText) && int.TryParse(sizeText, out var size))
                    style.Font.Size = size;
                context.StyleStack.Push(style);
            }
        }

        private static void ProcessBold(ref ParsingContext context, ref HtmlTag tag)
        {
            if (tag.IsSlash)
            {
                context.StyleStack.Pop();
            }
            else
            {
                var style = context.StyleStack.Peek();
                style.Font = style.Font.GetBold();
                context.StyleStack.Push(style);
            }
        }

        private static void ProcessItalic(ref ParsingContext context, ref HtmlTag tag)
        {
            if (tag.IsSlash)
            {
                context.StyleStack.Pop();
            }
            else
            {
                var style = context.StyleStack.Peek();
                style.Font = style.Font.GetItalic();
                context.StyleStack.Push(style);
            }
        }

        private static void ProcessSize(ref ParsingContext context, ref HtmlTag tag)
        {
            if (tag.IsSlash)
            {
                context.StyleStack.Pop();
            }
            else
            {
                var style = context.StyleStack.Peek();
                style.Font = new FontReference(style.Font);
                TryParseNumberTag(ref tag, string.Empty, style.Font.Size, out var size);
                style.Font.Size = (int)size;
                context.StyleStack.Push(style);
            }
        }

        private static void ProcessImage(ref ParsingContext context, ref HtmlTag tag)
        {
            // Get image brush
            IBrush image = null;
            if (tag.Attributes.TryGetValue(string.Empty, out var srcText) || tag.Attributes.TryGetValue("src", out srcText))
            {
                if (!context.Control.Images.TryGetValue(srcText, out image))
                {
                    var tex = (Texture)FindAsset(srcText, typeof(Texture));
                    if (tex)
                        image = new TextureBrush(tex);
                }
            }
            if (image == null)
                return;

            // Create image block
            var imageBlock = new TextBlock
            {
                Range = new TextRange
                {
                    StartIndex = tag.StartPosition,
                    EndIndex = tag.StartPosition - 1,
                },
                Style = context.StyleStack.Peek(),
                Bounds = new Rectangle(context.Caret, new Float2(64.0f)),
            };
            imageBlock.Style.BackgroundBrush = image;

            // Setup size
            var font = imageBlock.Style.Font.GetFont();
            if (font)
                imageBlock.Bounds.Size = new Float2(font.Height);
            var imageSize = image.Size;
            imageBlock.Bounds.Size.X *= imageSize.X / imageSize.Y; // Keep original aspect ratio
            bool hasWidth = TryParseNumberTag(ref tag, "width", imageBlock.Bounds.Width, out var width);
            imageBlock.Bounds.Width = width;
            bool hasHeight = TryParseNumberTag(ref tag, "height", imageBlock.Bounds.Height, out var height);
            imageBlock.Bounds.Height = height;
            if ((hasHeight || hasWidth) && (hasWidth != hasHeight))
            {
                // Maintain aspect ratio after scaling by just width or height
                if (hasHeight)
                    imageBlock.Bounds.Size.X = imageBlock.Bounds.Size.Y * imageSize.X / imageSize.Y;
                else
                    imageBlock.Bounds.Size.Y = imageBlock.Bounds.Size.X * imageSize.Y / imageSize.X;
            }
            TryParseNumberTag(ref tag, "scale", 1.0f, out var scale);
            imageBlock.Bounds.Size *= scale;

            // Image height defines the ascender so it's placed on the baseline by default
            imageBlock.Ascender = imageBlock.Bounds.Size.Y;

            context.AddTextBlock(ref imageBlock);
            context.Caret.X += imageBlock.Bounds.Size.X;
        }

        private static void ProcessVAlign(ref ParsingContext context, ref HtmlTag tag)
        {
            if (tag.IsSlash)
            {
                context.StyleStack.Pop();
            }
            else
            {
                var style = context.StyleStack.Peek();
                if (tag.Attributes.TryGetValue(string.Empty, out var valign))
                {
                    style.Alignment &= ~TextBlockStyle.Alignments.VerticalMask;
                    switch (valign)
                    {
                    case "top":
                        style.Alignment = TextBlockStyle.Alignments.Top;
                        break;
                    case "bottom":
                        style.Alignment = TextBlockStyle.Alignments.Bottom;
                        break;
                    case "middle":
                        style.Alignment = TextBlockStyle.Alignments.Middle;
                        break;
                    case "baseline":
                        style.Alignment = TextBlockStyle.Alignments.Baseline;
                        break;
                    }
                }
                context.StyleStack.Push(style);
            }
        }

        private static void ProcessAlign(ref ParsingContext context, ref HtmlTag tag)
        {
            if (tag.IsSlash)
            {
                context.StyleStack.Pop();
            }
            else
            {
                var style = context.StyleStack.Peek();
                if (tag.Attributes.TryGetValue(string.Empty, out var valign))
                {
                    style.Alignment &= ~TextBlockStyle.Alignments.VerticalMask;
                    switch (valign)
                    {
                    case "left":
                        style.Alignment = TextBlockStyle.Alignments.Left;
                        break;
                    case "right":
                        style.Alignment = TextBlockStyle.Alignments.Right;
                        break;
                    case "center":
                        style.Alignment = TextBlockStyle.Alignments.Center;
                        break;
                    }
                }
                context.StyleStack.Push(style);
            }
        }

        private static void ProcessCenter(ref ParsingContext context, ref HtmlTag tag)
        {
            if (tag.IsSlash)
            {
                context.StyleStack.Pop();
            }
            else
            {
                var style = context.StyleStack.Peek();
                style.Alignment = TextBlockStyle.Alignments.Center;
                context.StyleStack.Push(style);
            }
        }

        private static Asset FindAsset(string name, System.Type type)
        {
            var ids = Content.GetAllAssetsByType(type);
            foreach (var id in ids)
            {
                var path = Content.GetEditorAssetPath(id);
                if (!string.IsNullOrEmpty(path) &&
                    string.Equals(name, System.IO.Path.GetFileNameWithoutExtension(path), System.StringComparison.OrdinalIgnoreCase))
                {
                    return Content.LoadAsync(id, type);
                }
            }
            return null;
        }

        private static bool TryParseNumberTag(ref HtmlTag tag, string name, float input, out float output)
        {
            output = input;
            bool used = false;
            if (tag.Attributes.TryGetValue(name, out var text))
            {
                if (float.TryParse(text, out var width))
                    output = width;
                if (text.Length > 1 && text[text.Length - 1] == '%' && float.TryParse(text.Substring(0, text.Length - 1), out width))
                    output = input * width / 100.0f;
                used = true;
            }
            return used;
        }
    }
}
