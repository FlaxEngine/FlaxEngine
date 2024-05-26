// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.
#if !FLAX_EDITOR
using System;

namespace FlaxEngine
{
    /// <summary>
    /// The debug shapes rendering service. Not available in final game. For use only in the editor.
    /// </summary>
    [Unmanaged]
    [Tooltip("The debug shapes rendering service. Not available in final game. For use only in the editor.")]
    public static unsafe partial class DebugDraw
    {
        /// <summary>
        /// Draws the collected debug shapes to the output.
        /// </summary>
        /// <param name="renderContext">The rendering context.</param>
        /// <param name="target">The rendering output surface handle.</param>
        /// <param name="depthBuffer">The custom depth texture used for depth test. Can be MSAA. Must match target surface size.</param>
        /// <param name="enableDepthTest">True if perform manual depth test with scene depth buffer when rendering the primitives. Uses custom shader and the scene depth buffer.</param>
        public static void Draw(ref RenderContext renderContext, GPUTextureView target = null, GPUTextureView depthBuffer = null, bool enableDepthTest = false)
        {
        }

        /// <summary>
        /// Draws the debug shapes for the given collection of selected actors and other scene actors debug shapes.
        /// </summary>
        /// <param name="selectedActors">The list of actors to draw.</param>
        /// <param name="selectedActorsCount">The size of the list of actors.</param>
        public static void DrawActors(IntPtr selectedActors, int selectedActorsCount)
        {
        }

        /// <summary>
        /// Draws the lines axis from direction.
        /// </summary>
        /// <param name="origin">The origin of the line.</param>
        /// <param name="direction">The direction of the line.</param>
        /// <param name="color">The color.</param>
        /// <param name="Size">The size of the axis.</param>
        /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
        /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
        public static void DrawAxisFromDirection(Vector3 origin, Vector3 direction, Color color ,float Size = 100.0f, float duration = 0.0f, bool depthTest = true){}

        /// <summary>
        /// Draws the line in a direction.
        /// </summary>
        /// <param name="origin">The origin of the line.</param>
        /// <param name="direction">The direction of the line.</param>
        /// <param name="color">The color.</param>
        /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
        /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
        public static void DrawDirection(Vector3 origin, Vector3 direction, Color color, float duration = 0.0f, bool depthTest = true){}

        /// <summary>
        /// Draws the line in a direction.
        /// </summary>
        /// <param name="origin">The origin of the line.</param>
        /// <param name="direction">The direction of the line.</param>
        /// <param name="color">The color.</param>
        /// <param name="length">The length of the ray.</param>
        /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
        /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
        public static void DrawRay(Vector3 origin, Vector3 direction, Color color, float length = 3.402823466e+38f, float duration = 0.0f, bool depthTest = true){}

        /// <summary>
        /// Draws the line in a direction.
        /// </summary>
        /// <param name="ray">The ray.</param>
        /// <param name="color">The color.</param>
        /// <param name="length">The length of the ray.</param>
        /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
        /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
        public static void DrawRay(Ray ray,Color color, float length = 3.402823466e+38f, float duration = 0.0f, bool depthTest = true)
        {
        }

        /// <summary>
        /// Draws the line.
        /// </summary>
        /// <param name="start">The start point.</param>
        /// <param name="end">The end point.</param>
        /// <param name="color">The color.</param>
        /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
        /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
        public static void DrawLine(Vector3 start, Vector3 end, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        /// <summary>
        /// Draws the lines. Line positions are located one after another (e.g. l0.start, l0.end, l1.start, l1.end,...).
        /// </summary>
        /// <param name="lines">The list of vertices for lines (must have multiple of 2 elements).</param>
        /// <param name="transform">The custom matrix used to transform all line vertices.</param>
        /// <param name="color">The color.</param>
        /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
        /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
        public static void DrawLines(Vector3[] lines, Matrix transform, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        /// <summary>
        /// Draws the circle.
        /// </summary>
        /// <param name="position">The center position.</param>
        /// <param name="normal">The normal vector direction.</param>
        /// <param name="radius">The radius.</param>
        /// <param name="color">The color.</param>
        /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
        /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
        public static void DrawCircle(Vector3 position, Vector3 normal, float radius, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        /// <summary>
        /// Draws the wireframe triangle.
        /// </summary>
        /// <param name="v0">The first triangle vertex.</param>
        /// <param name="v1">The second triangle vertex.</param>
        /// <param name="v2">The third triangle vertex.</param>
        /// <param name="color">The color.</param>
        /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
        /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
        public static void DrawWireTriangle(Vector3 v0, Vector3 v1, Vector3 v2, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        /// <summary>
        /// Draws the triangle.
        /// </summary>
        /// <param name="v0">The first triangle vertex.</param>
        /// <param name="v1">The second triangle vertex.</param>
        /// <param name="v2">The third triangle vertex.</param>
        /// <param name="color">The color.</param>
        /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
        /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
        public static void DrawTriangle(Vector3 v0, Vector3 v1, Vector3 v2, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        /// <summary>
        /// Draws the triangles.
        /// </summary>
        /// <param name="vertices">The triangle vertices list (must have multiple of 3 elements).</param>
        /// <param name="color">The color.</param>
        /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
        /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
        public static void DrawTriangles(Vector3[] vertices, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        /// <summary>
        /// Draws the triangles using the given index buffer.
        /// </summary>
        /// <param name="vertices">The triangle vertices list.</param>
        /// <param name="indices">The triangle indices list (must have multiple of 3 elements).</param>
        /// <param name="color">The color.</param>
        /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
        /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
        public static void DrawTriangles(Vector3[] vertices, int[] indices, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        /// <summary>
        /// Draws the wireframe box.
        /// </summary>
        /// <param name="box">The box.</param>
        /// <param name="color">The color.</param>
        /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
        /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
        public static void DrawWireBox(BoundingBox box, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        /// <summary>
        /// Draws the wireframe frustum.
        /// </summary>
        /// <param name="frustum">The frustum.</param>
        /// <param name="color">The color.</param>
        /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
        /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
        public static void DrawWireFrustum(BoundingFrustum frustum, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        /// <summary>
        /// Draws the wireframe box.
        /// </summary>
        /// <param name="box">The box.</param>
        /// <param name="color">The color.</param>
        /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
        /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
        public static void DrawWireBox(OrientedBoundingBox box, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        /// <summary>
        /// Draws the wireframe sphere.
        /// </summary>
        /// <param name="sphere">The sphere.</param>
        /// <param name="color">The color.</param>
        /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
        /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
        public static void DrawWireSphere(BoundingSphere sphere, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        /// <summary>
        /// Draws the sphere.
        /// </summary>
        /// <param name="sphere">The sphere.</param>
        /// <param name="color">The color.</param>
        /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
        /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
        public static void DrawSphere(BoundingSphere sphere, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        /// <summary>
        /// Draws the wireframe tube.
        /// </summary>
        /// <param name="position">The center position.</param>
        /// <param name="orientation">The orientation.</param>
        /// <param name="radius">The radius.</param>
        /// <param name="length">The length.</param>
        /// <param name="color">The color.</param>
        /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
        /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
        public static void DrawWireTube(Vector3 position, Quaternion orientation, float radius, float length, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

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
        public static void DrawWireCylinder(Vector3 position, Quaternion orientation, float radius, float height, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

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
        public static void DrawWireArrow(Vector3 position, Quaternion orientation, float scale, float capScale, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        /// <summary>
        /// Draws the box.
        /// </summary>
        /// <param name="box">The box.</param>
        /// <param name="color">The color.</param>
        /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
        /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
        public static void DrawBox(BoundingBox box, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

        /// <summary>
        /// Draws the box.
        /// </summary>
        /// <param name="box">The box.</param>
        /// <param name="color">The color.</param>
        /// <param name="duration">The duration (in seconds). Use 0 to draw it only once.</param>
        /// <param name="depthTest">If set to <c>true</c> depth test will be performed, otherwise depth will be ignored.</param>
        public static void DrawBox(OrientedBoundingBox box, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

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
        public static void DrawArc(Vector3 position, Quaternion orientation, float radius, float angle, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

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
        public static void DrawWireArc(Vector3 position, Quaternion orientation, float radius, float angle, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

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
        public static void DrawCone(Vector3 position, Quaternion orientation, float radius, float angleXY, float angleXZ, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }

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
        public static void DrawWireCone(Vector3 position, Quaternion orientation, float radius, float angleXY, float angleXZ, Color color, float duration = 0.0f, bool depthTest = true)
        {
        }
    }
}
#endif
