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
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Runtime.CompilerServices;

namespace FlaxEngine
{
    [Serializable]
    partial struct OrientedBoundingBox : IEquatable<OrientedBoundingBox>, IFormattable
    {
        /// <summary>
        /// Creates an <see cref="OrientedBoundingBox" /> from a BoundingBox.
        /// </summary>
        /// <param name="bb">The BoundingBox to create from.</param>
        /// <remarks>Initially, the OBB is axis-aligned box, but it can be rotated and transformed later.</remarks>
        public OrientedBoundingBox(BoundingBox bb)
        {
            Vector3 center = bb.Minimum + (bb.Maximum - bb.Minimum) * 0.5f;
            Extents = bb.Maximum - center;
            Transformation = new Transform(center);
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="OrientedBoundingBox"/> struct.
        /// </summary>
        /// <param name="extents">The half lengths of the box along each axis.</param>
        /// <param name="transformation">The matrix which aligns and scales the box, and its translation vector represents the center of the box.</param>
        public OrientedBoundingBox(Vector3 extents, Matrix transformation)
        {
            Extents = extents;
            transformation.Decompose(out Transformation);
        }

        /// <summary>
        /// Creates an <see cref="OrientedBoundingBox" /> which contained between two minimum and maximum points.
        /// </summary>
        /// <param name="minimum">The minimum vertex of the bounding box.</param>
        /// <param name="maximum">The maximum vertex of the bounding box.</param>
        /// <remarks>Initially, the OrientedBoundingBox is axis-aligned box, but it can be rotated and transformed later.</remarks>
        public OrientedBoundingBox(Vector3 minimum, Vector3 maximum)
        {
            Vector3 center = minimum + (maximum - minimum) * 0.5f;
            Extents = maximum - center;
            Transformation = new Transform(center);
        }

        /// <summary>
        /// Creates an <see cref="OrientedBoundingBox" /> that fully contains the given points.
        /// </summary>
        /// <param name="points">The points that will be contained by the box.</param>
        /// <remarks>This method is not for computing the best tight-fitting OrientedBoundingBox. And initially, the OrientedBoundingBox is axis-aligned box, but it can be rotated and transformed later.</remarks>
        public OrientedBoundingBox(Vector3[] points)
        {
            if ((points == null) || (points.Length == 0))
                throw new ArgumentNullException(nameof(points));
            var minimum = points[0];
            var maximum = points[0];
            for (var i = 1; i < points.Length; ++i)
            {
                Vector3.Min(ref minimum, ref points[i], out minimum);
                Vector3.Max(ref maximum, ref points[i], out maximum);
            }
            Vector3 center = minimum + (maximum - minimum) * 0.5f;
            Extents = maximum - center;
            Transformation = new Transform(center);
        }

        /// <summary>
        /// Retrieves the eight corners of the bounding box.
        /// </summary>
        /// <returns>An array of points representing the eight corners of the bounding box.</returns>
        public Vector3[] GetCorners()
        {
            var corners = new Vector3[8];
            GetCorners(corners);
            return corners;
        }

        /// <summary>
        /// Retrieves the eight corners of the bounding box.
        /// </summary>
        /// <param name="corners">An array of points representing the eight corners of the bounding box.</param>
        public unsafe void GetCorners(Vector3[] corners)
        {
            if (corners == null || corners.Length != 8)
                throw new ArgumentException();
            fixed (Vector3* ptr = corners)
                GetCorners(ptr);
        }

        /// <summary>
        /// Retrieves the eight corners of the bounding box.
        /// </summary>
        /// <param name="corners">An array of points representing the eight corners of the bounding box.</param>
        public unsafe void GetCorners(Vector3* corners)
        {
            Vector3 xv = Transformation.LocalToWorldVector(new Vector3(Extents.X, 0, 0));
            Vector3 yv = Transformation.LocalToWorldVector(new Vector3(0, Extents.Y, 0));
            Vector3 zv = Transformation.LocalToWorldVector(new Vector3(0, 0, Extents.Z));
            Vector3 center = Transformation.Translation;

            corners[0] = center + xv + yv + zv;
            corners[1] = center + xv + yv - zv;
            corners[2] = center - xv + yv - zv;
            corners[3] = center - xv + yv + zv;
            corners[4] = center + xv - yv + zv;
            corners[5] = center + xv - yv - zv;
            corners[6] = center - xv - yv - zv;
            corners[7] = center - xv - yv + zv;
        }

        /// <summary>
        /// Retrieves the eight corners of the bounding box.
        /// </summary>
        /// <param name="corners">An collection to add the corners of the bounding box.</param>
        public void GetCorners(List<Vector3> corners)
        {
            if (corners == null)
                throw new ArgumentNullException();
            corners.AddRange(GetCorners());
        }

        /// <summary>
        /// Transforms this box using a transformation.
        /// </summary>
        /// <param name="transform">The transformation.</param>
        /// <remarks>While any kind of transformation can be applied, it is recommended to apply scaling using scale method instead, which scales the Extents and keeps the Transformation for rotation only, and that preserves collision detection accuracy.</remarks>
        public void Transform(ref Transform transform)
        {
            Transformation = transform.LocalToWorld(Transformation);
        }

        /// <summary>
        /// Transforms this box using a transformation matrix.
        /// </summary>
        /// <param name="mat">The transformation matrix.</param>
        /// <remarks>While any kind of transformation can be applied, it is recommended to apply scaling using scale method instead, which scales the Extents and keeps the Transformation matrix for rotation only, and that preserves collision detection accuracy.</remarks>
        public void Transform(ref Matrix mat)
        {
            mat.Decompose(out var transform);
            Transformation = transform.LocalToWorld(Transformation);
        }

        /// <summary>
        /// Transforms this box using a transformation matrix.
        /// </summary>
        /// <param name="mat">The transformation matrix.</param>
        /// <remarks>While any kind of transformation can be applied, it is recommended to apply scaling using scale method instead, which scales the Extents and keeps the Transformation matrix for rotation only, and that preserves collision detection accuracy.</remarks>
        public void Transform(Matrix mat)
        {
            Transform(ref mat);
        }

        /// <summary>
        /// Scales the <see cref="OrientedBoundingBox" /> by scaling its Extents without affecting the Transformation matrix, By keeping Transformation matrix scaling-free, the collision detection methods will be more accurate.
        /// </summary>
        /// <param name="scaling"></param>
        public void Scale(ref Vector3 scaling)
        {
            Extents *= scaling;
        }

        /// <summary>
        /// Scales the <see cref="OrientedBoundingBox" /> by scaling its Extents without affecting the Transformation matrix, By keeping Transformation matrix scaling-free, the collision detection methods will be more accurate.
        /// </summary>
        /// <param name="scaling"></param>
        public void Scale(Vector3 scaling)
        {
            Extents *= scaling;
        }

        /// <summary>
        /// Scales the <see cref="OrientedBoundingBox" /> by scaling its Extents without affecting the Transformation matrix, By keeping Transformation matrix scaling-free, the collision detection methods will be more accurate.
        /// </summary>
        /// <param name="scaling"></param>
        public void Scale(Real scaling)
        {
            Extents *= scaling;
        }

        /// <summary>
        /// Translates the <see cref="OrientedBoundingBox" /> to a new position using a translation vector;
        /// </summary>
        /// <param name="translation">the translation vector.</param>
        public void Translate(ref Vector3 translation)
        {
            Transformation.Translation += translation;
        }

        /// <summary>
        /// Translates the <see cref="OrientedBoundingBox" /> to a new position using a translation vector;
        /// </summary>
        /// <param name="translation">the translation vector.</param>
        public void Translate(Vector3 translation)
        {
            Transformation.Translation += translation;
        }

        /// <summary>
        /// The size of the <see cref="OrientedBoundingBox" /> if no scaling is applied to the transformation matrix.
        /// </summary>
        /// <remarks>The property will return the actual size even if the scaling is applied using Scale method, but if the scaling is applied to transformation matrix, use GetSize Function instead.</remarks>
        public Vector3 Size => Extents * 2;

        /// <summary>
        /// Returns the size of the <see cref="OrientedBoundingBox" /> taking into consideration the scaling applied to the transformation matrix.
        /// </summary>
        /// <returns>The size of the consideration</returns>
        /// <remarks>This method is computationally expensive, so if no scale is applied to the transformation matrix use <see cref="OrientedBoundingBox.Size" /> property instead.</remarks>
        public Vector3 GetSize()
        {
            Vector3 xv = Transformation.LocalToWorldVector(new Vector3(Extents.X * 2, 0, 0));
            Vector3 yv = Transformation.LocalToWorldVector(new Vector3(0, Extents.Y * 2, 0));
            Vector3 zv = Transformation.LocalToWorldVector(new Vector3(0, 0, Extents.Z * 2));
            return new Vector3(xv.Length, yv.Length, zv.Length);
        }

        /// <summary>
        /// Returns the square size of the <see cref="OrientedBoundingBox" /> taking into consideration the scaling applied to the transformation matrix.
        /// </summary>
        /// <returns>The size of the consideration</returns>
        public Vector3 GetSizeSquared()
        {
            Vector3 xv = Transformation.LocalToWorldVector(new Vector3(Extents.X * 2, 0, 0));
            Vector3 yv = Transformation.LocalToWorldVector(new Vector3(0, Extents.Y * 2, 0));
            Vector3 zv = Transformation.LocalToWorldVector(new Vector3(0, 0, Extents.Z * 2));
            return new Vector3(xv.LengthSquared, yv.LengthSquared, zv.LengthSquared);
        }

        /// <summary>
        /// Returns the center of the <see cref="OrientedBoundingBox" />.
        /// </summary>
        public Vector3 Center => Transformation.Translation;

        /// <summary>
        /// Determines whether a <see cref="OrientedBoundingBox" /> contains a point.
        /// </summary>
        /// <param name="point">The point to test.</param>
        /// <returns>The type of containment the two objects have.</returns>
        public ContainmentType Contains(ref Vector3 point)
        {
            // Transform the point into the obb coordinates
            Transformation.WorldToLocal(ref point, out Vector3 locPoint);
            locPoint.X = Math.Abs(locPoint.X);
            locPoint.Y = Math.Abs(locPoint.Y);
            locPoint.Z = Math.Abs(locPoint.Z);

            // Simple axes-aligned BB check
            if (Mathf.NearEqual(locPoint.X, Extents.X) && Mathf.NearEqual(locPoint.Y, Extents.Y) && Mathf.NearEqual(locPoint.Z, Extents.Z))
                return ContainmentType.Intersects;
            if ((locPoint.X < Extents.X) && (locPoint.Y < Extents.Y) && (locPoint.Z < Extents.Z))
                return ContainmentType.Contains;
            return ContainmentType.Disjoint;
        }

        /// <summary>
        /// Determines whether a <see cref="OrientedBoundingBox" /> contains a point.
        /// </summary>
        /// <param name="point">The point to test.</param>
        /// <returns>The type of containment the two objects have.</returns>
        public ContainmentType Contains(Vector3 point)
        {
            return Contains(ref point);
        }

        /// <summary>
        /// Determines whether a <see cref="OrientedBoundingBox" /> contains a <see cref="BoundingSphere" />.
        /// </summary>
        /// <param name="sphere">The sphere to test.</param>
        /// <param name="ignoreScale">Optimize the check operation by assuming that <see cref="OrientedBoundingBox" /> has no scaling applied.</param>
        /// <returns>The type of containment the two objects have.</returns>
        /// <remarks>This method is not designed for <see cref="OrientedBoundingBox" /> which has a non-uniform scaling applied to its transformation matrix. But any type of scaling applied using Scale method will keep this method accurate.</remarks>
        public ContainmentType Contains(BoundingSphere sphere, bool ignoreScale = false)
        {
            // Transform sphere center into the obb coordinates
            Transformation.WorldToLocal(ref sphere.Center, out Vector3 locCenter);

            Real locRadius;
            if (ignoreScale)
                locRadius = sphere.Radius;
            else
            {
                // Transform sphere radius into the obb coordinates
                Vector3 vRadius = Vector3.UnitX * sphere.Radius;
                Transformation.LocalToWorldVector(ref vRadius, out vRadius);
                locRadius = vRadius.Length;
            }

            // Perform regular BoundingBox to BoundingSphere containment check
            Vector3 minusExtens = -Extents;
            Vector3.Clamp(ref locCenter, ref minusExtens, ref Extents, out Vector3 vector);
            Real distance = Vector3.DistanceSquared(ref locCenter, ref vector);

            if (distance > locRadius * locRadius)
                return ContainmentType.Disjoint;
            if ((minusExtens.X + locRadius <= locCenter.X) && (locCenter.X <= Extents.X - locRadius) && (Extents.X - minusExtens.X > locRadius) && (minusExtens.Y + locRadius <= locCenter.Y) && (locCenter.Y <= Extents.Y - locRadius) && (Extents.Y - minusExtens.Y > locRadius) && (minusExtens.Z + locRadius <= locCenter.Z) && (locCenter.Z <= Extents.Z - locRadius) && (Extents.Z - minusExtens.Z > locRadius))
                return ContainmentType.Contains;
            return ContainmentType.Intersects;
        }

        /// <summary>
        /// Determines whether there is an intersection between a <see cref="Ray" /> and a <see cref="OrientedBoundingBox" />.
        /// </summary>
        /// <param name="ray">The ray to test.</param>
        /// <param name="point">When the method completes, contains the point of intersection, or <see cref="Vector3.Zero" /> if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public bool Intersects(ref Ray ray, out Vector3 point)
        {
            // Put ray in box space
            Ray bRay;
            Transformation.WorldToLocalVector(ref ray.Direction, out bRay.Direction);
            Transformation.WorldToLocal(ref ray.Position, out bRay.Position);

            // Perform a regular ray to BoundingBox check
            var bb = new BoundingBox(-Extents, Extents);
            bool intersects = CollisionsHelper.RayIntersectsBox(ref bRay, ref bb, out point);

            // Put the result intersection back to world
            if (intersects)
                Transformation.LocalToWorld(ref point, out point);

            return intersects;
        }

        /// <summary>
        /// Determines whether there is an intersection between a <see cref="Ray" /> and a <see cref="OrientedBoundingBox" />.
        /// </summary>
        /// <param name="ray">The ray to test.</param>
        /// <param name="distance">When the method completes, contains the distance of intersection from the ray start, or 0 if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public bool Intersects(ref Ray ray, out Real distance)
        {
            if (Intersects(ref ray, out Vector3 point))
            {
                Vector3.Distance(ref ray.Position, ref point, out distance);
                return true;
            }
            distance = 0;
            return false;
        }

        /// <summary>
        /// Determines whether there is an intersection between a <see cref="Ray" /> and a <see cref="OrientedBoundingBox" />.
        /// </summary>
        /// <param name="ray">The ray to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public bool Intersects(ref Ray ray)
        {
            return Intersects(ref ray, out Vector3 _);
        }

        /// <summary>
        /// Get the axis-aligned <see cref="BoundingBox" /> which contains all <see cref="OrientedBoundingBox" /> corners.
        /// </summary>
        /// <returns>The axis-aligned BoundingBox of this OrientedBoundingBox.</returns>
        public BoundingBox GetBoundingBox()
        {
            return BoundingBox.FromPoints(GetCorners());
        }

        /// <summary>
        /// Determines whether the specified <see cref="Vector4" /> is equal to this instance.
        /// </summary>
        /// <param name="value">The <see cref="Vector4" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="Vector4" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(ref OrientedBoundingBox value)
        {
            return Extents == value.Extents && Transformation == value.Transformation;
        }

        /// <summary>
        /// Determines whether the specified <see cref="Vector4" /> is equal to this instance.
        /// </summary>
        /// <param name="value">The <see cref="Vector4" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="Vector4" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(OrientedBoundingBox value)
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
            return value is OrientedBoundingBox other && Equals(ref other);
        }

        /// <summary>
        /// Transforms bounding box using the given transformation matrix.
        /// </summary>
        /// <param name="box">The bounding box to transform.</param>
        /// <param name="transform">The transformation matrix.</param>
        /// <returns>The result of the transformation.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static OrientedBoundingBox operator *(OrientedBoundingBox box, Matrix transform)
        {
            OrientedBoundingBox result = box;
            result.Transform(ref transform);
            return result;
        }

        /// <summary>
        /// Transforms bounding box using the given transformation.
        /// </summary>
        /// <param name="box">The bounding box to transform.</param>
        /// <param name="transform">The transformation.</param>
        /// <returns>The result of the transformation.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static OrientedBoundingBox operator *(OrientedBoundingBox box, Transform transform)
        {
            OrientedBoundingBox result = box;
            result.Transformation = transform.LocalToWorld(result.Transformation);
            return result;
        }

        /// <summary>
        /// Tests for equality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left" /> has the same value as <paramref name="right" />; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator ==(OrientedBoundingBox left, OrientedBoundingBox right)
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
        public static bool operator !=(OrientedBoundingBox left, OrientedBoundingBox right)
        {
            return !left.Equals(ref right);
        }

        /// <summary>
        /// Returns a hash code for this instance.
        /// </summary>
        /// <returns>A hash code for this instance, suitable for use in hashing algorithms and data structures like a hash table.</returns>
        public override int GetHashCode()
        {
            return Extents.GetHashCode() + Transformation.GetHashCode();
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public override string ToString()
        {
            return string.Format(CultureInfo.CurrentCulture, "Center: {0}, Extents: {1}", Center, Extents);
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
            return string.Format(CultureInfo.CurrentCulture, "Center: {0}, Extents: {1}", Center.ToString(format, CultureInfo.CurrentCulture), Extents.ToString(format, CultureInfo.CurrentCulture));
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <param name="formatProvider">The format provider.</param>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public string ToString(IFormatProvider formatProvider)
        {
            return string.Format(formatProvider, "Center: {0}, Extents: {1}", Center.ToString(), Extents.ToString());
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
            return string.Format(formatProvider, "Center: {0}, Extents: {1}", Center.ToString(format, formatProvider), Extents.ToString(format, formatProvider));
        }
    }
}
