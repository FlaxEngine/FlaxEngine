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

namespace FlaxEngine
{
    [Serializable]
    partial struct BoundingBox : IEquatable<BoundingBox>, IFormattable
    {
        /// <summary>
        /// A <see cref="BoundingBox"/> which represents an empty space.
        /// </summary>
        public static readonly BoundingBox Empty = new BoundingBox(Vector3.Maximum, Vector3.Minimum);

        /// <summary>
        /// A <see cref="BoundingBox"/> which is located in point (0, 0, 0) and has size equal (0, 0, 0).
        /// </summary>
        public static readonly BoundingBox Zero = new BoundingBox(Vector3.Zero, Vector3.Zero);

        /// <summary>
        /// Gets or sets the size.
        /// </summary>
        [NoSerialize]
        public Vector3 Size
        {
            get => Maximum - Minimum;
            set
            {
                var center = Center;
                var sizeHalf = value * 0.5f;
                Minimum = center - sizeHalf;
                Maximum = center + sizeHalf;
            }
        }

        /// <summary>
        /// Gets or sets the center point location.
        /// </summary>
        [NoSerialize]
        public Vector3 Center
        {
            get => Minimum + (Maximum - Minimum) * 0.5f;
            set
            {
                var sizeHalf = Size * 0.5f;
                Minimum = value - sizeHalf;
                Maximum = value + sizeHalf;
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="BoundingBox" /> struct.
        /// </summary>
        /// <param name="minimum">The minimum vertex of the bounding box.</param>
        /// <param name="maximum">The maximum vertex of the bounding box.</param>
        public BoundingBox(Vector3 minimum, Vector3 maximum)
        {
            Minimum = minimum;
            Maximum = maximum;
        }

        /// <summary>
        /// Retrieves the eight corners of the bounding box.
        /// </summary>
        /// <returns>An array of points representing the eight corners of the bounding box.</returns>
        public Vector3[] GetCorners()
        {
            var results = new Vector3[8];
            GetCorners(results);
            return results;
        }

        /// <summary>
        /// Retrieves the eight corners of the bounding box.
        /// </summary>
        /// <returns>An array of points representing the eight corners of the bounding box.</returns>
        public void GetCorners(Vector3[] corners)
        {
            corners[0] = new Vector3(Minimum.X, Maximum.Y, Maximum.Z);
            corners[1] = new Vector3(Maximum.X, Maximum.Y, Maximum.Z);
            corners[2] = new Vector3(Maximum.X, Minimum.Y, Maximum.Z);
            corners[3] = new Vector3(Minimum.X, Minimum.Y, Maximum.Z);
            corners[4] = new Vector3(Minimum.X, Maximum.Y, Minimum.Z);
            corners[5] = new Vector3(Maximum.X, Maximum.Y, Minimum.Z);
            corners[6] = new Vector3(Maximum.X, Minimum.Y, Minimum.Z);
            corners[7] = new Vector3(Minimum.X, Minimum.Y, Minimum.Z);
        }

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="Ray" />.
        /// </summary>
        /// <param name="ray">The ray to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public bool Intersects(ref Ray ray)
        {
            return CollisionsHelper.RayIntersectsBox(ref ray, ref this, out Real _);
        }

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="Ray" />.
        /// </summary>
        /// <param name="ray">The ray to test.</param>
        /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public bool Intersects(ref Ray ray, out Real distance)
        {
            return CollisionsHelper.RayIntersectsBox(ref ray, ref this, out distance);
        }

#if USE_LARGE_WORLDS
        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="Ray" />.
        /// [Deprecated on 26.05.2022, expires on 26.05.2024]
        /// </summary>
        /// <param name="ray">The ray to test.</param>
        /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        [Obsolete("Use Intersects with 'out Real distance' parameter instead")]
        public bool Intersects(ref Ray ray, out float distance)
        {
            var result = CollisionsHelper.RayIntersectsBox(ref ray, ref this, out Real dst);
            distance = (float)dst;
            return result;
        }
#endif

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="Ray" />.
        /// </summary>
        /// <param name="ray">The ray to test.</param>
        /// <param name="point">When the method completes, contains the point of intersection, or <see cref="Vector3.Zero" /> if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public bool Intersects(ref Ray ray, out Vector3 point)
        {
            return CollisionsHelper.RayIntersectsBox(ref ray, ref this, out point);
        }

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="Plane" />.
        /// </summary>
        /// <param name="plane">The plane to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public PlaneIntersectionType Intersects(ref Plane plane)
        {
            return CollisionsHelper.PlaneIntersectsBox(ref plane, ref this);
        }

        /* This implementation is wrong
        /// <summary>
        /// Determines if there is an intersection between the current object and a triangle.
        /// </summary>
        /// <param name="vertex1">The first vertex of the triangle to test.</param>
        /// <param name="vertex2">The second vertex of the triangle to test.</param>
        /// <param name="vertex3">The third vertex of the triangle to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public bool Intersects(ref Vector3 vertex1, ref Vector3 vertex2, ref Vector3 vertex3)
        {
            return Collision.BoxIntersectsTriangle(ref this, ref vertex1, ref vertex2, ref vertex3);
        }
        */

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="BoundingBox" />.
        /// </summary>
        /// <param name="box">The box to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public bool Intersects(ref BoundingBox box)
        {
            return CollisionsHelper.BoxIntersectsBox(ref this, ref box);
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
            return CollisionsHelper.BoxIntersectsSphere(ref this, ref sphere);
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
            return CollisionsHelper.BoxContainsPoint(ref this, ref point);
        }

        /// <summary>
        /// Determines whether the current objects contains a point.
        /// </summary>
        /// <param name="point">The point to test.</param>
        /// <returns>The type of containment the two objects have.</returns>
        public ContainmentType Contains(Vector3 point)
        {
            return Contains(ref point);
        }

        /* This implementation is wrong
        /// <summary>
        /// Determines whether the current objects contains a triangle.
        /// </summary>
        /// <param name="vertex1">The first vertex of the triangle to test.</param>
        /// <param name="vertex2">The second vertex of the triangle to test.</param>
        /// <param name="vertex3">The third vertex of the triangle to test.</param>
        /// <returns>The type of containment the two objects have.</returns>
        public ContainmentType Contains(ref Vector3 vertex1, ref Vector3 vertex2, ref Vector3 vertex3)
        {
            return Collision.BoxContainsTriangle(ref this, ref vertex1, ref vertex2, ref vertex3);
        }
        */

        /// <summary>
        /// Determines whether the current objects contains a <see cref="BoundingBox" />.
        /// </summary>
        /// <param name="box">The box to test.</param>
        /// <returns>The type of containment the two objects have.</returns>
        public ContainmentType Contains(ref BoundingBox box)
        {
            return CollisionsHelper.BoxContainsBox(ref this, ref box);
        }

        /// <summary>
        /// Determines whether the current objects contains a <see cref="BoundingBox" />.
        /// </summary>
        /// <param name="box">The box to test.</param>
        /// <returns>The type of containment the two objects have.</returns>
        public ContainmentType Contains(BoundingBox box)
        {
            return Contains(ref box);
        }

        /// <summary>
        /// Determines whether the current objects contains a <see cref="BoundingSphere" />.
        /// </summary>
        /// <param name="sphere">The sphere to test.</param>
        /// <returns>The type of containment the two objects have.</returns>
        public ContainmentType Contains(ref BoundingSphere sphere)
        {
            return CollisionsHelper.BoxContainsSphere(ref this, ref sphere);
        }

        /// <summary>
        /// Determines whether the current objects contains a <see cref="BoundingSphere" />.
        /// </summary>
        /// <param name="sphere">The sphere to test.</param>
        /// <returns>The type of containment the two objects have.</returns>
        public ContainmentType Contains(BoundingSphere sphere)
        {
            return Contains(ref sphere);
        }

        /// <summary>
        /// Constructs a <see cref="BoundingBox" /> that fully contains the given points.
        /// </summary>
        /// <param name="points">The points that will be contained by the box.</param>
        /// <param name="result">When the method completes, contains the newly constructed bounding box.</param>
        /// <exception cref="ArgumentNullException">Thrown when <paramref name="points" /> is <c>null</c>.</exception>
        public static void FromPoints(Vector3[] points, out BoundingBox result)
        {
            if (points == null)
                throw new ArgumentNullException(nameof(points));
            var min = Vector3.Maximum;
            var max = Vector3.Minimum;
            for (var i = 0; i < points.Length; ++i)
            {
                Vector3.Min(ref min, ref points[i], out min);
                Vector3.Max(ref max, ref points[i], out max);
            }
            result = new BoundingBox(min, max);
        }

        /// <summary>
        /// Constructs a <see cref="BoundingBox" /> that fully contains the given points.
        /// </summary>
        /// <param name="points">The points that will be contained by the box.</param>
        /// <returns>The newly constructed bounding box.</returns>
        /// <exception cref="ArgumentNullException">Thrown when <paramref name="points" /> is <c>null</c>.</exception>
        public static BoundingBox FromPoints(Vector3[] points)
        {
            if (points == null)
                throw new ArgumentNullException(nameof(points));
            var min = Vector3.Maximum;
            var max = Vector3.Minimum;
            for (var i = 0; i < points.Length; ++i)
            {
                Vector3.Min(ref min, ref points[i], out min);
                Vector3.Max(ref max, ref points[i], out max);
            }
            return new BoundingBox(min, max);
        }

        /// <summary>
        /// Constructs a <see cref="BoundingBox" /> from a given sphere.
        /// </summary>
        /// <param name="sphere">The sphere that will designate the extents of the box.</param>
        /// <param name="result">When the method completes, contains the newly constructed bounding box.</param>
        public static void FromSphere(ref BoundingSphere sphere, out BoundingBox result)
        {
            result.Minimum = new Vector3(sphere.Center.X - sphere.Radius, sphere.Center.Y - sphere.Radius, sphere.Center.Z - sphere.Radius);
            result.Maximum = new Vector3(sphere.Center.X + sphere.Radius, sphere.Center.Y + sphere.Radius, sphere.Center.Z + sphere.Radius);
        }

        /// <summary>
        /// Constructs a <see cref="BoundingBox" /> from a given sphere.
        /// </summary>
        /// <param name="sphere">The sphere that will designate the extents of the box.</param>
        /// <returns>The newly constructed bounding box.</returns>
        public static BoundingBox FromSphere(BoundingSphere sphere)
        {
            BoundingBox box;
            box.Minimum = new Vector3(sphere.Center.X - sphere.Radius, sphere.Center.Y - sphere.Radius, sphere.Center.Z - sphere.Radius);
            box.Maximum = new Vector3(sphere.Center.X + sphere.Radius, sphere.Center.Y + sphere.Radius, sphere.Center.Z + sphere.Radius);
            return box;
        }

        /// <summary>
        /// Constructs a <see cref="BoundingBox" /> that is as large as the total combined area of the two specified boxes.
        /// </summary>
        /// <param name="value1">The first box to merge.</param>
        /// <param name="value2">The second box to merge.</param>
        /// <param name="result">When the method completes, contains the newly constructed bounding box.</param>
        public static void Merge(ref BoundingBox value1, ref BoundingBox value2, out BoundingBox result)
        {
            Vector3.Min(ref value1.Minimum, ref value2.Minimum, out result.Minimum);
            Vector3.Max(ref value1.Maximum, ref value2.Maximum, out result.Maximum);
        }

        /// <summary>
        /// Constructs a <see cref="BoundingBox" /> that is as large as the total combined area of the two specified boxes.
        /// </summary>
        /// <param name="value1">The first box to merge.</param>
        /// <param name="value2">The second box to merge.</param>
        /// <returns>The newly constructed bounding box.</returns>
        public static BoundingBox Merge(BoundingBox value1, BoundingBox value2)
        {
            BoundingBox box;
            Vector3.Min(ref value1.Minimum, ref value2.Minimum, out box.Minimum);
            Vector3.Max(ref value1.Maximum, ref value2.Maximum, out box.Maximum);
            return box;
        }

        /// <summary>
        /// Constructs a <see cref="BoundingBox" /> that is as large as the box and point.
        /// </summary>
        /// <param name="value1">The box to merge.</param>
        /// <param name="value2">The point to merge.</param>
        /// <param name="result">When the method completes, contains the newly constructed bounding box.</param>
        public static void Merge(ref BoundingBox value1, ref Vector3 value2, out BoundingBox result)
        {
            Vector3.Min(ref value1.Minimum, ref value2, out result.Minimum);
            Vector3.Max(ref value1.Maximum, ref value2, out result.Maximum);
        }

        /// <summary>
        /// Constructs a <see cref="BoundingBox" /> that is as large as the box and point.
        /// </summary>
        /// <param name="value2">The point to merge.</param>
        /// <returns>The newly constructed bounding box.</returns>
        public BoundingBox Merge(Vector3 value2)
        {
            BoundingBox result;
            Vector3.Min(ref Minimum, ref value2, out result.Minimum);
            Vector3.Max(ref Maximum, ref value2, out result.Maximum);
            return result;
        }

        /// <summary>
        /// Transforms bounding box using the given transformation matrix.
        /// </summary>
        /// <param name="box">The bounding box to transform.</param>
        /// <param name="transform">The transformation matrix.</param>
        /// <returns>The result of the transformation.</returns>
        public static BoundingBox Transform(BoundingBox box, Matrix transform)
        {
            Transform(ref box, ref transform, out BoundingBox result);
            return result;
        }

        /// <summary>
        /// Transforms bounding box using the given transformation matrix.
        /// </summary>
        /// <param name="box">The bounding box to transform.</param>
        /// <param name="transform">The transformation matrix.</param>
        /// <param name="result">The result of the transformation.</param>
        public static void Transform(ref BoundingBox box, ref Matrix transform, out BoundingBox result)
        {
            // Reference: http://dev.theomader.com/transform-bounding-boxes/

            Double3 right = transform.Right;
            var xa = right * box.Minimum.X;
            var xb = right * box.Maximum.X;

            Double3 up = transform.Up;
            var ya = up * box.Minimum.Y;
            var yb = up * box.Maximum.Y;

            Double3 forward = transform.Forward;
            var za = forward * box.Minimum.Z;
            var zb = forward * box.Maximum.Z;

            var translation = transform.TranslationVector;
            var min = Vector3.Min(xa, xb) + Vector3.Min(ya, yb) + Vector3.Min(za, zb) + translation;
            var max = Vector3.Max(xa, xb) + Vector3.Max(ya, yb) + Vector3.Max(za, zb) + translation;
            result = new BoundingBox(min, max);
        }

        /// <summary>
        /// Transforms bounding box using the given transformation matrix.
        /// </summary>
        /// <param name="box">The bounding box to transform.</param>
        /// <param name="transform">The transformation matrix.</param>
        /// <returns>The result of the transformation.</returns>
        public static BoundingBox Transform(BoundingBox box, Transform transform)
        {
            Transform(ref box, ref transform, out BoundingBox result);
            return result;
        }

        /// <summary>
        /// Transforms bounding box using the given transformation.
        /// </summary>
        /// <param name="box">The bounding box to transform.</param>
        /// <param name="transform">The transformation.</param>
        /// <param name="result">The result of the transformation.</param>
        public static void Transform(ref BoundingBox box, ref Transform transform, out BoundingBox result)
        {
            // Reference: http://dev.theomader.com/transform-bounding-boxes/

            Double3 right = transform.Right;
            var xa = right * box.Minimum.X;
            var xb = right * box.Maximum.X;

            Double3 up = transform.Up;
            var ya = up * box.Minimum.Y;
            var yb = up * box.Maximum.Y;

            Double3 forward = transform.Forward;
            var za = forward * box.Minimum.Z;
            var zb = forward * box.Maximum.Z;

            var min = Vector3.Min(xa, xb) + Vector3.Min(ya, yb) + Vector3.Min(za, zb) + transform.Translation;
            var max = Vector3.Max(xa, xb) + Vector3.Max(ya, yb) + Vector3.Max(za, zb) + transform.Translation;
            result = new BoundingBox(min, max);
        }

        /// <summary>
        /// Creates the bounding box that is offseted by the given vector. Adds the offset value to minimum and maximum points.
        /// </summary>
        /// <param name="offset">The bounds offset.</param>
        /// <returns>The offsetted bounds.</returns>
        public BoundingBox MakeOffsetted(Vector3 offset)
        {
            BoundingBox result;
            Vector3.Add(ref Minimum, ref offset, out result.Minimum);
            Vector3.Add(ref Maximum, ref offset, out result.Maximum);
            return result;
        }

        /// <summary>
        /// Creates the bounding box that is offseted by the given vector. Adds the offset value to minimum and maximum points.
        /// </summary>
        /// <param name="box">The box.</param>
        /// <param name="offset">The bounds offset.</param>
        /// <returns>The offsetted bounds.</returns>
        public static BoundingBox MakeOffsetted(ref BoundingBox box, ref Vector3 offset)
        {
            BoundingBox result;
            Vector3.Add(ref box.Minimum, ref offset, out result.Minimum);
            Vector3.Add(ref box.Maximum, ref offset, out result.Maximum);
            return result;
        }

        /// <summary>
        /// Creates the bounding box that is scaled by the given factor. Applies scale to the size of the bounds.
        /// </summary>
        /// <param name="box">The box.</param>
        /// <param name="scale">The bounds scale.</param>
        /// <returns>The scaled bounds.</returns>
        public static BoundingBox MakeScaled(ref BoundingBox box, Real scale)
        {
            Vector3.Subtract(ref box.Maximum, ref box.Minimum, out var size);
            Vector3 sizeHalf = size * 0.5f;
            Vector3 center = box.Minimum + sizeHalf;
            sizeHalf *= scale;
            return new BoundingBox(center - sizeHalf, center + sizeHalf);
        }

        /// <summary>
        /// Transforms bounding box using the given transformation matrix.
        /// </summary>
        /// <param name="box">The bounding box to transform.</param>
        /// <param name="transform">The transformation matrix.</param>
        /// <returns>The result of the transformation.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static BoundingBox operator *(BoundingBox box, Matrix transform)
        {
            Transform(ref box, ref transform, out BoundingBox result);
            return result;
        }

        /// <summary>
        /// Tests for equality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left" /> has the same value as <paramref name="right" />; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator ==(BoundingBox left, BoundingBox right)
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
        public static bool operator !=(BoundingBox left, BoundingBox right)
        {
            return !left.Equals(ref right);
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public override string ToString()
        {
            return string.Format(CultureInfo.CurrentCulture, "Minimum:{0} Maximum:{1}", Minimum.ToString(), Maximum.ToString());
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
            return string.Format(CultureInfo.CurrentCulture, "Minimum:{0} Maximum:{1}", Minimum.ToString(format, CultureInfo.CurrentCulture), Maximum.ToString(format, CultureInfo.CurrentCulture));
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <param name="formatProvider">The format provider.</param>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public string ToString(IFormatProvider formatProvider)
        {
            return string.Format(formatProvider, "Minimum:{0} Maximum:{1}", Minimum.ToString(), Maximum.ToString());
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
            return string.Format(formatProvider, "Minimum:{0} Maximum:{1}", Minimum.ToString(format, formatProvider), Maximum.ToString(format, formatProvider));
        }

        /// <summary>
        /// Returns a hash code for this instance.
        /// </summary>
        /// <returns>A hash code for this instance, suitable for use in hashing algorithms and data structures like a hash table.</returns>
        public override int GetHashCode()
        {
            unchecked
            {
                return (Minimum.GetHashCode() * 397) ^ Maximum.GetHashCode();
            }
        }

        /// <summary>
        /// Determines whether the specified <see cref="Vector4" /> is equal to this instance.
        /// </summary>
        /// <param name="value">The <see cref="Vector4" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="Vector4" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(ref BoundingBox value)
        {
            return Minimum == value.Minimum && Maximum == value.Maximum;
        }

        /// <summary>
        /// Determines whether the specified <see cref="Vector4" /> is equal to this instance.
        /// </summary>
        /// <param name="value">The <see cref="Vector4" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="Vector4" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(BoundingBox value)
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
            return value is BoundingBox other && Equals(ref other);
        }
    }
}
