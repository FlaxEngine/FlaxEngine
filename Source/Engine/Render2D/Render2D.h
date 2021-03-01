// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Color.h"
#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Core/Types/Span.h"

struct SpriteHandle;
struct TextLayoutOptions;
struct Matrix;
struct Matrix3x3;
struct Viewport;
struct TextRange;
class Font;
class GPUPipelineState;
class GPUTexture;
class GPUTextureView;
class GPUContext;
class RenderTask;
class MaterialBase;
class TextureBase;

/// <summary>
/// Rendering 2D shapes and text using Graphics Device.
/// </summary>
API_CLASS(Static) class FLAXENGINE_API Render2D
{
DECLARE_SCRIPTING_TYPE_NO_SPAWN(Render2D);
public:

    /// <summary>
    /// The rendering features and options flags.
    /// </summary>
    API_ENUM(Attributes="Flags") enum class RenderingFeatures
    {
        /// <summary>
        /// The none.
        /// </summary>
        None = 0,

        /// <summary>
        /// Enables automatic geometry vertices snapping to integer coordinates in screen space. Reduces aliasing and sampling artifacts. Might be disabled for 3D projection viewport or for complex UI transformations.
        /// </summary>
        VertexSnapping = 1,
    };

public:

    /// <summary>
    /// Checks if interface is during rendering phrase (Draw calls may be performed without failing).
    /// </summary>
    /// <returns>True if is during rendering, otherwise false/</returns>
    static bool IsRendering();

    /// <summary>
    /// Gets the current rendering viewport.
    /// </summary>
    /// <returns>The viewport </returns>
    static const Viewport& GetViewport();

    /// <summary>
    /// The active rendering features flags.
    /// </summary>
    API_FIELD() static RenderingFeatures Features;

public:

    /// <summary>
    /// Called when frame rendering begins by the graphics device.
    /// </summary>
    static void BeginFrame();

    /// <summary>
    /// Begins the rendering phrase.
    /// </summary>
    /// <param name="context">The GPU commands context to use.</param>
    /// <param name="output">The output target.</param>
    /// <param name="depthBuffer">The depth buffer.</param>
    API_FUNCTION() static void Begin(GPUContext* context, GPUTexture* output, GPUTexture* depthBuffer = nullptr);

    /// <summary>
    /// Begins the rendering phrase.
    /// </summary>
    /// <param name="context">The GPU commands context to use.</param>
    /// <param name="output">The output target.</param>
    /// <param name="depthBuffer">The depth buffer.</param>
    /// <param name="viewProjection">The View*Projection matrix. Allows to render GUI in 3D or with custom transformations.</param>
    API_FUNCTION() static void Begin(GPUContext* context, GPUTexture* output, GPUTexture* depthBuffer, API_PARAM(Ref) const Matrix& viewProjection);

    /// <summary>
    /// Begins the rendering phrase.
    /// </summary>
    /// <param name="context">The GPU commands context to use.</param>
    /// <param name="output">The output target.</param>
    /// <param name="depthBuffer">The depth buffer.</param>
    /// <param name="viewport">The output viewport.</param>
    API_FUNCTION() static void Begin(GPUContext* context, GPUTextureView* output, GPUTextureView* depthBuffer, API_PARAM(Ref) const Viewport& viewport);

    /// <summary>
    /// Begins the rendering phrase.
    /// </summary>
    /// <param name="context">The GPU commands context to use.</param>
    /// <param name="output">The output target.</param>
    /// <param name="depthBuffer">The depth buffer.</param>
    /// <param name="viewport">The output viewport.</param>
    /// <param name="viewProjection">The View*Projection matrix. Allows to render GUI in 3D or with custom transformations.</param>
    API_FUNCTION() static void Begin(GPUContext* context, GPUTextureView* output, GPUTextureView* depthBuffer, API_PARAM(Ref) const Viewport& viewport, API_PARAM(Ref) const Matrix& viewProjection);

    /// <summary>
    /// Ends the rendering phrase.
    /// </summary>
    API_FUNCTION() static void End();

    /// <summary>
    /// Called when frame rendering ends by the graphics device.
    /// </summary>
    static void EndFrame();

public:

    /// <summary>
    /// Pushes transformation layer.
    /// </summary>
    /// <param name="transform">The transformation to apply.</param>
    API_FUNCTION() static void PushTransform(API_PARAM(Ref) const Matrix3x3& transform);

    /// <summary>
    /// Peeks the current transformation layer.
    /// </summary>
    /// <param name="transform">The output transformation to apply combined from all the transformations together (pushed into the transformation stack).</param>
    API_FUNCTION() static void PeekTransform(API_PARAM(Out) Matrix3x3& transform);

    /// <summary>
    /// Pops transformation layer.
    /// </summary>
    API_FUNCTION() static void PopTransform();

    /// <summary>
    /// Pushes clipping rectangle mask.
    /// </summary>
    /// <param name="clipRect">The axis aligned clipping mask rectangle.</param>
    API_FUNCTION() static void PushClip(API_PARAM(Ref) const Rectangle& clipRect);

    /// <summary>
    /// Peeks the current clipping rectangle mask.
    /// </summary>
    /// <param name="clipRect">The output axis aligned clipping mask rectangle combined from all the masks together (pushed into the masking stack).</param>
    API_FUNCTION() static void PeekClip(API_PARAM(Out) Rectangle& clipRect);

    /// <summary>
    /// Pops clipping rectangle mask.
    /// </summary>
    API_FUNCTION() static void PopClip();

    /// <summary>
    /// Pushes tint color.
    /// </summary>
    /// <param name="tint">The tint color.</param>
    /// <param name="inherit">Multiply <paramref ref="tint"/> by the last tint on the stack.</param>
    API_FUNCTION() static void PushTint(API_PARAM(Ref) const Color& tint, bool inherit = true);

    /// <summary>
    /// Peeks the current tint color.
    /// </summary>
    /// <param name="tint">The output tint color.</param>
    API_FUNCTION() static void PeekTint(API_PARAM(Out) Color& tint);

    /// <summary>
    /// Pops tint color.
    /// </summary>
    API_FUNCTION() static void PopTint();

public:

    /// <summary>
    /// Draws a text.
    /// </summary>
    /// <param name="font">The font to use.</param>
    /// <param name="text">The text to render.</param>
    /// <param name="color">The text color.</param>
    /// <param name="location">The text location.</param>
    /// <param name="customMaterial">The custom material for font characters rendering. It must contain texture parameter named Font used to sample font texture.</param>
    API_FUNCTION() static void DrawText(Font* font, const StringView& text, const Color& color, const Vector2& location, MaterialBase* customMaterial = nullptr);

    /// <summary>
    /// Draws a text.
    /// </summary>
    /// <param name="font">The font to use.</param>
    /// <param name="text">The text to render.</param>
    /// <param name="textRange">The input text range (substring range of the input text parameter).</param>
    /// <param name="color">The text color.</param>
    /// <param name="location">The text location.</param>
    /// <param name="customMaterial">The custom material for font characters rendering. It must contain texture parameter named Font used to sample font texture.</param>
    API_FUNCTION() static void DrawText(Font* font, const StringView& text, API_PARAM(Ref) const TextRange& textRange, const Color& color, const Vector2& location, MaterialBase* customMaterial = nullptr);

    /// <summary>
    /// Draws a text with formatting.
    /// </summary>
    /// <param name="font">The font to use.</param>
    /// <param name="text">The text to render.</param>
    /// <param name="color">The text color.</param>
    /// <param name="layout">The text layout properties.</param>
    /// <param name="customMaterial">The custom material for font characters rendering. It must contain texture parameter named Font used to sample font texture.</param>
    API_FUNCTION() static void DrawText(Font* font, const StringView& text, const Color& color, API_PARAM(Ref) const TextLayoutOptions& layout, MaterialBase* customMaterial = nullptr);

    /// <summary>
    /// Draws a text with formatting.
    /// </summary>
    /// <param name="font">The font to use.</param>
    /// <param name="text">The text to render.</param>
    /// <param name="textRange">The input text range (substring range of the input text parameter).</param>
    /// <param name="color">The text color.</param>
    /// <param name="layout">The text layout properties.</param>
    /// <param name="customMaterial">The custom material for font characters rendering. It must contain texture parameter named Font used to sample font texture.</param>
    API_FUNCTION() static void DrawText(Font* font, const StringView& text, API_PARAM(Ref) const TextRange& textRange, const Color& color, API_PARAM(Ref) const TextLayoutOptions& layout, MaterialBase* customMaterial = nullptr);

    /// <summary>
    /// Fills a rectangle area.
    /// </summary>
    /// <param name="rect">The rectangle to fill.</param>
    /// <param name="color">The color to use.</param>
    API_FUNCTION() static void FillRectangle(const Rectangle& rect, const Color& color);

    /// <summary>
    /// Fills a rectangle area.
    /// </summary>
    /// <param name="rect">The rectangle to fill.</param>
    /// <param name="color1">The color to use for upper left vertex.</param>
    /// <param name="color2">The color to use for upper right vertex.</param>
    /// <param name="color3">The color to use for bottom right vertex.</param>
    /// <param name="color4">The color to use for bottom left vertex.</param>
    API_FUNCTION() static void FillRectangle(const Rectangle& rect, const Color& color1, const Color& color2, const Color& color3, const Color& color4);

    /// <summary>
    /// Draws a rectangle borders.
    /// </summary>
    /// <param name="rect">The rectangle to draw.</param>
    /// <param name="color">The color to use.</param>
    /// <param name="thickness">The line thickness.</param>
    API_FUNCTION() FORCE_INLINE static void DrawRectangle(const Rectangle& rect, const Color& color, float thickness = 1.0f)
    {
        DrawRectangle(rect, color, color, color, color, thickness);
    }

    /// <summary>
    /// Draws a rectangle borders.
    /// </summary>
    /// <param name="rect">The rectangle to fill.</param>
    /// <param name="color1">The color to use for upper left vertex.</param>
    /// <param name="color2">The color to use for upper right vertex.</param>
    /// <param name="color3">The color to use for bottom right vertex.</param>
    /// <param name="color4">The color to use for bottom left vertex.</param>
    /// <param name="thickness">The line thickness.</param>
    API_FUNCTION() static void DrawRectangle(const Rectangle& rect, const Color& color1, const Color& color2, const Color& color3, const Color& color4, float thickness = 1.0f);

    /// <summary>
    /// Draws the render target.
    /// </summary>
    /// <param name="rt">The render target handle to draw.</param>
    /// <param name="rect">The rectangle to draw.</param>
    /// <param name="color">The color to multiply all texture pixels.</param>
    API_FUNCTION() static void DrawTexture(GPUTextureView* rt, const Rectangle& rect, const Color& color = Color::White);

    /// <summary>
    /// Draws the texture.
    /// </summary>
    /// <param name="t">The texture to draw.</param>
    /// <param name="rect">The rectangle to draw.</param>
    /// <param name="color">The color to multiply all texture pixels.</param>
    API_FUNCTION() static void DrawTexture(GPUTexture* t, const Rectangle& rect, const Color& color = Color::White);

    /// <summary>
    /// Draws the texture.
    /// </summary>
    /// <param name="t">The texture to draw.</param>
    /// <param name="rect">The rectangle to draw.</param>
    /// <param name="color">The color to multiply all texture pixels.</param>
    API_FUNCTION() static void DrawTexture(TextureBase* t, const Rectangle& rect, const Color& color = Color::White);

    /// <summary>
    /// Draws a sprite.
    /// </summary>
    /// <param name="spriteHandle">The sprite to draw.</param>
    /// <param name="rect">The rectangle to draw.</param>
    /// <param name="color">The color to multiply all texture pixels.</param>
    API_FUNCTION() static void DrawSprite(const SpriteHandle& spriteHandle, const Rectangle& rect, const Color& color = Color::White);

    /// <summary>
    /// Draws the texture (uses point sampler).
    /// </summary>
    /// <param name="t">The texture to draw.</param>
    /// <param name="rect">The rectangle to draw.</param>
    /// <param name="color">The color to multiply all texture pixels.</param>
    API_FUNCTION() static void DrawTexturePoint(GPUTexture* t, const Rectangle& rect, const Color& color = Color::White);

    /// <summary>
    /// Draws a sprite (uses point sampler).
    /// </summary>
    /// <param name="spriteHandle">The sprite to draw.</param>
    /// <param name="rect">The rectangle to draw.</param>
    /// <param name="color">The color to multiply all texture pixels.</param>
    API_FUNCTION() static void DrawSpritePoint(const SpriteHandle& spriteHandle, const Rectangle& rect, const Color& color = Color::White);

    /// <summary>
    /// Performs custom rendering.
    /// </summary>
    /// <param name="t">The texture to use.</param>
    /// <param name="rect">The rectangle area to draw.</param>
    /// <param name="ps">The custom pipeline state to use (input must match default Render2D vertex shader and can use single texture).</param>
    /// <param name="color">The color to multiply all texture pixels.</param>
    API_FUNCTION() static void DrawCustom(GPUTexture* t, const Rectangle& rect, GPUPipelineState* ps, const Color& color = Color::White);

    /// <summary>
    /// Draws a line.
    /// </summary>
    /// <param name="p1">The start point.</param>
    /// <param name="p2">The end point.</param>
    /// <param name="color">The line color.</param>
    /// <param name="thickness">The line thickness.</param>
    API_FUNCTION() FORCE_INLINE static void DrawLine(const Vector2& p1, const Vector2& p2, const Color& color, float thickness = 1.0f)
    {
        DrawLine(p1, p2, color, color, thickness);
    }

    /// <summary>
    /// Draws a line.
    /// </summary>
    /// <param name="p1">The start point.</param>
    /// <param name="p2">The end point.</param>
    /// <param name="color1">The line start color.</param>
    /// <param name="color2">The line end color.</param>
    /// <param name="thickness">The line thickness.</param>
    API_FUNCTION() static void DrawLine(const Vector2& p1, const Vector2& p2, const Color& color1, const Color& color2, float thickness = 1.0f);

    /// <summary>
    /// Draws a Bezier curve.
    /// </summary>
    /// <param name="p1">The start point.</param>
    /// <param name="p2">The first control point.</param>
    /// <param name="p3">The second control point.</param>
    /// <param name="p4">The end point.</param>
    /// <param name="color">The line color</param>
    /// <param name="thickness">The line thickness.</param>
    API_FUNCTION() static void DrawBezier(const Vector2& p1, const Vector2& p2, const Vector2& p3, const Vector2& p4, const Color& color, float thickness = 1.0f);

    /// <summary>
    /// Draws the GUI material.
    /// </summary>
    /// <param name="material">The material to render. Must be a GUI material type.</param>
    /// <param name="rect">The target rectangle to draw.</param>
    /// <param name="color">The color to use.</param>
    API_FUNCTION() static void DrawMaterial(MaterialBase* material, const Rectangle& rect, const Color& color);

    /// <summary>
    /// Draws the background blur.
    /// </summary>
    /// <param name="rect">The target rectangle to draw (blurs its background).</param>
    /// <param name="blurStrength">The blur strength defines how blurry the background is. Larger numbers increase blur, resulting in a larger runtime cost on the GPU.</param>
    API_FUNCTION() static void DrawBlur(const Rectangle& rect, float blurStrength);

    /// <summary>
    /// Draws vertices array.
    /// </summary>
    /// <param name="t">The texture.</param>
    /// <param name="vertices">The vertices array.</param>
    /// <param name="uvs">The uvs array.</param>
    API_FUNCTION() static void DrawTexturedTriangles(GPUTexture* t, const Span<Vector2>& vertices, const Span<Vector2>& uvs);

    /// <summary>
    /// Draws vertices array.
    /// </summary>
    /// <param name="vertices">The vertices array.</param>
    /// <param name="colors">The colors array.</param>
    /// <param name="useAlpha">If true alpha blending will be enabled.</param>
    API_FUNCTION() static void FillTriangles(const Span<Vector2>& vertices, const Span<Color>& colors, bool useAlpha);

    /// <summary>
    /// Fills a triangular area.
    /// </summary>
    /// <param name="p0">The first point.</param>
    /// <param name="p1">The second point.</param>
    /// <param name="p2">The third point.</param>
    /// <param name="color">The color.</param>
    API_FUNCTION() static void FillTriangle(const Vector2& p0, const Vector2& p1, const Vector2& p2, const Color& color);
};
