// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
using System.Globalization;
using System.Runtime.CompilerServices;
using FlaxEngine.Assertions;

namespace FlaxEngine
{
    [Serializable]
    partial struct Ray : IEquatable<Ray>, IFormattable
    {
        /// <summary>
        /// Identity ray (at zero origin pointing forwards).
        /// </summary>
        public static readonly Ray Identity = new Ray(new Vector3(0.0f, 0.0f, 0.0f), new Vector3(0.0f, 0.0f, 1.0f));

        /// <summary>
        /// Initializes a new instance of the <see cref="Ray" /> struct.
        /// </summary>
        /// <param name="position">The position in three dimensional space of the origin of the ray.</param>
        /// <param name="direction">The normalized direction of the ray.</param>
        public Ray(Vector3 position, Vector3 direction)
        {
            Position = position;
            Direction = direction;
            Assert.IsTrue(Direction.IsNormalized, $"The Ray Direction was not normalized (direction: {direction}, length: {direction.Length})");
        }

        /// <summary>
        /// Gets a point at distance long ray.
        /// </summary>
        /// <param name="distance">The distance from ray origin.</param>
        /// <returns>The calculated point.</returns>
        public Vector3 GetPoint(Real distance)
        {
            return Position + Direction * distance;
        }

        /// <summary>
        /// Determines if there is an intersection between the current object and a point.
        /// </summary>
        /// <param name="point">The point to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public bool Intersects(ref Vector3 point)
        {
            return CollisionsHelper.RayIntersectsPoint(ref this, ref point);
        }

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="Ray" />.
        /// </summary>
        /// <param name="ray">The ray to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public bool Intersects(ref Ray ray)
        {
            return CollisionsHelper.RayIntersectsRay(ref this, ref ray, out _);
        }

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="Ray" />.
        /// </summary>
        /// <param name="ray">The ray to test.</param>
        /// <param name="point">When the method completes, contains the point of intersection, or <see cref="Vector3.Zero" /> if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public bool Intersects(ref Ray ray, out Vector3 point)
        {
            return CollisionsHelper.RayIntersectsRay(ref this, ref ray, out point);
        }

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="Plane" />.
        /// </summary>
        /// <param name="plane">The plane to test</param>
        /// <returns>Whether the two objects intersected.</returns>
        public bool Intersects(ref Plane plane)
        {
            return CollisionsHelper.RayIntersectsPlane(ref this, ref plane, out Real _);
        }

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="Plane" />.
        /// </summary>
        /// <param name="plane">The plane to test.</param>
        /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public bool Intersects(ref Plane plane, out Real distance)
        {
            return CollisionsHelper.RayIntersectsPlane(ref this, ref plane, out distance);
        }

#if USE_LARGE_WORLDS
        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="Plane" />.
        /// [Deprecated on 26.05.2022, expires on 26.05.2024]
        /// </summary>
        /// <param name="plane">The plane to test.</param>
        /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        [Obsolete("Use Intersects with 'out Real distance' parameter instead")]
        public bool Intersects(ref Plane plane, out float distance)
        {
            return CollisionsHelper.RayIntersectsPlane(ref this, ref plane, out distance);
        }
#endif

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="Plane" />.
        /// </summary>
        /// <param name="plane">The plane to test.</param>
        /// <param name="point">When the method completes, contains the point of intersection, or <see cref="Vector3.Zero" /> if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public bool Intersects(ref Plane plane, out Vector3 point)
        {
            return CollisionsHelper.RayIntersectsPlane(ref this, ref plane, out point);
        }

        /// <summary>
        /// Determines if there is an intersection between the current object and a triangle.
        /// </summary>
        /// <param name="vertex1">The first vertex of the triangle to test.</param>
        /// <param name="vertex2">The second vertex of the triangle to test.</param>
        /// <param name="vertex3">The third vertex of the triangle to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public bool Intersects(ref Vector3 vertex1, ref Vector3 vertex2, ref Vector3 vertex3)
        {
            return CollisionsHelper.RayIntersectsTriangle(ref this, ref vertex1, ref vertex2, ref vertex3, out Real _);
        }

        /// <summary>
        /// Determines if there is an intersection between the current object and a triangle.
        /// </summary>
        /// <param name="vertex1">The first vertex of the triangle to test.</param>
        /// <param name="vertex2">The second vertex of the triangle to test.</param>
        /// <param name="vertex3">The third vertex of the triangle to test.</param>
        /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public bool Intersects(ref Vector3 vertex1, ref Vector3 vertex2, ref Vector3 vertex3, out Real distance)
        {
            return CollisionsHelper.RayIntersectsTriangle(ref this, ref vertex1, ref vertex2, ref vertex3, out distance);
        }

#if USE_LARGE_WORLDS
        /// <summary>
        /// Determines if there is an intersection between the current object and a triangle.
        /// [Deprecated on 26.05.2022, expires on 26.05.2024]
        /// </summary>
        /// <param name="vertex1">The first vertex of the triangle to test.</param>
        /// <param name="vertex2">The second vertex of the triangle to test.</param>
        /// <param name="vertex3">The third vertex of the triangle to test.</param>
        /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        [Obsolete("Use Intersects with 'out Real distance' parameter instead")]
        public bool Intersects(ref Vector3 vertex1, ref Vector3 vertex2, ref Vector3 vertex3, out float distance)
        {
            var result = CollisionsHelper.RayIntersectsTriangle(ref this, ref vertex1, ref vertex2, ref vertex3, out Real dst);
            distance = (float)dst;
            return result;
        }
#endif

        /// <summary>
        /// Determines if there is an intersection between the current object and a triangle.
        /// </summary>
        /// <param name="vertex1">The first vertex of the triangle to test.</param>
        /// <param name="vertex2">The second vertex of the triangle to test.</param>
        /// <param name="vertex3">The third vertex of the triangle to test.</param>
        /// <param name="point">When the method completes, contains the point of intersection, or <see cref="Vector3.Zero" /> if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public bool Intersects(ref Vector3 vertex1, ref Vector3 vertex2, ref Vector3 vertex3, out Vector3 point)
        {
            return CollisionsHelper.RayIntersectsTriangle(ref this, ref vertex1, ref vertex2, ref vertex3, out point);
        }

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="BoundingBox" />.
        /// </summary>
        /// <param name="box">The box to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public bool Intersects(ref BoundingBox box)
        {
            return CollisionsHelper.RayIntersectsBox(ref this, ref box, out Real _);
        }

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="BoundingBox" />.
        /// </summary>
        /// <param name="box">The box to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public bool Intersects(BoundingBox box)
        {
            return Intersects(ref box);
        }

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="BoundingBox" />.
        /// </summary>
        /// <param name="box">The box to test.</param>
        /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public bool Intersects(ref BoundingBox box, out Real distance)
        {
            return CollisionsHelper.RayIntersectsBox(ref this, ref box, out distance);
        }

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="BoundingBox" />.
        /// </summary>
        /// <param name="box">The box to test.</param>
        /// <param name="point">When the method completes, contains the point of intersection, or <see cref="Vector3.Zero" /> if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public bool Intersects(ref BoundingBox box, out Vector3 point)
        {
            return CollisionsHelper.RayIntersectsBox(ref this, ref box, out point);
        }

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="BoundingSphere" />.
        /// </summary>
        /// <param name="sphere">The sphere to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public bool Intersects(ref BoundingSphere sphere)
        {
            return CollisionsHelper.RayIntersectsSphere(ref this, ref sphere, out Real _);
        }

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="BoundingSphere" />.
        /// </summary>
        /// <param name="sphere">The sphere to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public bool Intersects(BoundingSphere sphere)
        {
            return Intersects(ref sphere);
        }

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="BoundingSphere" />.
        /// </summary>
        /// <param name="sphere">The sphere to test.</param>
        /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public bool Intersects(ref BoundingSphere sphere, out Real distance)
        {
            return CollisionsHelper.RayIntersectsSphere(ref this, ref sphere, out distance);
        }

#if USE_LARGE_WORLDS
        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="BoundingSphere" />.
        /// [Deprecated on 26.05.2022, expires on 26.05.2024]
        /// </summary>
        /// <param name="sphere">The sphere to test.</param>
        /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        [Obsolete("Use Intersects with 'out Real distance' parameter instead")]
        public bool Intersects(ref BoundingSphere sphere, out float distance)
        {
            var result = CollisionsHelper.RayIntersectsSphere(ref this, ref sphere, out Real dst);
            distance = (float)dst;
            return result;
        }
#endif

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="BoundingSphere" />.
        /// </summary>
        /// <param name="sphere">The sphere to test.</param>
        /// <param name="point">When the method completes, contains the point of intersection, or <see cref="Vector3.Zero" /> if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public bool Intersects(ref BoundingSphere sphere, out Vector3 point)
        {
            return CollisionsHelper.RayIntersectsSphere(ref this, ref sphere, out point);
        }

        /// <summary>
        /// Calculates a world space ray from 2d screen coordinates.
        /// </summary>
        /// <param name="x">The X coordinate on 2d screen.</param>
        /// <param name="y">The Y coordinate on 2d screen.</param>
        /// <param name="viewport">The screen viewport.</param>
        /// <param name="vp">The View*Projection matrix.</param>
        /// <returns>The resulting ray.</returns>
        public static Ray GetPickRay(float x, float y, ref Viewport viewport, ref Matrix vp)
        {
            Vector3 nearPoint = new Vector3(x, y, 0.0f);
            Vector3 farPoint = new Vector3(x, y, 1.0f);

            nearPoint = Vector3.Unproject(nearPoint, viewport.X, viewport.Y, viewport.Width, viewport.Height, viewport.MinDepth, viewport.MaxDepth, vp);
            farPoint = Vector3.Unproject(farPoint, viewport.X, viewport.Y, viewport.Width, viewport.Height, viewport.MinDepth, viewport.MaxDepth, vp);

            Vector3 direction = farPoint - nearPoint;
            direction.Normalize();

            return new Ray(nearPoint, direction);
        }

        /// <summary>
        /// Tests for equality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left" /> has the same value as <paramref name="right" />; otherwise,<c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator ==(Ray left, Ray right)
        {
            return left.Equals(ref right);
        }

        /// <summary>
        /// Tests for inequality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left" /> has a different value than <paramref name="right" />; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator !=(Ray left, Ray right)
        {
            return !left.Equals(ref right);
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public override string ToString()
        {
            return string.Format(CultureInfo.CurrentCulture, "Position:{0} Direction:{1}", Position.ToString(), Direction.ToString());
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <param name="format">The format.</param>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public string ToString(string format)
        {
            return string.Format(CultureInfo.CurrentCulture, "Position:{0} Direction:{1}", Position.ToString(format, CultureInfo.CurrentCulture), Direction.ToString(format, CultureInfo.CurrentCulture));
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <param name="formatProvider">The format provider.</param>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public string ToString(IFormatProvider formatProvider)
        {
            return string.Format(formatProvider, "Position:{0} Direction:{1}", Position.ToString(), Direction.ToString());
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <param name="format">The format.</param>
        /// <param name="formatProvider">The format provider.</param>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public string ToString(string format, IFormatProvider formatProvider)
        {
            return string.Format(formatProvider, "Position:{0} Direction:{1}", Position.ToString(format, formatProvider), Direction.ToString(format, formatProvider));
        }

        /// <summary>
        /// Returns a hash code for this instance.
        /// </summary>
        /// <returns>A hash code for this instance, suitable for use in hashing algorithms and data structures like a hash table.</returns>
        public override int GetHashCode()
        {
            unchecked
            {
                return (Position.GetHashCode() * 397) ^ Direction.GetHashCode();
            }
        }

        /// <summary>
        /// Determines whether the specified <see cref="Vector4" /> is equal to this instance.
        /// </summary>
        /// <param name="value">The <see cref="Vector4" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="Vector4" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(ref Ray value)
        {
            return (Position == value.Position) && (Direction == value.Direction);
        }

        /// <summary>
        /// Determines whether the specified <see cref="Vector4" /> is equal to this instance.
        /// </summary>
        /// <param name="value">The <see cref="Vector4" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="Vector4" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(Ray value)
        {
            return Equals(ref value);
        }

        /// <summary>
        /// Determines whether the specified <see cref="System.Object" /> is equal to this instance.
        /// </summary>
        /// <param name="value">The <see cref="System.Object" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="System.Object" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        public override bool Equals(object value)
        {
            return value is Ray other && Equals(ref other);
        }
    }
}
