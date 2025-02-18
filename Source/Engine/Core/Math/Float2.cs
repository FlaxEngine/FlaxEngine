// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
using System.Runtime.InteropServices;

namespace FlaxEngine
{
    [Serializable]
#if FLAX_EDITOR
    [System.ComponentModel.TypeConverter(typeof(TypeConverters.Float2Converter))]
#endif
    partial struct Float2 : IEquatable<Float2>, IFormattable
    {
        private static readonly string _formatString = "X:{0:F2} Y:{1:F2}";

        /// <summary>
        /// The size of the <see cref="Float2" /> type, in bytes.
        /// </summary>
        public static readonly int SizeInBytes = Marshal.SizeOf(typeof(Float2));

        /// <summary>
        /// A <see cref="Float2" /> with all of its components set to zero.
        /// </summary>
        public static readonly Float2 Zero;

        /// <summary>
        /// The X unit <see cref="Float2" /> (1, 0).
        /// </summary>
        public static readonly Float2 UnitX = new Float2(1.0f, 0.0f);

        /// <summary>
        /// The Y unit <see cref="Float2" /> (0, 1).
        /// </summary>
        public static readonly Float2 UnitY = new Float2(0.0f, 1.0f);

        /// <summary>
        /// A <see cref="Float2" /> with all of its components set to half.
        /// </summary>
        public static readonly Float2 Half = new Float2(0.5f, 0.5f);

        /// <summary>
        /// A <see cref="Float2" /> with all of its components set to one.
        /// </summary>
        public static readonly Float2 One = new Float2(1.0f, 1.0f);

        /// <summary>
        /// A <see cref="Float2" /> with all components equal to <see cref="float.MinValue"/>.
        /// </summary>
        public static readonly Float2 Minimum = new Float2(float.MinValue);

        /// <summary>
        /// A <see cref="Float2" /> with all components equal to <see cref="float.MaxValue"/>.
        /// </summary>
        public static readonly Float2 Maximum = new Float2(float.MaxValue);

        /// <summary>
        /// Initializes a new instance of the <see cref="Float2" /> struct.
        /// </summary>
        /// <param name="value">The value that will be assigned to all components.</param>
        public Float2(float value)
        {
            X = value;
            Y = value;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Float2" /> struct.
        /// </summary>
        /// <param name="x">Initial value for the X component of the vector.</param>
        /// <param name="y">Initial value for the Y component of the vector.</param>
        public Float2(float x, float y)
        {
            X = x;
            Y = y;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Float2" /> struct.
        /// </summary>
        /// <param name="value">A vector containing the values with which to initialize the X, Y and Z components.</param>
        public Float2(Vector2 value)
        {
            X = (float)value.X;
            Y = (float)value.Y;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Float2" /> struct.
        /// </summary>
        /// <param name="value">A vector containing the values with which to initialize the X, Y and Z components.</param>
        public Float2(Double2 value)
        {
            X = (float)value.X;
            Y = (float)value.Y;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Float2" /> struct.
        /// </summary>
        /// <param name="value">A vector containing the values with which to initialize the X and Y components.</param>
        public Float2(Float3 value)
        {
            X = value.X;
            Y = value.Y;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Float2" /> struct.
        /// </summary>
        /// <param name="value">A vector containing the values with which to initialize the X and Y components.</param>
        public Float2(Float4 value)
        {
            X = value.X;
            Y = value.Y;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Float2" /> struct.
        /// </summary>
        /// <param name="values">The values to assign to the X and Y components of the vector. This must be an array with two elements.</param>
        /// <exception cref="ArgumentNullException">Thrown when <paramref name="values" /> is <c>null</c>.</exception>
        /// <exception cref="ArgumentOutOfRangeException">Thrown when <paramref name="values" /> contains more or less than two elements.</exception>
        public Float2(float[] values)
        {
            if (values == null)
                throw new ArgumentNullException(nameof(values));
            if (values.Length != 2)
                throw new ArgumentOutOfRangeException(nameof(values), "There must be two and only two input values for Float2.");
            X = values[0];
            Y = values[1];
        }

        /// <summary>
        /// Gets a value indicting whether this instance is normalized.
        /// </summary>
        public bool IsNormalized => Mathf.Abs((X * X + Y * Y) - 1.0f) < 1e-4f;

        /// <summary>
        /// Gets a value indicting whether this vector is zero
        /// </summary>
        public bool IsZero => Mathf.IsZero(X) && Mathf.IsZero(Y);

        /// <summary>
        /// Gets a minimum component value
        /// </summary>
        public float MinValue => Mathf.Min(X, Y);

        /// <summary>
        /// Gets a maximum component value
        /// </summary>
        public float MaxValue => Mathf.Max(X, Y);

        /// <summary>
        /// Gets an arithmetic average value of all vector components.
        /// </summary>
        public float AvgValue => (X + Y) * (1.0f / 2.0f);

        /// <summary>
        /// Gets a sum of the component values.
        /// </summary>
        public float ValuesSum => X + Y;

        /// <summary>
        /// Gets a vector with values being absolute values of that vector.
        /// </summary>
        public Float2 Absolute => new Float2(Mathf.Abs(X), Mathf.Abs(Y));

        /// <summary>
        /// Gets a vector with values being opposite to values of that vector.
        /// </summary>
        public Float2 Negative => new Float2(-X, -Y);

        /// <summary>
        /// Gets or sets the component at the specified index.
        /// </summary>
        /// <value>The value of the X or Y component, depending on the index.</value>
        /// <param name="index">The index of the component to access. Use 0 for the X component and 1 for the Y component.</param>
        /// <returns>The value of the component at the specified index.</returns>
        /// <exception cref="System.ArgumentOutOfRangeException">Thrown when the <paramref name="index" /> is out of the range [0,1].</exception>
        public float this[int index]
        {
            get
            {
                switch (index)
                {
                case 0: return X;
                case 1: return Y;
                }
                throw new ArgumentOutOfRangeException(nameof(index), "Indices for Float2 run from 0 to 1, inclusive.");
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
                default: throw new ArgumentOutOfRangeException(nameof(index), "Indices for Float2 run from 0 to 1, inclusive.");
                }
            }
        }

        /// <summary>
        /// Calculates the length of the vector.
        /// </summary>
        /// <returns>The length of the vector.</returns>
        /// <remarks><see cref="Float2.LengthSquared" /> may be preferred when only the relative length is needed and speed is of the essence.</remarks>
        public float Length => (float)Math.Sqrt(X * X + Y * Y);

        /// <summary>
        /// Calculates the squared length of the vector.
        /// </summary>
        /// <returns>The squared length of the vector.</returns>
        /// <remarks>This method may be preferred to <see cref="Float2.Length" /> when only a relative length is needed and speed is of the essence.</remarks>
        public float LengthSquared => X * X + Y * Y;

        /// <summary>
        /// Gets the normalized vector. Returned vector has length equal 1.
        /// </summary>
        public Float2 Normalized
        {
            get
            {
                Float2 result = this;
                result.Normalize();
                return result;
            }
        }

        /// <summary>
        /// Converts the vector into a unit vector.
        /// </summary>
        public void Normalize()
        {
            float length = Length;
            if (length >= Mathf.Epsilon)
            {
                float inv = 1.0f / length;
                X *= inv;
                Y *= inv;
            }
        }

        /// <summary>
        /// Creates an array containing the elements of the vector.
        /// </summary>
        public float[] ToArray()
        {
            return new[] { X, Y };
        }

        /// <summary>
        /// Adds two vectors.
        /// </summary>
        /// <param name="left">The first vector to add.</param>
        /// <param name="right">The second vector to add.</param>
        /// <param name="result">When the method completes, contains the sum of the two vectors.</param>
        public static void Add(ref Float2 left, ref Float2 right, out Float2 result)
        {
            result = new Float2(left.X + right.X, left.Y + right.Y);
        }

        /// <summary>
        /// Adds two vectors.
        /// </summary>
        /// <param name="left">The first vector to add.</param>
        /// <param name="right">The second vector to add.</param>
        /// <returns>The sum of the two vectors.</returns>
        public static Float2 Add(Float2 left, Float2 right)
        {
            return new Float2(left.X + right.X, left.Y + right.Y);
        }

        /// <summary>
        /// Performs a component-wise addition.
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be added to elements</param>
        /// <param name="result">The vector with added scalar for each element.</param>
        public static void Add(ref Float2 left, ref float right, out Float2 result)
        {
            result = new Float2(left.X + right, left.Y + right);
        }

        /// <summary>
        /// Performs a component-wise addition.
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be added to elements</param>
        /// <returns>The vector with added scalar for each element.</returns>
        public static Float2 Add(Float2 left, float right)
        {
            return new Float2(left.X + right, left.Y + right);
        }

        /// <summary>
        /// Subtracts two vectors.
        /// </summary>
        /// <param name="left">The first vector to subtract.</param>
        /// <param name="right">The second vector to subtract.</param>
        /// <param name="result">When the method completes, contains the difference of the two vectors.</param>
        public static void Subtract(ref Float2 left, ref Float2 right, out Float2 result)
        {
            result = new Float2(left.X - right.X, left.Y - right.Y);
        }

        /// <summary>
        /// Subtracts two vectors.
        /// </summary>
        /// <param name="left">The first vector to subtract.</param>
        /// <param name="right">The second vector to subtract.</param>
        /// <returns>The difference of the two vectors.</returns>
        public static Float2 Subtract(Float2 left, Float2 right)
        {
            return new Float2(left.X - right.X, left.Y - right.Y);
        }

        /// <summary>
        /// Performs a component-wise subtraction.
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be subtracted from elements</param>
        /// <param name="result">The vector with subtracted scalar for each element.</param>
        public static void Subtract(ref Float2 left, ref float right, out Float2 result)
        {
            result = new Float2(left.X - right, left.Y - right);
        }

        /// <summary>
        /// Performs a component-wise subtraction.
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be subtracted from elements</param>
        /// <returns>The vector with subtracted scalar for each element.</returns>
        public static Float2 Subtract(Float2 left, float right)
        {
            return new Float2(left.X - right, left.Y - right);
        }

        /// <summary>
        /// Performs a component-wise subtraction.
        /// </summary>
        /// <param name="left">The scalar value to be subtracted from elements</param>
        /// <param name="right">The input vector</param>
        /// <param name="result">The vector with subtracted scalar for each element.</param>
        public static void Subtract(ref float left, ref Float2 right, out Float2 result)
        {
            result = new Float2(left - right.X, left - right.Y);
        }

        /// <summary>
        /// Performs a component-wise subtraction.
        /// </summary>
        /// <param name="left">The scalar value to be subtracted from elements</param>
        /// <param name="right">The input vector</param>
        /// <returns>The vector with subtracted scalar for each element.</returns>
        public static Float2 Subtract(float left, Float2 right)
        {
            return new Float2(left - right.X, left - right.Y);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="result">When the method completes, contains the scaled vector.</param>
        public static void Multiply(ref Float2 value, float scale, out Float2 result)
        {
            result = new Float2(value.X * scale, value.Y * scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float2 Multiply(Float2 value, float scale)
        {
            return new Float2(value.X * scale, value.Y * scale);
        }

        /// <summary>
        /// Multiplies a vector with another by performing component-wise multiplication.
        /// </summary>
        /// <param name="left">The first vector to multiply.</param>
        /// <param name="right">The second vector to multiply.</param>
        /// <param name="result">When the method completes, contains the multiplied vector.</param>
        public static void Multiply(ref Float2 left, ref Float2 right, out Float2 result)
        {
            result = new Float2(left.X * right.X, left.Y * right.Y);
        }

        /// <summary>
        /// Multiplies a vector with another by performing component-wise multiplication.
        /// </summary>
        /// <param name="left">The first vector to multiply.</param>
        /// <param name="right">The second vector to multiply.</param>
        /// <returns>The multiplied vector.</returns>
        public static Float2 Multiply(Float2 left, Float2 right)
        {
            return new Float2(left.X * right.X, left.Y * right.Y);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="result">When the method completes, contains the scaled vector.</param>
        public static void Divide(ref Float2 value, float scale, out Float2 result)
        {
            result = new Float2(value.X / scale, value.Y / scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float2 Divide(Float2 value, float scale)
        {
            return new Float2(value.X / scale, value.Y / scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="value">The vector to scale.</param>
        /// <param name="result">When the method completes, contains the scaled vector.</param>
        public static void Divide(float scale, ref Float2 value, out Float2 result)
        {
            result = new Float2(scale / value.X, scale / value.Y);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float2 Divide(float scale, Float2 value)
        {
            return new Float2(scale / value.X, scale / value.Y);
        }

        /// <summary>
        /// Reverses the direction of a given vector.
        /// </summary>
        /// <param name="value">The vector to negate.</param>
        /// <param name="result">When the method completes, contains a vector facing in the opposite direction.</param>
        public static void Negate(ref Float2 value, out Float2 result)
        {
            result = new Float2(-value.X, -value.Y);
        }

        /// <summary>
        /// Reverses the direction of a given vector.
        /// </summary>
        /// <param name="value">The vector to negate.</param>
        /// <returns>A vector facing in the opposite direction.</returns>
        public static Float2 Negate(Float2 value)
        {
            return new Float2(-value.X, -value.Y);
        }

        /// <summary>
        /// Returns a <see cref="Float2" /> containing the 2D Cartesian coordinates of a point specified in Barycentric
        /// coordinates relative to a 2D triangle.
        /// </summary>
        /// <param name="value1">A <see cref="Float2" /> containing the 2D Cartesian coordinates of vertex 1 of the triangle.</param>
        /// <param name="value2">A <see cref="Float2" /> containing the 2D Cartesian coordinates of vertex 2 of the triangle.</param>
        /// <param name="value3">A <see cref="Float2" /> containing the 2D Cartesian coordinates of vertex 3 of the triangle.</param>
        /// <param name="amount1">Barycentric coordinate b2, which expresses the weighting factor toward vertex 2 (specified in <paramref name="value2" />).</param>
        /// <param name="amount2">Barycentric coordinate b3, which expresses the weighting factor toward vertex 3 (specified in <paramref name="value3" />).</param>
        /// <param name="result">When the method completes, contains the 2D Cartesian coordinates of the specified point.</param>
        public static void Barycentric(ref Float2 value1, ref Float2 value2, ref Float2 value3, float amount1, float amount2, out Float2 result)
        {
            result = new Float2(value1.X + amount1 * (value2.X - value1.X) + amount2 * (value3.X - value1.X),
                                value1.Y + amount1 * (value2.Y - value1.Y) + amount2 * (value3.Y - value1.Y));
        }

        /// <summary>
        /// Returns a <see cref="Float2" /> containing the 2D Cartesian coordinates of a point specified in Barycentric
        /// coordinates relative to a 2D triangle.
        /// </summary>
        /// <param name="value1">A <see cref="Float2" /> containing the 2D Cartesian coordinates of vertex 1 of the triangle.</param>
        /// <param name="value2">A <see cref="Float2" /> containing the 2D Cartesian coordinates of vertex 2 of the triangle.</param>
        /// <param name="value3">A <see cref="Float2" /> containing the 2D Cartesian coordinates of vertex 3 of the triangle.</param>
        /// <param name="amount1">Barycentric coordinate b2, which expresses the weighting factor toward vertex 2 (specified in <paramref name="value2" />).</param>
        /// <param name="amount2">Barycentric coordinate b3, which expresses the weighting factor toward vertex 3 (specified in <paramref name="value3" />).</param>
        /// <returns>A new <see cref="Float2" /> containing the 2D Cartesian coordinates of the specified point.</returns>
        public static Float2 Barycentric(Float2 value1, Float2 value2, Float2 value3, float amount1, float amount2)
        {
            Barycentric(ref value1, ref value2, ref value3, amount1, amount2, out Float2 result);
            return result;
        }

        /// <summary>
        /// Restricts a value to be within a specified range.
        /// </summary>
        /// <param name="value">The value to clamp.</param>
        /// <param name="min">The minimum value.</param>
        /// <param name="max">The maximum value.</param>
        /// <param name="result">When the method completes, contains the clamped value.</param>
        public static void Clamp(ref Float2 value, ref Float2 min, ref Float2 max, out Float2 result)
        {
            float x = value.X;
            x = x > max.X ? max.X : x;
            x = x < min.X ? min.X : x;
            float y = value.Y;
            y = y > max.Y ? max.Y : y;
            y = y < min.Y ? min.Y : y;
            result = new Float2(x, y);
        }

        /// <summary>
        /// Restricts a value to be within a specified range.
        /// </summary>
        /// <param name="value">The value to clamp.</param>
        /// <param name="min">The minimum value.</param>
        /// <param name="max">The maximum value.</param>
        /// <returns>The clamped value.</returns>
        public static Float2 Clamp(Float2 value, Float2 min, Float2 max)
        {
            Clamp(ref value, ref min, ref max, out Float2 result);
            return result;
        }

        /// <summary>
        /// Saturates this instance in the range [0,1].
        /// </summary>
        public void Saturate()
        {
            X = X < 0.0f ? 0.0f : X > 1.0f ? 1.0f : X;
            Y = Y < 0.0f ? 0.0f : Y > 1.0f ? 1.0f : Y;
        }

        /// <summary>
        /// Calculates the area of the triangle.
        /// </summary>
        /// <param name="v0">The first triangle vertex.</param>
        /// <param name="v1">The second triangle vertex.</param>
        /// <param name="v2">The third triangle vertex.</param>
        /// <returns>The triangle area.</returns>
        public static float TriangleArea(ref Float2 v0, ref Float2 v1, ref Float2 v2)
        {
            return Mathf.Abs((v0.X * (v1.Y - v2.Y) + v1.X * (v2.Y - v0.Y) + v2.X * (v0.Y - v1.Y)) / 2);
        }

        /// <summary>
        /// Calculates the distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <param name="result">When the method completes, contains the distance between the two vectors.</param>
        /// <remarks><see cref="Float2.DistanceSquared(ref Float2, ref Float2, out float)" /> may be preferred when only the relative distance is needed and speed is of the essence.</remarks>
        public static void Distance(ref Float2 value1, ref Float2 value2, out float result)
        {
            float x = value1.X - value2.X;
            float y = value1.Y - value2.Y;
            result = (float)Math.Sqrt(x * x + y * y);
        }

        /// <summary>
        /// Calculates the distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <returns>The distance between the two vectors.</returns>
        /// <remarks><see cref="Float2.DistanceSquared(Float2, Float2)" /> may be preferred when only the relative distance is needed and speed is of the essence.</remarks>
        public static float Distance(Float2 value1, Float2 value2)
        {
            float x = value1.X - value2.X;
            float y = value1.Y - value2.Y;
            return (float)Math.Sqrt(x * x + y * y);
        }

        /// <summary>
        /// Calculates the distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <returns>The distance between the two vectors.</returns>
        /// <remarks><see cref="Float2.DistanceSquared(ref Float2, ref Float2, out float)" /> may be preferred when only the relative distance is needed and speed is of the essence.</remarks>
        public static float Distance(ref Float2 value1, ref Float2 value2)
        {
            float x = value1.X - value2.X;
            float y = value1.Y - value2.Y;
            return (float)Math.Sqrt(x * x + y * y);
        }

        /// <summary>
        /// Calculates the squared distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector</param>
        /// <param name="result">When the method completes, contains the squared distance between the two vectors.</param>
        public static void DistanceSquared(ref Float2 value1, ref Float2 value2, out float result)
        {
            float x = value1.X - value2.X;
            float y = value1.Y - value2.Y;
            result = x * x + y * y;
        }

        /// <summary>
        /// Calculates the squared distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector</param>
        /// <returns>The squared distance between the two vectors.</returns>
        public static float DistanceSquared(ref Float2 value1, ref Float2 value2)
        {
            float x = value1.X - value2.X;
            float y = value1.Y - value2.Y;
            return x * x + y * y;
        }

        /// <summary>
        /// Calculates the squared distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <returns>The squared distance between the two vectors.</returns>
        public static float DistanceSquared(Float2 value1, Float2 value2)
        {
            float x = value1.X - value2.X;
            float y = value1.Y - value2.Y;
            return x * x + y * y;
        }

        /// <summary>
        /// Tests whether one vector is near another vector.
        /// </summary>
        /// <param name="left">The left vector.</param>
        /// <param name="right">The right vector.</param>
        /// <param name="epsilon">The epsilon.</param>
        /// <returns><c>true</c> if left and right are near, <c>false</c> otherwise</returns>
        public static bool NearEqual(Float2 left, Float2 right, float epsilon = Mathf.Epsilon)
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
        public static bool NearEqual(ref Float2 left, ref Float2 right, float epsilon = Mathf.Epsilon)
        {
            return Mathf.WithinEpsilon(left.X, right.X, epsilon) && Mathf.WithinEpsilon(left.Y, right.Y, epsilon);
        }

        /// <summary>
        /// Calculates the dot product of two vectors.
        /// </summary>
        /// <param name="left">First source vector.</param>
        /// <param name="right">Second source vector.</param>
        /// <param name="result">When the method completes, contains the dot product of the two vectors.</param>
        public static void Dot(ref Float2 left, ref Float2 right, out float result)
        {
            result = left.X * right.X + left.Y * right.Y;
        }

        /// <summary>
        /// Calculates the dot product of two vectors.
        /// </summary>
        /// <param name="left">First source vector.</param>
        /// <param name="right">Second source vector.</param>
        /// <returns>The dot product of the two vectors.</returns>
        public static float Dot(ref Float2 left, ref Float2 right)
        {
            return left.X * right.X + left.Y * right.Y;
        }

        /// <summary>
        /// Calculates the dot product of two vectors.
        /// </summary>
        /// <param name="left">First source vector.</param>
        /// <param name="right">Second source vector.</param>
        /// <returns>The dot product of the two vectors.</returns>
        public static float Dot(Float2 left, Float2 right)
        {
            return left.X * right.X + left.Y * right.Y;
        }

        /// <summary>
        /// Calculates the cross product of two vectors.
        /// </summary>
        /// <param name="left">First source vector.</param>
        /// <param name="right">Second source vector.</param>
        /// <param name="result">When the method completes, contains the cross product of the two vectors.</param>
        public static void Cross(ref Float2 left, ref Float2 right, out float result)
        {
            result = left.X * right.Y - left.Y * right.X;
        }

        /// <summary>
        /// Calculates the cross product of two vectors.
        /// </summary>
        /// <param name="left">First source vector.</param>
        /// <param name="right">Second source vector.</param>
        /// <returns>The cross product of the two vectors.</returns>
        public static float Cross(ref Float2 left, ref Float2 right)
        {
            return left.X * right.Y - left.Y * right.X;
        }

        /// <summary>
        /// Calculates the cross product of two vectors.
        /// </summary>
        /// <param name="left">First source vector.</param>
        /// <param name="right">Second source vector.</param>
        /// <returns>The cross product of the two vectors.</returns>
        public static float Cross(Float2 left, Float2 right)
        {
            return left.X * right.Y - left.Y * right.X;
        }

        /// <summary>
        /// Converts the vector into a unit vector.
        /// </summary>
        /// <param name="value">The vector to normalize.</param>
        /// <param name="result">When the method completes, contains the normalized vector.</param>
        public static void Normalize(ref Float2 value, out Float2 result)
        {
            result = value;
            result.Normalize();
        }

        /// <summary>
        /// Converts the vector into a unit vector.
        /// </summary>
        /// <param name="value">The vector to normalize.</param>
        /// <returns>The normalized vector.</returns>
        public static Float2 Normalize(Float2 value)
        {
            value.Normalize();
            return value;
        }

        /// <summary>
        /// Makes sure that Length of the output vector is always below max and above 0.
        /// </summary>
        /// <param name="vector">Input vector.</param>
        /// <param name="max">Max Length</param>
        public static Float2 ClampLength(Float2 vector, float max)
        {
            return ClampLength(vector, 0, max);
        }

        /// <summary>
        /// Makes sure that Length of the output vector is always below max and above min.
        /// </summary>
        /// <param name="vector">Input vector.</param>
        /// <param name="min">Min Length</param>
        /// <param name="max">Max Length</param>
        public static Float2 ClampLength(Float2 vector, float min, float max)
        {
            ClampLength(vector, min, max, out Float2 result);
            return result;
        }

        /// <summary>
        /// Makes sure that Length of the output vector is always below max and above min.
        /// </summary>
        /// <param name="vector">Input vector.</param>
        /// <param name="min">Min Length</param>
        /// <param name="max">Max Length</param>
        /// <param name="result">The result value.</param>
        public static void ClampLength(Float2 vector, float min, float max, out Float2 result)
        {
            result = vector;
            float lenSq = result.LengthSquared;
            if (lenSq > max * max)
            {
                float scaleFactor = max / (float)Math.Sqrt(lenSq);
                result.X *= scaleFactor;
                result.Y *= scaleFactor;
            }
            if (lenSq < min * min)
            {
                float scaleFactor = min / (float)Math.Sqrt(lenSq);
                result.X *= scaleFactor;
                result.Y *= scaleFactor;
            }
        }

        /// <summary>
        /// Returns the vector with components rounded to the nearest integer.
        /// </summary>
        /// <param name="v">The value.</param>
        /// <returns>The result.</returns>
        public static Float2 Round(Float2 v)
        {
            return new Float2(Mathf.Round(v.X), Mathf.Round(v.Y));
        }

        /// <summary>
        /// Returns the vector with components containing the smallest integer greater to or equal to the original value.
        /// </summary>
        /// <param name="v">The value.</param>
        /// <returns>The result.</returns>
        public static Float2 Ceil(Float2 v)
        {
            return new Float2(Mathf.Ceil(v.X), Mathf.Ceil(v.Y));
        }

        /// <summary>
        /// Returns the vector with components containing the smallest integer smaller to or equal to the original value.
        /// </summary>
        /// <param name="v">The value.</param>
        /// <returns>The result.</returns>
        public static Float2 Floor(Float2 v)
        {
            return new Float2(Mathf.Floor(v.X), Mathf.Floor(v.Y));
        }

        /// <summary>
        /// Breaks the components of the vector into an integral and a fractional part. Returns vector made of fractional parts.
        /// </summary>
        /// <param name="v">The value.</param>
        /// <returns>The result.</returns>
        public static Float2 Mod(Float2 v)
        {
            return new Float2(v.X - (int)v.X, v.Y - (int)v.Y);
        }

        /// <summary>
        /// Performs a linear interpolation between two vectors.
        /// </summary>
        /// <param name="start">Start vector.</param>
        /// <param name="end">End vector.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end" />.</param>
        /// <param name="result">When the method completes, contains the linear interpolation of the two vectors.</param>
        /// <remarks>Passing <paramref name="amount" /> a value of 0 will cause <paramref name="start" /> to be returned; a value of 1 will cause <paramref name="end" /> to be returned.</remarks>
        public static void Lerp(ref Float2 start, ref Float2 end, float amount, out Float2 result)
        {
            result.X = Mathf.Lerp(start.X, end.X, amount);
            result.Y = Mathf.Lerp(start.Y, end.Y, amount);
        }

        /// <summary>
        /// Performs a linear interpolation between two vectors.
        /// </summary>
        /// <param name="start">Start vector.</param>
        /// <param name="end">End vector.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end" />.</param>
        /// <returns>The linear interpolation of the two vectors.</returns>
        /// <remarks>Passing <paramref name="amount" /> a value of 0 will cause <paramref name="start" /> to be returned; a value of 1 will cause <paramref name="end" /> to be returned.</remarks>
        public static Float2 Lerp(Float2 start, Float2 end, float amount)
        {
            Lerp(ref start, ref end, amount, out Float2 result);
            return result;
        }

        /// <summary>
        /// Performs a linear interpolation between two vectors.
        /// </summary>
        /// <param name="start">Start vector.</param>
        /// <param name="end">End vector.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end" />.</param>
        /// <param name="result">When the method completes, contains the linear interpolation of the two vectors.</param>
        /// <remarks>Passing <paramref name="amount" /> a value of 0 will cause <paramref name="start" /> to be returned; a value of 1 will cause <paramref name="end" /> to be returned.</remarks>
        public static void Lerp(ref Float2 start, ref Float2 end, ref Float2 amount, out Float2 result)
        {
            result.X = Mathf.Lerp(start.X, end.X, amount.X);
            result.Y = Mathf.Lerp(start.Y, end.Y, amount.Y);
        }

        /// <summary>
        /// Performs a linear interpolation between two vectors.
        /// </summary>
        /// <param name="start">Start vector.</param>
        /// <param name="end">End vector.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end" />.</param>
        /// <returns>The linear interpolation of the two vectors.</returns>
        /// <remarks>Passing <paramref name="amount" /> a value of 0 will cause <paramref name="start" /> to be returned; a value of 1 will cause <paramref name="end" /> to be returned.</remarks>
        public static Float2 Lerp(Float2 start, Float2 end, Float2 amount)
        {
            Lerp(ref start, ref end, ref amount, out Float2 result);
            return result;
        }

        /// <summary>
        /// Performs a cubic interpolation between two vectors.
        /// </summary>
        /// <param name="start">Start vector.</param>
        /// <param name="end">End vector.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end" />.</param>
        /// <param name="result">When the method completes, contains the cubic interpolation of the two vectors.</param>
        public static void SmoothStep(ref Float2 start, ref Float2 end, float amount, out Float2 result)
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
        public static Float2 SmoothStep(Float2 start, Float2 end, float amount)
        {
            SmoothStep(ref start, ref end, amount, out Float2 result);
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
        public static void Hermite(ref Float2 value1, ref Float2 tangent1, ref Float2 value2, ref Float2 tangent2, float amount, out Float2 result)
        {
            float squared = amount * amount;
            float cubed = amount * squared;
            float part1 = 2.0f * cubed - 3.0f * squared + 1.0f;
            float part2 = -2.0f * cubed + 3.0f * squared;
            float part3 = cubed - 2.0f * squared + amount;
            float part4 = cubed - squared;
            result.X = value1.X * part1 + value2.X * part2 + tangent1.X * part3 + tangent2.X * part4;
            result.Y = value1.Y * part1 + value2.Y * part2 + tangent1.Y * part3 + tangent2.Y * part4;
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
        public static Float2 Hermite(Float2 value1, Float2 tangent1, Float2 value2, Float2 tangent2, float amount)
        {
            Hermite(ref value1, ref tangent1, ref value2, ref tangent2, amount, out Float2 result);
            return result;
        }

        /// <summary>
        /// Calculates the 2D vector perpendicular to the given 2D vector. The result is always rotated 90-degrees in a counter-clockwise direction for a 2D coordinate system where the positive Y axis goes up.
        /// </summary>
        /// <param name="inDirection">The input direction.</param>
        /// <returns>The result.</returns>
        public static Float2 Perpendicular(Float2 inDirection)
        {
            return new Float2(-inDirection.Y, inDirection.X);
        }

        /// <summary>
        /// Calculates the 2D vector perpendicular to the given 2D vector. The result is always rotated 90-degrees in a counter-clockwise direction for a 2D coordinate system where the positive Y axis goes up.
        /// </summary>
        /// <param name="inDirection">The in direction.</param>
        /// <param name="result">When the method completes, contains the result of the calculation.</param>
        public static void Perpendicular(ref Float2 inDirection, out Float2 result)
        {
            result = new Float2(-inDirection.Y, inDirection.X);
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
        public static void CatmullRom(ref Float2 value1, ref Float2 value2, ref Float2 value3, ref Float2 value4, float amount, out Float2 result)
        {
            float squared = amount * amount;
            float cubed = amount * squared;
            result.X = 0.5f * (2.0f * value2.X + (-value1.X + value3.X) * amount +
                               (2.0f * value1.X - 5.0f * value2.X + 4.0f * value3.X - value4.X) * squared +
                               (-value1.X + 3.0f * value2.X - 3.0f * value3.X + value4.X) * cubed);
            result.Y = 0.5f * (2.0f * value2.Y + (-value1.Y + value3.Y) * amount +
                               (2.0f * value1.Y - 5.0f * value2.Y + 4.0f * value3.Y - value4.Y) * squared +
                               (-value1.Y + 3.0f * value2.Y - 3.0f * value3.Y + value4.Y) * cubed);
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
        public static Float2 CatmullRom(Float2 value1, Float2 value2, Float2 value3, Float2 value4, float amount)
        {
            CatmullRom(ref value1, ref value2, ref value3, ref value4, amount, out Float2 result);
            return result;
        }

        /// <summary>
        /// Returns a vector containing the largest components of the specified vectors.
        /// </summary>
        /// <param name="left">The first source vector.</param>
        /// <param name="right">The second source vector.</param>
        /// <param name="result">When the method completes, contains an new vector composed of the largest components of the source vectors.</param>
        public static void Max(ref Float2 left, ref Float2 right, out Float2 result)
        {
            result.X = left.X > right.X ? left.X : right.X;
            result.Y = left.Y > right.Y ? left.Y : right.Y;
        }

        /// <summary>
        /// Returns a vector containing the largest components of the specified vectors.
        /// </summary>
        /// <param name="left">The first source vector.</param>
        /// <param name="right">The second source vector.</param>
        /// <returns>A vector containing the largest components of the source vectors.</returns>
        public static Float2 Max(Float2 left, Float2 right)
        {
            Max(ref left, ref right, out Float2 result);
            return result;
        }

        /// <summary>
        /// Returns a vector containing the smallest components of the specified vectors.
        /// </summary>
        /// <param name="left">The first source vector.</param>
        /// <param name="right">The second source vector.</param>
        /// <param name="result">When the method completes, contains an new vector composed of the smallest components of the source vectors.</param>
        public static void Min(ref Float2 left, ref Float2 right, out Float2 result)
        {
            result.X = left.X < right.X ? left.X : right.X;
            result.Y = left.Y < right.Y ? left.Y : right.Y;
        }

        /// <summary>
        /// Returns a vector containing the smallest components of the specified vectors.
        /// </summary>
        /// <param name="left">The first source vector.</param>
        /// <param name="right">The second source vector.</param>
        /// <returns>A vector containing the smallest components of the source vectors.</returns>
        public static Float2 Min(Float2 left, Float2 right)
        {
            Min(ref left, ref right, out Float2 result);
            return result;
        }

        /// <summary>
        /// Returns the absolute value of a vector.
        /// </summary>
        /// <param name="v">The value.</param>
        /// <returns> A vector which components are less or equal to 0.</returns>
        public static Float2 Abs(Float2 v)
        {
            return new Float2(Math.Abs(v.X), Math.Abs(v.Y));
        }

        /// <summary>
        /// Returns the reflection of a vector off a surface that has the specified normal.
        /// </summary>
        /// <param name="vector">The source vector.</param>
        /// <param name="normal">Normal of the surface.</param>
        /// <param name="result">When the method completes, contains the reflected vector.</param>
        /// <remarks>Reflect only gives the direction of a reflection off a surface, it does not determine whether the original vector was close enough to the surface to hit it.</remarks>
        public static void Reflect(ref Float2 vector, ref Float2 normal, out Float2 result)
        {
            float dot = vector.X * normal.X + vector.Y * normal.Y;
            result.X = vector.X - 2.0f * dot * normal.X;
            result.Y = vector.Y - 2.0f * dot * normal.Y;
        }

        /// <summary>
        /// Returns the reflection of a vector off a surface that has the specified normal.
        /// </summary>
        /// <param name="vector">The source vector.</param>
        /// <param name="normal">Normal of the surface.</param>
        /// <returns>The reflected vector.</returns>
        /// <remarks>Reflect only gives the direction of a reflection off a surface, it does not determine whether the original vector was close enough to the surface to hit it.</remarks>
        public static Float2 Reflect(Float2 vector, Float2 normal)
        {
            Reflect(ref vector, ref normal, out Float2 result);
            return result;
        }

        /// <summary>
        /// Transforms a 2D vector by the given <see cref="Quaternion" /> rotation.
        /// </summary>
        /// <param name="vector">The vector to rotate.</param>
        /// <param name="rotation">The <see cref="Quaternion" /> rotation to apply.</param>
        /// <param name="result">When the method completes, contains the transformed <see cref="Float2" />.</param>
        public static void Transform(ref Float2 vector, ref Quaternion rotation, out Float2 result)
        {
            float x = rotation.X + rotation.X;
            float y = rotation.Y + rotation.Y;
            float z = rotation.Z + rotation.Z;
            float wz = rotation.W * z;
            float xx = rotation.X * x;
            float xy = rotation.X * y;
            float yy = rotation.Y * y;
            float zz = rotation.Z * z;
            result = new Float2(vector.X * (1.0f - yy - zz) + vector.Y * (xy - wz), vector.X * (xy + wz) + vector.Y * (1.0f - xx - zz));
        }

        /// <summary>
        /// Transforms a 2D vector by the given <see cref="Quaternion" /> rotation.
        /// </summary>
        /// <param name="vector">The vector to rotate.</param>
        /// <param name="rotation">The <see cref="Quaternion" /> rotation to apply.</param>
        /// <returns>The transformed <see cref="Float2" />.</returns>
        public static Float2 Transform(Float2 vector, Quaternion rotation)
        {
            Transform(ref vector, ref rotation, out Float2 result);
            return result;
        }

        /// <summary>
        /// Transforms a 2D vector by the given <see cref="Matrix" />.
        /// </summary>
        /// <param name="vector">The source vector.</param>
        /// <param name="transform">The transformation <see cref="Matrix" />.</param>
        /// <param name="result">When the method completes, contains the transformed <see cref="Float4" />.</param>
        public static void Transform(ref Float2 vector, ref Matrix transform, out Float4 result)
        {
            result = new Float4(vector.X * transform.M11 + vector.Y * transform.M21 + transform.M41,
                                vector.X * transform.M12 + vector.Y * transform.M22 + transform.M42,
                                vector.X * transform.M13 + vector.Y * transform.M23 + transform.M43,
                                vector.X * transform.M14 + vector.Y * transform.M24 + transform.M44);
        }

        /// <summary>
        /// Transforms a 2D vector by the given <see cref="Matrix" />.
        /// </summary>
        /// <param name="vector">The source vector.</param>
        /// <param name="transform">The transformation <see cref="Matrix" />.</param>
        /// <returns>The transformed <see cref="Float4" />.</returns>
        public static Float4 Transform(Float2 vector, Matrix transform)
        {
            Transform(ref vector, ref transform, out Float4 result);
            return result;
        }

        /// <summary>
        /// Performs a coordinate transformation using the given <see cref="Matrix" />.
        /// </summary>
        /// <param name="coordinate">The coordinate vector to transform.</param>
        /// <param name="transform">The transformation <see cref="Matrix" />.</param>
        /// <param name="result">When the method completes, contains the transformed coordinates.</param>
        /// <remarks>
        /// A coordinate transform performs the transformation with the assumption that the w component
        /// is one. The four dimensional vector obtained from the transformation operation has each
        /// component in the vector divided by the w component. This forces the w component to be one and
        /// therefore makes the vector homogeneous. The homogeneous vector is often preferred when working
        /// with coordinates as the w component can safely be ignored.
        /// </remarks>
        public static void TransformCoordinate(ref Float2 coordinate, ref Matrix transform, out Float2 result)
        {
            var vector = new Float4
            {
                X = coordinate.X * transform.M11 + coordinate.Y * transform.M21 + transform.M41,
                Y = coordinate.X * transform.M12 + coordinate.Y * transform.M22 + transform.M42,
                Z = coordinate.X * transform.M13 + coordinate.Y * transform.M23 + transform.M43,
                W = 1f / (coordinate.X * transform.M14 + coordinate.Y * transform.M24 + transform.M44)
            };
            result = new Float2(vector.X * vector.W, vector.Y * vector.W);
        }

        /// <summary>
        /// Performs a coordinate transformation using the given <see cref="Matrix" />.
        /// </summary>
        /// <param name="coordinate">The coordinate vector to transform.</param>
        /// <param name="transform">The transformation <see cref="Matrix" />.</param>
        /// <returns>The transformed coordinates.</returns>
        /// <remarks>
        /// A coordinate transform performs the transformation with the assumption that the w component
        /// is one. The four dimensional vector obtained from the transformation operation has each
        /// component in the vector divided by the w component. This forces the w component to be one and
        /// therefore makes the vector homogeneous. The homogeneous vector is often preferred when working
        /// with coordinates as the w component can safely be ignored.
        /// </remarks>
        public static Float2 TransformCoordinate(Float2 coordinate, Matrix transform)
        {
            TransformCoordinate(ref coordinate, ref transform, out Float2 result);
            return result;
        }

        /// <summary>
        /// Performs a normal transformation using the given <see cref="Matrix" />.
        /// </summary>
        /// <param name="normal">The normal vector to transform.</param>
        /// <param name="transform">The transformation <see cref="Matrix" />.</param>
        /// <param name="result">When the method completes, contains the transformed normal.</param>
        /// <remarks>
        /// A normal transform performs the transformation with the assumption that the w component
        /// is zero. This causes the fourth row and fourth column of the matrix to be unused. The
        /// end result is a vector that is not translated, but all other transformation properties
        /// apply. This is often preferred for normal vectors as normals purely represent direction
        /// rather than location because normal vectors should not be translated.
        /// </remarks>
        public static void TransformNormal(ref Float2 normal, ref Matrix transform, out Float2 result)
        {
            result = new Float2(normal.X * transform.M11 + normal.Y * transform.M21,
                                normal.X * transform.M12 + normal.Y * transform.M22);
        }

        /// <summary>
        /// Performs a normal transformation using the given <see cref="Matrix" />.
        /// </summary>
        /// <param name="normal">The normal vector to transform.</param>
        /// <param name="transform">The transformation <see cref="Matrix" />.</param>
        /// <returns>The transformed normal.</returns>
        /// <remarks>
        /// A normal transform performs the transformation with the assumption that the w component
        /// is zero. This causes the fourth row and fourth column of the matrix to be unused. The
        /// end result is a vector that is not translated, but all other transformation properties
        /// apply. This is often preferred for normal vectors as normals purely represent direction
        /// rather than location because normal vectors should not be translated.
        /// </remarks>
        public static Float2 TransformNormal(Float2 normal, Matrix transform)
        {
            TransformNormal(ref normal, ref transform, out Float2 result);
            return result;
        }

        /// <summary>
        /// Snaps the input position into the grid.
        /// </summary>
        /// <param name="pos">The position to snap.</param>
        /// <param name="gridSize">The size of the grid.</param>
        /// <returns>The position snapped to the grid.</returns>
        public static Float2 SnapToGrid(Float2 pos, Float2 gridSize)
        {
            if (Mathf.Abs(gridSize.X) > Mathf.Epsilon)
                pos.X = Mathf.Ceil((pos.X - (gridSize.X * 0.5f)) / gridSize.X) * gridSize.X;
            if (Mathf.Abs(gridSize.Y) > Mathf.Epsilon)
                pos.Y = Mathf.Ceil((pos.Y - (gridSize.Y * 0.5f)) / gridSize.Y) * gridSize.Y;
            return pos;
        }

        /// <summary>
        /// Adds two vectors.
        /// </summary>
        /// <param name="left">The first vector to add.</param>
        /// <param name="right">The second vector to add.</param>
        /// <returns>The sum of the two vectors.</returns>
        public static Float2 operator +(Float2 left, Float2 right)
        {
            return new Float2(left.X + right.X, left.Y + right.Y);
        }

        /// <summary>
        /// Multiplies a vector with another by performing component-wise multiplication equivalent to <see cref="Multiply(ref Float2,ref Float2,out Float2)" />.
        /// </summary>
        /// <param name="left">The first vector to multiply.</param>
        /// <param name="right">The second vector to multiply.</param>
        /// <returns>The multiplication of the two vectors.</returns>
        public static Float2 operator *(Float2 left, Float2 right)
        {
            return new Float2(left.X * right.X, left.Y * right.Y);
        }

        /// <summary>
        /// Assert a vector (return it unchanged).
        /// </summary>
        /// <param name="value">The vector to assert (unchanged).</param>
        /// <returns>The asserted (unchanged) vector.</returns>
        public static Float2 operator +(Float2 value)
        {
            return value;
        }

        /// <summary>
        /// Subtracts two vectors.
        /// </summary>
        /// <param name="left">The first vector to subtract.</param>
        /// <param name="right">The second vector to subtract.</param>
        /// <returns>The difference of the two vectors.</returns>
        public static Float2 operator -(Float2 left, Float2 right)
        {
            return new Float2(left.X - right.X, left.Y - right.Y);
        }

        /// <summary>
        /// Reverses the direction of a given vector.
        /// </summary>
        /// <param name="value">The vector to negate.</param>
        /// <returns>A vector facing in the opposite direction.</returns>
        public static Float2 operator -(Float2 value)
        {
            return new Float2(-value.X, -value.Y);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float2 operator *(float scale, Float2 value)
        {
            return new Float2(value.X * scale, value.Y * scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float2 operator *(Float2 value, float scale)
        {
            return new Float2(value.X * scale, value.Y * scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float2 operator /(Float2 value, float scale)
        {
            return new Float2(value.X / scale, value.Y / scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="value">The vector to scale.</param>
        /// <returns>The scaled vector.</returns>
        public static Float2 operator /(float scale, Float2 value)
        {
            return new Float2(scale / value.X, scale / value.Y);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float2 operator *(double scale, Float2 value)
        {
            var s = (float)scale;
            return new Float2(value.X * s, value.Y * s);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float2 operator *(Float2 value, double scale)
        {
            var s = (float)scale;
            return new Float2(value.X * s, value.Y * s);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float2 operator /(Float2 value, double scale)
        {
            var s = (float)scale;
            return new Float2(value.X / s, value.Y / s);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="value">The vector to scale.</param>
        /// <returns>The scaled vector.</returns>
        public static Float2 operator /(double scale, Float2 value)
        {
            var s = (float)scale;
            return new Float2(s / value.X, s / value.Y);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float2 operator /(Float2 value, Float2 scale)
        {
            return new Float2(value.X / scale.X, value.Y / scale.Y);
        }

        /// <summary>
        /// Remainder of value divided by scale.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The remained vector.</returns>
        public static Float2 operator %(Float2 value, float scale)
        {
            return new Float2(value.X % scale, value.Y % scale);
        }

        /// <summary>
        /// Remainder of value divided by scale.
        /// </summary>
        /// <param name="value">The amount by which to scale the vector.</param>
        /// <param name="scale">The vector to scale.</param>
        /// <returns>The remained vector.</returns>
        public static Float2 operator %(float value, Float2 scale)
        {
            return new Float2(value % scale.X, value % scale.Y);
        }

        /// <summary>
        /// Remainder of value divided by scale.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The remained vector.</returns>
        public static Float2 operator %(Float2 value, Float2 scale)
        {
            return new Float2(value.X % scale.X, value.Y % scale.Y);
        }

        /// <summary>
        /// Performs a component-wise addition.
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be added on elements</param>
        /// <returns>The vector with added scalar for each element.</returns>
        public static Float2 operator +(Float2 value, float scalar)
        {
            return new Float2(value.X + scalar, value.Y + scalar);
        }

        /// <summary>
        /// Performs a component-wise addition.
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be added on elements</param>
        /// <returns>The vector with added scalar for each element.</returns>
        public static Float2 operator +(float scalar, Float2 value)
        {
            return new Float2(scalar + value.X, scalar + value.Y);
        }

        /// <summary>
        /// Performs a component-wise subtraction.
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be subtracted from elements</param>
        /// <returns>The vector with subtracted scalar from each element.</returns>
        public static Float2 operator -(Float2 value, float scalar)
        {
            return new Float2(value.X - scalar, value.Y - scalar);
        }

        /// <summary>
        /// Performs a component-wise subtraction.
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be subtracted from elements</param>
        /// <returns>The vector with subtracted scalar from each element.</returns>
        public static Float2 operator -(float scalar, Float2 value)
        {
            return new Float2(scalar - value.X, scalar - value.Y);
        }

        /// <summary>
        /// Tests for equality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left" /> has the same value as <paramref name="right" />; otherwise,<c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator ==(Float2 left, Float2 right)
        {
            return Mathf.NearEqual(left.X, right.X) && Mathf.NearEqual(left.Y, right.Y);
        }

        /// <summary>
        /// Tests for inequality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left" /> has a different value than <paramref name="right" />; otherwise,<c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator !=(Float2 left, Float2 right)
        {
            return !Mathf.NearEqual(left.X, right.X) || !Mathf.NearEqual(left.Y, right.Y);
        }

        /// <summary>
        /// Performs an implicit conversion from <see cref="Float2" /> to <see cref="Vector2" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static implicit operator Vector2(Float2 value)
        {
            return new Vector2(value.X, value.Y);
        }

        /// <summary>
        /// Performs an implicit conversion from <see cref="Float2" /> to <see cref="Double2" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static implicit operator Double2(Float2 value)
        {
            return new Double2(value.X, value.Y);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Float2" /> to <see cref="Float3" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Float3(Float2 value)
        {
            return new Float3(value, 0.0f);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Float2" /> to <see cref="Float4" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Float4(Float2 value)
        {
            return new Float4(value, 0.0f, 0.0f);
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public override string ToString()
        {
            return string.Format(CultureInfo.CurrentCulture, "X:{0} Y:{1}", X, Y);
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
            return string.Format(CultureInfo.CurrentCulture, _formatString, X.ToString(format, CultureInfo.CurrentCulture), Y.ToString(format, CultureInfo.CurrentCulture));
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <param name="formatProvider">The format provider.</param>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public string ToString(IFormatProvider formatProvider)
        {
            return string.Format(formatProvider, _formatString, X, Y);
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
            return string.Format(formatProvider, _formatString, X.ToString(format, formatProvider), Y.ToString(format, formatProvider));
        }

        /// <summary>
        /// Returns a hash code for this instance.
        /// </summary>
        public override int GetHashCode()
        {
            unchecked
            {
                return (X.GetHashCode() * 397) ^ Y.GetHashCode();
            }
        }

        /// <summary>
        /// Determines whether the specified <see cref="Float2" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="Float2" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="Float2" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(ref Float2 other)
        {
            return Mathf.NearEqual(other.X, X) && Mathf.NearEqual(other.Y, Y);
        }

        /// <summary>
        /// Determines whether the specified <see cref="Float2"/> are equal.
        /// </summary>
        public static bool Equals(ref Float2 a, ref Float2 b)
        {
            return Mathf.NearEqual(a.X, b.X) && Mathf.NearEqual(a.Y, b.Y);
        }

        /// <summary>
        /// Determines whether the specified <see cref="Float2" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="Float2" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="Float2" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(Float2 other)
        {
            return Mathf.NearEqual(other.X, X) && Mathf.NearEqual(other.Y, Y);
        }

        /// <summary>
        /// Determines whether the specified <see cref="System.Object" /> is equal to this instance.
        /// </summary>
        /// <param name="value">The <see cref="System.Object" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="System.Object" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        public override bool Equals(object value)
        {
            return value is Float2 other && Mathf.NearEqual(other.X, X) && Mathf.NearEqual(other.Y, Y);
        }
    }
}
