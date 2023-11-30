using FlaxEngine.GUI;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace FlaxEngine
{
    public static class TextRenderUtils
    {
        /// <summary>
        /// Draws a text using the global fallback defined in styles.
        /// </summary>
        /// <param name="font">The font to use.</param>
        /// <param name="text">The text to render.</param>
        /// <param name="layoutRect">The size and position of the area in which the text is drawn.</param>
        /// <param name="color">The text color.</param>
        /// <param name="horizontalAlignment">The horizontal alignment of the text in a layout rectangle.</param>
        /// <param name="verticalAlignment">The vertical alignment of the text in a layout rectangle.</param>
        /// <param name="textWrapping">Describes how wrap text inside a layout rectangle.</param>
        /// <param name="baseLinesGapScale">The scale for distance one baseline from another. Default is 1.</param>
        /// <param name="scale">The text drawing scale. Default is 1.</param>
        public static void DrawTextWithFallback(Font font, string text, Rectangle layoutRect, Color color, TextAlignment horizontalAlignment = TextAlignment.Near, TextAlignment verticalAlignment = TextAlignment.Near, TextWrapping textWrapping = TextWrapping.NoWrap, float baseLinesGapScale = 1.0f, float scale = 1.0f)
        {
            var layout = new TextLayoutOptions
            {
                Bounds = layoutRect,
                HorizontalAlignment = horizontalAlignment,
                VerticalAlignment = verticalAlignment,
                TextWrapping = textWrapping,
                Scale = scale,
                BaseLinesGapScale = baseLinesGapScale,
            };


            Render2D.DrawText(font, Style.Current.Fallbacks, text, color, ref layout);
        }

        /// <summary>
        /// Draws a text using the global fallback defined in styles. Given material must have GUI domain and a public parameter named Font (texture parameter used for a font atlas sampling).
        /// </summary>
        /// <param name="font">The font to use.</param>
        /// <param name="customMaterial">Custom material for font characters rendering. It must contain texture parameter named Font used to sample font texture.</param>
        /// <param name="text">The text to render.</param>
        /// <param name="layoutRect">The size and position of the area in which the text is drawn.</param>
        /// <param name="color">The text color.</param>
        /// <param name="horizontalAlignment">The horizontal alignment of the text in a layout rectangle.</param>
        /// <param name="verticalAlignment">The vertical alignment of the text in a layout rectangle.</param>
        /// <param name="textWrapping">Describes how wrap text inside a layout rectangle.</param>
        /// <param name="baseLinesGapScale">The scale for distance one baseline from another. Default is 1.</param>
        /// <param name="scale">The text drawing scale. Default is 1.</param>
        public static void DrawTextWithFallback(Font font, MaterialBase customMaterial, string text, Rectangle layoutRect, Color color, TextAlignment horizontalAlignment = TextAlignment.Near, TextAlignment verticalAlignment = TextAlignment.Near, TextWrapping textWrapping = TextWrapping.NoWrap, float baseLinesGapScale = 1.0f, float scale = 1.0f)
        {
            var layout = new TextLayoutOptions
            {
                Bounds = layoutRect,
                HorizontalAlignment = horizontalAlignment,
                VerticalAlignment = verticalAlignment,
                TextWrapping = textWrapping,
                Scale = scale,
                BaseLinesGapScale = baseLinesGapScale,
            };

            Render2D.DrawText(font, Style.Current.Fallbacks, text, color, ref layout, customMaterial);
        }

        public static Float2 MeasureTextWithFallback(Font font, string text)
        {
            return font.MeasureText(Style.Current.Fallbacks, text);
        }

        public static Float2 MeasureTextWithFallback(Font font, string text, ref TextRange textRange)
        {
            return font.MeasureText(Style.Current.Fallbacks, text, ref textRange);
        }

        public static Float2 MeasureTextWithFallback(Font font, string text, ref TextLayoutOptions layout)
        {
            return font.MeasureText(Style.Current.Fallbacks, text, ref layout);
        }

        public static Float2 MeasureTextWithFallback(Font font, string text, ref TextRange textRange, ref TextLayoutOptions layout)
        {
            return font.MeasureText(Style.Current.Fallbacks, text, ref textRange, ref layout);
        }

        public static Float2 GetCharPositionWithFallback(Font font, string text, int index)
        {
            return font.GetCharPosition(Style.Current.Fallbacks, text, index);
        }

        public static Float2 GetCharPositionWithFallback(Font font, string text, ref TextRange textRange, int index)
        {
            return font.GetCharPosition(Style.Current.Fallbacks, text, ref textRange, index);
        }

        public static Float2 GetCharPositionWithFallback(Font font, string text, int index, ref TextLayoutOptions layout)
        {
            return font.GetCharPosition(Style.Current.Fallbacks, text, index, ref layout);
        }

        public static Float2 GetCharPositionWithFallback(Font font, string text, ref TextRange textRange, int index, ref TextLayoutOptions layout)
        {
            return font.GetCharPosition(Style.Current.Fallbacks, text, ref textRange, index, ref layout);
        }
    }
}
