// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_DEBUG_DRAW

#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Core/Math/Color.h"
#include "Engine/Core/Math/Color32.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Types/Span.h"

struct RenderView;
class Collider;
class Light;
struct RenderContext;
class GPUTextureView;
class GPUContext;
class GPUBuffer;
class RenderTask;
class SceneRenderTask;
class Actor;
struct Transform;

/// <summary>
/// The debug shapes rendering service. Not available in final game. For use only in the editor.
/// </summary>
API_CLASS(Static) class FLAXENGINE_API DebugDraw
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(DebugDraw);

    /// <summary>
    /// Vertex data for debug shapes.
    /// </summary>
    PACK_STRUCT(struct Vertex {
        Float3 Position;
        Color32 Color;
        });

#if USE_EDITOR
    /// <summary>
    /// Allocates the context for Debug Drawing. Can be used to redirect debug shapes collecting to a separate container (instead of global state).
    /// </summary>
    /// <returns>The context object. Release it wil FreeContext. Returns null if failed.</returns>
    API_FUNCTION() static void* AllocateContext();

    /// <summary>
    /// Frees the context for Debug Drawing.
    /// </summary>
    /// <param name="context">The context.</param>
    API_FUNCTION() static void FreeContext(void* context);

    /// <summary>
    /// Updates the context for Debug Drawing.
    /// </summary>
    /// <param name="context">The context.</param>
    /// <param name="deltaTime">The update delta time (in seconds).</param>
    API_FUNCTION() static void UpdateContext(void* context, float deltaTime);

    /// <summary>
    /// Sets the context for Debug Drawing to a custom or null to use global default.
    /// </summary>
    /// <param name="context">The context or null.</param>
    API_FUNCTION() static void SetContext(void* context);
#endif

    // Gets the last view position when rendering the current context. Can be sued for custom culling or LODing when drawing more complex shapes.
    static Vector3 GetViewPos();

    /// <summary>
    /// Draws the collected debug shapes to the output.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="target">The rendering output surface handle.</param>
    /// <param name="depthBuffer">The custom depth texture used for depth test. Can be MSAA. Must match target surface size.</param>
    /// <param name="enableDepthTest">True if perform manual depth test with scene depth buffer when rendering the primitives. Uses custom shader and the scene depth buffer.</param>
    API_FUNCTION() static void Draw(API_PARAM(Ref) RenderContext& renderContext, GPUTextureView* target = nullptr, GPUTextureView* depthBuffer = nullptr, bool enableDepthTest = false);

    /// <summary>
    /// Draws the debug shapes for the given collection of selected actors and other scene actors debug shapes.
    /// </summary>
    /// <param name="selectedActors">The list of actors to draw.</param>
    /// <param name="selectedActorsCount">The size of the list of actors.</param>
    /// <param name="drawScenes">True if draw all debug shapes from scenes too or false if draw just from specified actor list.</param>
    API_FUNCTION() static void DrawActors(Actor** selectedActors, int32 selectedActorsCount, bool drawScenes);

    /// <summary>
    /// Draws the debug shapes for the given actor and the actor's children
    /// </summary>
    /// <param name="actor">The actor to start drawing at.</param>
    API_FUNCTION() static void DrawActorsTree(Actor* actor);

#if USE_EDITOR
    /// <summary>
    /// Draws the physics debug shapes for the given collider. Editor Only
    /// </summary>
    /// <param name="collider">The collider to draw.</param>
    /// <param name="view">The render view to draw in.</param>
    API_FUNCTION() static void DrawColliderDebugPhysics(Collider* collider, RenderView& view);

    /// <summary>
    /// Draws the light debug shapes for the given light. Editor Only
    /// </summary>
    /// <param name="light">The light debug to draw.</param>
    /// <param name="view">The render view to draw in.</param>
    API_FUNCTION() static void DrawLightDebug(Light* light, RenderView& view);
#endif

    /// <summary>
    /// Draws the lines axis from direction.
    /// </summary>
    /// <param name="origin">The origin of the line.</param>
    /// <param name="direction">The direction of the line.</param>
    /// <param name="size">The size of the axis.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawAxisFromDirection(const Vector3& origin, const Vector3& direction, float size = 100.0f, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the line in a direction.
    /// </summary>
    /// <param name="origin">The origin of the line.</param>
    /// <param name="direction">The direction of the line.</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawDirection(const Vector3& origin, const Vector3& direction, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the line in a direction.
    /// [Deprecated in v1.8]
    /// </summary>
    /// <param name="origin">The origin of the line.</param>
    /// <param name="direction">The direction of the line.</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() DEPRECATED("Use DrawRay with length parameter instead") static void DrawRay(const Vector3& origin, const Vector3& direction, const Color& color, float duration, bool depthTest);

    /// <summary>
    /// Draws the line in a direction.
    /// </summary>
    /// <param name="origin">The origin of the line.</param>
    /// <param name="direction">The direction of the line.</param>
    /// <param name="color">The color.</param>
    /// <param name="length">The length of the ray.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawRay(const Vector3& origin, const Vector3& direction, const Color& color = Color::White, float length = MAX_float, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the line in a direction.
    /// </summary>
    /// <param name="ray">The ray.</param>
    /// <param name="color">The color.</param>
    /// <param name="length">The length of the ray.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawRay(const Ray& ray, const Color& color = Color::White, float length = MAX_float, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the line.
    /// </summary>
    /// <param name="start">The start point.</param>
    /// <param name="end">The end point.</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawLine(const Vector3& start, const Vector3& end, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the line.
    /// </summary>
    /// <param name="start">The start point.</param>
    /// <param name="end">The end point.</param>
    /// <param name="startColor">The color of the start point.</param>
    /// <param name="endColor">The color of the end point.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawLine(const Vector3& start, const Vector3& end, const Color& startColor, const Color& endColor, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the lines. Line positions are located one after another (e.g. l0.start, l0.end, l1.start, l1.end,...).
    /// </summary>
    /// <param name="lines">The list of vertices for lines (must have multiple of 2 elements).</param>
    /// <param name="transform">The custom matrix used to transform all line vertices.</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawLines(const Span<Float3>& lines, const Matrix& transform, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the lines using the provided vertex buffer that contains pairs of Vertex elements. Line positions are located one after another (e.g. l0.start, l0.end, l1.start, l1.end,...).
    /// </summary>
    /// <param name="lines">The GPU buffer with vertices for lines (must have multiple of 2 elements).</param>
    /// <param name="transform">The custom matrix used to transform all line vertices.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawLines(GPUBuffer* lines, const Matrix& transform, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the lines. Line positions are located one after another (e.g. l0.start, l0.end, l1.start, l1.end,...).
    /// </summary>
    /// <param name="lines">The list of vertices for lines (must have multiple of 2 elements).</param>
    /// <param name="transform">The custom matrix used to transform all line vertices.</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    static void DrawLines(const Array<Float3, HeapAllocation>& lines, const Matrix& transform, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the lines. Line positions are located one after another (e.g. l0.start, l0.end, l1.start, l1.end,...).
    /// </summary>
    /// <param name="lines">The list of vertices for lines (must have multiple of 2 elements).</param>
    /// <param name="transform">The custom matrix used to transform all line vertices.</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawLines(const Span<Double3>& lines, const Matrix& transform, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the lines. Line positions are located one after another (e.g. l0.start, l0.end, l1.start, l1.end,...).
    /// </summary>
    /// <param name="lines">The list of vertices for lines (must have multiple of 2 elements).</param>
    /// <param name="transform">The custom matrix used to transform all line vertices.</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    static void DrawLines(const Array<Double3, HeapAllocation>& lines, const Matrix& transform, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws a Bezier curve.
    /// </summary>
    /// <param name="p1">The start point.</param>
    /// <param name="p2">The first control point.</param>
    /// <param name="p3">The second control point.</param>
    /// <param name="p4">The end point.</param>
    /// <param name="color">The line color</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawBezier(const Vector3& p1, const Vector3& p2, const Vector3& p3, const Vector3& p4, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the circle.
    /// </summary>
    /// <param name="position">The center position.</param>
    /// <param name="normal">The normal vector direction.</param>
    /// <param name="radius">The radius.</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawCircle(const Vector3& position, const Float3& normal, float radius, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the wireframe triangle.
    /// </summary>
    /// <param name="v0">The first triangle vertex.</param>
    /// <param name="v1">The second triangle vertex.</param>
    /// <param name="v2">The third triangle vertex.</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawWireTriangle(const Vector3& v0, const Vector3& v1, const Vector3& v2, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the triangle.
    /// </summary>
    /// <param name="v0">The first triangle vertex.</param>
    /// <param name="v1">The second triangle vertex.</param>
    /// <param name="v2">The third triangle vertex.</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawTriangle(const Vector3& v0, const Vector3& v1, const Vector3& v2, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the triangles.
    /// </summary>
    /// <param name="vertices">The triangle vertices list (must have multiple of 3 elements).</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawTriangles(const Span<Float3>& vertices, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the triangles.
    /// </summary>
    /// <param name="vertices">The triangle vertices list (must have multiple of 3 elements).</param>
    /// <param name="transform">The custom matrix used to transform all line vertices.</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawTriangles(const Span<Float3>& vertices, const Matrix& transform, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the triangles.
    /// </summary>
    /// <param name="vertices">The triangle vertices list (must have multiple of 3 elements).</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    static void DrawTriangles(const Array<Float3, HeapAllocation>& vertices, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the triangles.
    /// </summary>
    /// <param name="vertices">The triangle vertices list (must have multiple of 3 elements).</param>
    /// <param name="transform">The custom matrix used to transform all line vertices.</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    static void DrawTriangles(const Array<Float3, HeapAllocation>& vertices, const Matrix& transform, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the triangles using the given index buffer.
    /// </summary>
    /// <param name="vertices">The triangle vertices list.</param>
    /// <param name="indices">The triangle indices list (must have multiple of 3 elements).</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawTriangles(const Span<Float3>& vertices, const Span<int32>& indices, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the triangles using the given index buffer.
    /// </summary>
    /// <param name="vertices">The triangle vertices list.</param>
    /// <param name="indices">The triangle indices list (must have multiple of 3 elements).</param>
    /// <param name="transform">The custom matrix used to transform all line vertices.</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawTriangles(const Span<Float3>& vertices, const Span<int32>& indices, const Matrix& transform, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the triangles using the given index buffer.
    /// </summary>
    /// <param name="vertices">The triangle vertices list.</param>
    /// <param name="indices">The triangle indices list (must have multiple of 3 elements).</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    static void DrawTriangles(const Array<Float3, HeapAllocation>& vertices, const Array<int32, HeapAllocation>& indices, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the triangles using the given index buffer.
    /// </summary>
    /// <param name="vertices">The triangle vertices list.</param>
    /// <param name="indices">The triangle indices list (must have multiple of 3 elements).</param>
    /// <param name="transform">The custom matrix used to transform all line vertices.</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    static void DrawTriangles(const Array<Float3, HeapAllocation>& vertices, const Array<int32, HeapAllocation>& indices, const Matrix& transform, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the triangles.
    /// </summary>
    /// <param name="vertices">The triangle vertices list (must have multiple of 3 elements).</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawTriangles(const Span<Double3>& vertices, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the triangles.
    /// </summary>
    /// <param name="vertices">The triangle vertices list (must have multiple of 3 elements).</param>
    /// <param name="transform">The custom matrix used to transform all line vertices.</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawTriangles(const Span<Double3>& vertices, const Matrix& transform, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the triangles.
    /// </summary>
    /// <param name="vertices">The triangle vertices list (must have multiple of 3 elements).</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    static void DrawTriangles(const Array<Double3, HeapAllocation>& vertices, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the triangles.
    /// </summary>
    /// <param name="vertices">The triangle vertices list (must have multiple of 3 elements).</param>
    /// <param name="transform">The custom matrix used to transform all line vertices.</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    static void DrawTriangles(const Array<Double3, HeapAllocation>& vertices, const Matrix& transform, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the triangles using the given index buffer.
    /// </summary>
    /// <param name="vertices">The triangle vertices list.</param>
    /// <param name="indices">The triangle indices list (must have multiple of 3 elements).</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawTriangles(const Span<Double3>& vertices, const Span<int32>& indices, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the triangles using the given index buffer.
    /// </summary>
    /// <param name="vertices">The triangle vertices list.</param>
    /// <param name="indices">The triangle indices list (must have multiple of 3 elements).</param>
    /// <param name="transform">The custom matrix used to transform all line vertices.</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawTriangles(const Span<Double3>& vertices, const Span<int32>& indices, const Matrix& transform, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the triangles using the given index buffer.
    /// </summary>
    /// <param name="vertices">The triangle vertices list.</param>
    /// <param name="indices">The triangle indices list (must have multiple of 3 elements).</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    static void DrawTriangles(const Array<Double3, HeapAllocation>& vertices, const Array<int32, HeapAllocation>& indices, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the triangles using the given index buffer.
    /// </summary>
    /// <param name="vertices">The triangle vertices list.</param>
    /// <param name="indices">The triangle indices list (must have multiple of 3 elements).</param>
    /// <param name="transform">The custom matrix used to transform all line vertices.</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    static void DrawTriangles(const Array<Double3, HeapAllocation>& vertices, const Array<int32, HeapAllocation>& indices, const Matrix& transform, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the wireframe triangles.
    /// </summary>
    /// <param name="vertices">The triangle vertices list (must have multiple of 3 elements).</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawWireTriangles(const Span<Float3>& vertices, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the wireframe triangles.
    /// </summary>
    /// <param name="vertices">The triangle vertices list (must have multiple of 3 elements).</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    static void DrawWireTriangles(const Array<Float3, HeapAllocation>& vertices, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the wireframe triangles using the given index buffer.
    /// </summary>
    /// <param name="vertices">The triangle vertices list.</param>
    /// <param name="indices">The triangle indices list (must have multiple of 3 elements).</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawWireTriangles(const Span<Float3>& vertices, const Span<int32>& indices, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the wireframe triangles using the given index buffer.
    /// </summary>
    /// <param name="vertices">The triangle vertices list.</param>
    /// <param name="indices">The triangle indices list (must have multiple of 3 elements).</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    static void DrawWireTriangles(const Array<Float3, HeapAllocation>& vertices, const Array<int32, HeapAllocation>& indices, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the wireframe triangles.
    /// </summary>
    /// <param name="vertices">The triangle vertices list (must have multiple of 3 elements).</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawWireTriangles(const Span<Double3>& vertices, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the wireframe triangles.
    /// </summary>
    /// <param name="vertices">The triangle vertices list (must have multiple of 3 elements).</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    static void DrawWireTriangles(const Array<Double3, HeapAllocation>& vertices, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the wireframe triangles using the given index buffer.
    /// </summary>
    /// <param name="vertices">The triangle vertices list.</param>
    /// <param name="indices">The triangle indices list (must have multiple of 3 elements).</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawWireTriangles(const Span<Double3>& vertices, const Span<int32>& indices, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the wireframe triangles using the given index buffer.
    /// </summary>
    /// <param name="vertices">The triangle vertices list.</param>
    /// <param name="indices">The triangle indices list (must have multiple of 3 elements).</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    static void DrawWireTriangles(const Array<Double3, HeapAllocation>& vertices, const Array<int32, HeapAllocation>& indices, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the wireframe box.
    /// </summary>
    /// <param name="box">The box.</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawWireBox(const BoundingBox& box, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the wireframe frustum.
    /// </summary>
    /// <param name="frustum">The frustum.</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawWireFrustum(const BoundingFrustum& frustum, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the wireframe box.
    /// </summary>
    /// <param name="box">The box.</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawWireBox(const OrientedBoundingBox& box, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the wireframe sphere.
    /// </summary>
    /// <param name="sphere">The sphere.</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawWireSphere(const BoundingSphere& sphere, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the sphere.
    /// </summary>
    /// <param name="sphere">The sphere.</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawSphere(const BoundingSphere& sphere, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the tube.
    /// [Deprecated in v1.10]
    /// </summary>
    /// <param name="position">The center position.</param>
    /// <param name="orientation">The orientation.</param>
    /// <param name="radius">The radius.</param>
    /// <param name="length">The length.</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() DEPRECATED("Depricated v1.10, use DrawCapsule instead.") static void DrawTube(const Vector3& position, const Quaternion& orientation, float radius, float length, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws a capsule.
    /// </summary>
    /// <param name="position">The center position.</param>
    /// <param name="orientation">The orientation.</param>
    /// <param name="radius">The radius.</param>
    /// <param name="length">The length.</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawCapsule(const Vector3& position, const Quaternion& orientation, float radius, float length, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the wireframe tube.
    /// [Deprecated in v1.10]
    /// </summary>
    /// <param name="position">The center position.</param>
    /// <param name="orientation">The orientation.</param>
    /// <param name="radius">The radius.</param>
    /// <param name="length">The length.</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() DEPRECATED("Depricated v1.10, use DrawWireCapsule instead.") static void DrawWireTube(const Vector3& position, const Quaternion& orientation, float radius, float length, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the wireframe capsule.
    /// </summary>
    /// <param name="position">The center position.</param>
    /// <param name="orientation">The orientation.</param>
    /// <param name="radius">The radius.</param>
    /// <param name="length">The length.</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawWireCapsule(const Vector3& position, const Quaternion& orientation, float radius, float length, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the cylinder.
    /// </summary>
    /// <param name="position">The center position.</param>
    /// <param name="orientation">The orientation.</param>
    /// <param name="radius">The radius.</param>
    /// <param name="height">The height.</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawCylinder(const Vector3& position, const Quaternion& orientation, float radius, float height, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the wireframe cylinder.
    /// </summary>
    /// <param name="position">The center position.</param>
    /// <param name="orientation">The orientation.</param>
    /// <param name="radius">The radius.</param>
    /// <param name="height">The height.</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawWireCylinder(const Vector3& position, const Quaternion& orientation, float radius, float height, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the cone.
    /// </summary>
    /// <param name="position">The center position.</param>
    /// <param name="orientation">The orientation.</param>
    /// <param name="radius">The radius.</param>
    /// <param name="angleXY">The angle (in radians) of the cone over the XY axis (cone forward is over X).</param>
    /// <param name="angleXZ">The angle (in radians) of the cone over the XZ axis (cone forward is over X).</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawCone(const Vector3& position, const Quaternion& orientation, float radius, float angleXY, float angleXZ, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the wireframe cone.
    /// </summary>
    /// <param name="position">The center position.</param>
    /// <param name="orientation">The orientation.</param>
    /// <param name="radius">The radius.</param>
    /// <param name="angleXY">The angle (in radians) of the cone over the XY axis (cone forward is over X).</param>
    /// <param name="angleXZ">The angle (in radians) of the cone over the XZ axis (cone forward is over X).</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawWireCone(const Vector3& position, const Quaternion& orientation, float radius, float angleXY, float angleXZ, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the arc.
    /// </summary>
    /// <param name="position">The center position.</param>
    /// <param name="orientation">The orientation.</param>
    /// <param name="radius">The radius.</param>
    /// <param name="angle">The angle (in radians) of the arc (arc is facing positive Z axis - forward). Use PI*2 for full disc (360 degrees).</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawArc(const Vector3& position, const Quaternion& orientation, float radius, float angle, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the wireframe arc.
    /// </summary>
    /// <param name="position">The center position.</param>
    /// <param name="orientation">The orientation.</param>
    /// <param name="radius">The radius.</param>
    /// <param name="angle">The angle (in radians) of the arc (arc is facing positive Z axis - forward). Use PI*2 for full disc (360 degrees).</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawWireArc(const Vector3& position, const Quaternion& orientation, float radius, float angle, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the wireframe arrow.
    /// </summary>
    /// <param name="position">The arrow origin position.</param>
    /// <param name="orientation">The orientation (defines the arrow direction).</param>
    /// <param name="scale">The arrow scale (used to adjust the arrow size).</param>
    /// <param name="capScale">The arrow cap scale.</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawWireArrow(const Vector3& position, const Quaternion& orientation, float scale, float capScale, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the box.
    /// </summary>
    /// <param name="box">The box.</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawBox(const BoundingBox& box, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the box.
    /// </summary>
    /// <param name="box">The box.</param>
    /// <param name="color">The color.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
    API_FUNCTION() static void DrawBox(const OrientedBoundingBox& box, const Color& color = Color::White, float duration = 0.0f, bool depthTest = true);

    /// <summary>
    /// Draws the text on a screen (2D).
    /// </summary>
    /// <param name="text">The text.</param>
    /// <param name="position">The position of the text on the screen (in screen-space coordinates).</param>
    /// <param name="color">The color.</param>
    /// <param name="size">The font size.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    API_FUNCTION() static void DrawText(const StringView& text, const Float2& position, const Color& color = Color::White, int32 size = 20, float duration = 0.0f);

    /// <summary>
    /// Draws the text (3D) that automatically faces the camera.
    /// </summary>
    /// <param name="text">The text.</param>
    /// <param name="position">The position of the text (world-space).</param>
    /// <param name="color">The color.</param>
    /// <param name="size">The font size.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    /// <param name="scale">The text scale.</param>
    API_FUNCTION() static void DrawText(const StringView& text, const Vector3& position, const Color& color = Color::White, int32 size = 32, float duration = 0.0f, float scale = 1.0f);

    /// <summary>
    /// Draws the text (3D).
    /// </summary>
    /// <param name="text">The text.</param>
    /// <param name="transform">The transformation of the text (world-space).</param>
    /// <param name="color">The color.</param>
    /// <param name="size">The font size.</param>
    /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
    API_FUNCTION() static void DrawText(const StringView& text, const Transform& transform, const Color& color = Color::White, int32 size = 32, float duration = 0.0f);

    /// <summary>
    /// Clears all debug shapes displayed on screen.
    /// </summary>
    /// <param name="context">The context.</param>
    API_FUNCTION() static void Clear(void* context = nullptr);
};

#define DEBUG_DRAW_AXIS_FROM_DIRECTION(origin, direction, size, duration, depthTest)                        DebugDraw::DrawAxisFromDirection(origin, direction, size, duration, depthTest);
#define DEBUG_DRAW_DIRECTION(origin, direction, color, duration, depthTest)                                 DebugDraw::DrawDirection(origin, direction, color, duration, depthTest);
#define DEBUG_DRAW_RAY(origin, direction, color, length, duration, depthTest)                               DebugDraw::DrawRay(origin, direction, color, length, duration, depthTest);
#define DEBUG_DRAW_RAY(ray, color, length, duration, depthTest)                                             DebugDraw::DrawRay(ray, color, length, duration, depthTest);
#define DEBUG_DRAW_LINE(start, end, color, duration, depthTest)                                             DebugDraw::DrawLine(start, end, color, duration, depthTest)
#define DEBUG_DRAW_LINES(lines, transform, color, duration, depthTest)                                      DebugDraw::DrawLines(lines, transform, color, duration, depthTest)
#define DEBUG_DRAW_BEZIER(p1, p2, p3, p4, color, duration, depthTest)                                       DebugDraw::DrawBezier(p1, p2, p3, p4, color, duration, depthTest)
#define DEBUG_DRAW_CIRCLE(position, normal, radius, color, duration, depthTest)                             DebugDraw::DrawCircle(position, normal, radius, color, duration, depthTest)
#define DEBUG_DRAW_TRIANGLE(v0, v1, v2, color, duration, depthTest)                                         DebugDraw::DrawTriangle(v0, v1, v2, color, duration, depthTest)
#define DEBUG_DRAW_TRIANGLES(vertices, color, duration, depthTest)                                          DebugDraw::DrawTriangles(vertices, color, duration, depthTest)
#define DEBUG_DRAW_TRIANGLES_EX(vertices, indices, color, duration, depthTest)                              DebugDraw::DrawTriangles(vertices, indices, color, duration, depthTest)
#define DEBUG_DRAW_TRIANGLES_EX2(vertices, indices, transform, color, duration, depthTest)                  DebugDraw::DrawTriangles(vertices, indices, transform, color, duration, depthTest)
#define DEBUG_DRAW_SPHERE(sphere, color, duration, depthTest)                                               DebugDraw::DrawSphere(sphere, color, duration, depthTest)
// [Deprecated v1.10. Use DEBUG_DRAW_CAPSULE]
#define DEBUG_DRAW_TUBE(position, orientation, radius, length, color, duration, depthTest)                  DebugDraw::DrawCapsule(position, orientation, radius, length, color, duration, depthTest)
#define DEBUG_DRAW_CAPSULE(position, orientation, radius, length, color, duration, depthTest)               DebugDraw::DrawCapsule(position, orientation, radius, length, color, duration, depthTest)
#define DEBUG_DRAW_BOX(box, color, duration, depthTest)                                                     DebugDraw::DrawBox(box, color, duration, depthTest)
#define DEBUG_DRAW_CYLINDER(position, orientation, radius, height, color, duration, depthTest)              DebugDraw::DrawCylinder(position, orientation, radius, height, color, duration, depthTest)
#define DEBUG_DRAW_CONE(position, orientation, radius, angleXY, angleXZ, color, duration, depthTest)        DebugDraw::DrawCone(position, orientation, radius, angleXY, angleXZ, color, duration, depthTest)
#define DEBUG_DRAW_ARC(position, orientation, radius, angle, color, duration, depthTest)                    DebugDraw::DrawArc(position, orientation, radius, angle, color, duration, depthTest)
#define DEBUG_DRAW_WIRE_TRIANGLE(v0, v1, v2, color, duration, depthTest)                                    DebugDraw::DrawWireTriangle(v0, v1, v2, color, duration, depthTest)
#define DEBUG_DRAW_WIRE_TRIANGLES(vertices, color, duration, depthTest)                                     DebugDraw::DrawWireTriangles(vertices, color, duration, depthTest)
#define DEBUG_DRAW_WIRE_TRIANGLES_EX(vertices, indices, color, duration, depthTest)                         DebugDraw::DrawWireTriangles(vertices, indices, color, duration, depthTest)
#define DEBUG_DRAW_WIRE_BOX(box, color, duration, depthTest)                                                DebugDraw::DrawWireBox(box, color, duration, depthTest)
#define DEBUG_DRAW_WIRE_FRUSTUM(frustum, color, duration, depthTest)                                        DebugDraw::DrawWireFrustum(frustum, color, duration, depthTest)
#define DEBUG_DRAW_WIRE_SPHERE(sphere, color, duration, depthTest)                                          DebugDraw::DrawWireSphere(sphere, color, duration, depthTest)
// [Deprecated v1.10. Use DEBUG_DRAW_WIRE_CAPSULE]
#define DEBUG_DRAW_WIRE_TUBE(position, orientation, radius, length, color, duration, depthTest)             DebugDraw::DrawWireCapsule(position, orientation, radius, length, color, duration, depthTest)
#define DEBUG_DRAW_WIRE_CAPSULE(position, orientation, radius, length, color, duration, depthTest)          DebugDraw::DrawWireCapsule(position, orientation, radius, length, color, duration, depthTest)
#define DEBUG_DRAW_WIRE_CYLINDER(position, orientation, radius, height, color, duration, depthTest)         DebugDraw::DrawWireCylinder(position, orientation, radius, height, color, duration, depthTest)
#define DEBUG_DRAW_WIRE_CONE(position, orientation, radius, angleXY, angleXZ, color, duration, depthTest)   DebugDraw::DrawWireCone(position, orientation, radius, angleXY, angleXZ, color, duration, depthTest)
#define DEBUG_DRAW_WIRE_ARC(position, orientation, radius, angle, color, duration, depthTest)               DebugDraw::DrawWireArc(position, orientation, radius, angle, color, duration, depthTest)
#define DEBUG_DRAW_WIRE_ARROW(position, orientation, scale, capScale, color, duration, depthTest)           DebugDraw::DrawWireArrow(position, orientation, scale, capScale, color, duration, depthTest)
#define DEBUG_DRAW_TEXT(text, position, color, size, duration)                                              DebugDraw::DrawText(text, position, color, size, duration)
#define DEBUG_DRAW_CLEAR(context)                                                                           DebugDraw::Clear(context)

#else

#define DEBUG_DRAW_AXIS_FROM_DIRECTION(origin, direction, size, duration, depthTest)
#define DEBUG_DRAW_DIRECTION(origin, direction,color,duration, depthTest)
#define DEBUG_DRAW_RAY(ray, color, length, duration, depthTest)
#define DEBUG_DRAW_LINE(start, end, color, duration, depthTest)
#define DEBUG_DRAW_LINES(lines, transform, color, duration, depthTest)
#define DEBUG_DRAW_BEZIER(p1, p2, p3, p4, color, duration, depthTest)
#define DEBUG_DRAW_CIRCLE(position, normal, radius, color, duration, depthTest)
#define DEBUG_DRAW_TRIANGLE(v0, v1, v2, color, duration, depthTest)
#define DEBUG_DRAW_TRIANGLES(vertices, color, duration, depthTest)
#define DEBUG_DRAW_TRIANGLES_EX(vertices, indices, color, duration, depthTest)
#define DEBUG_DRAW_TRIANGLES_EX2(vertices, indices, transform, color, duration, depthTest)
#define DEBUG_DRAW_SPHERE(sphere, color, duration, depthTest)
#define DEBUG_DRAW_TUBE(position, orientation, radius, length, color, duration, depthTest)
#define DEBUG_DRAW_BOX(box, color, duration, depthTest)
#define DEBUG_DRAW_CYLINDER(position, orientation, radius, height, color, duration, depthTest)
#define DEBUG_DRAW_CONE(position, orientation, radius, angleXY, angleXZ, color, duration, depthTest)
#define DEBUG_DRAW_ARC(position, orientation, radius, angle, color, duration, depthTest)
#define DEBUG_DRAW_WIRE_TRIANGLE(v0, v1, v2, color, duration, depthTest)
#define DEBUG_DRAW_WIRE_TRIANGLES(vertices, color, duration, depthTest)
#define DEBUG_DRAW_WIRE_TRIANGLES_EX(vertices, indices, color, duration, depthTest)
#define DEBUG_DRAW_WIRE_BOX(box, color, duration, depthTest)
#define DEBUG_DRAW_WIRE_FRUSTUM(frustum, color, duration, depthTest)
#define DEBUG_DRAW_WIRE_SPHERE(sphere, color, duration, depthTest)
#define DEBUG_DRAW_WIRE_TUBE(position, orientation, radius, length, color, duration, depthTest)
#define DEBUG_DRAW_WIRE_CYLINDER(position, orientation, radius, height, color, duration, depthTest)
#define DEBUG_DRAW_WIRE_CONE(position, orientation, radius, angleXY, angleXZ, color, duration, depthTest)
#define DEBUG_DRAW_WIRE_ARC(position, orientation, radius, angle, color, duration, depthTest)
#define DEBUG_DRAW_WIRE_ARROW(position, orientation, scale, capScale, color, duration, depthTest)
#define DEBUG_DRAW_TEXT(text, position, color, size, duration)
#define DEBUG_DRAW_CLEAR(context)

#endif
