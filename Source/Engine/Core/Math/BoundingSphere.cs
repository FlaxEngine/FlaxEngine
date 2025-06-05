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
using System.Globalization;
using System.Runtime.CompilerServices;

namespace FlaxEngine
{
    [Serializable]
    partial struct BoundingSphere : IEquatable<BoundingSphere>, IFormattable
    {
        /// <summary>
        /// A <see cref="BoundingSphere"/> which represents an empty space.
        /// </summary>
        public static readonly BoundingSphere Empty = new BoundingSphere(Vector3.Zero, 0.0f);

        /// <summary>
        /// Initializes a new instance of the <see cref="BoundingBox" /> struct.
        /// </summary>
        /// <param name="center">The center of the sphere in three dimensional space.</param>
        /// <param name="radius">The radius of the sphere.</param>
        public BoundingSphere(Vector3 center, Real radius)
        {
            Center = center;
            Radius = radius;
        }

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="Ray" />.
        /// </summary>
        /// <param name="ray">The ray to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public bool Intersects(ref Ray ray)
        {
            return CollisionsHelper.RayIntersectsSphere(ref ray, ref this, out Real _);
        }

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="Ray" />.
        /// </summary>
        /// <param name="ray">The ray to test.</param>
        /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public bool Intersects(ref Ray ray, out Real distance)
        {
            return CollisionsHelper.RayIntersectsSphere(ref ray, ref this, out distance);
        }

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="Ray" />.
        /// </summary>
        /// <param name="ray">The ray to test.</param>
        /// <param name="point">When the method completes, contains the point of intersection, or <see cref="Vector3.Zero" /> if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public bool Intersects(ref Ray ray, out Vector3 point)
        {
            return CollisionsHelper.RayIntersectsSphere(ref ray, ref this, out point);
        }

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="Plane" />.
        /// </summary>
        /// <param name="plane">The plane to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public PlaneIntersectionType Intersects(ref Plane plane)
        {
            return CollisionsHelper.PlaneIntersectsSphere(ref plane, ref this);
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
            return CollisionsHelper.SphereIntersectsTriangle(ref this, ref vertex1, ref vertex2, ref vertex3);
        }

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="BoundingBox" />.
        /// </summary>
        /// <param name="box">The box to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public bool Intersects(ref BoundingBox box)
        {
            return CollisionsHelper.BoxIntersectsSphere(ref box, ref this);
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
        /// Determines if there is an intersection between the current object and a <see cref="BoundingSphere" />.
        /// </summary>
        /// <param name="sphere">The sphere to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public bool Intersects(ref BoundingSphere sphere)
        {
            return CollisionsHelper.SphereIntersectsSphere(ref this, ref sphere);
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
        /// Determines whether the current objects contains a point.
        /// </summary>
        /// <param name="point">The point to test.</param>
        /// <returns>The type of containment the two objects have.</returns>
        public ContainmentType Contains(ref Vector3 point)
        {
            return CollisionsHelper.SphereContainsPoint(ref this, ref point);
        }

        /// <summary>
        /// Determines whether the current objects contains a triangle.
        /// </summary>
        /// <param name="vertex1">The first vertex of the triangle to test.</param>
        /// <param name="vertex2">The second vertex of the triangle to test.</param>
        /// <param name="vertex3">The third vertex of the triangle to test.</param>
        /// <returns>The type of containment the two objects have.</returns>
        public ContainmentType Contains(ref Vector3 vertex1, ref Vector3 vertex2, ref Vector3 vertex3)
        {
            return CollisionsHelper.SphereContainsTriangle(ref this, ref vertex1, ref vertex2, ref vertex3);
        }

        /// <summary>
        /// Determines whether the current objects contains a <see cref="BoundingBox" />.
        /// </summary>
        /// <param name="box">The box to test.</param>
        /// <returns>The type of containment the two objects have.</returns>
        public ContainmentType Contains(ref BoundingBox box)
        {
            return CollisionsHelper.SphereContainsBox(ref this, ref box);
        }

        /// <summary>
        /// Determines whether the current objects contains a <see cref="BoundingSphere" />.
        /// </summary>
        /// <param name="sphere">The sphere to test.</param>
        /// <returns>The type of containment the two objects have.</returns>
        public ContainmentType Contains(ref BoundingSphere sphere)
        {
            return CollisionsHelper.SphereContainsSphere(ref this, ref sphere);
        }

        /// <summary>
        /// Constructs a <see cref="BoundingSphere" /> that fully contains the given points.
        /// </summary>
        /// <param name="points">The points that will be contained by the sphere.</param>
        /// <param name="start">The start index from points array to start compute the bounding sphere.</param>
        /// <param name="count">The count of points to process to compute the bounding sphere.</param>
        /// <param name="result">When the method completes, contains the newly constructed bounding sphere.</param>
        /// <exception cref="System.ArgumentNullException">points</exception>
        /// <exception cref="System.ArgumentOutOfRangeException">start or count</exception>
        public static void FromPoints(Vector3[] points, int start, int count, out BoundingSphere result)
        {
            if (points == null)
                throw new ArgumentNullException(nameof(points));
            if (start < 0 || start >= points.Length)
                throw new ArgumentOutOfRangeException(nameof(start), start, string.Format("Must be in the range [0, {0}]", points.Length - 1));
            if (count <= 0 || start + count > points.Length)
                throw new ArgumentOutOfRangeException(nameof(count), count, string.Format("Must be in the range <= {0}", points.Length));

            int upperEnd = start + count;

            // Find the center of all points
            Vector3 center = Vector3.Zero;
            for (int i = start; i < upperEnd; ++i)
                Vector3.Add(ref points[i], ref center, out center);
            center /= (Real)count;

            // Find the radius of the sphere
            Real radius = 0f;
            for (int i = start; i < upperEnd; ++i)
            {
                // We are doing a relative distance comparison to find the maximum distance from the center of our sphere
                Vector3.DistanceSquared(ref center, ref points[i], out Real distance);
                if (distance > radius)
                    radius = distance;
            }

            // Construct the sphere
            result.Center = center;
            result.Radius = Mathf.Sqrt(radius);
        }

        /// <summary>
        /// Constructs a <see cref="BoundingSphere" /> that fully contains the given points.
        /// </summary>
        /// <param name="points">The points that will be contained by the sphere.</param>
        /// <param name="result">When the method completes, contains the newly constructed bounding sphere.</param>
        public static void FromPoints(Vector3[] points, out BoundingSphere result)
        {
            if (points == null)
                throw new ArgumentNullException(nameof(points));
            FromPoints(points, 0, points.Length, out result);
        }

        /// <summary>
        /// Constructs a <see cref="BoundingSphere" /> that fully contains the given points.
        /// </summary>
        /// <param name="points">The points that will be contained by the sphere.</param>
        /// <returns>The newly constructed bounding sphere.</returns>
        public static BoundingSphere FromPoints(Vector3[] points)
        {
            FromPoints(points, out var result);
            return result;
        }

        /// <summary>
        /// Constructs a <see cref="BoundingSphere" /> from a given box.
        /// </summary>
        /// <param name="box">The box that will designate the extents of the sphere.</param>
        /// <param name="result">When the method completes, the newly constructed bounding sphere.</param>
        public static void FromBox(ref BoundingBox box, out BoundingSphere result)
        {
            var x = box.Maximum.X - box.Minimum.X;
            var y = box.Maximum.Y - box.Minimum.Y;
            var z = box.Maximum.Z - box.Minimum.Z;
            result.Center.X = box.Minimum.X + x * 0.5f;
            result.Center.Y = box.Minimum.Y + y * 0.5f;
            result.Center.Z = box.Minimum.Z + z * 0.5f;
            result.Radius = (Real)Math.Sqrt(x * x + y * y + z * z) * 0.5f;
        }

        /// <summary>
        /// Constructs a <see cref="BoundingSphere" /> from a given box.
        /// </summary>
        /// <param name="box">The box that will designate the extents of the sphere.</param>
        /// <returns>The newly constructed bounding sphere.</returns>
        public static BoundingSphere FromBox(BoundingBox box)
        {
            FromBox(ref box, out var result);
            return result;
        }

        /// <summary>
        /// Constructs a <see cref="BoundingSphere" /> that is the as large as the total combined area of the two specified
        /// spheres.
        /// </summary>
        /// <param name="value1">The first sphere to merge.</param>
        /// <param name="value2">The second sphere to merge.</param>
        /// <param name="result">When the method completes, contains the newly constructed bounding sphere.</param>
        public static void Merge(ref BoundingSphere value1, ref BoundingSphere value2, out BoundingSphere result)
        {
            // Pre-exit if one of the bounding sphere by assuming that a merge with an empty sphere is equivalent at taking the non-empty sphere
            if (value1 == Empty)
            {
                result = value2;
                return;
            }
            if (value2 == Empty)
            {
                result = value1;
                return;
            }

            Vector3 difference = value2.Center - value1.Center;
            Real length = difference.Length;
            Real radius = value1.Radius;
            Real radius2 = value2.Radius;

            if (radius + radius2 >= length)
            {
                if (radius - radius2 >= length)
                {
                    result = value1;
                    return;
                }

                if (radius2 - radius >= length)
                {
                    result = value2;
                    return;
                }
            }

            Vector3 vector = difference * (1.0f / length);
            var min = Mathf.Min(-radius, length - radius2);
            var max = (Mathf.Max(radius, length + radius2) - min) * 0.5f;

            result.Center = value1.Center + vector * (max + min);
            result.Radius = max;
        }

        /// <summary>
        /// Constructs a <see cref="BoundingSphere" /> that is the as large as the total combined area of the two specified
        /// spheres.
        /// </summary>
        /// <param name="value1">The first sphere to merge.</param>
        /// <param name="value2">The second sphere to merge.</param>
        /// <returns>The newly constructed bounding sphere.</returns>
        public static BoundingSphere Merge(BoundingSphere value1, BoundingSphere value2)
        {
            Merge(ref value1, ref value2, out var result);
            return result;
        }

        /// <summary>
        /// Transforms the bounding sphere using the specified matrix.
        /// </summary>
        /// <param name="sphere">The sphere.</param>
        /// <param name="matrix">The matrix.</param>
        /// <remarks>The result transformed sphere.</remarks>
        public static BoundingSphere Transform(BoundingSphere sphere, Matrix matrix)
        {
            Transform(ref sphere, ref matrix, out var result);
            return result;
        }

        /// <summary>
        /// Transforms the bounding sphere using the specified matrix.
        /// </summary>
        /// <param name="sphere">The sphere.</param>
        /// <param name="matrix">The matrix.</param>
        /// <param name="result">The result transformed sphere.</param>
        public static void Transform(ref BoundingSphere sphere, ref Matrix matrix, out BoundingSphere result)
        {
            Vector3.Transform(ref sphere.Center, ref matrix, out result.Center);
            result.Radius = sphere.Radius * matrix.ScaleVector.Absolute.MaxValue;
        }

        /// <summary>
        /// Tests for equality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left" /> has the same value as <paramref name="right" />; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator ==(BoundingSphere left, BoundingSphere right)
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
        public static bool operator !=(BoundingSphere left, BoundingSphere right)
        {
            return !left.Equals(ref right);
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public override string ToString()
        {
            return string.Format(CultureInfo.CurrentCulture, "Center:{0} Radius:{1}", Center.ToString(), Radius.ToString());
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <param name="format">The format.</param>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public string ToString(string format)
        {
            if (format == null)
                return ToString();

            return string.Format(CultureInfo.CurrentCulture,
                                 "Center:{0} Radius:{1}",
                                 Center.ToString(format, CultureInfo.CurrentCulture),
                                 Radius.ToString(format, CultureInfo.CurrentCulture));
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <param name="formatProvider">The format provider.</param>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public string ToString(IFormatProvider formatProvider)
        {
            return string.Format(formatProvider,
                                 "Center:{0} Radius:{1}",
                                 Center.ToString(),
                                 Radius.ToString());
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <param name="format">The format.</param>
        /// <param name="formatProvider">The format provider.</param>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public string ToString(string format, IFormatProvider formatProvider)
        {
            if (format == null)
                return ToString(formatProvider);

            return string.Format(formatProvider,
                                 "Center:{0} Radius:{1}",
                                 Center.ToString(format, formatProvider),
                                 Radius.ToString(format, formatProvider));
        }

        /// <summary>
        /// Returns a hash code for this instance.
        /// </summary>
        /// <returns>A hash code for this instance, suitable for use in hashing algorithms and data structures like a hash table.</returns>
        public override int GetHashCode()
        {
            unchecked
            {
                return (Center.GetHashCode() * 397) ^ Radius.GetHashCode();
            }
        }

        /// <summary>
        /// Determines whether the specified <see cref="Vector4" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="Vector4" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="Vector4" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(ref BoundingSphere other)
        {
            return Center == other.Center && Radius == other.Radius;
        }

        /// <summary>
        /// Determines whether the specified <see cref="Vector4" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="Vector4" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="Vector4" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(BoundingSphere other)
        {
            return Equals(ref other);
        }

        /// <summary>
        /// Determines whether the specified <see cref="System.Object" /> is equal to this instance.
        /// </summary>
        /// <param name="value">The <see cref="System.Object" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="System.Object" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        public override bool Equals(object value)
        {
            return value is BoundingSphere other && Equals(ref other);
        }
    }
}
