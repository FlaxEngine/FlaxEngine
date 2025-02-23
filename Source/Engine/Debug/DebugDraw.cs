// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.
#if !FLAX_EDITOR
using System;

#pragma warning disable 1591

namespace FlaxEngine
{
    /// <summary>
    /// The debug shapes rendering service. Not available in final game. For use only in the editor.
    /// </summary>
    [Unmanaged]
    [Tooltip("The debug shapes rendering service. Not available in final game. For use only in the editor.")]
    public static unsafe partial class DebugDraw
    {
        public static void Draw(ref RenderContext renderContext, GPUTextureView target = null, GPUTextureView depthBuffer = null, bool enableDepthTest = false)
        {
        }

        public static void DrawActors(IntPtr selectedActors, int selectedActorsCount, bool drawScenes)
        {
        }

        public static void DrawAxisFromDirection(Vector3 origin, Vector3 direction, float size = 100.0f, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawDirection(Vector3 origin, Vector3 direction, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        [Obsolete("Use DrawRay with length parameter instead")]
        public static void DrawRay(Vector3 origin, Vector3 direction, Color color, float duration, bool depthTest)
        {
        }

        public static void DrawRay(Vector3 origin, Vector3 direction, Color color, float length = float.MaxValue, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawRay(Ray ray, Color color, float length = float.MaxValue, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawLine(Vector3 start, Vector3 end, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawLine(Vector3 start, Vector3 end, Color startColor, Color endColor, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawLines(Float3[] lines, Matrix transform, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawLines(GPUBuffer lines, Matrix transform, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawLines(Double3[] lines, Matrix transform, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawBezier(Vector3 p1, Vector3 p2, Vector3 p3, Vector3 p4, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawCircle(Vector3 position, Float3 normal, float radius, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawWireTriangle(Vector3 v0, Vector3 v1, Vector3 v2, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawTriangle(Vector3 v0, Vector3 v1, Vector3 v2, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawTriangles(Float3[] vertices, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawTriangles(Float3[] vertices, Matrix transform, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawTriangles(Float3[] vertices, int[] indices, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawTriangles(Float3[] vertices, int[] indices, Matrix transform, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawTriangles(Double3[] vertices, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawTriangles(Double3[] vertices, Matrix transform, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawTriangles(Double3[] vertices, int[] indices, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawTriangles(Double3[] vertices, int[] indices, Matrix transform, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawWireTriangles(Float3[] vertices, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawWireTriangles(Float3[] vertices, int[] indices, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawWireTriangles(Double3[] vertices, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawWireTriangles(Double3[] vertices, int[] indices, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawWireBox(BoundingBox box, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawWireFrustum(BoundingFrustum frustum, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawWireBox(OrientedBoundingBox box, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawWireSphere(BoundingSphere sphere, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        [Unmanaged]
        public static void DrawSphere(BoundingSphere sphere, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        [Obsolete("Depricated v1.10, use DrawCapsule instead.")]
        public static void DrawTube(Vector3 position, Quaternion orientation, float radius, float length, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawCapsule(Vector3 position, Quaternion orientation, float radius, float length, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        [Obsolete("Depricated v1.10, use DrawWireCapsule instead.")]
        public static void DrawWireTube(Vector3 position, Quaternion orientation, float radius, float length, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawWireCapsule(Vector3 position, Quaternion orientation, float radius, float length, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawCylinder(Vector3 position, Quaternion orientation, float radius, float height, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawWireCylinder(Vector3 position, Quaternion orientation, float radius, float height, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawCone(Vector3 position, Quaternion orientation, float radius, float angleXY, float angleXZ, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawWireCone(Vector3 position, Quaternion orientation, float radius, float angleXY, float angleXZ, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawArc(Vector3 position, Quaternion orientation, float radius, float angle, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawWireArc(Vector3 position, Quaternion orientation, float radius, float angle, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawWireArrow(Vector3 position, Quaternion orientation, float scale, float capScale, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawBox(BoundingBox box, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawBox(OrientedBoundingBox box, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        public static void DrawText(string text, Float2 position, Color color, int size = 20, float duration = 0.0f)
        {
        }

        public static void DrawText(string text, Vector3 position, Color color, int size = 32, float duration = 0.0f, float scale = 1.0f)
        {
        }

        public static void DrawText(string text, Transform transform, Color color, int size = 32, float duration = 0.0f)
        {
        }

        public static void Clear(IntPtr context = new IntPtr())
        {
        }
    }
}
#endif
