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
    }
}
