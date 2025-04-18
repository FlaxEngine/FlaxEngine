// Copyright (c) Wojciech Figat. All rights reserved.

#if USE_LARGE_WORLDS
using Real = System.Double;
using Mathr = FlaxEngine.Mathd;
#else
using Real = System.Single;
using Mathr = FlaxEngine.Mathf;
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
    partial struct Plane : IEquatable<Plane>, IFormattable
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="T:FlaxEngine.Plane" /> class.
        /// </summary>
        /// <param name="point">Any point that lies along the plane.</param>
        /// <param name="normal">The normal vector to the plane.</param>
        public Plane(Vector3 point, Vector3 normal)
        {
            Normal = normal;
            D = -Vector3.Dot(normal, point);
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Plane" /> struct.
        /// </summary>
        /// <param name="value">The normal of the plane.</param>
        /// <param name="d">The distance of the plane along its normal from the origin</param>
        public Plane(Vector3 value, Real d)
        {
            Normal = value;
            D = d;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Plane" /> struct.
        /// </summary>
        /// <param name="point1">First point of a triangle defining the plane.</param>
        /// <param name="point2">Second point of a triangle defining the plane.</param>
        /// <param name="point3">Third point of a triangle defining the plane.</param>
        public Plane(Vector3 point1, Vector3 point2, Vector3 point3)
        {
            Real x1 = point2.X - point1.X;
            Real y1 = point2.Y - point1.Y;
            Real z1 = point2.Z - point1.Z;
            Real x2 = point3.X - point1.X;
            Real y2 = point3.Y - point1.Y;
            Real z2 = point3.Z - point1.Z;
            Real yz = y1 * z2 - z1 * y2;
            Real xz = z1 * x2 - x1 * z2;
            Real xy = x1 * y2 - y1 * x2;
            Real invPyth = 1.0f / (Real)Math.Sqrt(yz * yz + xz * xz + xy * xy);

            Normal.X = yz * invPyth;
            Normal.Y = xz * invPyth;
            Normal.Z = xy * invPyth;
            D = -(Normal.X * point1.X + Normal.Y * point1.Y + Normal.Z * point1.Z);
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Plane" /> struct.
        /// </summary>
        /// <param name="values">The values to assign to the A, B, C, and D components of the plane. This must be an array with four elements.</param>
        /// <exception cref="ArgumentNullException">Thrown when <paramref name="values" /> is <c>null</c>.</exception>
        /// <exception cref="ArgumentOutOfRangeException">Thrown when <paramref name="values" /> contains more or less than four elements.</exception>
        public Plane(Real[] values)
        {
            if (values == null)
                throw new ArgumentNullException(nameof(values));
            if (values.Length != 4)
                throw new ArgumentOutOfRangeException(nameof(values), "There must be four and only four input values for Plane.");
            Normal.X = values[0];
            Normal.Y = values[1];
            Normal.Z = values[2];
            D = values[3];
        }

        /// <summary>
        /// Gets or sets the component at the specified index.
        /// </summary>
        /// <value>The value of the A, B, C, or D component, depending on the index.</value>
        /// <param name="index">The index of the component to access. Use 0 for the A component, 1 for the B component, 2 for the C component, and 3 for the D component.</param>
        /// <returns>The value of the component at the specified index.</returns>
        /// <exception cref="System.ArgumentOutOfRangeException">Thrown when the <paramref name="index" /> is out of the range [0,3].</exception>
        public Real this[int index]
        {
            get
            {
                switch (index)
                {
                case 0: return Normal.X;
                case 1: return Normal.Y;
                case 2: return Normal.Z;
                case 3: return D;
                }
                throw new ArgumentOutOfRangeException(nameof(index), "Indices for Plane run from 0 to 3, inclusive.");
            }
            set
            {
                switch (index)
                {
                case 0:
                    Normal.X = value;
                    break;
                case 1:
                    Normal.Y = value;
                    break;
                case 2:
                    Normal.Z = value;
                    break;
                case 3:
                    D = value;
                    break;
                default: throw new ArgumentOutOfRangeException(nameof(index), "Indices for Plane run from 0 to 3, inclusive.");
                }
            }
        }

        /// <summary>
        /// Changes the coefficients of the normal vector of the plane to make it of unit length.
        /// </summary>
        public void Normalize()
        {
            Real length = Normal.Length;
            if (length >= Mathr.Epsilon)
            {
                Real rcp = 1.0f / length;
                Normal.X *= rcp;
                Normal.Y *= rcp;
                Normal.Z *= rcp;
                D *= rcp;
            }
        }

        /// <summary>
        /// Creates an array containing the elements of the plane.
        /// </summary>
        /// <returns>A four-element array containing the components of the plane.</returns>
        public Real[] ToArray()
        {
            return new[] { Normal.X, Normal.Y, Normal.Z, D };
        }

        /// <summary>
        /// Determines if there is an intersection between the current object and a point.
        /// </summary>
        /// <param name="point">The point to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public PlaneIntersectionType Intersects(ref Vector3 point)
        {
            return CollisionsHelper.PlaneIntersectsPoint(ref this, ref point);
        }

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="Ray" />.
        /// </summary>
        /// <param name="ray">The ray to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public bool Intersects(ref Ray ray)
        {
            return CollisionsHelper.RayIntersectsPlane(ref ray, ref this, out Real _);
        }

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="Ray" />.
        /// </summary>
        /// <param name="ray">The ray to test.</param>
        /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public bool Intersects(ref Ray ray, out Real distance)
        {
            return CollisionsHelper.RayIntersectsPlane(ref ray, ref this, out distance);
        }

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="Ray" />.
        /// </summary>
        /// <param name="ray">The ray to test.</param>
        /// <param name="point">When the method completes, contains the point of intersection, or <see cref="Vector3.Zero" /> if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public bool Intersects(ref Ray ray, out Vector3 point)
        {
            return CollisionsHelper.RayIntersectsPlane(ref ray, ref this, out point);
        }

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="Plane" />.
        /// </summary>
        /// <param name="plane">The plane to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public bool Intersects(ref Plane plane)
        {
            return CollisionsHelper.PlaneIntersectsPlane(ref this, ref plane);
        }

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="Plane" />.
        /// </summary>
        /// <param name="plane">The plane to test.</param>
        /// <param name="line">When the method completes, contains the line of intersection as a <see cref="Ray" />, or a zero ray if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public bool Intersects(ref Plane plane, out Ray line)
        {
            return CollisionsHelper.PlaneIntersectsPlane(ref this, ref plane, out line);
        }

        /// <summary>
        /// Determines if there is an intersection between the current object and a triangle.
        /// </summary>
        /// <param name="vertex1">The first vertex of the triangle to test.</param>
        /// <param name="vertex2">The second vertex of the triangle to test.</param>
        /// <param name="vertex3">The third vertex of the triangle to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public PlaneIntersectionType Intersects(ref Vector3 vertex1, ref Vector3 vertex2, ref Vector3 vertex3)
        {
            return CollisionsHelper.PlaneIntersectsTriangle(ref this, ref vertex1, ref vertex2, ref vertex3);
        }

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="BoundingBox" />.
        /// </summary>
        /// <param name="box">The box to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public PlaneIntersectionType Intersects(ref BoundingBox box)
        {
            return CollisionsHelper.PlaneIntersectsBox(ref this, ref box);
        }

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="BoundingSphere" />.
        /// </summary>
        /// <param name="sphere">The sphere to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        public PlaneIntersectionType Intersects(ref BoundingSphere sphere)
        {
            return CollisionsHelper.PlaneIntersectsSphere(ref this, ref sphere);
        }

        /// <summary>
        /// Scales the plane by the given scaling factor.
        /// </summary>
        /// <param name="value">The plane to scale.</param>
        /// <param name="scale">The amount by which to scale the plane.</param>
        /// <param name="result">When the method completes, contains the scaled plane.</param>
        public static void Multiply(ref Plane value, Real scale, out Plane result)
        {
            result.Normal.X = value.Normal.X * scale;
            result.Normal.Y = value.Normal.Y * scale;
            result.Normal.Z = value.Normal.Z * scale;
            result.D = value.D * scale;
        }

        /// <summary>
        /// Scales the plane by the given scaling factor.
        /// </summary>
        /// <param name="value">The plane to scale.</param>
        /// <param name="scale">The amount by which to scale the plane.</param>
        /// <returns>The scaled plane.</returns>
        public static Plane Multiply(Plane value, Real scale)
        {
            return new Plane(value.Normal * scale, value.D * scale);
        }

        /// <summary>
        /// Calculates the dot product of the specified vector and plane.
        /// </summary>
        /// <param name="left">The source plane.</param>
        /// <param name="right">The source vector.</param>
        /// <param name="result">When the method completes, contains the dot product of the specified plane and vector.</param>
        public static void Dot(ref Plane left, ref Vector4 right, out Real result)
        {
            result = left.Normal.X * right.X + left.Normal.Y * right.Y + left.Normal.Z * right.Z + left.D * right.W;
        }

        /// <summary>
        /// Calculates the dot product of the specified vector and plane.
        /// </summary>
        /// <param name="left">The source plane.</param>
        /// <param name="right">The source vector.</param>
        /// <returns>The dot product of the specified plane and vector.</returns>
        public static Real Dot(Plane left, Vector4 right)
        {
            return left.Normal.X * right.X + left.Normal.Y * right.Y + left.Normal.Z * right.Z + left.D * right.W;
        }

        /// <summary>
        /// Calculates the dot product of a specified vector and the normal of the plane plus the distance value of the plane.
        /// </summary>
        /// <param name="left">The source plane.</param>
        /// <param name="right">The source vector.</param>
        /// <param name="result">When the method completes, contains the dot product of a specified vector and the normal of the Plane plus the distance value of the plane.</param>
        public static void DotCoordinate(ref Plane left, ref Vector3 right, out Real result)
        {
            result = left.Normal.X * right.X + left.Normal.Y * right.Y + left.Normal.Z * right.Z + left.D;
        }

        /// <summary>
        /// Calculates the dot product of a specified vector and the normal of the plane plus the distance value of the plane.
        /// </summary>
        /// <param name="left">The source plane.</param>
        /// <param name="right">The source vector.</param>
        /// <returns>The dot product of a specified vector and the normal of the Plane plus the distance value of the plane.</returns>
        public static Real DotCoordinate(Plane left, Vector3 right)
        {
            return left.Normal.X * right.X + left.Normal.Y * right.Y + left.Normal.Z * right.Z + left.D;
        }

        /// <summary>
        /// Calculates the dot product of the specified vector and the normal of the plane.
        /// </summary>
        /// <param name="left">The source plane.</param>
        /// <param name="right">The source vector.</param>
        /// <param name="result">When the method completes, contains the dot product of the specified vector and the normal of the plane.</param>
        public static void DotNormal(ref Plane left, ref Vector3 right, out Real result)
        {
            result = left.Normal.X * right.X + left.Normal.Y * right.Y + left.Normal.Z * right.Z;
        }

        /// <summary>
        /// Calculates the dot product of the specified vector and the normal of the plane.
        /// </summary>
        /// <param name="left">The source plane.</param>
        /// <param name="right">The source vector.</param>
        /// <returns>The dot product of the specified vector and the normal of the plane.</returns>
        public static Real DotNormal(Plane left, Vector3 right)
        {
            return left.Normal.X * right.X + left.Normal.Y * right.Y + left.Normal.Z * right.Z;
        }

        /// <summary>
        /// Changes the coefficients of the normal vector of the plane to make it of unit length.
        /// </summary>
        /// <param name="plane">The source plane.</param>
        /// <param name="result">When the method completes, contains the normalized plane.</param>
        public static void Normalize(ref Plane plane, out Plane result)
        {
            Real magnitude = 1.0f / (Real)Math.Sqrt(plane.Normal.X * plane.Normal.X + plane.Normal.Y * plane.Normal.Y + plane.Normal.Z * plane.Normal.Z);
            result.Normal.X = plane.Normal.X * magnitude;
            result.Normal.Y = plane.Normal.Y * magnitude;
            result.Normal.Z = plane.Normal.Z * magnitude;
            result.D = plane.D * magnitude;
        }

        /// <summary>
        /// Changes the coefficients of the normal vector of the plane to make it of unit length.
        /// </summary>
        /// <param name="plane">The source plane.</param>
        /// <returns>The normalized plane.</returns>
        public static Plane Normalize(Plane plane)
        {
            Real magnitude = 1.0f / (Real)Math.Sqrt(plane.Normal.X * plane.Normal.X + plane.Normal.Y * plane.Normal.Y + plane.Normal.Z * plane.Normal.Z);
            return new Plane(plane.Normal * magnitude, plane.D * magnitude);
        }

        /// <summary>
        /// Transforms a normalized plane by a quaternion rotation.
        /// </summary>
        /// <param name="plane">The normalized source plane.</param>
        /// <param name="rotation">The quaternion rotation.</param>
        /// <param name="result">When the method completes, contains the transformed plane.</param>
        public static void Transform(ref Plane plane, ref Quaternion rotation, out Plane result)
        {
            Real x2 = rotation.X + rotation.X;
            Real y2 = rotation.Y + rotation.Y;
            Real z2 = rotation.Z + rotation.Z;
            Real wx = rotation.W * x2;
            Real wy = rotation.W * y2;
            Real wz = rotation.W * z2;
            Real xx = rotation.X * x2;
            Real xy = rotation.X * y2;
            Real xz = rotation.X * z2;
            Real yy = rotation.Y * y2;
            Real yz = rotation.Y * z2;
            Real zz = rotation.Z * z2;

            Real x = plane.Normal.X;
            Real y = plane.Normal.Y;
            Real z = plane.Normal.Z;

            result.Normal.X = x * (1.0f - yy - zz) + y * (xy - wz) + z * (xz + wy);
            result.Normal.Y = x * (xy + wz) + y * (1.0f - xx - zz) + z * (yz - wx);
            result.Normal.Z = x * (xz - wy) + y * (yz + wx) + z * (1.0f - xx - yy);
            result.D = plane.D;
        }

        /// <summary>
        /// Transforms a normalized plane by a quaternion rotation.
        /// </summary>
        /// <param name="plane">The normalized source plane.</param>
        /// <param name="rotation">The quaternion rotation.</param>
        /// <returns>The transformed plane.</returns>
        public static Plane Transform(Plane plane, Quaternion rotation)
        {
            Transform(ref plane, ref rotation, out var result);
            return result;
        }

        /// <summary>
        /// Transforms a normalized plane by a matrix.
        /// </summary>
        /// <param name="plane">The normalized source plane.</param>
        /// <param name="transformation">The transformation matrix.</param>
        /// <param name="result">When the method completes, contains the transformed plane.</param>
        public static void Transform(ref Plane plane, ref Matrix transformation, out Plane result)
        {
            Real x = plane.Normal.X;
            Real y = plane.Normal.Y;
            Real z = plane.Normal.Z;
            Real d = plane.D;
            Matrix.Invert(ref transformation, out Matrix inverse);
            result.Normal.X = x * inverse.M11 + y * inverse.M12 + z * inverse.M13 + d * inverse.M14;
            result.Normal.Y = x * inverse.M21 + y * inverse.M22 + z * inverse.M23 + d * inverse.M24;
            result.Normal.Z = x * inverse.M31 + y * inverse.M32 + z * inverse.M33 + d * inverse.M34;
            result.D = x * inverse.M41 + y * inverse.M42 + z * inverse.M43 + d * inverse.M44;
        }

        /// <summary>
        /// Transforms a normalized plane by a matrix.
        /// </summary>
        /// <param name="plane">The normalized source plane.</param>
        /// <param name="transformation">The transformation matrix.</param>
        /// <returns>When the method completes, contains the transformed plane.</returns>
        public static Plane Transform(Plane plane, Matrix transformation)
        {
            Transform(ref plane, ref transformation, out var result);
            return result;
        }

        /// <summary>
        /// Scales a plane by the given value.
        /// </summary>
        /// <param name="scale">The amount by which to scale the plane.</param>
        /// <param name="plane">The plane to scale.</param>
        /// <returns>The scaled plane.</returns>
        public static Plane operator *(Real scale, Plane plane)
        {
            return new Plane(plane.Normal * scale, plane.D * scale);
        }

        /// <summary>
        /// Scales a plane by the given value.
        /// </summary>
        /// <param name="plane">The plane to scale.</param>
        /// <param name="scale">The amount by which to scale the plane.</param>
        /// <returns>The scaled plane.</returns>
        public static Plane operator *(Plane plane, Real scale)
        {
            return new Plane(plane.Normal * scale, plane.D * scale);
        }

        /// <summary>
        /// Tests for equality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left" /> has the same value as <paramref name="right" />; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator ==(Plane left, Plane right)
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
        public static bool operator !=(Plane left, Plane right)
        {
            return !left.Equals(ref right);
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public override string ToString()
        {
            return string.Format(CultureInfo.CurrentCulture, "A:{0} B:{1} C:{2} D:{3}", Normal.X, Normal.Y, Normal.Z, D);
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <param name="format">The format.</param>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public string ToString(string format)
        {
            return string.Format(CultureInfo.CurrentCulture, "A:{0} B:{1} C:{2} D:{3}", Normal.X.ToString(format, CultureInfo.CurrentCulture), Normal.Y.ToString(format, CultureInfo.CurrentCulture), Normal.Z.ToString(format, CultureInfo.CurrentCulture), D.ToString(format, CultureInfo.CurrentCulture));
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <param name="formatProvider">The format provider.</param>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public string ToString(IFormatProvider formatProvider)
        {
            return string.Format(formatProvider, "A:{0} B:{1} C:{2} D:{3}", Normal.X, Normal.Y, Normal.Z, D);
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <param name="format">The format.</param>
        /// <param name="formatProvider">The format provider.</param>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public string ToString(string format, IFormatProvider formatProvider)
        {
            return string.Format(formatProvider, "A:{0} B:{1} C:{2} D:{3}", Normal.X.ToString(format, formatProvider), Normal.Y.ToString(format, formatProvider), Normal.Z.ToString(format, formatProvider), D.ToString(format, formatProvider));
        }

        /// <summary>
        /// Returns a hash code for this instance.
        /// </summary>
        /// <returns>A hash code for this instance, suitable for use in hashing algorithms and data structures like a hash table.</returns>
        public override int GetHashCode()
        {
            unchecked
            {
                return (Normal.GetHashCode() * 397) ^ D.GetHashCode();
            }
        }

        /// <summary>
        /// Determines whether the specified <see cref="Vector4" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="Vector4" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="Vector4" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(ref Plane other)
        {
            return Normal == other.Normal && D == other.D;
        }

        /// <summary>
        /// Determines whether the specified <see cref="Vector4" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="Vector4" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="Vector4" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(Plane other)
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
            return value is Plane other && Equals(ref other);
        }
    }
}
