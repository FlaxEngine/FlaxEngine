// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

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
                    else if (alphaText.Length > 1 && alphaText[alphaText.Length - 1] == '%')
                        style.Color.A = float.Parse(alphaText.Substring(0, alphaText.Length - 1)) / 100.0f;
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
                    EndIndex = tag.StartPosition,
                },
                Style = context.StyleStack.Peek(),
                Bounds = new Rectangle(context.Caret, new Float2(64.0f)),
            };
            imageBlock.Style.BackgroundBrush = image;

            // Setup size
            var font = imageBlock.Style.Font.GetFont();
            if (font)
                imageBlock.Bounds.Size = new Float2(font.Height);
            imageBlock.Bounds.Size.X *= image.Size.X / image.Size.Y; // Keep aspect ration
            TryParseNumberTag(ref tag, "width", imageBlock.Bounds.Width, out var width);
            imageBlock.Bounds.Width = width;
            TryParseNumberTag(ref tag, "height", imageBlock.Bounds.Height, out var height);
            imageBlock.Bounds.Height = height;
            TryParseNumberTag(ref tag, "scale", 1.0f, out var scale);
            imageBlock.Bounds.Size *= scale;
            
            context.AddTextBlock(ref imageBlock);
            context.Caret.X += imageBlock.Bounds.Size.X;
        }

        private static Asset FindAsset(string name, System.Type type)
        {
            var ids = Content.GetAllAssetsByType(type);
            foreach (var id in ids)
            {
                if (Content.GetAssetInfo(id, out var info) && string.Equals(name, System.IO.Path.GetFileNameWithoutExtension(info.Path), System.StringComparison.OrdinalIgnoreCase))
                {
                    return Content.LoadAsync(id, type);
                }
            }
            return null;
        }

        private static void TryParseNumberTag(ref HtmlTag tag, string name, float input, out float output)
        {
            output = input;
            if (tag.Attributes.TryGetValue(name, out var text))
            {
                if (float.TryParse(text, out var width))
                    output = width;
                if (text.Length > 1 && text[text.Length - 1] == '%')
                    output = input * float.Parse(text.Substring(0, text.Length - 1)) / 100.0f;
            }
        }
    }
}
