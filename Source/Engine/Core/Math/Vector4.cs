// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
using System.ComponentModel;
using System.Globalization;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace FlaxEngine
{
    [Serializable]
    [TypeConverter(typeof(TypeConverters.Vector4Converter))]
    partial struct Vector4 : IEquatable<Vector4>, IFormattable
    {
        private static readonly string _formatString = "X:{0:F2} Y:{1:F2} Z:{2:F2} W:{3:F2}";

        /// <summary>
        /// The size of the <see cref="Vector4" /> type, in bytes.
        /// </summary>
        public static readonly int SizeInBytes = Marshal.SizeOf(typeof(Vector4));

        /// <summary>
        /// A <see cref="Vector4" /> with all of its components set to zero.
        /// </summary>
        public static readonly Vector4 Zero;

        /// <summary>
        /// The X unit <see cref="Vector4" /> (1, 0, 0, 0).
        /// </summary>
        public static readonly Vector4 UnitX = new Vector4(1.0f, 0.0f, 0.0f, 0.0f);

        /// <summary>
        /// The Y unit <see cref="Vector4" /> (0, 1, 0, 0).
        /// </summary>
        public static readonly Vector4 UnitY = new Vector4(0.0f, 1.0f, 0.0f, 0.0f);

        /// <summary>
        /// The Z unit <see cref="Vector4" /> (0, 0, 1, 0).
        /// </summary>
        public static readonly Vector4 UnitZ = new Vector4(0.0f, 0.0f, 1.0f, 0.0f);

        /// <summary>
        /// The W unit <see cref="Vector4" /> (0, 0, 0, 1).
        /// </summary>
        public static readonly Vector4 UnitW = new Vector4(0.0f, 0.0f, 0.0f, 1.0f);

        /// <summary>
        /// A <see cref="Vector4" /> with all of its components set to half.
        /// </summary>
        public static readonly Vector4 Half = new Vector4(0.5f, 0.5f, 0.5f, 0.5f);

        /// <summary>
        /// A <see cref="Vector4" /> with all of its components set to one.
        /// </summary>
        public static readonly Vector4 One = new Vector4(1.0f, 1.0f, 1.0f, 1.0f);

        /// <summary>
        /// A <see cref="Vector4" /> with all components equal to <see cref="float.MinValue"/>.
        /// </summary>
        public static readonly Vector4 Minimum = new Vector4(float.MinValue);

        /// <summary>
        /// A <see cref="Vector4" /> with all components equal to <see cref="float.MaxValue"/>.
        /// </summary>
        public static readonly Vector4 Maximum = new Vector4(float.MaxValue);

        /// <summary>
        /// Initializes a new instance of the <see cref="Vector4" /> struct.
        /// </summary>
        /// <param name="value">The value that will be assigned to all components.</param>
        public Vector4(float value)
        {
            X = value;
            Y = value;
            Z = value;
            W = value;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Vector4" /> struct.
        /// </summary>
        /// <param name="x">Initial value for the X component of the vector.</param>
        /// <param name="y">Initial value for the Y component of the vector.</param>
        /// <param name="z">Initial value for the Z component of the vector.</param>
        /// <param name="w">Initial value for the W component of the vector.</param>
        public Vector4(float x, float y, float z, float w)
        {
            X = x;
            Y = y;
            Z = z;
            W = w;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Vector4" /> struct.
        /// </summary>
        /// <param name="value">A vector containing the values with which to initialize the X, Y, and Z components.</param>
        /// <param name="w">Initial value for the W component of the vector.</param>
        public Vector4(Vector3 value, float w)
        {
            X = value.X;
            Y = value.Y;
            Z = value.Z;
            W = w;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Vector4" /> struct.
        /// </summary>
        /// <param name="value">A vector containing the values with which to initialize the X and Y components.</param>
        /// <param name="z">Initial value for the Z component of the vector.</param>
        /// <param name="w">Initial value for the W component of the vector.</param>
        public Vector4(Vector2 value, float z, float w)
        {
            X = value.X;
            Y = value.Y;
            Z = z;
            W = w;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Vector4" /> struct.
        /// </summary>
        /// <param name="values">
        /// The values to assign to the X, Y, Z, and W components of the vector. This must be an array with
        /// four elements.
        /// </param>
        /// <exception cref="ArgumentNullException">Thrown when <paramref name="values" /> is <c>null</c>.</exception>
        /// <exception cref="ArgumentOutOfRangeException">
        /// Thrown when <paramref name="values" /> contains more or less than four
        /// elements.
        /// </exception>
        public Vector4(float[] values)
        {
            if (values == null)
                throw new ArgumentNullException(nameof(values));
            if (values.Length != 4)
                throw new ArgumentOutOfRangeException(nameof(values), "There must be four and only four input values for Vector4.");

            X = values[0];
            Y = values[1];
            Z = values[2];
            W = values[3];
        }

        /// <summary>
        /// Gets a value indicting whether this instance is normalized.
        /// </summary>
        public bool IsNormalized => Mathf.IsOne(X * X + Y * Y + Z * Z + W * W);

        /// <summary>
        /// Gets a value indicting whether this vector is zero
        /// </summary>
        public bool IsZero => Mathf.IsZero(X) && Mathf.IsZero(Y) && Mathf.IsZero(Z) && Mathf.IsZero(W);

        /// <summary>
        /// Gets a value indicting whether this vector is one
        /// </summary>
        public bool IsOne => Mathf.IsOne(X) && Mathf.IsOne(Y) && Mathf.IsOne(Z) && Mathf.IsOne(W);

        /// <summary>
        /// Gets a minimum component value
        /// </summary>
        public float MinValue => Mathf.Min(X, Mathf.Min(Y, Mathf.Min(Z, W)));

        /// <summary>
        /// Gets a maximum component value
        /// </summary>
        public float MaxValue => Mathf.Max(X, Mathf.Max(Y, Mathf.Max(Z, W)));

        /// <summary>
        /// Gets an arithmetic average value of all vector components.
        /// </summary>
        public float AvgValue => (X + Y + Z + W) * (1.0f / 4.0f);

        /// <summary>
        /// Gets a sum of the component values.
        /// </summary>
        public float ValuesSum => X + Y + Z + W;

        /// <summary>
        /// Gets or sets the component at the specified index.
        /// </summary>
        /// <value>The value of the X, Y, Z, or W component, depending on the index.</value>
        /// <param name="index">
        /// The index of the component to access. Use 0 for the X component, 1 for the Y component, 2 for the Z
        /// component, and 3 for the W component.
        /// </param>
        /// <returns>The value of the component at the specified index.</returns>
        /// <exception cref="System.ArgumentOutOfRangeException">
        /// Thrown when the <paramref name="index" /> is out of the range [0,
        /// 3].
        /// </exception>
        public float this[int index]
        {
            get
            {
                switch (index)
                {
                case 0: return X;
                case 1: return Y;
                case 2: return Z;
                case 3: return W;
                }

                throw new ArgumentOutOfRangeException(nameof(index), "Indices for Vector4 run from 0 to 3, inclusive.");
            }

            set
            {
                switch (index)
                {
                case 0:
                    X = value;
                    break;
                case 1:
                    Y = value;
                    break;
                case 2:
                    Z = value;
                    break;
                case 3:
                    W = value;
                    break;
                default: throw new ArgumentOutOfRangeException(nameof(index), "Indices for Vector4 run from 0 to 3, inclusive.");
                }
            }
        }

        /// <summary>
        /// Calculates the length of the vector.
        /// </summary>
        /// <returns>The length of the vector.</returns>
        /// <remarks>
        /// <see cref="Vector4.LengthSquared" /> may be preferred when only the relative length is needed
        /// and speed is of the essence.
        /// </remarks>
        public float Length => (float)Math.Sqrt(X * X + Y * Y + Z * Z + W * W);

        /// <summary>
        /// Calculates the squared length of the vector.
        /// </summary>
        /// <returns>The squared length of the vector.</returns>
        /// <remarks>
        /// This method may be preferred to <see cref="Vector4.Length" /> when only a relative length is needed
        /// and speed is of the essence.
        /// </remarks>
        public float LengthSquared => X * X + Y * Y + Z * Z + W * W;

        /// <summary>
        /// Converts the vector into a unit vector.
        /// </summary>
        public void Normalize()
        {
            float length = Length;
            if (!Mathf.IsZero(length))
            {
                float inverse = 1.0f / length;
                X *= inverse;
                Y *= inverse;
                Z *= inverse;
                W *= inverse;
            }
        }

        /// <summary>
        /// Creates an array containing the elements of the vector.
        /// </summary>
        /// <returns>A four-element array containing the components of the vector.</returns>
        public float[] ToArray()
        {
            return new[]
            {
                X,
                Y,
                Z,
                W
            };
        }

        /// <summary>
        /// Adds two vectors.
        /// </summary>
        /// <param name="left">The first vector to add.</param>
        /// <param name="right">The second vector to add.</param>
        /// <param name="result">When the method completes, contains the sum of the two vectors.</param>
        public static void Add(ref Vector4 left, ref Vector4 right, out Vector4 result)
        {
            result = new Vector4(left.X + right.X, left.Y + right.Y, left.Z + right.Z, left.W + right.W);
        }

        /// <summary>
        /// Adds two vectors.
        /// </summary>
        /// <param name="left">The first vector to add.</param>
        /// <param name="right">The second vector to add.</param>
        /// <returns>The sum of the two vectors.</returns>
        public static Vector4 Add(Vector4 left, Vector4 right)
        {
            return new Vector4(left.X + right.X, left.Y + right.Y, left.Z + right.Z, left.W + right.W);
        }

        /// <summary>
        /// Perform a component-wise addition
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be added to elements</param>
        /// <param name="result">The vector with added scalar for each element.</param>
        public static void Add(ref Vector4 left, ref float right, out Vector4 result)
        {
            result = new Vector4(left.X + right, left.Y + right, left.Z + right, left.W + right);
        }

        /// <summary>
        /// Perform a component-wise addition
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be added to elements</param>
        /// <returns>The vector with added scalar for each element.</returns>
        public static Vector4 Add(Vector4 left, float right)
        {
            return new Vector4(left.X + right, left.Y + right, left.Z + right, left.W + right);
        }

        /// <summary>
        /// Subtracts two vectors.
        /// </summary>
        /// <param name="left">The first vector to subtract.</param>
        /// <param name="right">The second vector to subtract.</param>
        /// <param name="result">When the method completes, contains the difference of the two vectors.</param>
        public static void Subtract(ref Vector4 left, ref Vector4 right, out Vector4 result)
        {
            result = new Vector4(left.X - right.X, left.Y - right.Y, left.Z - right.Z, left.W - right.W);
        }

        /// <summary>
        /// Subtracts two vectors.
        /// </summary>
        /// <param name="left">The first vector to subtract.</param>
        /// <param name="right">The second vector to subtract.</param>
        /// <returns>The difference of the two vectors.</returns>
        public static Vector4 Subtract(Vector4 left, Vector4 right)
        {
            return new Vector4(left.X - right.X, left.Y - right.Y, left.Z - right.Z, left.W - right.W);
        }

        /// <summary>
        /// Perform a component-wise subtraction
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be subtracted from elements</param>
        /// <param name="result">The vector with subtracted scalar for each element.</param>
        public static void Subtract(ref Vector4 left, ref float right, out Vector4 result)
        {
            result = new Vector4(left.X - right, left.Y - right, left.Z - right, left.W - right);
        }

        /// <summary>
        /// Perform a component-wise subtraction
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be subtracted from elements</param>
        /// <returns>The vector with subtracted scalar for each element.</returns>
        public static Vector4 Subtract(Vector4 left, float right)
        {
            return new Vector4(left.X - right, left.Y - right, left.Z - right, left.W - right);
        }

        /// <summary>
        /// Perform a component-wise subtraction
        /// </summary>
        /// <param name="left">The scalar value to be subtracted from elements</param>
        /// <param name="right">The input vector.</param>
        /// <param name="result">The vector with subtracted scalar for each element.</param>
        public static void Subtract(ref float left, ref Vector4 right, out Vector4 result)
        {
            result = new Vector4(left - right.X, left - right.Y, left - right.Z, left - right.W);
        }

        /// <summary>
        /// Perform a component-wise subtraction
        /// </summary>
        /// <param name="left">The scalar value to be subtracted from elements</param>
        /// <param name="right">The input vector.</param>
        /// <returns>The vector with subtracted scalar for each element.</returns>
        public static Vector4 Subtract(float left, Vector4 right)
        {
            return new Vector4(left - right.X, left - right.Y, left - right.Z, left - right.W);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="result">When the method completes, contains the scaled vector.</param>
        public static void Multiply(ref Vector4 value, float scale, out Vector4 result)
        {
            result = new Vector4(value.X * scale, value.Y * scale, value.Z * scale, value.W * scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Vector4 Multiply(Vector4 value, float scale)
        {
            return new Vector4(value.X * scale, value.Y * scale, value.Z * scale, value.W * scale);
        }

        /// <summary>
        /// Multiplies a vector with another by performing component-wise multiplication.
        /// </summary>
        /// <param name="left">The first vector to multiply.</param>
        /// <param name="right">The second vector to multiply.</param>
        /// <param name="result">When the method completes, contains the multiplied vector.</param>
        public static void Multiply(ref Vector4 left, ref Vector4 right, out Vector4 result)
        {
            result = new Vector4(left.X * right.X, left.Y * right.Y, left.Z * right.Z, left.W * right.W);
        }

        /// <summary>
        /// Multiplies a vector with another by performing component-wise multiplication.
        /// </summary>
        /// <param name="left">The first vector to multiply.</param>
        /// <param name="right">The second vector to multiply.</param>
        /// <returns>The multiplied vector.</returns>
        public static Vector4 Multiply(Vector4 left, Vector4 right)
        {
            return new Vector4(left.X * right.X, left.Y * right.Y, left.Z * right.Z, left.W * right.W);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="result">When the method completes, contains the scaled vector.</param>
        public static void Divide(ref Vector4 value, float scale, out Vector4 result)
        {
            result = new Vector4(value.X / scale, value.Y / scale, value.Z / scale, value.W / scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Vector4 Divide(Vector4 value, float scale)
        {
            return new Vector4(value.X / scale, value.Y / scale, value.Z / scale, value.W / scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="value">The vector to scale.</param>
        /// <param name="result">When the method completes, contains the scaled vector.</param>
        public static void Divide(float scale, ref Vector4 value, out Vector4 result)
        {
            result = new Vector4(scale / value.X, scale / value.Y, scale / value.Z, scale / value.W);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Vector4 Divide(float scale, Vector4 value)
        {
            return new Vector4(scale / value.X, scale / value.Y, scale / value.Z, scale / value.W);
        }

        /// <summary>
        /// Reverses the direction of a given vector.
        /// </summary>
        /// <param name="value">The vector to negate.</param>
        /// <param name="result">When the method completes, contains a vector facing in the opposite direction.</param>
        public static void Negate(ref Vector4 value, out Vector4 result)
        {
            result = new Vector4(-value.X, -value.Y, -value.Z, -value.W);
        }

        /// <summary>
        /// Reverses the direction of a given vector.
        /// </summary>
        /// <param name="value">The vector to negate.</param>
        /// <returns>A vector facing in the opposite direction.</returns>
        public static Vector4 Negate(Vector4 value)
        {
            return new Vector4(-value.X, -value.Y, -value.Z, -value.W);
        }

        /// <summary>
        /// Returns a <see cref="Vector4" /> containing the 4D Cartesian coordinates of a point specified in Barycentric
        /// coordinates relative to a 4D triangle.
        /// </summary>
        /// <param name="value1">A <see cref="Vector4" /> containing the 4D Cartesian coordinates of vertex 1 of the triangle.</param>
        /// <param name="value2">A <see cref="Vector4" /> containing the 4D Cartesian coordinates of vertex 2 of the triangle.</param>
        /// <param name="value3">A <see cref="Vector4" /> containing the 4D Cartesian coordinates of vertex 3 of the triangle.</param>
        /// <param name="amount1">
        /// Barycentric coordinate b2, which expresses the weighting factor toward vertex 2 (specified in
        /// <paramref name="value2" />).
        /// </param>
        /// <param name="amount2">
        /// Barycentric coordinate b3, which expresses the weighting factor toward vertex 3 (specified in
        /// <paramref name="value3" />).
        /// </param>
        /// <param name="result">When the method completes, contains the 4D Cartesian coordinates of the specified point.</param>
        public static void Barycentric(ref Vector4 value1, ref Vector4 value2, ref Vector4 value3, float amount1, float amount2, out Vector4 result)
        {
            result = new Vector4(value1.X + amount1 * (value2.X - value1.X) + amount2 * (value3.X - value1.X),
                                 value1.Y + amount1 * (value2.Y - value1.Y) + amount2 * (value3.Y - value1.Y),
                                 value1.Z + amount1 * (value2.Z - value1.Z) + amount2 * (value3.Z - value1.Z),
                                 value1.W + amount1 * (value2.W - value1.W) + amount2 * (value3.W - value1.W));
        }

        /// <summary>
        /// Returns a <see cref="Vector4" /> containing the 4D Cartesian coordinates of a point specified in Barycentric
        /// coordinates relative to a 4D triangle.
        /// </summary>
        /// <param name="value1">A <see cref="Vector4" /> containing the 4D Cartesian coordinates of vertex 1 of the triangle.</param>
        /// <param name="value2">A <see cref="Vector4" /> containing the 4D Cartesian coordinates of vertex 2 of the triangle.</param>
        /// <param name="value3">A <see cref="Vector4" /> containing the 4D Cartesian coordinates of vertex 3 of the triangle.</param>
        /// <param name="amount1">
        /// Barycentric coordinate b2, which expresses the weighting factor toward vertex 2 (specified in
        /// <paramref name="value2" />).
        /// </param>
        /// <param name="amount2">
        /// Barycentric coordinate b3, which expresses the weighting factor toward vertex 3 (specified in
        /// <paramref name="value3" />).
        /// </param>
        /// <returns>A new <see cref="Vector4" /> containing the 4D Cartesian coordinates of the specified point.</returns>
        public static Vector4 Barycentric(Vector4 value1, Vector4 value2, Vector4 value3, float amount1, float amount2)
        {
            Barycentric(ref value1, ref value2, ref value3, amount1, amount2, out Vector4 result);
            return result;
        }

        /// <summary>
        /// Restricts a value to be within a specified range.
        /// </summary>
        /// <param name="value">The value to clamp.</param>
        /// <param name="min">The minimum value.</param>
        /// <param name="max">The maximum value.</param>
        /// <param name="result">When the method completes, contains the clamped value.</param>
        public static void Clamp(ref Vector4 value, ref Vector4 min, ref Vector4 max, out Vector4 result)
        {
            float x = value.X;
            x = x > max.X ? max.X : x;
            x = x < min.X ? min.X : x;

            float y = value.Y;
            y = y > max.Y ? max.Y : y;
            y = y < min.Y ? min.Y : y;

            float z = value.Z;
            z = z > max.Z ? max.Z : z;
            z = z < min.Z ? min.Z : z;

            float w = value.W;
            w = w > max.W ? max.W : w;
            w = w < min.W ? min.W : w;

            result = new Vector4(x, y, z, w);
        }

        /// <summary>
        /// Restricts a value to be within a specified range.
        /// </summary>
        /// <param name="value">The value to clamp.</param>
        /// <param name="min">The minimum value.</param>
        /// <param name="max">The maximum value.</param>
        /// <returns>The clamped value.</returns>
        public static Vector4 Clamp(Vector4 value, Vector4 min, Vector4 max)
        {
            Clamp(ref value, ref min, ref max, out Vector4 result);
            return result;
        }

        /// <summary>
        /// Calculates the distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <param name="result">When the method completes, contains the distance between the two vectors.</param>
        /// <remarks>
        /// <see cref="Vector4.DistanceSquared(ref Vector4, ref Vector4, out float)" /> may be preferred when only the relative
        /// distance is needed
        /// and speed is of the essence.
        /// </remarks>
        public static void Distance(ref Vector4 value1, ref Vector4 value2, out float result)
        {
            float x = value1.X - value2.X;
            float y = value1.Y - value2.Y;
            float z = value1.Z - value2.Z;
            float w = value1.W - value2.W;

            result = (float)Math.Sqrt(x * x + y * y + z * z + w * w);
        }

        /// <summary>
        /// Calculates the distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <returns>The distance between the two vectors.</returns>
        /// <remarks>
        /// <see cref="Vector4.DistanceSquared(Vector4, Vector4)" /> may be preferred when only the relative distance is needed
        /// and speed is of the essence.
        /// </remarks>
        public static float Distance(Vector4 value1, Vector4 value2)
        {
            float x = value1.X - value2.X;
            float y = value1.Y - value2.Y;
            float z = value1.Z - value2.Z;
            float w = value1.W - value2.W;

            return (float)Math.Sqrt(x * x + y * y + z * z + w * w);
        }

        /// <summary>
        /// Calculates the squared distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <param name="result">When the method completes, contains the squared distance between the two vectors.</param>
        /// <remarks>
        /// Distance squared is the value before taking the square root.
        /// Distance squared can often be used in place of distance if relative comparisons are being made.
        /// For example, consider three points A, B, and C. To determine whether B or C is further from A,
        /// compare the distance between A and B to the distance between A and C. Calculating the two distances
        /// involves two square roots, which are computationally expensive. However, using distance squared
        /// provides the same information and avoids calculating two square roots.
        /// </remarks>
        public static void DistanceSquared(ref Vector4 value1, ref Vector4 value2, out float result)
        {
            float x = value1.X - value2.X;
            float y = value1.Y - value2.Y;
            float z = value1.Z - value2.Z;
            float w = value1.W - value2.W;

            result = x * x + y * y + z * z + w * w;
        }

        /// <summary>
        /// Calculates the squared distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <returns>The squared distance between the two vectors.</returns>
        /// <remarks>
        /// Distance squared is the value before taking the square root.
        /// Distance squared can often be used in place of distance if relative comparisons are being made.
        /// For example, consider three points A, B, and C. To determine whether B or C is further from A,
        /// compare the distance between A and B to the distance between A and C. Calculating the two distances
        /// involves two square roots, which are computationally expensive. However, using distance squared
        /// provides the same information and avoids calculating two square roots.
        /// </remarks>
        public static float DistanceSquared(Vector4 value1, Vector4 value2)
        {
            float x = value1.X - value2.X;
            float y = value1.Y - value2.Y;
            float z = value1.Z - value2.Z;
            float w = value1.W - value2.W;

            return x * x + y * y + z * z + w * w;
        }

        /// <summary>
        /// Tests whether one vector is near another vector.
        /// </summary>
        /// <param name="left">The left vector.</param>
        /// <param name="right">The right vector.</param>
        /// <param name="epsilon">The epsilon.</param>
        /// <returns><c>true</c> if left and right are near another, <c>false</c> otherwise</returns>
        public static bool NearEqual(Vector4 left, Vector4 right, float epsilon = Mathf.Epsilon)
        {
            return NearEqual(ref left, ref right, epsilon);
        }

        /// <summary>
        /// Tests whether one vector is near another vector.
        /// </summary>
        /// <param name="left">The left vector.</param>
        /// <param name="right">The right vector.</param>
        /// <param name="epsilon">The epsilon.</param>
        /// <returns><c>true</c> if left and right are near another, <c>false</c> otherwise</returns>
        public static bool NearEqual(ref Vector4 left, ref Vector4 right, float epsilon = Mathf.Epsilon)
        {
            return Mathf.WithinEpsilon(left.X, right.X, epsilon) &&
                   Mathf.WithinEpsilon(left.Y, right.Y, epsilon) &&
                   Mathf.WithinEpsilon(left.Z, right.Z, epsilon);
        }

        /// <summary>
        /// Calculates the dot product of two vectors.
        /// </summary>
        /// <param name="left">First source vector</param>
        /// <param name="right">Second source vector.</param>
        /// <param name="result">When the method completes, contains the dot product of the two vectors.</param>
        public static void Dot(ref Vector4 left, ref Vector4 right, out float result)
        {
            result = left.X * right.X + left.Y * right.Y + left.Z * right.Z + left.W * right.W;
        }

        /// <summary>
        /// Calculates the dot product of two vectors.
        /// </summary>
        /// <param name="left">First source vector.</param>
        /// <param name="right">Second source vector.</param>
        /// <returns>The dot product of the two vectors.</returns>
        public static float Dot(Vector4 left, Vector4 right)
        {
            return left.X * right.X + left.Y * right.Y + left.Z * right.Z + left.W * right.W;
        }

        /// <summary>
        /// Converts the vector into a unit vector.
        /// </summary>
        /// <param name="value">The vector to normalize.</param>
        /// <param name="result">When the method completes, contains the normalized vector.</param>
        public static void Normalize(ref Vector4 value, out Vector4 result)
        {
            Vector4 temp = value;
            result = temp;
            result.Normalize();
        }

        /// <summary>
        /// Converts the vector into a unit vector.
        /// </summary>
        /// <param name="value">The vector to normalize.</param>
        /// <returns>The normalized vector.</returns>
        public static Vector4 Normalize(Vector4 value)
        {
            value.Normalize();
            return value;
        }

        /// <summary>
        /// Makes sure that Length of the output vector is always below max and above 0.
        /// </summary>
        /// <param name="vector">Input Vector.</param>
        /// <param name="max">Max Length</param>
        public static Vector4 ClampLength(Vector4 vector, float max)
        {
            return ClampLength(vector, 0, max);
        }

        /// <summary>
        /// Makes sure that Length of the output vector is always below max and above min.
        /// </summary>
        /// <param name="vector">Input Vector.</param>
        /// <param name="min">Min Length</param>
        /// <param name="max">Max Length</param>
        public static Vector4 ClampLength(Vector4 vector, float min, float max)
        {
            ClampLength(ref vector, min, max, out Vector4 retVect);
            return retVect;
        }

        /// <summary>
        /// Makes sure that Length of the output vector is always below max and above min.
        /// </summary>
        /// <param name="vector">Input Vector.</param>
        /// <param name="min">Min Length</param>
        /// <param name="max">Max Length</param>
        /// <param name="retVect">The Return Vector</param>
        public static void ClampLength(ref Vector4 vector, float min, float max, out Vector4 retVect)
        {
            retVect.X = vector.X;
            retVect.Y = vector.Y;
            retVect.Z = vector.Z;
            retVect.W = vector.W;

            float lenSq = retVect.LengthSquared;
            if (lenSq > max * max)
            {
                float scaleFactor = max / (float)Math.Sqrt(lenSq);
                retVect.X = retVect.X * scaleFactor;
                retVect.Y = retVect.Y * scaleFactor;
                retVect.Z = retVect.Z * scaleFactor;
                retVect.W = retVect.W * scaleFactor;
            }
            if (lenSq < min * min)
            {
                float scaleFactor = min / (float)Math.Sqrt(lenSq);
                retVect.X = retVect.X * scaleFactor;
                retVect.Y = retVect.Y * scaleFactor;
                retVect.Z = retVect.Z * scaleFactor;
                retVect.W = retVect.W * scaleFactor;
            }
        }

        /// <summary>
        /// Performs a linear interpolation between two vectors.
        /// </summary>
        /// <param name="start">Start vector.</param>
        /// <param name="end">End vector.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end" />.</param>
        /// <param name="result">When the method completes, contains the linear interpolation of the two vectors.</param>
        /// <remarks>
        /// Passing <paramref name="amount" /> a value of 0 will cause <paramref name="start" /> to be returned; a value of 1
        /// will cause <paramref name="end" /> to be returned.
        /// </remarks>
        public static void Lerp(ref Vector4 start, ref Vector4 end, float amount, out Vector4 result)
        {
            result.X = Mathf.Lerp(start.X, end.X, amount);
            result.Y = Mathf.Lerp(start.Y, end.Y, amount);
            result.Z = Mathf.Lerp(start.Z, end.Z, amount);
            result.W = Mathf.Lerp(start.W, end.W, amount);
        }

        /// <summary>
        /// Performs a linear interpolation between two vectors.
        /// </summary>
        /// <param name="start">Start vector.</param>
        /// <param name="end">End vector.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end" />.</param>
        /// <returns>The linear interpolation of the two vectors.</returns>
        /// <remarks>
        /// Passing <paramref name="amount" /> a value of 0 will cause <paramref name="start" /> to be returned; a value of 1
        /// will cause <paramref name="end" /> to be returned.
        /// </remarks>
        public static Vector4 Lerp(Vector4 start, Vector4 end, float amount)
        {
            Lerp(ref start, ref end, amount, out Vector4 result);
            return result;
        }

        /// <summary>
        /// Performs a cubic interpolation between two vectors.
        /// </summary>
        /// <param name="start">Start vector.</param>
        /// <param name="end">End vector.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end" />.</param>
        /// <param name="result">When the method completes, contains the cubic interpolation of the two vectors.</param>
        public static void SmoothStep(ref Vector4 start, ref Vector4 end, float amount, out Vector4 result)
        {
            amount = Mathf.SmoothStep(amount);
            Lerp(ref start, ref end, amount, out result);
        }

        /// <summary>
        /// Performs a cubic interpolation between two vectors.
        /// </summary>
        /// <param name="start">Start vector.</param>
        /// <param name="end">End vector.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end" />.</param>
        /// <returns>The cubic interpolation of the two vectors.</returns>
        public static Vector4 SmoothStep(Vector4 start, Vector4 end, float amount)
        {
            SmoothStep(ref start, ref end, amount, out Vector4 result);
            return result;
        }

        /// <summary>
        /// Performs a Hermite spline interpolation.
        /// </summary>
        /// <param name="value1">First source position vector.</param>
        /// <param name="tangent1">First source tangent vector.</param>
        /// <param name="value2">Second source position vector.</param>
        /// <param name="tangent2">Second source tangent vector.</param>
        /// <param name="amount">Weighting factor.</param>
        /// <param name="result">When the method completes, contains the result of the Hermite spline interpolation.</param>
        public static void Hermite(ref Vector4 value1, ref Vector4 tangent1, ref Vector4 value2, ref Vector4 tangent2, float amount, out Vector4 result)
        {
            float squared = amount * amount;
            float cubed = amount * squared;
            float part1 = 2.0f * cubed - 3.0f * squared + 1.0f;
            float part2 = -2.0f * cubed + 3.0f * squared;
            float part3 = cubed - 2.0f * squared + amount;
            float part4 = cubed - squared;

            result = new Vector4(value1.X * part1 + value2.X * part2 + tangent1.X * part3 + tangent2.X * part4,
                                 value1.Y * part1 + value2.Y * part2 + tangent1.Y * part3 + tangent2.Y * part4,
                                 value1.Z * part1 + value2.Z * part2 + tangent1.Z * part3 + tangent2.Z * part4,
                                 value1.W * part1 + value2.W * part2 + tangent1.W * part3 + tangent2.W * part4);
        }

        /// <summary>
        /// Performs a Hermite spline interpolation.
        /// </summary>
        /// <param name="value1">First source position vector.</param>
        /// <param name="tangent1">First source tangent vector.</param>
        /// <param name="value2">Second source position vector.</param>
        /// <param name="tangent2">Second source tangent vector.</param>
        /// <param name="amount">Weighting factor.</param>
        /// <returns>The result of the Hermite spline interpolation.</returns>
        public static Vector4 Hermite(Vector4 value1, Vector4 tangent1, Vector4 value2, Vector4 tangent2, float amount)
        {
            Hermite(ref value1, ref tangent1, ref value2, ref tangent2, amount, out Vector4 result);
            return result;
        }

        /// <summary>
        /// Performs a Catmull-Rom interpolation using the specified positions.
        /// </summary>
        /// <param name="value1">The first position in the interpolation.</param>
        /// <param name="value2">The second position in the interpolation.</param>
        /// <param name="value3">The third position in the interpolation.</param>
        /// <param name="value4">The fourth position in the interpolation.</param>
        /// <param name="amount">Weighting factor.</param>
        /// <param name="result">When the method completes, contains the result of the Catmull-Rom interpolation.</param>
        public static void CatmullRom(ref Vector4 value1, ref Vector4 value2, ref Vector4 value3, ref Vector4 value4, float amount, out Vector4 result)
        {
            float squared = amount * amount;
            float cubed = amount * squared;

            result.X = 0.5f * (2.0f * value2.X + (-value1.X + value3.X) * amount + (2.0f * value1.X - 5.0f * value2.X + 4.0f * value3.X - value4.X) * squared + (-value1.X + 3.0f * value2.X - 3.0f * value3.X + value4.X) * cubed);
            result.Y = 0.5f * (2.0f * value2.Y + (-value1.Y + value3.Y) * amount + (2.0f * value1.Y - 5.0f * value2.Y + 4.0f * value3.Y - value4.Y) * squared + (-value1.Y + 3.0f * value2.Y - 3.0f * value3.Y + value4.Y) * cubed);
            result.Z = 0.5f * (2.0f * value2.Z + (-value1.Z + value3.Z) * amount + (2.0f * value1.Z - 5.0f * value2.Z + 4.0f * value3.Z - value4.Z) * squared + (-value1.Z + 3.0f * value2.Z - 3.0f * value3.Z + value4.Z) * cubed);
            result.W = 0.5f * (2.0f * value2.W + (-value1.W + value3.W) * amount + (2.0f * value1.W - 5.0f * value2.W + 4.0f * value3.W - value4.W) * squared + (-value1.W + 3.0f * value2.W - 3.0f * value3.W + value4.W) * cubed);
        }

        /// <summary>
        /// Performs a Catmull-Rom interpolation using the specified positions.
        /// </summary>
        /// <param name="value1">The first position in the interpolation.</param>
        /// <param name="value2">The second position in the interpolation.</param>
        /// <param name="value3">The third position in the interpolation.</param>
        /// <param name="value4">The fourth position in the interpolation.</param>
        /// <param name="amount">Weighting factor.</param>
        /// <returns>A vector that is the result of the Catmull-Rom interpolation.</returns>
        public static Vector4 CatmullRom(Vector4 value1, Vector4 value2, Vector4 value3, Vector4 value4, float amount)
        {
            CatmullRom(ref value1, ref value2, ref value3, ref value4, amount, out Vector4 result);
            return result;
        }

        /// <summary>
        /// Returns a vector containing the largest components of the specified vectors.
        /// </summary>
        /// <param name="left">The first source vector.</param>
        /// <param name="right">The second source vector.</param>
        /// <param name="result">
        /// When the method completes, contains an new vector composed of the largest components of the source
        /// vectors.
        /// </param>
        public static void Max(ref Vector4 left, ref Vector4 right, out Vector4 result)
        {
            result.X = left.X > right.X ? left.X : right.X;
            result.Y = left.Y > right.Y ? left.Y : right.Y;
            result.Z = left.Z > right.Z ? left.Z : right.Z;
            result.W = left.W > right.W ? left.W : right.W;
        }

        /// <summary>
        /// Returns a vector containing the largest components of the specified vectors.
        /// </summary>
        /// <param name="left">The first source vector.</param>
        /// <param name="right">The second source vector.</param>
        /// <returns>A vector containing the largest components of the source vectors.</returns>
        public static Vector4 Max(Vector4 left, Vector4 right)
        {
            Max(ref left, ref right, out Vector4 result);
            return result;
        }

        /// <summary>
        /// Returns a vector containing the smallest components of the specified vectors.
        /// </summary>
        /// <param name="left">The first source vector.</param>
        /// <param name="right">The second source vector.</param>
        /// <param name="result">
        /// When the method completes, contains an new vector composed of the smallest components of the
        /// source vectors.
        /// </param>
        public static void Min(ref Vector4 left, ref Vector4 right, out Vector4 result)
        {
            result.X = left.X < right.X ? left.X : right.X;
            result.Y = left.Y < right.Y ? left.Y : right.Y;
            result.Z = left.Z < right.Z ? left.Z : right.Z;
            result.W = left.W < right.W ? left.W : right.W;
        }

        /// <summary>
        /// Returns a vector containing the smallest components of the specified vectors.
        /// </summary>
        /// <param name="left">The first source vector.</param>
        /// <param name="right">The second source vector.</param>
        /// <returns>A vector containing the smallest components of the source vectors.</returns>
        public static Vector4 Min(Vector4 left, Vector4 right)
        {
            Min(ref left, ref right, out Vector4 result);
            return result;
        }

        /// <summary>
        /// Returns the absolute value of a vector.
        /// </summary>
        /// <param name="v">The value.</param>
        /// <returns> A vector which components are less or equal to 0.</returns>
        public static Vector4 Abs(Vector4 v)
        {
            return new Vector4(Math.Abs(v.X), Math.Abs(v.Y), Math.Abs(v.Z), Math.Abs(v.W));
        }

        /// <summary>
        /// Orthogonalizes a list of vectors.
        /// </summary>
        /// <param name="destination">The list of orthogonalized vectors.</param>
        /// <param name="source">The list of vectors to orthogonalize.</param>
        /// <remarks>
        /// <para>
        ///   Orthogonalization is the process of making all vectors orthogonal to each other. This
        ///   means that any given vector in the list will be orthogonal to any other given vector in the
        ///   list.
        /// </para>
        /// <para>
        ///   Because this method uses the modified Gram-Schmidt process, the resulting vectors
        ///   tend to be numerically unstable. The numeric stability decreases according to the vectors
        ///   position in the list so that the first vector is the most stable and the last vector is the
        ///   least stable.
        /// </para>
        /// </remarks>
        /// <exception cref="ArgumentNullException">
        /// Thrown when <paramref name="source" /> or <paramref name="destination" /> is
        /// <c>null</c>.
        /// </exception>
        /// <exception cref="ArgumentOutOfRangeException">
        /// Thrown when <paramref name="destination" /> is shorter in length than
        /// <paramref name="source" />.
        /// </exception>
        public static void Orthogonalize(Vector4[] destination, params Vector4[] source)
        {
            //Uses the modified Gram-Schmidt process.
            //q1 = m1
            //q2 = m2 - ((q1 ⋅ m2) / (q1 ⋅ q1)) * q1
            //q3 = m3 - ((q1 ⋅ m3) / (q1 ⋅ q1)) * q1 - ((q2 ⋅ m3) / (q2 ⋅ q2)) * q2
            //q4 = m4 - ((q1 ⋅ m4) / (q1 ⋅ q1)) * q1 - ((q2 ⋅ m4) / (q2 ⋅ q2)) * q2 - ((q3 ⋅ m4) / (q3 ⋅ q3)) * q3
            //q5 = ...

            if (source == null)
                throw new ArgumentNullException(nameof(source));

            if (destination == null)
                throw new ArgumentNullException(nameof(destination));

            if (destination.Length < source.Length)
                throw new ArgumentOutOfRangeException(nameof(destination), "The destination array must be of same length or larger length than the source array.");

            for (var i = 0; i < source.Length; ++i)
            {
                Vector4 newvector = source[i];

                for (var r = 0; r < i; ++r)
                    newvector -= Dot(destination[r], newvector) / Dot(destination[r], destination[r]) * destination[r];

                destination[i] = newvector;
            }
        }

        /// <summary>
        /// Orthonormalizes a list of vectors.
        /// </summary>
        /// <param name="destination">The list of orthonormalized vectors.</param>
        /// <param name="source">The list of vectors to orthonormalize.</param>
        /// <remarks>
        /// <para>
        ///   Orthonormalization is the process of making all vectors orthogonal to each
        ///   other and making all vectors of unit length. This means that any given vector will
        ///   be orthogonal to any other given vector in the list.
        /// </para>
        /// <para>
        ///   Because this method uses the modified Gram-Schmidt process, the resulting vectors
        ///   tend to be numerically unstable. The numeric stability decreases according to the vectors
        ///   position in the list so that the first vector is the most stable and the last vector is the
        ///   least stable.
        /// </para>
        /// </remarks>
        /// <exception cref="ArgumentNullException">
        /// Thrown when <paramref name="source" /> or <paramref name="destination" /> is
        /// <c>null</c>.
        /// </exception>
        /// <exception cref="ArgumentOutOfRangeException">
        /// Thrown when <paramref name="destination" /> is shorter in length than
        /// <paramref name="source" />.
        /// </exception>
        public static void Orthonormalize(Vector4[] destination, params Vector4[] source)
        {
            //Uses the modified Gram-Schmidt process.
            //Because we are making unit vectors, we can optimize the math for orthogonalization
            //and simplify the projection operation to remove the division.
            //q1 = m1 / |m1|
            //q2 = (m2 - (q1 ⋅ m2) * q1) / |m2 - (q1 ⋅ m2) * q1|
            //q3 = (m3 - (q1 ⋅ m3) * q1 - (q2 ⋅ m3) * q2) / |m3 - (q1 ⋅ m3) * q1 - (q2 ⋅ m3) * q2|
            //q4 = (m4 - (q1 ⋅ m4) * q1 - (q2 ⋅ m4) * q2 - (q3 ⋅ m4) * q3) / |m4 - (q1 ⋅ m4) * q1 - (q2 ⋅ m4) * q2 - (q3 ⋅ m4) * q3|
            //q5 = ...

            if (source == null)
                throw new ArgumentNullException(nameof(source));

            if (destination == null)
                throw new ArgumentNullException(nameof(destination));

            if (destination.Length < source.Length)
                throw new ArgumentOutOfRangeException(nameof(destination), "The destination array must be of same length or larger length than the source array.");

            for (var i = 0; i < source.Length; ++i)
            {
                Vector4 newvector = source[i];

                for (var r = 0; r < i; ++r)
                    newvector -= Dot(destination[r], newvector) * destination[r];

                newvector.Normalize();
                destination[i] = newvector;
            }
        }

        /// <summary>
        /// Transforms a 4D vector by the given <see cref="Quaternion" /> rotation.
        /// </summary>
        /// <param name="vector">The vector to rotate.</param>
        /// <param name="rotation">The <see cref="Quaternion" /> rotation to apply.</param>
        /// <param name="result">When the method completes, contains the transformed <see cref="Vector4" />.</param>
        public static void Transform(ref Vector4 vector, ref Quaternion rotation, out Vector4 result)
        {
            float x = rotation.X + rotation.X;
            float y = rotation.Y + rotation.Y;
            float z = rotation.Z + rotation.Z;
            float wx = rotation.W * x;
            float wy = rotation.W * y;
            float wz = rotation.W * z;
            float xx = rotation.X * x;
            float xy = rotation.X * y;
            float xz = rotation.X * z;
            float yy = rotation.Y * y;
            float yz = rotation.Y * z;
            float zz = rotation.Z * z;

            result = new Vector4(
                vector.X * (1.0f - yy - zz) + vector.Y * (xy - wz) + vector.Z * (xz + wy),
                vector.X * (xy + wz) + vector.Y * (1.0f - xx - zz) + vector.Z * (yz - wx),
                vector.X * (xz - wy) + vector.Y * (yz + wx) + vector.Z * (1.0f - xx - yy),
                vector.W);
        }

        /// <summary>
        /// Transforms a 4D vector by the given <see cref="Quaternion" /> rotation.
        /// </summary>
        /// <param name="vector">The vector to rotate.</param>
        /// <param name="rotation">The <see cref="Quaternion" /> rotation to apply.</param>
        /// <returns>The transformed <see cref="Vector4" />.</returns>
        public static Vector4 Transform(Vector4 vector, Quaternion rotation)
        {
            Transform(ref vector, ref rotation, out Vector4 result);
            return result;
        }

        /// <summary>
        /// Transforms an array of vectors by the given <see cref="Quaternion" /> rotation.
        /// </summary>
        /// <param name="source">The array of vectors to transform.</param>
        /// <param name="rotation">The <see cref="Quaternion" /> rotation to apply.</param>
        /// <param name="destination">
        /// The array for which the transformed vectors are stored.
        /// This array may be the same array as <paramref name="source" />.
        /// </param>
        /// <exception cref="ArgumentNullException">
        /// Thrown when <paramref name="source" /> or <paramref name="destination" /> is
        /// <c>null</c>.
        /// </exception>
        /// <exception cref="ArgumentOutOfRangeException">
        /// Thrown when <paramref name="destination" /> is shorter in length than
        /// <paramref name="source" />.
        /// </exception>
        public static void Transform(Vector4[] source, ref Quaternion rotation, Vector4[] destination)
        {
            if (source == null)
                throw new ArgumentNullException(nameof(source));

            if (destination == null)
                throw new ArgumentNullException(nameof(destination));

            if (destination.Length < source.Length)
                throw new ArgumentOutOfRangeException(nameof(destination), "The destination array must be of same length or larger length than the source array.");

            float x = rotation.X + rotation.X;
            float y = rotation.Y + rotation.Y;
            float z = rotation.Z + rotation.Z;
            float wx = rotation.W * x;
            float wy = rotation.W * y;
            float wz = rotation.W * z;
            float xx = rotation.X * x;
            float xy = rotation.X * y;
            float xz = rotation.X * z;
            float yy = rotation.Y * y;
            float yz = rotation.Y * z;
            float zz = rotation.Z * z;

            float num1 = 1.0f - yy - zz;
            float num2 = xy - wz;
            float num3 = xz + wy;
            float num4 = xy + wz;
            float num5 = 1.0f - xx - zz;
            float num6 = yz - wx;
            float num7 = xz - wy;
            float num8 = yz + wx;
            float num9 = 1.0f - xx - yy;

            for (var i = 0; i < source.Length; ++i)
                destination[i] = new Vector4(
                    source[i].X * num1 + source[i].Y * num2 + source[i].Z * num3,
                    source[i].X * num4 + source[i].Y * num5 + source[i].Z * num6,
                    source[i].X * num7 + source[i].Y * num8 + source[i].Z * num9,
                    source[i].W);
        }

        /// <summary>
        /// Transforms a 4D vector by the given <see cref="Matrix" />.
        /// </summary>
        /// <param name="vector">The source vector.</param>
        /// <param name="transform">The transformation <see cref="Matrix" />.</param>
        /// <param name="result">When the method completes, contains the transformed <see cref="Vector4" />.</param>
        public static void Transform(ref Vector4 vector, ref Matrix transform, out Vector4 result)
        {
            result = new Vector4(
                vector.X * transform.M11 + vector.Y * transform.M21 + vector.Z * transform.M31 + vector.W * transform.M41,
                vector.X * transform.M12 + vector.Y * transform.M22 + vector.Z * transform.M32 + vector.W * transform.M42,
                vector.X * transform.M13 + vector.Y * transform.M23 + vector.Z * transform.M33 + vector.W * transform.M43,
                vector.X * transform.M14 + vector.Y * transform.M24 + vector.Z * transform.M34 + vector.W * transform.M44);
        }

        /// <summary>
        /// Transforms a 4D vector by the given <see cref="Matrix" />.
        /// </summary>
        /// <param name="vector">The source vector.</param>
        /// <param name="transform">The transformation <see cref="Matrix" />.</param>
        /// <returns>The transformed <see cref="Vector4" />.</returns>
        public static Vector4 Transform(Vector4 vector, Matrix transform)
        {
            Transform(ref vector, ref transform, out Vector4 result);
            return result;
        }

        /// <summary>
        /// Transforms an array of 4D vectors by the given <see cref="Matrix" />.
        /// </summary>
        /// <param name="source">The array of vectors to transform.</param>
        /// <param name="transform">The transformation <see cref="Matrix" />.</param>
        /// <param name="destination">
        /// The array for which the transformed vectors are stored.
        /// This array may be the same array as <paramref name="source" />.
        /// </param>
        /// <exception cref="ArgumentNullException">
        /// Thrown when <paramref name="source" /> or <paramref name="destination" /> is
        /// <c>null</c>.
        /// </exception>
        /// <exception cref="ArgumentOutOfRangeException">
        /// Thrown when <paramref name="destination" /> is shorter in length than
        /// <paramref name="source" />.
        /// </exception>
        public static void Transform(Vector4[] source, ref Matrix transform, Vector4[] destination)
        {
            if (source == null)
                throw new ArgumentNullException(nameof(source));

            if (destination == null)
                throw new ArgumentNullException(nameof(destination));

            if (destination.Length < source.Length)
                throw new ArgumentOutOfRangeException(nameof(destination), "The destination array must be of same length or larger length than the source array.");

            for (var i = 0; i < source.Length; ++i)
                Transform(ref source[i], ref transform, out destination[i]);
        }

        /// <summary>
        /// Adds two vectors.
        /// </summary>
        /// <param name="left">The first vector to add.</param>
        /// <param name="right">The second vector to add.</param>
        /// <returns>The sum of the two vectors.</returns>
        public static Vector4 operator +(Vector4 left, Vector4 right)
        {
            return new Vector4(left.X + right.X, left.Y + right.Y, left.Z + right.Z, left.W + right.W);
        }

        /// <summary>
        /// Multiplies a vector with another by performing component-wise multiplication equivalent to
        /// <see cref="Multiply(ref Vector4,ref Vector4,out Vector4)" />.
        /// </summary>
        /// <param name="left">The first vector to multiply.</param>
        /// <param name="right">The second vector to multiply.</param>
        /// <returns>The multiplication of the two vectors.</returns>
        public static Vector4 operator *(Vector4 left, Vector4 right)
        {
            return new Vector4(left.X * right.X, left.Y * right.Y, left.Z * right.Z, left.W * right.W);
        }

        /// <summary>
        /// Assert a vector (return it unchanged).
        /// </summary>
        /// <param name="value">The vector to assert (unchanged).</param>
        /// <returns>The asserted (unchanged) vector.</returns>
        public static Vector4 operator +(Vector4 value)
        {
            return value;
        }

        /// <summary>
        /// Subtracts two vectors.
        /// </summary>
        /// <param name="left">The first vector to subtract.</param>
        /// <param name="right">The second vector to subtract.</param>
        /// <returns>The difference of the two vectors.</returns>
        public static Vector4 operator -(Vector4 left, Vector4 right)
        {
            return new Vector4(left.X - right.X, left.Y - right.Y, left.Z - right.Z, left.W - right.W);
        }

        /// <summary>
        /// Reverses the direction of a given vector.
        /// </summary>
        /// <param name="value">The vector to negate.</param>
        /// <returns>A vector facing in the opposite direction.</returns>
        public static Vector4 operator -(Vector4 value)
        {
            return new Vector4(-value.X, -value.Y, -value.Z, -value.W);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Vector4 operator *(float scale, Vector4 value)
        {
            return new Vector4(value.X * scale, value.Y * scale, value.Z * scale, value.W * scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Vector4 operator *(Vector4 value, float scale)
        {
            return new Vector4(value.X * scale, value.Y * scale, value.Z * scale, value.W * scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Vector4 operator /(Vector4 value, float scale)
        {
            return new Vector4(value.X / scale, value.Y / scale, value.Z / scale, value.W / scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="value">The vector to scale.</param>
        /// <returns>The scaled vector.</returns>
        public static Vector4 operator /(float scale, Vector4 value)
        {
            return new Vector4(scale / value.X, scale / value.Y, scale / value.Z, scale / value.W);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Vector4 operator /(Vector4 value, Vector4 scale)
        {
            return new Vector4(value.X / scale.X, value.Y / scale.Y, value.Z / scale.Z, value.W / scale.W);
        }

        /// <summary>
        /// Remainder of value divided by scale.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The remained vector.</returns>
        public static Vector4 operator %(Vector4 value, float scale)
        {
            return new Vector4(value.X % scale, value.Y % scale, value.Z % scale, value.W % scale);
        }

        /// <summary>
        /// Remainder of value divided by scale.
        /// </summary>
        /// <param name="value">The amount by which to scale the vector.</param>
        /// <param name="scale">The vector to scale.</param>
        /// <returns>The remained vector.</returns>
        public static Vector4 operator %(float value, Vector4 scale)
        {
            return new Vector4(value % scale.X, value % scale.Y, value % scale.Z, value % scale.W);
        }

        /// <summary>
        /// Remainder of value divided by scale.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The remained vector.</returns>
        public static Vector4 operator %(Vector4 value, Vector4 scale)
        {
            return new Vector4(value.X % scale.X, value.Y % scale.Y, value.Z % scale.Z, value.W % scale.W);
        }

        /// <summary>
        /// Perform a component-wise addition
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be added on elements</param>
        /// <returns>The vector with added scalar for each element.</returns>
        public static Vector4 operator +(Vector4 value, float scalar)
        {
            return new Vector4(value.X + scalar, value.Y + scalar, value.Z + scalar, value.W + scalar);
        }

        /// <summary>
        /// Perform a component-wise addition
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be added on elements</param>
        /// <returns>The vector with added scalar for each element.</returns>
        public static Vector4 operator +(float scalar, Vector4 value)
        {
            return new Vector4(scalar + value.X, scalar + value.Y, scalar + value.Z, scalar + value.W);
        }

        /// <summary>
        /// Perform a component-wise subtraction
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be subtracted from elements</param>
        /// <returns>The vector with subtracted scalar from each element.</returns>
        public static Vector4 operator -(Vector4 value, float scalar)
        {
            return new Vector4(value.X - scalar, value.Y - scalar, value.Z - scalar, value.W - scalar);
        }

        /// <summary>
        /// Perform a component-wise subtraction
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be subtracted from elements</param>
        /// <returns>The vector with subtracted scalar from each element.</returns>
        public static Vector4 operator -(float scalar, Vector4 value)
        {
            return new Vector4(scalar - value.X, scalar - value.Y, scalar - value.Z, scalar - value.W);
        }

        /// <summary>
        /// Tests for equality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns>
        /// <c>true</c> if <paramref name="left" /> has the same value as <paramref name="right" />; otherwise,
        /// <c>false</c>.
        /// </returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator ==(Vector4 left, Vector4 right)
        {
            return left.Equals(ref right);
        }

        /// <summary>
        /// Tests for inequality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns>
        /// <c>true</c> if <paramref name="left" /> has a different value than <paramref name="right" />; otherwise,
        /// <c>false</c>.
        /// </returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator !=(Vector4 left, Vector4 right)
        {
            return !left.Equals(ref right);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Vector4" /> to <see cref="Vector2" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Vector2(Vector4 value)
        {
            return new Vector2(value.X, value.Y);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Vector4" /> to <see cref="Vector3" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Vector3(Vector4 value)
        {
            return new Vector3(value.X, value.Y, value.Z);
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <returns>
        /// A <see cref="System.String" /> that represents this instance.
        /// </returns>
        public override string ToString()
        {
            return string.Format(CultureInfo.CurrentCulture, _formatString, X, Y, Z, W);
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <param name="format">The format.</param>
        /// <returns>
        /// A <see cref="System.String" /> that represents this instance.
        /// </returns>
        public string ToString(string format)
        {
            if (format == null)
                return ToString();

            return string.Format(CultureInfo.CurrentCulture, _formatString, X.ToString(format, CultureInfo.CurrentCulture),
                                 Y.ToString(format, CultureInfo.CurrentCulture), Z.ToString(format, CultureInfo.CurrentCulture), W.ToString(format, CultureInfo.CurrentCulture));
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <param name="formatProvider">The format provider.</param>
        /// <returns>
        /// A <see cref="System.String" /> that represents this instance.
        /// </returns>
        public string ToString(IFormatProvider formatProvider)
        {
            return string.Format(formatProvider, _formatString, X, Y, Z, W);
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <param name="format">The format.</param>
        /// <param name="formatProvider">The format provider.</param>
        /// <returns>
        /// A <see cref="System.String" /> that represents this instance.
        /// </returns>
        public string ToString(string format, IFormatProvider formatProvider)
        {
            if (format == null)
                return ToString(formatProvider);

            return string.Format(formatProvider, "X:{0} Y:{1} Z:{2} W:{3}", X.ToString(format, formatProvider),
                                 Y.ToString(format, formatProvider), Z.ToString(format, formatProvider), W.ToString(format, formatProvider));
        }

        /// <summary>
        /// Returns a hash code for this instance.
        /// </summary>
        /// <returns>
        /// A hash code for this instance, suitable for use in hashing algorithms and data structures like a hash table.
        /// </returns>
        public override int GetHashCode()
        {
            unchecked
            {
                int hashCode = X.GetHashCode();
                hashCode = (hashCode * 397) ^ Y.GetHashCode();
                hashCode = (hashCode * 397) ^ Z.GetHashCode();
                hashCode = (hashCode * 397) ^ W.GetHashCode();
                return hashCode;
            }
        }

        /// <summary>
        /// Determines whether the specified <see cref="Vector4" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="Vector4" /> to compare with this instance.</param>
        /// <returns>
        /// <c>true</c> if the specified <see cref="Vector4" /> is equal to this instance; otherwise, <c>false</c>.
        /// </returns>
        public bool Equals(ref Vector4 other)
        {
            return Mathf.NearEqual(other.X, X) &&
                   Mathf.NearEqual(other.Y, Y) &&
                   Mathf.NearEqual(other.Z, Z) &&
                   Mathf.NearEqual(other.W, W);
        }

        /// <summary>
        /// Determines whether the specified <see cref="Vector4" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="Vector4" /> to compare with this instance.</param>
        /// <returns>
        /// <c>true</c> if the specified <see cref="Vector4" /> is equal to this instance; otherwise, <c>false</c>.
        /// </returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(Vector4 other)
        {
            return Equals(ref other);
        }

        /// <summary>
        /// Determines whether the specified <see cref="System.Object" /> is equal to this instance.
        /// </summary>
        /// <param name="value">The <see cref="System.Object" /> to compare with this instance.</param>
        /// <returns>
        /// <c>true</c> if the specified <see cref="System.Object" /> is equal to this instance; otherwise, <c>false</c>.
        /// </returns>
        public override bool Equals(object value)
        {
            if (!(value is Vector4))
                return false;

            var strongValue = (Vector4)value;
            return Equals(ref strongValue);
        }
    }
}
