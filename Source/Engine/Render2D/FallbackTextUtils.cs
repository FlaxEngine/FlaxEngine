
using System.Runtime.CompilerServices;

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

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void DrawText(Font font, string text, Color color, ref TextLayoutOptions layout, MaterialBase customMaterial = null, bool useFallback = true)
        {
            if (Fallbacks != null && useFallback)
            {
                Render2D.DrawText(font, Fallbacks, text, color, ref layout, customMaterial);
            }
            else
            {
                Render2D.DrawText(font, text, color, ref layout, customMaterial);
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void DrawText(Font font, string text, Rectangle layoutRect, Color color, TextAlignment horizontalAlignment = TextAlignment.Near, TextAlignment verticalAlignment = TextAlignment.Near, TextWrapping textWrapping = TextWrapping.NoWrap, float baseLinesGapScale = 1.0f, float scale = 1.0f, bool useFallback = true)
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

            if (Fallbacks != null && useFallback)
            {
                Render2D.DrawText(font, Fallbacks, text, color, ref layout);
            }
            else
            {
                Render2D.DrawText(font, text, color, ref layout);
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void DrawText(Font font, MaterialBase customMaterial, string text, Rectangle layoutRect, Color color, TextAlignment horizontalAlignment = TextAlignment.Near, TextAlignment verticalAlignment = TextAlignment.Near, TextWrapping textWrapping = TextWrapping.NoWrap, float baseLinesGapScale = 1.0f, float scale = 1.0f, bool useFallback = true)
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

            if (Fallbacks != null && useFallback)
            {
                Render2D.DrawText(font, Fallbacks, text, color, ref layout, customMaterial);
            }
            else
            {
                Render2D.DrawText(font, text, color, ref layout, customMaterial);
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Float2 MeasureText(Font font, string text, bool useFallback = true)
        {
            if (Fallbacks != null && useFallback)
            {
                return font.MeasureText(Fallbacks, text);
            }
            else
            {
                return font.MeasureText(text);
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Float2 MeasureText(Font font, string text, ref TextRange textRange, bool useFallback = true)
        {
            if (Fallbacks != null && useFallback)
            {
                return font.MeasureText(Fallbacks, text, ref textRange);
            }
            else
            {
                return font.MeasureText(text, ref textRange);
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Float2 MeasureText(Font font, string text, ref TextLayoutOptions layout, bool useFallback = true)
        {
            if (Fallbacks != null && useFallback)
            {
                return font.MeasureText(Fallbacks, text, ref layout);
            }
            else
            {
                return font.MeasureText(text, ref layout);
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Float2 MeasureText(Font font, string text, ref TextRange textRange, ref TextLayoutOptions layout, bool useFallback = true)
        {
            if (Fallbacks != null && useFallback)
            {
                return font.MeasureText(Fallbacks, text, ref textRange, ref layout);
            }
            else
            {
                return font.MeasureText(text, ref textRange, ref layout);
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static int HitTestText(Font font, string text, Float2 location, bool useFallback = true)
        {
            if (Fallbacks != null && useFallback)
            {
                return font.HitTestText(Fallbacks, text, location);
            }
            else
            {
                return font.HitTestText(text, location);
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static int HitTestText(Font font, string text, ref TextRange textRange, Float2 location, bool useFallback = true)
        {
            if (Fallbacks != null && useFallback)
            {
                return font.HitTestText(Fallbacks, text, ref textRange, location);
            }
            else
            {
                return font.HitTestText(text, ref textRange, location);
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static int HitTestText(Font font, string text, Float2 location, ref TextLayoutOptions layout, bool useFallback = true)
        {
            if (Fallbacks != null && useFallback)
            {
                return font.HitTestText(Fallbacks, text, location, ref layout);
            }
            else
            {
                return font.HitTestText(text, location, ref layout);
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static int HitTestText(Font font, string text, ref TextRange textRange, Float2 location, ref TextLayoutOptions layout, bool useFallback = true)
        {
            if (Fallbacks != null && useFallback)
            {
                return font.HitTestText(Fallbacks, text, ref textRange, location, ref layout);
            }
            else
            {
                return font.HitTestText(text, ref textRange, location, ref layout);
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Float2 GetCharPosition(Font font, string text, int index, bool useFallback = true)
        {
            if (Fallbacks != null && useFallback)
            {
                return font.GetCharPosition(Fallbacks, text, index);
            }
            else
            {
                return font.GetCharPosition(text, index);
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Float2 GetCharPosition(Font font, string text, ref TextRange textRange, int index, bool useFallback = true)
        {
            if (Fallbacks != null && useFallback)
            {
                return font.GetCharPosition(Fallbacks, text, ref textRange, index);
            }
            else
            {
                return font.GetCharPosition(text, ref textRange, index);
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Float2 GetCharPosition(Font font, string text, int index, ref TextLayoutOptions layout, bool useFallback = true)
        {
            if (Fallbacks != null && useFallback)
            {
                return font.GetCharPosition(Fallbacks, text, index, ref layout);
            }
            else
            {
                return font.GetCharPosition(text, index, ref layout);
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Float2 GetCharPosition(Font font, string text, ref TextRange textRange, int index, ref TextLayoutOptions layout, bool useFallback = true)
        {
            if (Fallbacks != null && useFallback)
            {
                return font.GetCharPosition(Fallbacks, text, ref textRange, index, ref layout);
            }
            else
            {
                return font.GetCharPosition(text, ref textRange, index, ref layout);
            }
        }
    }
}
