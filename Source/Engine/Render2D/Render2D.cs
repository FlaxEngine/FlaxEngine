// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    partial class Render2D
    {
        /// <summary>
        /// Pushes transformation layer.
        /// </summary>
        /// <param name="transform">The transformation to apply.</param>
        public static void PushTransform(Matrix3x3 transform)
        {
            Internal_PushTransform(ref transform);
        }

        /// <summary>
        /// Pushes clipping rectangle mask.
        /// </summary>
        /// <param name="clipRect">The axis aligned clipping mask rectangle.</param>
        public static void PushClip(Rectangle clipRect)
        {
            Internal_PushClip(ref clipRect);
        }

        /// <summary>
        /// Draws the render target.
        /// </summary>
        /// <param name="rt">The render target handle to draw.</param>
        /// <param name="rect">The rectangle to draw.</param>
        public static void DrawTexture(GPUTextureView rt, Rectangle rect)
        {
            var color = Color.White;
            Internal_DrawTexture(FlaxEngine.Object.GetUnmanagedPtr(rt), ref rect, ref color);
        }

        /// <summary>
        /// Draws the texture.
        /// </summary>
        /// <param name="t">The texture to draw.</param>
        /// <param name="rect">The rectangle to draw.</param>
        public static void DrawTexture(GPUTexture t, Rectangle rect)
        {
            var color = Color.White;
            Internal_DrawTexture1(FlaxEngine.Object.GetUnmanagedPtr(t), ref rect, ref color);
        }

        /// <summary>
        /// Draws the texture.
        /// </summary>
        /// <param name="t">The texture to draw.</param>
        /// <param name="rect">The rectangle to draw.</param>
        public static void DrawTexture(TextureBase t, Rectangle rect)
        {
            var color = Color.White;
            Internal_DrawTexture2(FlaxEngine.Object.GetUnmanagedPtr(t), ref rect, ref color);
        }

        /// <summary>
        /// Draws a sprite.
        /// </summary>
        /// <param name="spriteHandle">The sprite to draw.</param>
        /// <param name="rect">The rectangle to draw.</param>
        public static void DrawSprite(SpriteHandle spriteHandle, Rectangle rect)
        {
            DrawSprite(spriteHandle, rect, Color.White);
        }

        /// <summary>
        /// Draws the texture (uses point sampler).
        /// </summary>
        /// <param name="t">The texture to draw.</param>
        /// <param name="rect">The rectangle to draw.</param>
        public static void DrawTexturePoint(GPUTexture t, Rectangle rect)
        {
            var color = Color.White;
            Internal_DrawTexturePoint(FlaxEngine.Object.GetUnmanagedPtr(t), ref rect, ref color);
        }

        /// <summary>
        /// Draws a sprite (uses point sampler).
        /// </summary>
        /// <param name="spriteHandle">The sprite to draw.</param>
        /// <param name="rect">The rectangle to draw.</param>
        public static void DrawSpritePoint(SpriteHandle spriteHandle, Rectangle rect)
        {
            DrawSpritePoint(spriteHandle, rect, Color.White);
        }

        /// <summary>
        /// Draws the GUI material.
        /// </summary>
        /// <param name="material">The material to render. Must be a GUI material type.</param>
        /// <param name="rect">The target rectangle to draw.</param>
        public static void DrawMaterial(MaterialBase material, Rectangle rect)
        {
            var color = Color.White;
            Internal_DrawMaterial(FlaxEngine.Object.GetUnmanagedPtr(material), ref rect, ref color);
        }

        /// <summary>
        /// Draws a text.
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
            DrawText(font, text, color, ref layout);
        }

        /// <summary>
        /// Draws a text using a custom material shader. Given material must have GUI domain and a public parameter named Font (texture parameter used for a font atlas sampling).
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
            DrawText(font, text, color, ref layout, customMaterial);
        }

        /// <summary>
        /// Calls drawing GUI to the texture.
        /// </summary>
        /// <param name="drawableElement">The root container for Draw methods.</param>
        /// <param name="context">The GPU context to handle graphics commands.</param>
        /// <param name="output">The output render target.</param>
        public static void CallDrawing(IDrawable drawableElement, GPUContext context, GPUTexture output)
        {
            if (context == null || output == null || drawableElement == null)
                throw new ArgumentNullException();

            Begin(context, output);
            try
            {
                drawableElement.Draw();
            }
            finally
            {
                End();
            }
        }

        /// <summary>
        /// Calls drawing GUI to the texture using custom View*Projection matrix.
        /// If depth buffer texture is provided there will be depth test performed during rendering.
        /// </summary>
        /// <param name="drawableElement">The root container for Draw methods.</param>
        /// <param name="context">The GPU context to handle graphics commands.</param>
        /// <param name="output">The output render target.</param>
        /// <param name="depthBuffer">The depth buffer render target. It's optional parameter but if provided must match output texture.</param>
        /// <param name="viewProjection">The View*Projection matrix used to transform all rendered vertices.</param>
        public static void CallDrawing(IDrawable drawableElement, GPUContext context, GPUTexture output, GPUTexture depthBuffer, ref Matrix viewProjection)
        {
            if (context == null || output == null || drawableElement == null)
                throw new ArgumentNullException();
            if (depthBuffer != null)
            {
                if (!depthBuffer.IsAllocated)
                    throw new InvalidOperationException("Depth buffer is not allocated. Use GPUTexture.Init before rendering.");
                if (output.Size != depthBuffer.Size)
                    throw new InvalidOperationException("Output buffer and depth buffer dimensions must be equal.");
            }

            Begin(context, output, depthBuffer, ref viewProjection);
            try
            {
                drawableElement.Draw();
            }
            finally
            {
                End();
            }
        }
    }
}
