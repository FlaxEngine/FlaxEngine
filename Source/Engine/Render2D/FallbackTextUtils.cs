
namespace FlaxEngine
{
    /// <summary>
    /// A collection of functions to handle text rendering with fallback font
    /// </summary>
    public static class FallbackTextUtils
    {
        public static FallbackFonts Fallbacks
        {
            get; set;
        } = null;

        public static void DrawText(Font font, string text, Rectangle layoutRect, Color color, TextAlignment horizontalAlignment = TextAlignment.Near, TextAlignment verticalAlignment = TextAlignment.Near, TextWrapping textWrapping = TextWrapping.NoWrap, float baseLinesGapScale = 1.0f, float scale = 1.0f)
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

            if (Fallbacks != null)
            {
                Render2D.DrawText(font, Fallbacks, text, color, ref layout);
            }
            else
            {
                Render2D.DrawText(font, text, color, ref layout);
            }
        }

        public static void DrawText(Font font, MaterialBase customMaterial, string text, Rectangle layoutRect, Color color, TextAlignment horizontalAlignment = TextAlignment.Near, TextAlignment verticalAlignment = TextAlignment.Near, TextWrapping textWrapping = TextWrapping.NoWrap, float baseLinesGapScale = 1.0f, float scale = 1.0f)
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

            if (Fallbacks != null)
            {
                Render2D.DrawText(font, Fallbacks, text, color, ref layout, customMaterial);
            }
            else
            {
                Render2D.DrawText(font, text, color, ref layout, customMaterial);
            }
        }

        public static Float2 MeasureText(Font font, string text)
        {
            if (Fallbacks != null)
            {
                return font.MeasureText(Fallbacks, text);
            }
            else
            {
                return font.MeasureText(text);
            }
        }

        public static Float2 MeasureText(Font font, string text, ref TextRange textRange)
        {
            if (Fallbacks != null)
            {
                return font.MeasureText(Fallbacks, text, ref textRange);
            }
            else
            {
                return font.MeasureText(text, ref textRange);
            }
        }

        public static Float2 MeasureText(Font font, string text, ref TextLayoutOptions layout)
        {
            if (Fallbacks != null)
            {
                return font.MeasureText(Fallbacks, text, ref layout);
            }
            else
            {
                return font.MeasureText(text, ref layout);
            }
        }

        public static Float2 MeasureText(Font font, string text, ref TextRange textRange, ref TextLayoutOptions layout)
        {
            if (Fallbacks != null)
            {
                return font.MeasureText(Fallbacks, text, ref textRange, ref layout);
            }
            else
            {
                return font.MeasureText(text, ref textRange, ref layout);
            }
        }

        public static Float2 GetCharPosition(Font font, string text, int index)
        {
            return font.GetCharPosition(Style.Current.Fallbacks, text, index);
        }

        public static Float2 GetCharPosition(Font font, string text, ref TextRange textRange, int index)
        {
            return font.GetCharPosition(Style.Current.Fallbacks, text, ref textRange, index);
        }

        public static Float2 GetCharPosition(Font font, string text, int index, ref TextLayoutOptions layout)
        {
            return font.GetCharPosition(Style.Current.Fallbacks, text, index, ref layout);
        }

        public static Float2 GetCharPosition(Font font, string text, ref TextRange textRange, int index, ref TextLayoutOptions layout)
        {
            return font.GetCharPosition(Style.Current.Fallbacks, text, ref textRange, index, ref layout);
        }
    }
}
