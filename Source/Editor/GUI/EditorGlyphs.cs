// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI
{
    /// <summary>
    /// Draws small UI glyphs (cross, chevrons) using font rasterization instead of low-resolution
    /// atlas sprites (Cross12, ArrowDown12, ArrowRight12). Because the font subsystem re-rasterizes
    /// glyphs at the current interface DPI (see <c>Font.SetGlobalScale</c>), the resulting shapes
    /// stay crisp at any Interface Scale, unlike the fixed 12-px sprites which become blurred (with
    /// linear sampling) or blocky (with point sampling) when the UI is scaled up.
    /// </summary>
    [HideInEditor]
    public static class EditorGlyphs
    {
        // Only the cross uses a font glyph (× / MULTIPLICATION SIGN) since it is present in every
        // font we ship. The chevrons are drawn as filled triangles because the small-triangle
        // Unicode codepoints (U+25BE / U+25B8) are outside the coverage of the default editor font
        // and would render as tofu boxes.
        private const string CrossChar = "\u00D7"; // ×

        private static Font GetGlyphFont(float size)
        {
            var style = Style.Current;
            var font = style?.FontMedium ?? style?.FontSmall;
            if (font == null)
                return null;
            float targetSize = Mathf.Max(6.0f, size);
            var asset = font.Asset;
            return asset != null ? asset.CreateFont(targetSize) : font;
        }

        /// <summary>
        /// Draws a crisp cross (×) glyph fitting the given rectangle.
        /// </summary>
        public static void DrawCross(Rectangle rect, Color color)
        {
            float size = Mathf.Min(rect.Width, rect.Height);
            var font = GetGlyphFont(size * 1.3f);
            if (font == null)
                return;
            Render2D.DrawText(font, CrossChar, rect, color, TextAlignment.Center, TextAlignment.Center);
        }

        /// <summary>
        /// Draws a crisp down chevron glyph fitting the given rectangle.
        /// </summary>
        public static void DrawArrowDown(Rectangle rect, Color color)
        {
            float size = Mathf.Min(rect.Width, rect.Height) * 0.55f;
            float cx = rect.Location.X + rect.Width * 0.5f;
            float cy = rect.Location.Y + rect.Height * 0.5f;
            float halfW = size * 0.5f;
            float halfH = size * 0.4f;
            var p0 = new Float2(cx - halfW, cy - halfH);
            var p1 = new Float2(cx + halfW, cy - halfH);
            var p2 = new Float2(cx, cy + halfH);
            Render2D.FillTriangle(p0, p1, p2, color);
        }

        /// <summary>
        /// Draws a crisp right chevron glyph fitting the given rectangle.
        /// </summary>
        public static void DrawArrowRight(Rectangle rect, Color color)
        {
            float size = Mathf.Min(rect.Width, rect.Height) * 0.55f;
            float cx = rect.Location.X + rect.Width * 0.5f;
            float cy = rect.Location.Y + rect.Height * 0.5f;
            float halfW = size * 0.4f;
            float halfH = size * 0.5f;
            var p0 = new Float2(cx - halfW, cy - halfH);
            var p1 = new Float2(cx - halfW, cy + halfH);
            var p2 = new Float2(cx + halfW, cy);
            Render2D.FillTriangle(p0, p1, p2, color);
        }
    }

    /// <summary>
    /// Brush that draws a cross (×) glyph using the editor font. Replacement for the low-resolution
    /// Cross12 sprite brush so the glyph stays crisp at any Interface Scale.
    /// </summary>
    [HideInEditor]
    public sealed class CrossBrush : IBrush
    {
        /// <inheritdoc />
        public Float2 Size => new Float2(12.0f);

        /// <inheritdoc />
        public void Draw(Rectangle rect, Color color) => EditorGlyphs.DrawCross(rect, color);

        /// <inheritdoc />
        public int CompareTo(object obj) => obj is CrossBrush ? 0 : 1;
    }

    /// <summary>
    /// Brush that draws a down chevron (▾) glyph using the editor font. Replacement for the
    /// low-resolution ArrowDown12 sprite brush so the glyph stays crisp at any Interface Scale.
    /// </summary>
    [HideInEditor]
    public sealed class ArrowDownBrush : IBrush
    {
        /// <inheritdoc />
        public Float2 Size => new Float2(12.0f);

        /// <inheritdoc />
        public void Draw(Rectangle rect, Color color) => EditorGlyphs.DrawArrowDown(rect, color);

        /// <inheritdoc />
        public int CompareTo(object obj) => obj is ArrowDownBrush ? 0 : 1;
    }

    /// <summary>
    /// Brush that draws a right chevron (▸) glyph using the editor font. Replacement for the
    /// low-resolution ArrowRight12 sprite brush so the glyph stays crisp at any Interface Scale.
    /// </summary>
    [HideInEditor]
    public sealed class ArrowRightBrush : IBrush
    {
        /// <inheritdoc />
        public Float2 Size => new Float2(12.0f);

        /// <inheritdoc />
        public void Draw(Rectangle rect, Color color) => EditorGlyphs.DrawArrowRight(rect, color);

        /// <inheritdoc />
        public int CompareTo(object obj) => obj is ArrowRightBrush ? 0 : 1;
    }
}
