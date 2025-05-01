// Copyright (c) Wojciech Figat. All rights reserved.

#if USE_LARGE_WORLDS
using Real = System.Double;
#else
using Real = System.Single;
#endif

// -----------------------------------------------------------------------------
// Original code from SharpDX project. https://github.com/sharpdx/SharpDX/
// Greetings to Alexandre Mutel. Original code published with the following license:
// -----------------------------------------------------------------------------
// Copyright (c) 2010-2014 SharpDX - Alexandre Mutel
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// -----------------------------------------------------------------------------
// Original code from SlimMath project. http://code.google.com/p/slimmath/
// Greetings to SlimDX Group. Original code published with the following license:
// -----------------------------------------------------------------------------
/*
* Copyright (c) 2007-2011 SlimDX Group
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*/
using System;

namespace FlaxEngine
{
    /// <summary>
    /// Describes how one bounding volume contains another.
    /// </summary>
    public enum ContainmentType
    {
        /// <summary>
        /// The two bounding volumes don't intersect at all.
        /// </summary>
        Disjoint,

        /// <summary>
        /// One bounding volume completely contains another.
        /// </summary>
        Contains,

        /// <summary>
        /// The two bounding volumes overlap.
        /// </summary>
        Intersects
    }

    /// <summary>
    /// Describes the result of an intersection with a plane in three dimensions.
    /// </summary>
    public enum PlaneIntersectionType
    {
        /// <summary>
        /// The object is behind the plane.
        /// </summary>
        Back,

        /// <summary>
        /// The object is in front of the plane.
        /// </summary>
        Front,

        /// <summary>
        /// The object is intersecting the plane.
        /// </summary>
        Intersecting
    }

    /*
     * This class is organized so that the least complex objects come first so that the least
     * complex objects will have the most methods in most cases. Note that not all shapes exist
     * at this time and not all shapes have a corresponding struct. Only the objects that have
     * a corresponding struct should come first in naming and in parameter order. The order of
     * complexity is as follows:
     * 
     * 1. Point
     * 2. Ray
     * 3. Segment
     * 4. Plane
     * 5. Triangle
     * 6. Polygon
     * 7. Box
     * 8. Sphere
     * 9. Ellipsoid
     * 10. Cylinder
     * 11. Cone
     * 12. Capsule
     * 13. Torus
     * 14. Polyhedron
     * 15. Frustum
    */

    /// <summary>
    /// Contains static methods to help in determining intersections, containment, etc.
    /// </summary>
    public static class CollisionsHelper
    {
        /// <summary>
        /// Determines the closest point between a point and a line segment.
        /// </summary>
        /// <param name="point">The point to test.</param>
        /// <param name="p0">The line first point.</param>
        /// <param name="p1">The line second point.</param>
        /// <param name="result">When the method completes, contains the closest point between the two objects.</param>
        public static void ClosestPointPointLine(ref Float2 point, ref Float2 p0, ref Float2 p1, out Float2 result)
        {
            var p = point - p0;
            var n = p1 - p0;

            float length = n.Length;
            if (length < 1e-10)
            {
                // Both points are the same, just give any
                result = p0;
                return;
            }
            n /= length;

            float dot = Float2.Dot(n, p);
            if (dot <= 0.0)
            {
                // Before first point
                result = p0;
            }
            else if (dot >= length)
            {
                // After first point
                result = p1;
            }
            else
            {
                // Inside
                result = p0 + n * dot;
            }
        }

        /// <summary>
        /// Determines the closest point between a point and a line.
        /// </summary>
        /// <param name="point">The point to test.</param>
        /// <param name="p0">The line first point.</param>
        /// <param name="p1">The line second point.</param>
        /// <param name="result">When the method completes, contains the closest point between the two objects.</param>
        public static void ClosestPointPointLine(ref Vector3 point, ref Vector3 p0, ref Vector3 p1, out Vector3 result)
        {
            Vector3 p = point - p0;
            Vector3 n = p1 - p0;
            Real length = n.Length;
            if (length < 1e-10f)
            {
                result = p0;
                return;
            }
            n /= length;
            Real dot = Vector3.Dot(ref n, ref p);
            if (dot <= 0.0f)
            {
                result = p0;
                return;
            }
            if (dot >= length)
            {
                result = p1;
                return;
            }
            result = p0 + n * dot;
        }

        /// <summary>
        /// Determines the closest point between a point and a triangle.
        /// </summary>
        /// <param name="point">The point to test.</param>
        /// <param name="vertex1">The first vertex to test.</param>
        /// <param name="vertex2">The second vertex to test.</param>
        /// <param name="vertex3">The third vertex to test.</param>
        /// <param name="result">When the method completes, contains the closest point between the two objects.</param>
        public static void ClosestPointPointTriangle(ref Vector3 point, ref Vector3 vertex1, ref Vector3 vertex2, ref Vector3 vertex3, out Vector3 result)
        {
            //Source: Real-Time Collision Detection by Christer Ericson
            //Reference: Page 136

            //Check if P in vertex region outside A
            Vector3 ab = vertex2 - vertex1;
            Vector3 ac = vertex3 - vertex1;
            Vector3 ap = point - vertex1;

            Real d1 = Vector3.Dot(ab, ap);
            Real d2 = Vector3.Dot(ac, ap);
            if ((d1 <= 0.0f) && (d2 <= 0.0f))
            {
                result = vertex1; //Barycentric coordinates (1,0,0)
                return;
            }

            //Check if P in vertex region outside B
            Vector3 bp = point - vertex2;
            Real d3 = Vector3.Dot(ab, bp);
            Real d4 = Vector3.Dot(ac, bp);
            if ((d3 >= 0.0f) && (d4 <= d3))
            {
                result = vertex2; // Barycentric coordinates (0,1,0)
                return;
            }

            //Check if P in edge region of AB, if so return projection of P onto AB
            Real vc = d1 * d4 - d3 * d2;
            if ((vc <= 0.0f) && (d1 >= 0.0f) && (d3 <= 0.0f))
            {
                Real v = d1 / (d1 - d3);
                result = vertex1 + v * ab; //Barycentric coordinates (1-v,v,0)
                return;
            }

            //Check if P in vertex region outside C
            Vector3 cp = point - vertex3;
            Real d5 = Vector3.Dot(ab, cp);
            Real d6 = Vector3.Dot(ac, cp);
            if ((d6 >= 0.0f) && (d5 <= d6))
            {
                result = vertex3; //Barycentric coordinates (0,0,1)
                return;
            }

            //Check if P in edge region of AC, if so return projection of P onto AC
            Real vb = d5 * d2 - d1 * d6;
            if ((vb <= 0.0f) && (d2 >= 0.0f) && (d6 <= 0.0f))
            {
                Real w = d2 / (d2 - d6);
                result = vertex1 + w * ac; //Barycentric coordinates (1-w,0,w)
                return;
            }

            //Check if P in edge region of BC, if so return projection of P onto BC
            Real va = d3 * d6 - d5 * d4;
            if ((va <= 0.0f) && (d4 - d3 >= 0.0f) && (d5 - d6 >= 0.0f))
            {
                Real w = (d4 - d3) / (d4 - d3 + (d5 - d6));
                result = vertex2 + w * (vertex3 - vertex2); //Barycentric coordinates (0,1-w,w)
                return;
            }

            //P inside face region. Compute Q through its Barycentric coordinates (u,v,w)
            Real denom = 1.0f / (va + vb + vc);
            Real v2 = vb * denom;
            Real w2 = vc * denom;
            result = vertex1 + ab * v2 + ac * w2; //= u*vertex1 + v*vertex2 + w*vertex3, u = va * denom = 1.0f - v - w
        }

        /// <summary>
        /// Determines the closest point between a <see cref="Plane" /> and a point.
        /// </summary>
        /// <param name="plane">The plane to test.</param>
        /// <param name="point">The point to test.</param>
        /// <param name="result">When the method completes, contains the closest point between the two objects.</param>
        public static void ClosestPointPlanePoint(ref Plane plane, ref Vector3 point, out Vector3 result)
        {
            //Source: Real-Time Collision Detection by Christer Ericson
            //Reference: Page 126

            Vector3.Dot(ref plane.Normal, ref point, out Real dot);
            Real t = dot - plane.D;

            result = point - t * plane.Normal;
        }

        /// <summary>
        /// Determines the closest point between a <see cref="BoundingBox" /> and a point.
        /// </summary>
        /// <param name="box">The box to test.</param>
        /// <param name="point">The point to test.</param>
        /// <param name="result">When the method completes, contains the closest point between the two objects.</param>
        public static void ClosestPointBoxPoint(ref BoundingBox box, ref Vector3 point, out Vector3 result)
        {
            //Source: Real-Time Collision Detection by Christer Ericson
            //Reference: Page 130

            Vector3.Max(ref point, ref box.Minimum, out Vector3 temp);
            Vector3.Min(ref temp, ref box.Maximum, out result);
        }

        /// <summary>
        /// Determines the closest point between a <see cref="Rectangle" /> and a point.
        /// </summary>
        /// <param name="rect">The rectangle to test.</param>
        /// <param name="point">The point to test.</param>
        /// <param name="result">When the method completes, contains the closest point between the two objects.</param>
        public static void ClosestPointRectanglePoint(ref Rectangle rect, ref Float2 point, out Float2 result)
        {
            Float2 end = rect.Location + rect.Size;
            Float2.Max(ref point, ref rect.Location, out var temp);
            Float2.Min(ref temp, ref end, out result);
        }

        /// <summary>
        /// Determines the closest point between a <see cref="BoundingSphere" /> and a point.
        /// </summary>
        /// <param name="sphere"></param>
        /// <param name="point">The point to test.</param>
        /// <param name="result">When the method completes, contains the closest point between the two objects; or, if the point is directly in the center of the sphere, contains <see cref="Vector3.Zero" />.
        /// </param>
        public static void ClosestPointSpherePoint(ref BoundingSphere sphere, ref Vector3 point, out Vector3 result)
        {
            //Source: Jorgy343
            //Reference: None

            //Get the unit direction from the sphere's center to the point.
            Vector3.Subtract(ref point, ref sphere.Center, out result);
            result.Normalize();

            //Multiply the unit direction by the sphere's radius to get a vector
            //the length of the sphere.
            result *= sphere.Radius;

            //Add the sphere's center to the direction to get a point on the sphere.
            result += sphere.Center;
        }

        /// <summary>
        /// Determines the closest point between a <see cref="BoundingSphere" /> and a <see cref="BoundingSphere" />.
        /// </summary>
        /// <param name="sphere1">The first sphere to test.</param>
        /// <param name="sphere2">The second sphere to test.</param>
        /// <param name="result">When the method completes, contains the closest point between the two objects; or, if the point is directly in the center of the sphere, contains <see cref="Vector3.Zero" />.</param>
        /// <remarks>
        /// If the two spheres are overlapping, but not directly on top of each other, the closest point
        /// is the 'closest' point of intersection. This can also be considered is the deepest point of
        /// intersection.
        /// </remarks>
        public static void ClosestPointSphereSphere(ref BoundingSphere sphere1, ref BoundingSphere sphere2, out Vector3 result)
        {
            //Source: Jorgy343
            //Reference: None

            //Get the unit direction from the first sphere's center to the second sphere's center.
            Vector3.Subtract(ref sphere2.Center, ref sphere1.Center, out result);
            result.Normalize();

            //Multiply the unit direction by the first sphere's radius to get a vector
            //the length of the first sphere.
            result *= sphere1.Radius;

            //Add the first sphere's center to the direction to get a point on the first sphere.
            result += sphere1.Center;
        }

        /// <summary>
        /// Determines the distance between a <see cref="Plane" /> and a point.
        /// </summary>
        /// <param name="plane">The plane to test.</param>
        /// <param name="point">The point to test.</param>
        /// <returns>The distance between the two objects.</returns>
        public static Real DistancePlanePoint(ref Plane plane, ref Vector3 point)
        {
            //Source: Real-Time Collision Detection by Christer Ericson
            //Reference: Page 127

            Vector3.Dot(ref plane.Normal, ref point, out Real dot);
            return dot - plane.D;
        }

        /// <summary>
        /// Determines the distance between a <see cref="BoundingBox" /> and a point.
        /// </summary>
        /// <param name="box">The box to test.</param>
        /// <param name="point">The point to test.</param>
        /// <returns>The distance between the two objects.</returns>
        public static Real DistanceBoxPoint(ref BoundingBox box, ref Vector3 point)
        {
            //Source: Real-Time Collision Detection by Christer Ericson
            //Reference: Page 131

            Real distance = 0f;
            if (point.X < box.Minimum.X)
                distance += (box.Minimum.X - point.X) * (box.Minimum.X - point.X);
            if (point.X > box.Maximum.X)
                distance += (point.X - box.Maximum.X) * (point.X - box.Maximum.X);

            if (point.Y < box.Minimum.Y)
                distance += (box.Minimum.Y - point.Y) * (box.Minimum.Y - point.Y);
            if (point.Y > box.Maximum.Y)
                distance += (point.Y - box.Maximum.Y) * (point.Y - box.Maximum.Y);

            if (point.Z < box.Minimum.Z)
                distance += (box.Minimum.Z - point.Z) * (box.Minimum.Z - point.Z);
            if (point.Z > box.Maximum.Z)
                distance += (point.Z - box.Maximum.Z) * (point.Z - box.Maximum.Z);

            return (Real)Math.Sqrt(distance);
        }

        /// <summary>
        /// Determines the distance between a <see cref="BoundingBox" /> and a <see cref="BoundingBox" />.
        /// </summary>
        /// <param name="box1">The first box to test.</param>
        /// <param name="box2">The second box to test.</param>
        /// <returns>The distance between the two objects.</returns>
        public static Real DistanceBoxBox(ref BoundingBox box1, ref BoundingBox box2)
        {
            //Source:
            //Reference:

            Real distance = 0f;

            //Distance for X.
            if (box1.Minimum.X > box2.Maximum.X)
            {
                Real delta = box2.Maximum.X - box1.Minimum.X;
                distance += delta * delta;
            }
            else if (box2.Minimum.X > box1.Maximum.X)
            {
                Real delta = box1.Maximum.X - box2.Minimum.X;
                distance += delta * delta;
            }

            //Distance for Y.
            if (box1.Minimum.Y > box2.Maximum.Y)
            {
                Real delta = box2.Maximum.Y - box1.Minimum.Y;
                distance += delta * delta;
            }
            else if (box2.Minimum.Y > box1.Maximum.Y)
            {
                Real delta = box1.Maximum.Y - box2.Minimum.Y;
                distance += delta * delta;
            }

            //Distance for Z.
            if (box1.Minimum.Z > box2.Maximum.Z)
            {
                Real delta = box2.Maximum.Z - box1.Minimum.Z;
                distance += delta * delta;
            }
            else if (box2.Minimum.Z > box1.Maximum.Z)
            {
                Real delta = box1.Maximum.Z - box2.Minimum.Z;
                distance += delta * delta;
            }

            return (Real)Math.Sqrt(distance);
        }

        /// <summary>
        /// Determines the distance between a <see cref="BoundingSphere" /> and a point.
        /// </summary>
        /// <param name="sphere">The sphere to test.</param>
        /// <param name="point">The point to test.</param>
        /// <returns>The distance between the two objects.</returns>
        public static Real DistanceSpherePoint(ref BoundingSphere sphere, ref Vector3 point)
        {
            //Source: Jorgy343
            //Reference: None

            Vector3.Distance(ref sphere.Center, ref point, out Real distance);
            distance -= sphere.Radius;
            return Math.Max(distance, 0f);
        }

        /// <summary>
        /// Determines the distance between a <see cref="BoundingSphere" /> and a <see cref="BoundingSphere" />.
        /// </summary>
        /// <param name="sphere1">The first sphere to test.</param>
        /// <param name="sphere2">The second sphere to test.</param>
        /// <returns>The distance between the two objects.</returns>
        public static Real DistanceSphereSphere(ref BoundingSphere sphere1, ref BoundingSphere sphere2)
        {
            //Source: Jorgy343
            //Reference: None

            Vector3.Distance(ref sphere1.Center, ref sphere2.Center, out Real distance);
            distance -= sphere1.Radius + sphere2.Radius;
            return Math.Max(distance, 0f);
        }

        /// <summary>
        /// Determines whether there is an intersection between a <see cref="Ray" /> and a point.
        /// </summary>
        /// <param name="ray">The ray to test.</param>
        /// <param name="point">The point to test.</param>
        /// <returns>Whether the two objects intersect.</returns>
        public static bool RayIntersectsPoint(ref Ray ray, ref Vector3 point)
        {
            //Source: RayIntersectsSphere
            //Reference: None

            Vector3.Subtract(ref ray.Position, ref point, out Vector3 m);

            //Same thing as RayIntersectsSphere except that the radius of the sphere (point) is the epsilon for zero.
            Real b = Vector3.Dot(m, ray.Direction);
            Real c = Vector3.Dot(m, m) - Mathf.Epsilon;
            if ((c > 0f) && (b > 0f))
                return false;
            Real discriminant = b * b - c;
            if (discriminant < 0f)
                return false;
            return true;
        }

        /// <summary>
        /// Determines whether there is an intersection between a <see cref="Ray" /> and a <see cref="Ray" />.
        /// </summary>
        /// <param name="ray1">The first ray to test.</param>
        /// <param name="ray2">The second ray to test.</param>
        /// <param name="point">When the method completes, contains the point of intersection, or <see cref="Vector3.Zero" /> if there was no intersection.</param>
        /// <returns>Whether the two objects intersect.</returns>
        /// <remarks>
        /// This method performs a ray vs ray intersection test based on the following formula
        /// from Goldman.
        /// <code>s = det([o_2 - o_1, d_2, d_1 x d_2]) / ||d_1 x d_2||^2</code>
        /// <code>t = det([o_2 - o_1, d_1, d_1 x d_2]) / ||d_1 x d_2||^2</code>
        /// Where o_1 is the position of the first ray, o_2 is the position of the second ray,
        /// d_1 is the normalized direction of the first ray, d_2 is the normalized direction
        /// of the second ray, det denotes the determinant of a matrix, x denotes the cross
        /// product, [ ] denotes a matrix, and || || denotes the length or magnitude of a vector.
        /// </remarks>
        public static bool RayIntersectsRay(ref Ray ray1, ref Ray ray2, out Vector3 point)
        {
            //Source: Real-Time Rendering, Third Edition
            //Reference: Page 780

            Vector3.Cross(ref ray1.Direction, ref ray2.Direction, out Vector3 cross);
            Real denominator = cross.Length;

            //Lines are parallel.
            if (Mathf.IsZero(denominator))
                if (Mathf.NearEqual(ray2.Position.X, ray1.Position.X) &&
                    Mathf.NearEqual(ray2.Position.Y, ray1.Position.Y) &&
                    Mathf.NearEqual(ray2.Position.Z, ray1.Position.Z))
                {
                    point = Vector3.Zero;
                    return true;
                }

            denominator *= denominator;

            //3x3 matrix for the first ray.
            Real m11 = ray2.Position.X - ray1.Position.X;
            Real m12 = ray2.Position.Y - ray1.Position.Y;
            Real m13 = ray2.Position.Z - ray1.Position.Z;
            Real m21 = ray2.Direction.X;
            Real m22 = ray2.Direction.Y;
            Real m23 = ray2.Direction.Z;
            Real m31 = cross.X;
            Real m32 = cross.Y;
            Real m33 = cross.Z;

            //Determinant of first matrix.
            Real dets =
            m11 * m22 * m33 +
            m12 * m23 * m31 +
            m13 * m21 * m32 -
            m11 * m23 * m32 -
            m12 * m21 * m33 -
            m13 * m22 * m31;

            //3x3 matrix for the second ray.
            m21 = ray1.Direction.X;
            m22 = ray1.Direction.Y;
            m23 = ray1.Direction.Z;

            //Determinant of the second matrix.
            Real dett =
            m11 * m22 * m33 +
            m12 * m23 * m31 +
            m13 * m21 * m32 -
            m11 * m23 * m32 -
            m12 * m21 * m33 -
            m13 * m22 * m31;

            //t values of the point of intersection.
            Real s = dets / denominator;
            Real t = dett / denominator;

            //The points of intersection.
            Vector3 point1 = ray1.Position + s * ray1.Direction;
            Vector3 point2 = ray2.Position + t * ray2.Direction;

            //If the points are not equal, no intersection has occurred.
            if (!Mathf.NearEqual(point2.X, point1.X) ||
                !Mathf.NearEqual(point2.Y, point1.Y) ||
                !Mathf.NearEqual(point2.Z, point1.Z))
            {
                point = Vector3.Zero;
                return false;
            }

            point = point1;
            return true;
        }

        /// <summary>
        /// Determines whether there is an intersection between a <see cref="Ray" /> and a <see cref="Plane" />.
        /// </summary>
        /// <param name="ray">The ray to test.</param>
        /// <param name="plane">The plane to test.</param>
        /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.</param>
        /// <returns>Whether the two objects intersect.</returns>
        public static bool RayIntersectsPlane(ref Ray ray, ref Plane plane, out Real distance)
        {
            //Source: Real-Time Collision Detection by Christer Ericson
            //Reference: Page 175

            Vector3.Dot(ref plane.Normal, ref ray.Direction, out Real direction);
            if (Mathf.IsZero(direction))
            {
                distance = 0f;
                return false;
            }
            Vector3.Dot(ref plane.Normal, ref ray.Position, out Real position);
            distance = (-plane.D - position) / direction;
            if (distance < 0f)
            {
                distance = 0f;
                return false;
            }
            return true;
        }

#if USE_LARGE_WORLDS
        /// <summary>
        /// Determines whether there is an intersection between a <see cref="Ray" /> and a <see cref="Plane" />.
        /// [Deprecated on 26.05.2022, expires on 26.05.2024]
        /// </summary>
        /// <param name="ray">The ray to test.</param>
        /// <param name="plane">The plane to test.</param>
        /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.</param>
        /// <returns>Whether the two objects intersect.</returns>
        [Obsolete("Use RayIntersectsPlane with 'out Real distance' parameter instead")]
        public static bool RayIntersectsPlane(ref Ray ray, ref Plane plane, out float distance)
        {
            //Source: Real-Time Collision Detection by Christer Ericson
            //Reference: Page 175

            Vector3.Dot(ref plane.Normal, ref ray.Direction, out Real direction);
            if (Mathf.IsZero(direction))
            {
                distance = 0f;
                return false;
            }
            Vector3.Dot(ref plane.Normal, ref ray.Position, out Real position);
            distance = (float)((-plane.D - position) / direction);
            if (distance < 0f)
            {
                distance = 0f;
                return false;
            }
            return true;
        }
#endif

        /// <summary>
        /// Determines whether there is an intersection between a <see cref="Ray" /> and a <see cref="Plane" />.
        /// </summary>
        /// <param name="ray">The ray to test.</param>
        /// <param name="plane">The plane to test</param>
        /// <param name="point">When the method completes, contains the point of intersection, or <see cref="Vector3.Zero" /> if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public static bool RayIntersectsPlane(ref Ray ray, ref Plane plane, out Vector3 point)
        {
            //Source: Real-Time Collision Detection by Christer Ericson
            //Reference: Page 175
            if (!RayIntersectsPlane(ref ray, ref plane, out Real distance))
            {
                point = Vector3.Zero;
                return false;
            }
            point = ray.Position + ray.Direction * distance;
            return true;
        }

        /// <summary>
        /// Determines whether there is an intersection between a <see cref="Ray" /> and a triangle.
        /// </summary>
        /// <param name="ray">The ray to test.</param>
        /// <param name="vertex1">The first vertex of the triangle to test.</param>
        /// <param name="vertex2">The second vertex of the triangle to test.</param>
        /// <param name="vertex3">The third vertex of the triangle to test.</param>
        /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        /// <remarks>
        /// This method tests if the ray intersects either the front or back of the triangle.
        /// If the ray is parallel to the triangle's plane, no intersection is assumed to have
        /// happened. If the intersection of the ray and the triangle is behind the origin of
        /// the ray, no intersection is assumed to have happened. In both cases of assumptions,
        /// this method returns false.
        /// </remarks>
        public static bool RayIntersectsTriangle(ref Ray ray, ref Vector3 vertex1, ref Vector3 vertex2, ref Vector3 vertex3, out Real distance)
        {
            //Source: Fast Minimum Storage Ray / Triangle Intersection
            //Reference: http://www.cs.virginia.edu/~gfx/Courses/2003/ImageSynthesis/papers/Acceleration/Fast%20MinimumStorage%20RayTriangle%20Intersection.pdf

            //Compute vectors along two edges of the triangle.
            Vector3 edge1, edge2;

            //Edge 1
            edge1.X = vertex2.X - vertex1.X;
            edge1.Y = vertex2.Y - vertex1.Y;
            edge1.Z = vertex2.Z - vertex1.Z;

            //Edge2
            edge2.X = vertex3.X - vertex1.X;
            edge2.Y = vertex3.Y - vertex1.Y;
            edge2.Z = vertex3.Z - vertex1.Z;

            //Cross product of ray direction and edge2 - first part of determinant.
            Vector3 directioncrossedge2;
            directioncrossedge2.X = ray.Direction.Y * edge2.Z - ray.Direction.Z * edge2.Y;
            directioncrossedge2.Y = ray.Direction.Z * edge2.X - ray.Direction.X * edge2.Z;
            directioncrossedge2.Z = ray.Direction.X * edge2.Y - ray.Direction.Y * edge2.X;

            //Compute the determinant.
            Real determinant;
            //Dot product of edge1 and the first part of determinant.
            determinant = edge1.X * directioncrossedge2.X + edge1.Y * directioncrossedge2.Y + edge1.Z * directioncrossedge2.Z;

            //If the ray is parallel to the triangle plane, there is no collision.
            //This also means that we are not culling, the ray may hit both the
            //back and the front of the triangle.
            if (Mathf.IsZero(determinant))
            {
                distance = 0f;
                return false;
            }

            Real inversedeterminant = 1.0f / determinant;

            //Calculate the U parameter of the intersection point.
            Vector3 distanceVector;
            distanceVector.X = ray.Position.X - vertex1.X;
            distanceVector.Y = ray.Position.Y - vertex1.Y;
            distanceVector.Z = ray.Position.Z - vertex1.Z;

            Real triangleU;
            triangleU = distanceVector.X * directioncrossedge2.X + distanceVector.Y * directioncrossedge2.Y + distanceVector.Z * directioncrossedge2.Z;
            triangleU *= inversedeterminant;

            //Make sure it is inside the triangle.
            if ((triangleU < 0f) || (triangleU > 1f))
            {
                distance = 0f;
                return false;
            }

            //Calculate the V parameter of the intersection point.
            Vector3 distancecrossedge1;
            distancecrossedge1.X = distanceVector.Y * edge1.Z - distanceVector.Z * edge1.Y;
            distancecrossedge1.Y = distanceVector.Z * edge1.X - distanceVector.X * edge1.Z;
            distancecrossedge1.Z = distanceVector.X * edge1.Y - distanceVector.Y * edge1.X;

            Real triangleV;
            triangleV = ray.Direction.X * distancecrossedge1.X + ray.Direction.Y * distancecrossedge1.Y + ray.Direction.Z * distancecrossedge1.Z;
            triangleV *= inversedeterminant;

            //Make sure it is inside the triangle.
            if ((triangleV < 0f) || (triangleU + triangleV > 1f))
            {
                distance = 0f;
                return false;
            }

            //Compute the distance along the ray to the triangle.
            Real raydistance;
            raydistance = edge2.X * distancecrossedge1.X + edge2.Y * distancecrossedge1.Y + edge2.Z * distancecrossedge1.Z;
            raydistance *= inversedeterminant;

            //Is the triangle behind the ray origin?
            if (raydistance < 0f)
            {
                distance = 0f;
                return false;
            }

            distance = raydistance;
            return true;
        }

        /// <summary>
        /// Determines whether there is an intersection between a <see cref="Ray" /> and a triangle.
        /// </summary>
        /// <param name="ray">The ray to test.</param>
        /// <param name="vertex1">The first vertex of the triangle to test.</param>
        /// <param name="vertex2">The second vertex of the triangle to test.</param>
        /// <param name="vertex3">The third vertex of the triangle to test.</param>
        /// <param name="point">When the method completes, contains the point of intersection,
        /// or <see cref="Vector3.Zero" /> if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public static bool RayIntersectsTriangle(ref Ray ray, ref Vector3 vertex1, ref Vector3 vertex2, ref Vector3 vertex3, out Vector3 point)
        {
            if (!RayIntersectsTriangle(ref ray, ref vertex1, ref vertex2, ref vertex3, out Real distance))
            {
                point = Vector3.Zero;
                return false;
            }
            point = ray.Position + ray.Direction * distance;
            return true;
        }

        /// <summary>
        /// Determines whether there is an intersection between a <see cref="Ray" /> and a <see cref="BoundingBox" />.
        /// </summary>
        /// <param name="ray">The ray to test.</param>
        /// <param name="box">The box to test.</param>
        /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.
        /// </param>
        /// <returns>Whether the two objects intersected.</returns>
        public static bool RayIntersectsBox(ref Ray ray, ref BoundingBox box, out Real distance)
        {
            //Source: Real-Time Collision Detection by Christer Ericson
            //Reference: Page 179

            distance = 0f;
            Real tmax = Real.MaxValue;

            if (Mathf.IsZero(ray.Direction.X))
            {
                if ((ray.Position.X < box.Minimum.X) || (ray.Position.X > box.Maximum.X))
                {
                    distance = 0f;
                    return false;
                }
            }
            else
            {
                Real inverse = 1.0f / ray.Direction.X;
                Real t1 = (box.Minimum.X - ray.Position.X) * inverse;
                Real t2 = (box.Maximum.X - ray.Position.X) * inverse;

                if (t1 > t2)
                {
                    Real temp = t1;
                    t1 = t2;
                    t2 = temp;
                }

                distance = Math.Max(t1, distance);
                tmax = Math.Min(t2, tmax);

                if (distance > tmax)
                {
                    distance = 0f;
                    return false;
                }
            }

            if (Mathf.IsZero(ray.Direction.Y))
            {
                if ((ray.Position.Y < box.Minimum.Y) || (ray.Position.Y > box.Maximum.Y))
                {
                    distance = 0f;
                    return false;
                }
            }
            else
            {
                Real inverse = 1.0f / ray.Direction.Y;
                Real t1 = (box.Minimum.Y - ray.Position.Y) * inverse;
                Real t2 = (box.Maximum.Y - ray.Position.Y) * inverse;

                if (t1 > t2)
                {
                    Real temp = t1;
                    t1 = t2;
                    t2 = temp;
                }

                distance = Math.Max(t1, distance);
                tmax = Math.Min(t2, tmax);

                if (distance > tmax)
                {
                    distance = 0f;
                    return false;
                }
            }

            if (Mathf.IsZero(ray.Direction.Z))
            {
                if ((ray.Position.Z < box.Minimum.Z) || (ray.Position.Z > box.Maximum.Z))
                {
                    distance = 0f;
                    return false;
                }
            }
            else
            {
                Real inverse = 1.0f / ray.Direction.Z;
                Real t1 = (box.Minimum.Z - ray.Position.Z) * inverse;
                Real t2 = (box.Maximum.Z - ray.Position.Z) * inverse;

                if (t1 > t2)
                {
                    Real temp = t1;
                    t1 = t2;
                    t2 = temp;
                }

                distance = Math.Max(t1, distance);
                tmax = Math.Min(t2, tmax);

                if (distance > tmax)
                {
                    distance = 0f;
                    return false;
                }
            }

            return true;
        }

        /// <summary>
        /// Determines whether there is an intersection between a <see cref="Ray" /> and a <see cref="Plane" />.
        /// </summary>
        /// <param name="ray">The ray to test.</param>
        /// <param name="box">The box to test.</param>
        /// <param name="point">When the method completes, contains the point of intersection, or <see cref="Vector3.Zero" /> if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public static bool RayIntersectsBox(ref Ray ray, ref BoundingBox box, out Vector3 point)
        {
            if (!RayIntersectsBox(ref ray, ref box, out Real distance))
            {
                point = Vector3.Zero;
                return false;
            }

            point = ray.Position + ray.Direction * distance;
            return true;
        }

        /// <summary>
        /// Determines whether there is an intersection between a <see cref="Ray" /> and a <see cref="BoundingSphere" />.
        /// </summary>
        /// <param name="ray">The ray to test.</param>
        /// <param name="sphere">The sphere to test.</param>
        /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public static bool RayIntersectsSphere(ref Ray ray, ref BoundingSphere sphere, out Real distance)
        {
            //Source: Real-Time Collision Detection by Christer Ericson
            //Reference: Page 177

            Vector3.Subtract(ref ray.Position, ref sphere.Center, out Vector3 m);

            Real b = Vector3.Dot(m, ray.Direction);
            Real c = Vector3.Dot(m, m) - sphere.Radius * sphere.Radius;

            if ((c > 0f) && (b > 0f))
            {
                distance = 0f;
                return false;
            }

            Real discriminant = b * b - c;

            if (discriminant < 0f)
            {
                distance = 0f;
                return false;
            }

            distance = -b - (Real)Math.Sqrt(discriminant);

            if (distance < 0f)
                distance = 0f;

            return true;
        }

        /// <summary>
        /// Determines whether there is an intersection between a <see cref="Ray" /> and a <see cref="BoundingSphere" />.
        /// </summary>
        /// <param name="ray">The ray to test.</param>
        /// <param name="sphere">The sphere to test.</param>
        /// <param name="point">When the method completes, contains the point of intersection, or <see cref="Vector3.Zero" /> if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public static bool RayIntersectsSphere(ref Ray ray, ref BoundingSphere sphere, out Vector3 point)
        {
            if (!RayIntersectsSphere(ref ray, ref sphere, out Real distance))
            {
                point = Vector3.Zero;
                return false;
            }

            point = ray.Position + ray.Direction * distance;
            return true;
        }

        /// <summary>
        /// Determines whether there is an intersection between a <see cref="Plane" /> and a point.
        /// </summary>
        /// <param name="plane">The plane to test.</param>
        /// <param name="point">The point to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public static PlaneIntersectionType PlaneIntersectsPoint(ref Plane plane, ref Vector3 point)
        {
            Vector3.Dot(ref plane.Normal, ref point, out Real distance);
            distance += plane.D;

            if (distance > 0f)
                return PlaneIntersectionType.Front;

            if (distance < 0f)
                return PlaneIntersectionType.Back;

            return PlaneIntersectionType.Intersecting;
        }

        /// <summary>
        /// Determines whether there is an intersection between a <see cref="Plane" /> and a <see cref="Plane" />.
        /// </summary>
        /// <param name="plane1">The first plane to test.</param>
        /// <param name="plane2">The second plane to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public static bool PlaneIntersectsPlane(ref Plane plane1, ref Plane plane2)
        {
            Vector3.Cross(ref plane1.Normal, ref plane2.Normal, out Vector3 direction);

            //If direction is the zero vector, the planes are parallel and possibly
            //coincident. It is not an intersection. The dot product will tell us.
            Vector3.Dot(ref direction, ref direction, out Real denominator);

            if (Mathf.IsZero(denominator))
                return false;

            return true;
        }

        /// <summary>
        /// Determines whether there is an intersection between a <see cref="Plane" /> and a <see cref="Plane" />.
        /// </summary>
        /// <param name="plane1">The first plane to test.</param>
        /// <param name="plane2">The second plane to test.</param>
        /// <param name="line">When the method completes, contains the line of intersection as a <see cref="Ray" />, or a zero ray if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        /// <remarks>
        /// Although a ray is set to have an origin, the ray returned by this method is really
        /// a line in three dimensions which has no real origin. The ray is considered valid when
        /// both the positive direction is used and when the negative direction is used.
        /// </remarks>
        public static bool PlaneIntersectsPlane(ref Plane plane1, ref Plane plane2, out Ray line)
        {
            //Source: Real-Time Collision Detection by Christer Ericson
            //Reference: Page 207

            Vector3.Cross(ref plane1.Normal, ref plane2.Normal, out Vector3 direction);

            //If direction is the zero vector, the planes are parallel and possibly
            //coincident. It is not an intersection. The dot product will tell us.
            Vector3.Dot(ref direction, ref direction, out Real denominator);

            //We assume the planes are normalized, therefore the denominator
            //only serves as a parallel and coincident check. Otherwise we need
            //to divide the point by the denominator.
            if (Mathf.IsZero(denominator))
            {
                line = new Ray();
                return false;
            }

            Vector3 temp = plane1.D * plane2.Normal - plane2.D * plane1.Normal;
            Vector3.Cross(ref temp, ref direction, out Vector3 point);

            line.Position = point;
            line.Direction = direction;
            line.Direction.Normalize();

            return true;
        }

        /// <summary>
        /// Determines whether there is an intersection between a <see cref="Plane" /> and a triangle.
        /// </summary>
        /// <param name="plane">The plane to test.</param>
        /// <param name="vertex1">The first vertex of the triangle to test.</param>
        /// <param name="vertex2">The second vertex of the triangle to test.</param>
        /// <param name="vertex3">The third vertex of the triangle to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public static PlaneIntersectionType PlaneIntersectsTriangle(ref Plane plane, ref Vector3 vertex1, ref Vector3 vertex2, ref Vector3 vertex3)
        {
            //Source: Real-Time Collision Detection by Christer Ericson
            //Reference: Page 207

            PlaneIntersectionType test1 = PlaneIntersectsPoint(ref plane, ref vertex1);
            PlaneIntersectionType test2 = PlaneIntersectsPoint(ref plane, ref vertex2);
            PlaneIntersectionType test3 = PlaneIntersectsPoint(ref plane, ref vertex3);

            if ((test1 == PlaneIntersectionType.Front) && (test2 == PlaneIntersectionType.Front) && (test3 == PlaneIntersectionType.Front))
                return PlaneIntersectionType.Front;

            if ((test1 == PlaneIntersectionType.Back) && (test2 == PlaneIntersectionType.Back) && (test3 == PlaneIntersectionType.Back))
                return PlaneIntersectionType.Back;

            return PlaneIntersectionType.Intersecting;
        }

        /// <summary>
        /// Determines whether there is an intersection between a <see cref="Plane" /> and a <see cref="BoundingBox" />.
        /// </summary>
        /// <param name="plane">The plane to test.</param>
        /// <param name="box">The box to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public static PlaneIntersectionType PlaneIntersectsBox(ref Plane plane, ref BoundingBox box)
        {
            //Source: Real-Time Collision Detection by Christer Ericson
            //Reference: Page 161

            Vector3 min;
            Vector3 max;

            max.X = plane.Normal.X >= 0.0f ? box.Minimum.X : box.Maximum.X;
            max.Y = plane.Normal.Y >= 0.0f ? box.Minimum.Y : box.Maximum.Y;
            max.Z = plane.Normal.Z >= 0.0f ? box.Minimum.Z : box.Maximum.Z;
            min.X = plane.Normal.X >= 0.0f ? box.Maximum.X : box.Minimum.X;
            min.Y = plane.Normal.Y >= 0.0f ? box.Maximum.Y : box.Minimum.Y;
            min.Z = plane.Normal.Z >= 0.0f ? box.Maximum.Z : box.Minimum.Z;

            Vector3.Dot(ref plane.Normal, ref max, out Real distance);

            if (distance + plane.D > 0.0f)
                return PlaneIntersectionType.Front;

            distance = Vector3.Dot(plane.Normal, min);

            if (distance + plane.D < 0.0f)
                return PlaneIntersectionType.Back;

            return PlaneIntersectionType.Intersecting;
        }

        /// <summary>
        /// Determines whether there is an intersection between a <see cref="Plane" /> and a <see cref="BoundingSphere" />.
        /// </summary>
        /// <param name="plane">The plane to test.</param>
        /// <param name="sphere">The sphere to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public static PlaneIntersectionType PlaneIntersectsSphere(ref Plane plane, ref BoundingSphere sphere)
        {
            //Source: Real-Time Collision Detection by Christer Ericson
            //Reference: Page 160

            Vector3.Dot(ref plane.Normal, ref sphere.Center, out Real distance);
            distance += plane.D;

            if (distance > sphere.Radius)
                return PlaneIntersectionType.Front;

            if (distance < -sphere.Radius)
                return PlaneIntersectionType.Back;

            return PlaneIntersectionType.Intersecting;
        }

        /* This implementation is wrong
        /// <summary>
        /// Determines whether there is an intersection between a <see cref="SharpDX.BoundingBox"/> and a triangle.
        /// </summary>
        /// <param name="box">The box to test.</param>
        /// <param name="vertex1">The first vertex of the triangle to test.</param>
        /// <param name="vertex2">The second vertex of the triangle to test.</param>
        /// <param name="vertex3">The third vertex of the triangle to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public static bool BoxIntersectsTriangle(ref BoundingBox box, ref Vector3 vertex1, ref Vector3 vertex2, ref Vector3 vertex3)
        {
            if (BoxContainsPoint(ref box, ref vertex1) == ContainmentType.Contains)
                return true;

            if (BoxContainsPoint(ref box, ref vertex2) == ContainmentType.Contains)
                return true;

            if (BoxContainsPoint(ref box, ref vertex3) == ContainmentType.Contains)
                return true;

            return false;
        }
        */

        /// <summary>
        /// Determines whether there is an intersection between a <see cref="BoundingBox" /> and a <see cref="BoundingBox" />.
        /// </summary>
        /// <param name="box1">The first box to test.</param>
        /// <param name="box2">The second box to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public static bool BoxIntersectsBox(ref BoundingBox box1, ref BoundingBox box2)
        {
            if ((box1.Minimum.X > box2.Maximum.X) || (box2.Minimum.X > box1.Maximum.X))
                return false;

            if ((box1.Minimum.Y > box2.Maximum.Y) || (box2.Minimum.Y > box1.Maximum.Y))
                return false;

            if ((box1.Minimum.Z > box2.Maximum.Z) || (box2.Minimum.Z > box1.Maximum.Z))
                return false;

            return true;
        }

        /// <summary>
        /// Determines whether there is an intersection between a <see cref="BoundingBox" /> and a <see cref="BoundingSphere" />.
        /// </summary>
        /// <param name="box">The box to test.</param>
        /// <param name="sphere">The sphere to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public static bool BoxIntersectsSphere(ref BoundingBox box, ref BoundingSphere sphere)
        {
            //Source: Real-Time Collision Detection by Christer Ericson
            //Reference: Page 166

            Vector3.Clamp(ref sphere.Center, ref box.Minimum, ref box.Maximum, out Vector3 vector);
            Real distance = Vector3.DistanceSquared(ref sphere.Center, ref vector);
            return distance <= sphere.Radius * sphere.Radius;
        }

        /// <summary>
        /// Determines whether there is an intersection between a <see cref="BoundingSphere" /> and a triangle.
        /// </summary>
        /// <param name="sphere">The sphere to test.</param>
        /// <param name="vertex1">The first vertex of the triangle to test.</param>
        /// <param name="vertex2">The second vertex of the triangle to test.</param>
        /// <param name="vertex3">The third vertex of the triangle to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public static bool SphereIntersectsTriangle(ref BoundingSphere sphere, ref Vector3 vertex1, ref Vector3 vertex2, ref Vector3 vertex3)
        {
            //Source: Real-Time Collision Detection by Christer Ericson
            //Reference: Page 167

            ClosestPointPointTriangle(ref sphere.Center, ref vertex1, ref vertex2, ref vertex3, out Vector3 point);
            Vector3 v = point - sphere.Center;

            Vector3.Dot(ref v, ref v, out Real dot);

            return dot <= sphere.Radius * sphere.Radius;
        }

        /// <summary>
        /// Determines whether there is an intersection between a <see cref="BoundingSphere" /> and a
        /// <see cref="BoundingSphere" />.
        /// </summary>
        /// <param name="sphere1">First sphere to test.</param>
        /// <param name="sphere2">Second sphere to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public static bool SphereIntersectsSphere(ref BoundingSphere sphere1, ref BoundingSphere sphere2)
        {
            Real radiisum = sphere1.Radius + sphere2.Radius;
            return Vector3.DistanceSquared(ref sphere1.Center, ref sphere2.Center) <= radiisum * radiisum;
        }

        /// <summary>
        /// Determines whether a <see cref="BoundingBox" /> contains a point.
        /// </summary>
        /// <param name="box">The box to test.</param>
        /// <param name="point">The point to test.</param>
        /// <returns>The type of containment the two objects have.</returns>
        public static ContainmentType BoxContainsPoint(ref BoundingBox box, ref Vector3 point)
        {
            if ((box.Minimum.X <= point.X) && (box.Maximum.X >= point.X) &&
                (box.Minimum.Y <= point.Y) && (box.Maximum.Y >= point.Y) &&
                (box.Minimum.Z <= point.Z) && (box.Maximum.Z >= point.Z))
                return ContainmentType.Contains;

            return ContainmentType.Disjoint;
        }

        /* This implementation is wrong
        /// <summary>
        /// Determines whether a <see cref="SharpDX.BoundingBox"/> contains a triangle.
        /// </summary>
        /// <param name="box">The box to test.</param>
        /// <param name="vertex1">The first vertex of the triangle to test.</param>
        /// <param name="vertex2">The second vertex of the triangle to test.</param>
        /// <param name="vertex3">The third vertex of the triangle to test.</param>
        /// <returns>The type of containment the two objects have.</returns>
        public static ContainmentType BoxContainsTriangle(ref BoundingBox box, ref Vector3 vertex1, ref Vector3 vertex2, ref Vector3 vertex3)
        {
            ContainmentType test1 = BoxContainsPoint(ref box, ref vertex1);
            ContainmentType test2 = BoxContainsPoint(ref box, ref vertex2);
            ContainmentType test3 = BoxContainsPoint(ref box, ref vertex3);

            if (test1 == ContainmentType.Contains && test2 == ContainmentType.Contains && test3 == ContainmentType.Contains)
                return ContainmentType.Contains;

            if (test1 == ContainmentType.Contains || test2 == ContainmentType.Contains || test3 == ContainmentType.Contains)
                return ContainmentType.Intersects;

            return ContainmentType.Disjoint;
        }
        */

        /// <summary>
        /// Determines whether a <see cref="BoundingBox" /> contains a <see cref="BoundingBox" />.
        /// </summary>
        /// <param name="box1">The first box to test.</param>
        /// <param name="box2">The second box to test.</param>
        /// <returns>The type of containment the two objects have.</returns>
        public static ContainmentType BoxContainsBox(ref BoundingBox box1, ref BoundingBox box2)
        {
            if ((box1.Maximum.X < box2.Minimum.X) || (box1.Minimum.X > box2.Maximum.X))
                return ContainmentType.Disjoint;

            if ((box1.Maximum.Y < box2.Minimum.Y) || (box1.Minimum.Y > box2.Maximum.Y))
                return ContainmentType.Disjoint;

            if ((box1.Maximum.Z < box2.Minimum.Z) || (box1.Minimum.Z > box2.Maximum.Z))
                return ContainmentType.Disjoint;

            if ((box1.Minimum.X <= box2.Minimum.X) && (box2.Maximum.X <= box1.Maximum.X) && (box1.Minimum.Y <= box2.Minimum.Y) && (box2.Maximum.Y <= box1.Maximum.Y) && (box1.Minimum.Z <= box2.Minimum.Z) && (box2.Maximum.Z <= box1.Maximum.Z))
                return ContainmentType.Contains;

            return ContainmentType.Intersects;
        }

        /// <summary>
        /// Determines whether a <see cref="BoundingBox" /> contains a <see cref="BoundingSphere" />.
        /// </summary>
        /// <param name="box">The box to test.</param>
        /// <param name="sphere">The sphere to test.</param>
        /// <returns>The type of containment the two objects have.</returns>
        public static ContainmentType BoxContainsSphere(ref BoundingBox box, ref BoundingSphere sphere)
        {
            Vector3.Clamp(ref sphere.Center, ref box.Minimum, ref box.Maximum, out Vector3 vector);
            Real distance = Vector3.DistanceSquared(ref sphere.Center, ref vector);

            if (distance > sphere.Radius * sphere.Radius)
                return ContainmentType.Disjoint;

            if ((box.Minimum.X + sphere.Radius <= sphere.Center.X) && (sphere.Center.X <= box.Maximum.X - sphere.Radius) && (box.Maximum.X - box.Minimum.X > sphere.Radius) && (box.Minimum.Y + sphere.Radius <= sphere.Center.Y) && (sphere.Center.Y <= box.Maximum.Y - sphere.Radius) && (box.Maximum.Y - box.Minimum.Y > sphere.Radius) && (box.Minimum.Z + sphere.Radius <= sphere.Center.Z) && (sphere.Center.Z <= box.Maximum.Z - sphere.Radius) && (box.Maximum.Z - box.Minimum.Z > sphere.Radius))
                return ContainmentType.Contains;

            return ContainmentType.Intersects;
        }

        /// <summary>
        /// Determines whether a <see cref="BoundingSphere" /> contains a point.
        /// </summary>
        /// <param name="sphere">The sphere to test.</param>
        /// <param name="point">The point to test.</param>
        /// <returns>The type of containment the two objects have.</returns>
        public static ContainmentType SphereContainsPoint(ref BoundingSphere sphere, ref Vector3 point)
        {
            if (Vector3.DistanceSquared(ref point, ref sphere.Center) <= sphere.Radius * sphere.Radius)
                return ContainmentType.Contains;

            return ContainmentType.Disjoint;
        }

        /// <summary>
        /// Determines whether a <see cref="BoundingSphere" /> contains a triangle.
        /// </summary>
        /// <param name="sphere">The sphere to test.</param>
        /// <param name="vertex1">The first vertex of the triangle to test.</param>
        /// <param name="vertex2">The second vertex of the triangle to test.</param>
        /// <param name="vertex3">The third vertex of the triangle to test.</param>
        /// <returns>The type of containment the two objects have.</returns>
        public static ContainmentType SphereContainsTriangle(ref BoundingSphere sphere, ref Vector3 vertex1, ref Vector3 vertex2, ref Vector3 vertex3)
        {
            //Source: Jorgy343
            //Reference: None

            ContainmentType test1 = SphereContainsPoint(ref sphere, ref vertex1);
            ContainmentType test2 = SphereContainsPoint(ref sphere, ref vertex2);
            ContainmentType test3 = SphereContainsPoint(ref sphere, ref vertex3);

            if ((test1 == ContainmentType.Contains) && (test2 == ContainmentType.Contains) && (test3 == ContainmentType.Contains))
                return ContainmentType.Contains;

            if (SphereIntersectsTriangle(ref sphere, ref vertex1, ref vertex2, ref vertex3))
                return ContainmentType.Intersects;

            return ContainmentType.Disjoint;
        }

        /// <summary>
        /// Determines whether a <see cref="BoundingSphere" /> contains a <see cref="BoundingBox" />.
        /// </summary>
        /// <param name="sphere">The sphere to test.</param>
        /// <param name="box">The box to test.</param>
        /// <returns>The type of containment the two objects have.</returns>
        public static ContainmentType SphereContainsBox(ref BoundingSphere sphere, ref BoundingBox box)
        {
            Vector3 vector;

            if (!BoxIntersectsSphere(ref box, ref sphere))
                return ContainmentType.Disjoint;

            Real radiusSquared = sphere.Radius * sphere.Radius;

            vector.X = sphere.Center.X - box.Minimum.X;
            vector.Y = sphere.Center.Y - box.Maximum.Y;
            vector.Z = sphere.Center.Z - box.Maximum.Z;
            if (vector.LengthSquared > radiusSquared)
                return ContainmentType.Intersects;

            vector.X = sphere.Center.X - box.Maximum.X;
            vector.Y = sphere.Center.Y - box.Maximum.Y;
            vector.Z = sphere.Center.Z - box.Maximum.Z;
            if (vector.LengthSquared > radiusSquared)
                return ContainmentType.Intersects;

            vector.X = sphere.Center.X - box.Maximum.X;
            vector.Y = sphere.Center.Y - box.Minimum.Y;
            vector.Z = sphere.Center.Z - box.Maximum.Z;
            if (vector.LengthSquared > radiusSquared)
                return ContainmentType.Intersects;

            vector.X = sphere.Center.X - box.Minimum.X;
            vector.Y = sphere.Center.Y - box.Minimum.Y;
            vector.Z = sphere.Center.Z - box.Maximum.Z;
            if (vector.LengthSquared > radiusSquared)
                return ContainmentType.Intersects;

            vector.X = sphere.Center.X - box.Minimum.X;
            vector.Y = sphere.Center.Y - box.Maximum.Y;
            vector.Z = sphere.Center.Z - box.Minimum.Z;
            if (vector.LengthSquared > radiusSquared)
                return ContainmentType.Intersects;

            vector.X = sphere.Center.X - box.Maximum.X;
            vector.Y = sphere.Center.Y - box.Maximum.Y;
            vector.Z = sphere.Center.Z - box.Minimum.Z;
            if (vector.LengthSquared > radiusSquared)
                return ContainmentType.Intersects;

            vector.X = sphere.Center.X - box.Maximum.X;
            vector.Y = sphere.Center.Y - box.Minimum.Y;
            vector.Z = sphere.Center.Z - box.Minimum.Z;
            if (vector.LengthSquared > radiusSquared)
                return ContainmentType.Intersects;

            vector.X = sphere.Center.X - box.Minimum.X;
            vector.Y = sphere.Center.Y - box.Minimum.Y;
            vector.Z = sphere.Center.Z - box.Minimum.Z;
            if (vector.LengthSquared > radiusSquared)
                return ContainmentType.Intersects;

            return ContainmentType.Contains;
        }

        /// <summary>
        /// Determines whether a <see cref="BoundingSphere" /> contains a <see cref="BoundingSphere" />.
        /// </summary>
        /// <param name="sphere1">The first sphere to test.</param>
        /// <param name="sphere2">The second sphere to test.</param>
        /// <returns>The type of containment the two objects have.</returns>
        public static ContainmentType SphereContainsSphere(ref BoundingSphere sphere1, ref BoundingSphere sphere2)
        {
            Real distance = Vector3.Distance(ref sphere1.Center, ref sphere2.Center);

            if (sphere1.Radius + sphere2.Radius < distance)
                return ContainmentType.Disjoint;

            if (sphere1.Radius - sphere2.Radius < distance)
                return ContainmentType.Intersects;

            return ContainmentType.Contains;
        }

        /// <summary>
        /// Determines whether a line intersects with the other line.
        /// </summary>
        /// <param name="l1p1">The first line point 0.</param>
        /// <param name="l1p2">The first line point 1.</param>
        /// <param name="l2p1">The second line point 0.</param>
        /// <param name="l2p2">The second line point 1.</param>
        /// <returns>True if line intersects with the other line</returns>
        public static bool LineIntersectsLine(ref Float2 l1p1, ref Float2 l1p2, ref Float2 l2p1, ref Float2 l2p2)
        {
            float q = (l1p1.Y - l2p1.Y) * (l2p2.X - l2p1.X) - (l1p1.X - l2p1.X) * (l2p2.Y - l2p1.Y);
            float d = (l1p2.X - l1p1.X) * (l2p2.Y - l2p1.Y) - (l1p2.Y - l1p1.Y) * (l2p2.X - l2p1.X);
            if (Mathf.IsZero(d))
                return false;
            float r = q / d;
            q = (l1p1.Y - l2p1.Y) * (l1p2.X - l1p1.X) - (l1p1.X - l2p1.X) * (l1p2.Y - l1p1.Y);
            float s = q / d;
            return !(r < 0 || r > 1 || s < 0 || s > 1);
        }

        /// <summary>
        /// Determines whether a line intersects with the rectangle.
        /// </summary>
        /// <param name="p1">The line point 0.</param>
        /// <param name="p2">The line point 1.</param>
        /// <param name="rect">The rectangle.</param>
        /// <returns>True if line intersects with the rectangle</returns>
        public static bool LineIntersectsRect(ref Float2 p1, ref Float2 p2, ref Rectangle rect)
        {
            // TODO: optimize it
            var pA = new Float2(rect.Right, rect.Y);
            var pB = new Float2(rect.Right, rect.Bottom);
            var pC = new Float2(rect.X, rect.Bottom);
            return LineIntersectsLine(ref p1, ref p2, ref rect.Location, ref pA) ||
                   LineIntersectsLine(ref p1, ref p2, ref pA, ref pB) ||
                   LineIntersectsLine(ref p1, ref p2, ref pB, ref pC) ||
                   LineIntersectsLine(ref p1, ref p2, ref pC, ref rect.Location) ||
                   (rect.Contains(ref p1) && rect.Contains(ref p2));
        }

        /// <summary>
        /// Determines whether the given 2D point is inside the specified triangle.
        /// </summary>
        /// <param name="point">The point to check.</param>
        /// <param name="a">The first vertex of the triangle.</param>
        /// <param name="b">The second vertex of the triangle.</param>
        /// <param name="c">The third vertex of the triangle.</param>
        /// <returns><c>true</c> if point is inside the triangle; otherwise, <c>false</c>.</returns>
        public static bool IsPointInTriangle(ref Float2 point, ref Float2 a, ref Float2 b, ref Float2 c)
        {
            var an = a - point;
            var bn = b - point;
            var cn = c - point;
            bool orientation = Float2.Cross(an, bn) > 0;
            if (Float2.Cross(bn, cn) > 0 != orientation)
                return false;
            return Float2.Cross(cn, an) > 0 == orientation;
        }
    }
}
