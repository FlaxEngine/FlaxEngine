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
using System.Runtime.InteropServices;

namespace FlaxEngine
{
    [Serializable]
#if FLAX_EDITOR
    [System.ComponentModel.TypeConverter(typeof(TypeConverters.Double3Converter))]
#endif
    partial struct Double3 : IEquatable<Double3>, IFormattable
    {
        private static readonly string _formatString = "X:{0:F2} Y:{1:F2} Z:{2:F2}";

        /// <summary>
        /// The size of the <see cref="Double3" /> type, in bytes.
        /// </summary>
        public static readonly int SizeInBytes = Marshal.SizeOf(typeof(Double3));

        /// <summary>
        /// A <see cref="Double3" /> with all of its components set to zero.
        /// </summary>
        public static readonly Double3 Zero;

        /// <summary>
        /// The X unit <see cref="Double3" /> (1, 0, 0).
        /// </summary>
        public static readonly Double3 UnitX = new Double3(1.0, 0.0, 0.0);

        /// <summary>
        /// The Y unit <see cref="Double3" /> (0, 1, 0).
        /// </summary>
        public static readonly Double3 UnitY = new Double3(0.0, 1.0, 0.0);

        /// <summary>
        /// The Z unit <see cref="Double3" /> (0, 0, 1).
        /// </summary>
        public static readonly Double3 UnitZ = new Double3(0.0, 0.0, 1.0);

        /// <summary>
        /// A <see cref="Double3" /> with all of its components set to one.
        /// </summary>
        public static readonly Double3 One = new Double3(1.0, 1.0, 1.0);

        /// <summary>
        /// A <see cref="Double3" /> with all of its components set to half.
        /// </summary>
        public static readonly Double3 Half = new Double3(0.5f, 0.5f, 0.5f);

        /// <summary>
        /// A unit <see cref="Double3" /> designating up (0, 1, 0).
        /// </summary>
        public static readonly Double3 Up = new Double3(0.0, 1.0, 0.0);

        /// <summary>
        /// A unit <see cref="Double3" /> designating down (0, -1, 0).
        /// </summary>
        public static readonly Double3 Down = new Double3(0.0, -1.0, 0.0);

        /// <summary>
        /// A unit <see cref="Double3" /> designating left (-1, 0, 0).
        /// </summary>
        public static readonly Double3 Left = new Double3(-1.0, 0.0, 0.0);

        /// <summary>
        /// A unit <see cref="Double3" /> designating right (1, 0, 0).
        /// </summary>
        public static readonly Double3 Right = new Double3(1.0, 0.0, 0.0);

        /// <summary>
        /// A unit <see cref="Double3" /> designating forward in a left-handed coordinate system (0, 0, 1).
        /// </summary>
        public static readonly Double3 Forward = new Double3(0.0, 0.0, 1.0);

        /// <summary>
        /// A unit <see cref="Double3" /> designating backward in a left-handed coordinate system (0, 0, -1).
        /// </summary>
        public static readonly Double3 Backward = new Double3(0.0, 0.0, -1.0);

        /// <summary>
        /// A <see cref="Double3" /> with all components equal to <see cref="double.MinValue"/>.
        /// </summary>
        public static readonly Double3 Minimum = new Double3(double.MinValue);

        /// <summary>
        /// A <see cref="Double3" /> with all components equal to <see cref="double.MaxValue"/>.
        /// </summary>
        public static readonly Double3 Maximum = new Double3(double.MaxValue);

        /// <summary>
        /// Initializes a new instance of the <see cref="Double3" /> struct.
        /// </summary>
        /// <param name="value">The value that will be assigned to all components.</param>
        public Double3(double value)
        {
            X = value;
            Y = value;
            Z = value;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Double3" /> struct.
        /// </summary>
        /// <param name="x">Initial value for the X component of the vector.</param>
        /// <param name="y">Initial value for the Y component of the vector.</param>
        /// <param name="z">Initial value for the Z component of the vector.</param>
        public Double3(double x, double y, double z)
        {
            X = x;
            Y = y;
            Z = z;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Double3" /> struct.
        /// </summary>
        /// <param name="value">A vector containing the values with which to initialize the X and Y components.</param>
        /// <param name="z">Initial value for the Z component of the vector.</param>
        public Double3(Double2 value, double z)
        {
            X = value.X;
            Y = value.Y;
            Z = z;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Double3" /> struct.
        /// </summary>
        /// <param name="value">A vector containing the values with which to initialize the X, Y and Z components.</param>
        public Double3(Vector3 value)
        {
            X = value.X;
            Y = value.Y;
            Z = value.Z;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Double3" /> struct.
        /// </summary>
        /// <param name="value">A vector containing the values with which to initialize the X, Y and Z components.</param>
        public Double3(Float3 value)
        {
            X = value.X;
            Y = value.Y;
            Z = value.Z;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Double3" /> struct.
        /// </summary>
        /// <param name="value">A vector containing the values with which to initialize the X, Y and Z components.</param>
        public Double3(Double4 value)
        {
            X = value.X;
            Y = value.Y;
            Z = value.Z;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Double3" /> struct.
        /// </summary>
        /// <param name="values">The values to assign to the X, Y, and Z components of the vector. This must be an array with three elements.</param>
        /// <exception cref="ArgumentNullException">Thrown when <paramref name="values" /> is <c>null</c>.</exception>
        /// <exception cref="ArgumentOutOfRangeException"> Thrown when <paramref name="values" /> contains more or less than three elements.</exception>
        public Double3(double[] values)
        {
            if (values == null)
                throw new ArgumentNullException(nameof(values));
            if (values.Length != 3)
                throw new ArgumentOutOfRangeException(nameof(values), "There must be three and only three input values for Double3.");
            X = values[0];
            Y = values[1];
            Z = values[2];
        }

        /// <summary>
        /// Gets a value indicting whether this instance is normalized.
        /// </summary>
        public bool IsNormalized => Mathd.Abs((X * X + Y * Y + Z * Z) - 1.0f) < 1e-4f;

        /// <summary>
        /// Gets the normalized vector. Returned vector has length equal 1.
        /// </summary>
        public Double3 Normalized
        {
            get
            {
                Double3 result = this;
                result.Normalize();
                return result;
            }
        }

        /// <summary>
        /// Gets a value indicting whether this vector is zero
        /// </summary>
        public bool IsZero => Mathd.IsZero(X) && Mathd.IsZero(Y) && Mathd.IsZero(Z);

        /// <summary>
        /// Gets a value indicting whether this vector is one
        /// </summary>
        public bool IsOne => Mathd.IsOne(X) && Mathd.IsOne(Y) && Mathd.IsOne(Z);

        /// <summary>
        /// Gets a minimum component value
        /// </summary>
        public double MinValue => Mathd.Min(X, Mathd.Min(Y, Z));

        /// <summary>
        /// Gets a maximum component value
        /// </summary>
        public double MaxValue => Mathd.Max(X, Mathd.Max(Y, Z));

        /// <summary>
        /// Gets an arithmetic average value of all vector components.
        /// </summary>
        public double AvgValue => (X + Y + Z) * (1.0 / 3.0);

        /// <summary>
        /// Gets a sum of the component values.
        /// </summary>
        public double ValuesSum => X + Y + Z;

        /// <summary>
        /// Gets a vector with values being absolute values of that vector.
        /// </summary>
        public Double3 Absolute => new Double3(Math.Abs(X), Math.Abs(Y), Math.Abs(Z));

        /// <summary>
        /// Gets a vector with values being opposite to values of that vector.
        /// </summary>
        public Double3 Negative => new Double3(-X, -Y, -Z);

        /// <summary>
        /// Gets or sets the component at the specified index.
        /// </summary>
        /// <value>The value of the X, Y, or Z component, depending on the index.</value>
        /// <param name="index">The index of the component to access. Use 0 for the X component, 1 for the Y component, and 2 for the Z component.</param>
        /// <returns>The value of the component at the specified index.</returns>
        /// <exception cref="System.ArgumentOutOfRangeException">Thrown when the <paramref name="index" /> is out of the range [0, 2].</exception>
        public double this[int index]
        {
            get
            {
                switch (index)
                {
                case 0: return X;
                case 1: return Y;
                case 2: return Z;
                }
                throw new ArgumentOutOfRangeException(nameof(index), "Indices for Double3 run from 0 to 2, inclusive.");
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
                default: throw new ArgumentOutOfRangeException(nameof(index), "Indices for Double3 run from 0 to 2, inclusive.");
                }
            }
        }

        /// <summary>
        /// Calculates the length of the vector.
        /// </summary>
        /// <returns>The length of the vector.</returns>
        /// <remarks><see cref="Double3.LengthSquared" /> may be preferred when only the relative length is needed and speed is of the essence.</remarks>
        public double Length => Math.Sqrt(X * X + Y * Y + Z * Z);

        /// <summary>
        /// Calculates the squared length of the vector.
        /// </summary>
        /// <returns>The squared length of the vector.</returns>
        /// <remarks>This method may be preferred to <see cref="Double3.Length" /> when only a relative length is needed and speed is of the essence.</remarks>
        public double LengthSquared => X * X + Y * Y + Z * Z;

        /// <summary>
        /// Converts the vector into a unit vector.
        /// </summary>
        public void Normalize()
        {
            double length = Length;
            if (length >= Mathd.Epsilon)
            {
                double inv = 1.0 / length;
                X *= inv;
                Y *= inv;
                Z *= inv;
            }
        }

        /// <summary>
        /// When this vector contains Euler angles (degrees), ensure that angles are between +/-180
        /// </summary>
        public void UnwindEuler()
        {
            X = Mathd.UnwindDegrees(X);
            Y = Mathd.UnwindDegrees(Y);
            Z = Mathd.UnwindDegrees(Z);
        }

        /// <summary>
        /// Creates an array containing the elements of the vector.
        /// </summary>
        /// <returns>A three-element array containing the components of the vector.</returns>
        public double[] ToArray()
        {
            return new[] { X, Y, Z };
        }

        /// <summary>
        /// Adds two vectors.
        /// </summary>
        /// <param name="left">The first vector to add.</param>
        /// <param name="right">The second vector to add.</param>
        /// <param name="result">When the method completes, contains the sum of the two vectors.</param>
        public static void Add(ref Double3 left, ref Double3 right, out Double3 result)
        {
            result = new Double3(left.X + right.X, left.Y + right.Y, left.Z + right.Z);
        }

        /// <summary>
        /// Adds two vectors.
        /// </summary>
        /// <param name="left">The first vector to add.</param>
        /// <param name="right">The second vector to add.</param>
        /// <returns>The sum of the two vectors.</returns>
        public static Double3 Add(Double3 left, Double3 right)
        {
            return new Double3(left.X + right.X, left.Y + right.Y, left.Z + right.Z);
        }

        /// <summary>
        /// Performs a component-wise addition.
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be added to elements</param>
        /// <param name="result">The vector with added scalar for each element.</param>
        public static void Add(ref Double3 left, ref double right, out Double3 result)
        {
            result = new Double3(left.X + right, left.Y + right, left.Z + right);
        }

        /// <summary>
        /// Performs a component-wise addition.
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be added to elements</param>
        /// <returns>The vector with added scalar for each element.</returns>
        public static Double3 Add(Double3 left, double right)
        {
            return new Double3(left.X + right, left.Y + right, left.Z + right);
        }

        /// <summary>
        /// Subtracts two vectors.
        /// </summary>
        /// <param name="left">The first vector to subtract.</param>
        /// <param name="right">The second vector to subtract.</param>
        /// <param name="result">When the method completes, contains the difference of the two vectors.</param>
        public static void Subtract(ref Double3 left, ref Double3 right, out Double3 result)
        {
            result = new Double3(left.X - right.X, left.Y - right.Y, left.Z - right.Z);
        }

        /// <summary>
        /// Subtracts two vectors.
        /// </summary>
        /// <param name="left">The first vector to subtract.</param>
        /// <param name="right">The second vector to subtract.</param>
        /// <returns>The difference of the two vectors.</returns>
        public static Double3 Subtract(Double3 left, Double3 right)
        {
            return new Double3(left.X - right.X, left.Y - right.Y, left.Z - right.Z);
        }

        /// <summary>
        /// Performs a component-wise subtraction.
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be subtracted from elements</param>
        /// <param name="result">The vector with subtracted scalar for each element.</param>
        public static void Subtract(ref Double3 left, ref double right, out Double3 result)
        {
            result = new Double3(left.X - right, left.Y - right, left.Z - right);
        }

        /// <summary>
        /// Performs a component-wise subtraction.
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be subtracted from elements</param>
        /// <returns>The vector with subtracted scalar for each element.</returns>
        public static Double3 Subtract(Double3 left, double right)
        {
            return new Double3(left.X - right, left.Y - right, left.Z - right);
        }

        /// <summary>
        /// Performs a component-wise subtraction.
        /// </summary>
        /// <param name="left">The scalar value to be subtracted from elements</param>
        /// <param name="right">The input vector.</param>
        /// <param name="result">The vector with subtracted scalar for each element.</param>
        public static void Subtract(ref double left, ref Double3 right, out Double3 result)
        {
            result = new Double3(left - right.X, left - right.Y, left - right.Z);
        }

        /// <summary>
        /// Performs a component-wise subtraction.
        /// </summary>
        /// <param name="left">The scalar value to be subtracted from elements</param>
        /// <param name="right">The input vector.</param>
        /// <returns>The vector with subtracted scalar for each element.</returns>
        public static Double3 Subtract(double left, Double3 right)
        {
            return new Double3(left - right.X, left - right.Y, left - right.Z);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="result">When the method completes, contains the scaled vector.</param>
        public static void Multiply(ref Double3 value, double scale, out Double3 result)
        {
            result = new Double3(value.X * scale, value.Y * scale, value.Z * scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Double3 Multiply(Double3 value, double scale)
        {
            return new Double3(value.X * scale, value.Y * scale, value.Z * scale);
        }

        /// <summary>
        /// Multiply a vector with another by performing component-wise multiplication.
        /// </summary>
        /// <param name="left">The first vector to multiply.</param>
        /// <param name="right">The second vector to multiply.</param>
        /// <param name="result">When the method completes, contains the multiplied vector.</param>
        public static void Multiply(ref Double3 left, ref Double3 right, out Double3 result)
        {
            result = new Double3(left.X * right.X, left.Y * right.Y, left.Z * right.Z);
        }

        /// <summary>
        /// Multiply a vector with another by performing component-wise multiplication.
        /// </summary>
        /// <param name="left">The first vector to Multiply.</param>
        /// <param name="right">The second vector to multiply.</param>
        /// <returns>The multiplied vector.</returns>
        public static Double3 Multiply(Double3 left, Double3 right)
        {
            return new Double3(left.X * right.X, left.Y * right.Y, left.Z * right.Z);
        }

        /// <summary>
        /// Divides a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector (per component).</param>
        /// <param name="result">When the method completes, contains the divided vector.</param>
        public static void Divide(ref Double3 value, ref Double3 scale, out Double3 result)
        {
            result = new Double3(value.X / scale.X, value.Y / scale.Y, value.Z / scale.Z);
        }

        /// <summary>
        /// Divides a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector (per component).</param>
        /// <returns>The divided vector.</returns>
        public static Double3 Divide(Double3 value, Double3 scale)
        {
            return new Double3(value.X / scale.X, value.Y / scale.Y, value.Z / scale.Z);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="result">When the method completes, contains the scaled vector.</param>
        public static void Divide(ref Double3 value, double scale, out Double3 result)
        {
            result = new Double3(value.X / scale, value.Y / scale, value.Z / scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Double3 Divide(Double3 value, double scale)
        {
            return new Double3(value.X / scale, value.Y / scale, value.Z / scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="value">The vector to scale.</param>
        /// <param name="result">When the method completes, contains the scaled vector.</param>
        public static void Divide(double scale, ref Double3 value, out Double3 result)
        {
            result = new Double3(scale / value.X, scale / value.Y, scale / value.Z);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Double3 Divide(double scale, Double3 value)
        {
            return new Double3(scale / value.X, scale / value.Y, scale / value.Z);
        }

        /// <summary>
        /// Reverses the direction of a given vector.
        /// </summary>
        /// <param name="value">The vector to negate.</param>
        /// <param name="result">When the method completes, contains a vector facing in the opposite direction.</param>
        public static void Negate(ref Double3 value, out Double3 result)
        {
            result = new Double3(-value.X, -value.Y, -value.Z);
        }

        /// <summary>
        /// Reverses the direction of a given vector.
        /// </summary>
        /// <param name="value">The vector to negate.</param>
        /// <returns>A vector facing in the opposite direction.</returns>
        public static Double3 Negate(Double3 value)
        {
            return new Double3(-value.X, -value.Y, -value.Z);
        }

        /// <summary>
        /// Returns a <see cref="Double3" /> containing the 3D Cartesian coordinates of a point specified in Barycentric coordinates relative to a 3D triangle.
        /// </summary>
        /// <param name="value1">A <see cref="Double3" /> containing the 3D Cartesian coordinates of vertex 1 of the triangle.</param>
        /// <param name="value2">A <see cref="Double3" /> containing the 3D Cartesian coordinates of vertex 2 of the triangle.</param>
        /// <param name="value3">A <see cref="Double3" /> containing the 3D Cartesian coordinates of vertex 3 of the triangle.</param>
        /// <param name="amount1">Barycentric coordinate b2, which expresses the weighting factor toward vertex 2 (specified in <paramref name="value2" />).</param>
        /// <param name="amount2">Barycentric coordinate b3, which expresses the weighting factor toward vertex 3 (specified in <paramref name="value3" />).</param>
        /// <param name="result">When the method completes, contains the 3D Cartesian coordinates of the specified point.</param>
        public static void Barycentric(ref Double3 value1, ref Double3 value2, ref Double3 value3, double amount1, double amount2, out Double3 result)
        {
            result = new Double3(value1.X + amount1 * (value2.X - value1.X) + amount2 * (value3.X - value1.X),
                                 value1.Y + amount1 * (value2.Y - value1.Y) + amount2 * (value3.Y - value1.Y),
                                 value1.Z + amount1 * (value2.Z - value1.Z) + amount2 * (value3.Z - value1.Z));
        }

        /// <summary>
        /// Returns a <see cref="Double3" /> containing the 3D Cartesian coordinates of a point specified in Barycentric coordinates relative to a 3D triangle.
        /// </summary>
        /// <param name="value1">A <see cref="Double3" /> containing the 3D Cartesian coordinates of vertex 1 of the triangle.</param>
        /// <param name="value2">A <see cref="Double3" /> containing the 3D Cartesian coordinates of vertex 2 of the triangle.</param>
        /// <param name="value3">A <see cref="Double3" /> containing the 3D Cartesian coordinates of vertex 3 of the triangle.</param>
        /// <param name="amount1">Barycentric coordinate b2, which expresses the weighting factor toward vertex 2 (specified in <paramref name="value2" />).</param>
        /// <param name="amount2">Barycentric coordinate b3, which expresses the weighting factor toward vertex 3 (specified in <paramref name="value3" />).</param>
        /// <returns>A new <see cref="Double3" /> containing the 3D Cartesian coordinates of the specified point.</returns>
        public static Double3 Barycentric(Double3 value1, Double3 value2, Double3 value3, double amount1, double amount2)
        {
            Barycentric(ref value1, ref value2, ref value3, amount1, amount2, out var result);
            return result;
        }

        /// <summary>
        /// Restricts a value to be within a specified range.
        /// </summary>
        /// <param name="value">The value to clamp.</param>
        /// <param name="min">The minimum value.</param>
        /// <param name="max">The maximum value.</param>
        /// <param name="result">When the method completes, contains the clamped value.</param>
        public static void Clamp(ref Double3 value, ref Double3 min, ref Double3 max, out Double3 result)
        {
            double x = value.X;
            x = x > max.X ? max.X : x;
            x = x < min.X ? min.X : x;
            double y = value.Y;
            y = y > max.Y ? max.Y : y;
            y = y < min.Y ? min.Y : y;
            double z = value.Z;
            z = z > max.Z ? max.Z : z;
            z = z < min.Z ? min.Z : z;
            result = new Double3(x, y, z);
        }

        /// <summary>
        /// Restricts a value to be within a specified range.
        /// </summary>
        /// <param name="value">The value to clamp.</param>
        /// <param name="min">The minimum value.</param>
        /// <param name="max">The maximum value.</param>
        /// <returns>The clamped value.</returns>
        public static Double3 Clamp(Double3 value, Double3 min, Double3 max)
        {
            Clamp(ref value, ref min, ref max, out var result);
            return result;
        }

        /// <summary>
        /// Calculates the cross product of two vectors.
        /// </summary>
        /// <param name="left">First source vector.</param>
        /// <param name="right">Second source vector.</param>
        /// <param name="result">When the method completes, contains he cross product of the two vectors.</param>
        public static void Cross(ref Double3 left, ref Double3 right, out Double3 result)
        {
            result = new Double3(left.Y * right.Z - left.Z * right.Y,
                                 left.Z * right.X - left.X * right.Z,
                                 left.X * right.Y - left.Y * right.X);
        }

        /// <summary>
        /// Calculates the cross product of two vectors.
        /// </summary>
        /// <param name="left">First source vector.</param>
        /// <param name="right">Second source vector.</param>
        /// <returns>The cross product of the two vectors.</returns>
        public static Double3 Cross(Double3 left, Double3 right)
        {
            Cross(ref left, ref right, out var result);
            return result;
        }

        /// <summary>
        /// Calculates the distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <param name="result">When the method completes, contains the distance between the two vectors.</param>
        /// <remarks><see cref="Double3.DistanceSquared(ref Double3, ref Double3, out double)" /> may be preferred when only the relative distance is needed and speed is of the essence.</remarks>
        public static void Distance(ref Double3 value1, ref Double3 value2, out double result)
        {
            double x = value1.X - value2.X;
            double y = value1.Y - value2.Y;
            double z = value1.Z - value2.Z;
            result = Math.Sqrt(x * x + y * y + z * z);
        }

        /// <summary>
        /// Calculates the distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <returns>The distance between the two vectors.</returns>
        /// <remarks><see cref="Double3.DistanceSquared(ref Double3, ref Double3, out double)" /> may be preferred when only the relative distance is needed and speed is of the essence.</remarks>
        public static double Distance(ref Double3 value1, ref Double3 value2)
        {
            double x = value1.X - value2.X;
            double y = value1.Y - value2.Y;
            double z = value1.Z - value2.Z;
            return Math.Sqrt(x * x + y * y + z * z);
        }

        /// <summary>
        /// Calculates the distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <returns>The distance between the two vectors.</returns>
        /// <remarks><see cref="Double3.DistanceSquared(Double3, Double3)" /> may be preferred when only the relative distance is needed and speed is of the essence.</remarks>
        public static double Distance(Double3 value1, Double3 value2)
        {
            double x = value1.X - value2.X;
            double y = value1.Y - value2.Y;
            double z = value1.Z - value2.Z;
            return Math.Sqrt(x * x + y * y + z * z);
        }

        /// <summary>
        /// Calculates the squared distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <param name="result">When the method completes, contains the squared distance between the two vectors.</param>
        public static void DistanceSquared(ref Double3 value1, ref Double3 value2, out double result)
        {
            double x = value1.X - value2.X;
            double y = value1.Y - value2.Y;
            double z = value1.Z - value2.Z;
            result = x * x + y * y + z * z;
        }

        /// <summary>
        /// Calculates the squared distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <returns>The squared distance between the two vectors.</returns>
        public static double DistanceSquared(ref Double3 value1, ref Double3 value2)
        {
            double x = value1.X - value2.X;
            double y = value1.Y - value2.Y;
            double z = value1.Z - value2.Z;
            return x * x + y * y + z * z;
        }

        /// <summary>
        /// Calculates the squared distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <returns>The squared distance between the two vectors.</returns>
        public static double DistanceSquared(Double3 value1, Double3 value2)
        {
            double x = value1.X - value2.X;
            double y = value1.Y - value2.Y;
            double z = value1.Z - value2.Z;
            return x * x + y * y + z * z;
        }

        /// <summary>
        /// Calculates the distance between two vectors on the XY plane (ignoring Z).
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <param name="result">When the method completes, contains the distance between the two vectors in the XY plane.</param>
        public static void DistanceXY(ref Double3 value1, ref Double3 value2, out double result)
        {
            double x = value1.X - value2.X;
            double y = value1.Y - value2.Y;
            result = Math.Sqrt(x * x + y * y);
        }

        /// <summary>
        /// Calculates the squared distance between two vectors on the XY plane (ignoring Z).
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector</param>
        /// <param name="result">When the method completes, contains the squared distance between the two vectors in the XY plane.</param>
        public static void DistanceXYSquared(ref Double3 value1, ref Double3 value2, out double result)
        {
            double x = value1.X - value2.X;
            double y = value1.Y - value2.Y;
            result = x * x + y * y;
        }

        /// <summary>
        /// Calculates the distance between two vectors on the XZ plane (ignoring Y).
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <param name="result">When the method completes, contains the distance between the two vectors in the XY plane.</param>
        public static void DistanceXZ(ref Double3 value1, ref Double3 value2, out double result)
        {
            double x = value1.X - value2.X;
            double z = value1.Z - value2.Z;
            result = Math.Sqrt(x * x + z * z);
        }

        /// <summary>
        /// Calculates the squared distance between two vectors on the XZ plane (ignoring Y).
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector</param>
        /// <param name="result">When the method completes, contains the squared distance between the two vectors in the XY plane.</param>
        public static void DistanceXZSquared(ref Double3 value1, ref Double3 value2, out double result)
        {
            double x = value1.X - value2.X;
            double z = value1.Z - value2.Z;
            result = x * x + z * z;
        }

        /// <summary>
        /// Calculates the distance between two vectors on the YZ plane (ignoring X).
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <param name="result">When the method completes, contains the distance between the two vectors in the YZ plane.</param>
        public static void DistanceYZ(ref Double3 value1, ref Double3 value2, out double result)
        {
            double y = value1.Y - value2.Y;
            double z = value1.Z - value2.Z;
            result = Math.Sqrt(y * y + z * z);
        }

        /// <summary>
        /// Calculates the squared distance between two vectors on the YZ plane (ignoring X).
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector</param>
        /// <param name="result">When the method completes, contains the squared distance between the two vectors in the YZ plane.</param>
        public static void DistanceYZSquared(ref Double3 value1, ref Double3 value2, out double result)
        {
            double y = value1.Y - value2.Y;
            double z = value1.Z - value2.Z;
            result = y * y + z * z;
        }

        /// <summary>
        /// Tests whether one vector is near another vector.
        /// </summary>
        /// <param name="left">The left vector.</param>
        /// <param name="right">The right vector.</param>
        /// <param name="epsilon">The epsilon.</param>
        /// <returns><c>true</c> if left and right are near another, <c>false</c> otherwise</returns>
        public static bool NearEqual(Double3 left, Double3 right, double epsilon = Mathd.Epsilon)
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
        public static bool NearEqual(ref Double3 left, ref Double3 right, double epsilon = Mathd.Epsilon)
        {
            return Mathd.WithinEpsilon(left.X, right.X, epsilon) && Mathd.WithinEpsilon(left.Y, right.Y, epsilon) && Mathd.WithinEpsilon(left.Z, right.Z, epsilon);
        }

        /// <summary>
        /// Calculates the dot product of two vectors.
        /// </summary>
        /// <param name="left">First source vector.</param>
        /// <param name="right">Second source vector.</param>
        /// <param name="result">When the method completes, contains the dot product of the two vectors.</param>
        public static void Dot(ref Double3 left, ref Double3 right, out double result)
        {
            result = left.X * right.X + left.Y * right.Y + left.Z * right.Z;
        }

        /// <summary>
        /// Calculates the dot product of two vectors.
        /// </summary>
        /// <param name="left">First source vector.</param>
        /// <param name="right">Second source vector.</param>
        /// <returns>The dot product of the two vectors.</returns>
        public static double Dot(ref Double3 left, ref Double3 right)
        {
            return left.X * right.X + left.Y * right.Y + left.Z * right.Z;
        }

        /// <summary>
        /// Calculates the dot product of two vectors.
        /// </summary>
        /// <param name="left">First source vector.</param>
        /// <param name="right">Second source vector.</param>
        /// <returns>The dot product of the two vectors.</returns>
        public static double Dot(Double3 left, Double3 right)
        {
            return left.X * right.X + left.Y * right.Y + left.Z * right.Z;
        }

        /// <summary>
        /// Converts the vector into a unit vector.
        /// </summary>
        /// <param name="value">The vector to normalize.</param>
        /// <param name="result">When the method completes, contains the normalized vector.</param>
        public static void Normalize(ref Double3 value, out Double3 result)
        {
            result = value;
            result.Normalize();
        }

        /// <summary>
        /// Converts the vector into a unit vector.
        /// </summary>
        /// <param name="value">The vector to normalize.</param>
        /// <returns>The normalized vector.</returns>
        public static Double3 Normalize(Double3 value)
        {
            value.Normalize();
            return value;
        }

        /// <summary>
        /// Makes sure that Length of the output vector is always below max and above 0.
        /// </summary>
        /// <param name="vector">Input Vector.</param>
        /// <param name="max">Max Length</param>
        public static Double3 ClampLength(Double3 vector, double max)
        {
            return ClampLength(vector, 0, max);
        }

        /// <summary>
        /// Makes sure that Length of the output vector is always below max and above min.
        /// </summary>
        /// <param name="vector">Input Vector.</param>
        /// <param name="min">Min Length</param>
        /// <param name="max">Max Length</param>
        public static Double3 ClampLength(Double3 vector, double min, double max)
        {
            ClampLength(vector, min, max, out Double3 result);
            return result;
        }

        /// <summary>
        /// Makes sure that Length of the output vector is always below max and above min.
        /// </summary>
        /// <param name="vector">Input Vector.</param>
        /// <param name="min">Min Length</param>
        /// <param name="max">Max Length</param>
        /// <param name="result">The result vector.</param>
        public static void ClampLength(Double3 vector, double min, double max, out Double3 result)
        {
            result.X = vector.X;
            result.Y = vector.Y;
            result.Z = vector.Z;
            double lenSq = result.LengthSquared;
            if (lenSq > max * max)
            {
                double scaleFactor = max / Math.Sqrt(lenSq);
                result.X *= scaleFactor;
                result.Y *= scaleFactor;
                result.Z *= scaleFactor;
            }
            if (lenSq < min * min)
            {
                double scaleFactor = min / Math.Sqrt(lenSq);
                result.X *= scaleFactor;
                result.Y *= scaleFactor;
                result.Z *= scaleFactor;
            }
        }

        /// <summary>
        /// Performs a linear interpolation between two vectors.
        /// </summary>
        /// <param name="start">Start vector.</param>
        /// <param name="end">End vector.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end" />.</param>
        /// <param name="result">When the method completes, contains the linear interpolation of the two vectors.</param>
        /// <remarks>Passing <paramref name="amount" /> a value of 0 will cause <paramref name="start" /> to be returned; a value of 1 will cause <paramref name="end" /> to be returned.</remarks>
        public static void Lerp(ref Double3 start, ref Double3 end, double amount, out Double3 result)
        {
            result.X = Mathd.Lerp(start.X, end.X, amount);
            result.Y = Mathd.Lerp(start.Y, end.Y, amount);
            result.Z = Mathd.Lerp(start.Z, end.Z, amount);
        }

        /// <summary>
        /// Performs a linear interpolation between two vectors.
        /// </summary>
        /// <param name="start">Start vector.</param>
        /// <param name="end">End vector.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end" />.</param>
        /// <returns>The linear interpolation of the two vectors.</returns>
        /// <remarks>Passing <paramref name="amount" /> a value of 0 will cause <paramref name="start" /> to be returned; a value of 1 will cause <paramref name="end" /> to be returned.</remarks>
        public static Double3 Lerp(Double3 start, Double3 end, double amount)
        {
            Lerp(ref start, ref end, amount, out var result);
            return result;
        }

        /// <summary>
        /// Performs a cubic interpolation between two vectors.
        /// </summary>
        /// <param name="start">Start vector.</param>
        /// <param name="end">End vector.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end" />.</param>
        /// <param name="result">When the method completes, contains the cubic interpolation of the two vectors.</param>
        public static void SmoothStep(ref Double3 start, ref Double3 end, double amount, out Double3 result)
        {
            amount = Mathd.SmoothStep(amount);
            Lerp(ref start, ref end, amount, out result);
        }

        /// <summary>
        /// Performs a cubic interpolation between two vectors.
        /// </summary>
        /// <param name="start">Start vector.</param>
        /// <param name="end">End vector.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end" />.</param>
        /// <returns>The cubic interpolation of the two vectors.</returns>
        public static Double3 SmoothStep(Double3 start, Double3 end, double amount)
        {
            SmoothStep(ref start, ref end, amount, out var result);
            return result;
        }

        /// <summary>
        /// Moves a value current towards target.
        /// </summary>
        /// <param name="current">The position to move from.</param>
        /// <param name="target">The position to move towards.</param>
        /// <param name="maxDistanceDelta">The maximum distance that can be applied to the value.</param>
        /// <returns>The new position.</returns>
        public static Double3 MoveTowards(Double3 current, Double3 target, double maxDistanceDelta)
        {
            var to = target - current;
            var distanceSq = to.LengthSquared;
            if (distanceSq == 0 || (maxDistanceDelta >= 0 && distanceSq <= maxDistanceDelta * maxDistanceDelta))
                return target;
            var scale = maxDistanceDelta / Mathd.Sqrt(distanceSq);
            return new Double3(current.X + to.X * scale, current.Y + to.Y * scale, current.Z + to.Z * scale);
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
        public static void Hermite(ref Double3 value1, ref Double3 tangent1, ref Double3 value2, ref Double3 tangent2, double amount, out Double3 result)
        {
            double squared = amount * amount;
            double cubed = amount * squared;
            double part1 = 2.0 * cubed - 3.0 * squared + 1.0;
            double part2 = -2.0 * cubed + 3.0 * squared;
            double part3 = cubed - 2.0 * squared + amount;
            double part4 = cubed - squared;
            result.X = value1.X * part1 + value2.X * part2 + tangent1.X * part3 + tangent2.X * part4;
            result.Y = value1.Y * part1 + value2.Y * part2 + tangent1.Y * part3 + tangent2.Y * part4;
            result.Z = value1.Z * part1 + value2.Z * part2 + tangent1.Z * part3 + tangent2.Z * part4;
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
        public static Double3 Hermite(Double3 value1, Double3 tangent1, Double3 value2, Double3 tangent2, double amount)
        {
            Hermite(ref value1, ref tangent1, ref value2, ref tangent2, amount, out var result);
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
        public static void CatmullRom(ref Double3 value1, ref Double3 value2, ref Double3 value3, ref Double3 value4, double amount, out Double3 result)
        {
            double squared = amount * amount;
            double cubed = amount * squared;
            result.X = 0.5f * (2.0 * value2.X + (-value1.X + value3.X) * amount +
                               (2.0 * value1.X - 5.0 * value2.X + 4.0 * value3.X - value4.X) * squared +
                               (-value1.X + 3.0 * value2.X - 3.0 * value3.X + value4.X) * cubed);
            result.Y = 0.5f * (2.0 * value2.Y + (-value1.Y + value3.Y) * amount +
                               (2.0 * value1.Y - 5.0 * value2.Y + 4.0 * value3.Y - value4.Y) * squared +
                               (-value1.Y + 3.0 * value2.Y - 3.0 * value3.Y + value4.Y) * cubed);
            result.Z = 0.5f * (2.0 * value2.Z + (-value1.Z + value3.Z) * amount +
                               (2.0 * value1.Z - 5.0 * value2.Z + 4.0 * value3.Z - value4.Z) * squared +
                               (-value1.Z + 3.0 * value2.Z - 3.0 * value3.Z + value4.Z) * cubed);
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
        public static Double3 CatmullRom(Double3 value1, Double3 value2, Double3 value3, Double3 value4, double amount)
        {
            CatmullRom(ref value1, ref value2, ref value3, ref value4, amount, out var result);
            return result;
        }

        /// <summary>
        /// Returns a vector containing the largest components of the specified vectors.
        /// </summary>
        /// <param name="left">The first source vector.</param>
        /// <param name="right">The second source vector.</param>
        /// <param name="result">When the method completes, contains an new vector composed of the largest components of the source vectors.</param>
        public static void Max(ref Double3 left, ref Double3 right, out Double3 result)
        {
            result.X = left.X > right.X ? left.X : right.X;
            result.Y = left.Y > right.Y ? left.Y : right.Y;
            result.Z = left.Z > right.Z ? left.Z : right.Z;
        }

        /// <summary>
        /// Returns a vector containing the largest components of the specified vectors.
        /// </summary>
        /// <param name="left">The first source vector.</param>
        /// <param name="right">The second source vector.</param>
        /// <returns>A vector containing the largest components of the source vectors.</returns>
        public static Double3 Max(Double3 left, Double3 right)
        {
            Max(ref left, ref right, out var result);
            return result;
        }

        /// <summary>
        /// Returns a vector containing the smallest components of the specified vectors.
        /// </summary>
        /// <param name="left">The first source vector.</param>
        /// <param name="right">The second source vector.</param>
        /// <param name="result">When the method completes, contains an new vector composed of the smallest components of the source vectors.</param>
        public static void Min(ref Double3 left, ref Double3 right, out Double3 result)
        {
            result.X = left.X < right.X ? left.X : right.X;
            result.Y = left.Y < right.Y ? left.Y : right.Y;
            result.Z = left.Z < right.Z ? left.Z : right.Z;
        }

        /// <summary>
        /// Returns a vector containing the smallest components of the specified vectors.
        /// </summary>
        /// <param name="left">The first source vector.</param>
        /// <param name="right">The second source vector.</param>
        /// <returns>A vector containing the smallest components of the source vectors.</returns>
        public static Double3 Min(Double3 left, Double3 right)
        {
            Min(ref left, ref right, out var result);
            return result;
        }

        /// <summary>
        /// Returns the absolute value of a vector.
        /// </summary>
        /// <param name="v">The value.</param>
        /// <returns> A vector which components are less or equal to 0.</returns>
        public static Double3 Abs(Double3 v)
        {
            return new Double3(Math.Abs(v.X), Math.Abs(v.Y), Math.Abs(v.Z));
        }

        /// <summary>
        /// Projects a vector onto another vector.
        /// </summary>
        /// <param name="vector">The vector to project.</param>
        /// <param name="onNormal">The projection normal vector.</param>
        /// <returns>The projected vector.</returns>
        public static Double3 Project(Double3 vector, Double3 onNormal)
        {
            double sqrMag = Dot(onNormal, onNormal);
            if (sqrMag < Mathd.Epsilon)
                return Zero;
            return onNormal * Dot(vector, onNormal) / sqrMag;
        }

        /// <summary>
        /// Projects a vector onto a plane defined by a normal orthogonal to the plane.
        /// </summary>
        /// <param name="vector">The vector to project.</param>
        /// <param name="planeNormal">The plane normal vector.</param>
        /// <returns>The projected vector.</returns>
        public static Double3 ProjectOnPlane(Double3 vector, Double3 planeNormal)
        {
            return vector - Project(vector, planeNormal);
        }

        /// <summary>
        /// Calculates the angle (in degrees) between <paramref name="from"/> and <paramref name="to"/>. This is always the smallest value.
        /// </summary>
        /// <param name="from">The first vector.</param>
        /// <param name="to">The second vector.</param>
        /// <returns>The angle (in degrees).</returns>
        public static double Angle(Double3 from, Double3 to)
        {
            double dot = Mathd.Clamp(Dot(from.Normalized, to.Normalized), -1.0, 1.0);
            if (Math.Abs(dot) > (1 - Mathd.Epsilon))
                return dot > 0.0 ? 0.0 : 180.0;
            return Math.Acos(dot) * Mathd.RadiansToDegrees;
        }

        /// <summary>
        /// Projects a 3D vector from object space into screen space.
        /// </summary>
        /// <param name="vector">The vector to project.</param>
        /// <param name="x">The X position of the viewport.</param>
        /// <param name="y">The Y position of the viewport.</param>
        /// <param name="width">The width of the viewport.</param>
        /// <param name="height">The height of the viewport.</param>
        /// <param name="minZ">The minimum depth of the viewport.</param>
        /// <param name="maxZ">The maximum depth of the viewport.</param>
        /// <param name="worldViewProjection">The combined world-view-projection matrix.</param>
        /// <param name="result">When the method completes, contains the vector in screen space.</param>
        public static void Project(ref Double3 vector, double x, double y, double width, double height, double minZ, double maxZ, ref Matrix worldViewProjection, out Double3 result)
        {
            TransformCoordinate(ref vector, ref worldViewProjection, out var v);
            result = new Double3((1.0 + v.X) * 0.5f * width + x, (1.0 - v.Y) * 0.5f * height + y, v.Z * (maxZ - minZ) + minZ);
        }

        /// <summary>
        /// Projects a 3D vector from object space into screen space.
        /// </summary>
        /// <param name="vector">The vector to project.</param>
        /// <param name="x">The X position of the viewport.</param>
        /// <param name="y">The Y position of the viewport.</param>
        /// <param name="width">The width of the viewport.</param>
        /// <param name="height">The height of the viewport.</param>
        /// <param name="minZ">The minimum depth of the viewport.</param>
        /// <param name="maxZ">The maximum depth of the viewport.</param>
        /// <param name="worldViewProjection">The combined world-view-projection matrix.</param>
        /// <returns>The vector in screen space.</returns>
        public static Double3 Project(Double3 vector, double x, double y, double width, double height, double minZ, double maxZ, Matrix worldViewProjection)
        {
            Project(ref vector, x, y, width, height, minZ, maxZ, ref worldViewProjection, out var result);
            return result;
        }

        /// <summary>
        /// Projects a 3D vector from screen space into object space.
        /// </summary>
        /// <param name="vector">The vector to project.</param>
        /// <param name="x">The X position of the viewport.</param>
        /// <param name="y">The Y position of the viewport.</param>
        /// <param name="width">The width of the viewport.</param>
        /// <param name="height">The height of the viewport.</param>
        /// <param name="minZ">The minimum depth of the viewport.</param>
        /// <param name="maxZ">The maximum depth of the viewport.</param>
        /// <param name="worldViewProjection">The combined world-view-projection matrix.</param>
        /// <param name="result">When the method completes, contains the vector in object space.</param>
        public static void Unproject(ref Double3 vector, double x, double y, double width, double height, double minZ, double maxZ, ref Matrix worldViewProjection, out Double3 result)
        {
            Matrix.Invert(ref worldViewProjection, out var matrix);
            var v = new Double3
            {
                X = (vector.X - x) / width * 2.0 - 1.0,
                Y = -((vector.Y - y) / height * 2.0 - 1.0),
                Z = (vector.Z - minZ) / (maxZ - minZ)
            };
            TransformCoordinate(ref v, ref matrix, out result);
        }

        /// <summary>
        /// Projects a 3D vector from screen space into object space.
        /// </summary>
        /// <param name="vector">The vector to project.</param>
        /// <param name="x">The X position of the viewport.</param>
        /// <param name="y">The Y position of the viewport.</param>
        /// <param name="width">The width of the viewport.</param>
        /// <param name="height">The height of the viewport.</param>
        /// <param name="minZ">The minimum depth of the viewport.</param>
        /// <param name="maxZ">The maximum depth of the viewport.</param>
        /// <param name="worldViewProjection">The combined world-view-projection matrix.</param>
        /// <returns>The vector in object space.</returns>
        public static Double3 Unproject(Double3 vector, double x, double y, double width, double height, double minZ, double maxZ, Matrix worldViewProjection)
        {
            Unproject(ref vector, x, y, width, height, minZ, maxZ, ref worldViewProjection, out var result);
            return result;
        }

        /// <summary>
        /// Returns the reflection of a vector off a surface that has the specified normal.
        /// </summary>
        /// <param name="vector">The source vector.</param>
        /// <param name="normal">Normal of the surface.</param>
        /// <param name="result">When the method completes, contains the reflected vector.</param>
        /// <remarks>Reflect only gives the direction of a reflection off a surface, it does not determine whether the original vector was close enough to the surface to hit it.</remarks>
        public static void Reflect(ref Double3 vector, ref Double3 normal, out Double3 result)
        {
            double dot = vector.X * normal.X + vector.Y * normal.Y + vector.Z * normal.Z;
            result.X = vector.X - 2.0 * dot * normal.X;
            result.Y = vector.Y - 2.0 * dot * normal.Y;
            result.Z = vector.Z - 2.0 * dot * normal.Z;
        }

        /// <summary>
        /// Returns the reflection of a vector off a surface that has the specified normal.
        /// </summary>
        /// <param name="vector">The source vector.</param>
        /// <param name="normal">Normal of the surface.</param>
        /// <returns>The reflected vector.</returns>
        /// <remarks>Reflect only gives the direction of a reflection off a surface, it does not determine whether the original vector was close enough to the surface to hit it.</remarks>
        public static Double3 Reflect(Double3 vector, Double3 normal)
        {
            Reflect(ref vector, ref normal, out var result);
            return result;
        }

        /// <summary>
        /// Transforms a 3D vector by the given <see cref="Quaternion" /> rotation.
        /// </summary>
        /// <param name="vector">The vector to rotate.</param>
        /// <param name="rotation">The <see cref="Quaternion" /> rotation to apply.</param>
        /// <param name="result">When the method completes, contains the transformed <see cref="Double3" />.</param>
        public static void Transform(ref Double3 vector, ref Quaternion rotation, out Double3 result)
        {
            double x = rotation.X + rotation.X;
            double y = rotation.Y + rotation.Y;
            double z = rotation.Z + rotation.Z;
            double wx = rotation.W * x;
            double wy = rotation.W * y;
            double wz = rotation.W * z;
            double xx = rotation.X * x;
            double xy = rotation.X * y;
            double xz = rotation.X * z;
            double yy = rotation.Y * y;
            double yz = rotation.Y * z;
            double zz = rotation.Z * z;
            result = new Double3(vector.X * (1.0 - yy - zz) + vector.Y * (xy - wz) + vector.Z * (xz + wy),
                                 vector.X * (xy + wz) + vector.Y * (1.0 - xx - zz) + vector.Z * (yz - wx),
                                 vector.X * (xz - wy) + vector.Y * (yz + wx) + vector.Z * (1.0 - xx - yy));
        }

        /// <summary>
        /// Transforms a 3D vector by the given <see cref="Quaternion" /> rotation.
        /// </summary>
        /// <param name="vector">The vector to rotate.</param>
        /// <param name="rotation">The <see cref="Quaternion" /> rotation to apply.</param>
        /// <returns>The transformed <see cref="Double3" />.</returns>
        public static Double3 Transform(Double3 vector, Quaternion rotation)
        {
            Transform(ref vector, ref rotation, out var result);
            return result;
        }

        /// <summary>
        /// Transforms a 3D vector by the given <see cref="Matrix3x3"/>.
        /// </summary>
        /// <param name="vector">The source vector.</param>
        /// <param name="transform">The transformation <see cref="Matrix3x3"/>.</param>
        /// <param name="result">When the method completes, contains the transformed <see cref="Double3"/>.</param>
        public static void Transform(ref Double3 vector, ref Matrix3x3 transform, out Double3 result)
        {
            result = new Double3((vector.X * transform.M11) + (vector.Y * transform.M21) + (vector.Z * transform.M31),
                                 (vector.X * transform.M12) + (vector.Y * transform.M22) + (vector.Z * transform.M32),
                                 (vector.X * transform.M13) + (vector.Y * transform.M23) + (vector.Z * transform.M33));
        }

        /// <summary>
        /// Transforms a 3D vector by the given <see cref="Matrix3x3"/>.
        /// </summary>
        /// <param name="vector">The source vector.</param>
        /// <param name="transform">The transformation <see cref="Matrix3x3"/>.</param>
        /// <returns>The transformed <see cref="Double3"/>.</returns>
        public static Double3 Transform(Double3 vector, Matrix3x3 transform)
        {
            Transform(ref vector, ref transform, out var result);
            return result;
        }

        /// <summary>
        /// Transforms a 3D vector by the given <see cref="Matrix" />.
        /// </summary>
        /// <param name="vector">The source vector.</param>
        /// <param name="transform">The transformation <see cref="Matrix" />.</param>
        /// <param name="result">When the method completes, contains the transformed <see cref="Double3" />.</param>
        public static void Transform(ref Double3 vector, ref Matrix transform, out Double3 result)
        {
            result = new Double3(vector.X * transform.M11 + vector.Y * transform.M21 + vector.Z * transform.M31 + transform.M41,
                                 vector.X * transform.M12 + vector.Y * transform.M22 + vector.Z * transform.M32 + transform.M42,
                                 vector.X * transform.M13 + vector.Y * transform.M23 + vector.Z * transform.M33 + transform.M43);
        }

        /// <summary>
        /// Transforms a 3D vector by the given <see cref="Matrix" />.
        /// </summary>
        /// <param name="vector">The source vector.</param>
        /// <param name="transform">The transformation <see cref="Matrix" />.</param>
        /// <param name="result">When the method completes, contains the transformed <see cref="Double4" />.</param>
        public static void Transform(ref Double3 vector, ref Matrix transform, out Double4 result)
        {
            result = new Double4(vector.X * transform.M11 + vector.Y * transform.M21 + vector.Z * transform.M31 + transform.M41,
                                 vector.X * transform.M12 + vector.Y * transform.M22 + vector.Z * transform.M32 + transform.M42,
                                 vector.X * transform.M13 + vector.Y * transform.M23 + vector.Z * transform.M33 + transform.M43,
                                 vector.X * transform.M14 + vector.Y * transform.M24 + vector.Z * transform.M34 + transform.M44);
        }

        /// <summary>
        /// Transforms a 3D vector by the given <see cref="Matrix" />.
        /// </summary>
        /// <param name="vector">The source vector.</param>
        /// <param name="transform">The transformation <see cref="Matrix" />.</param>
        /// <returns>The transformed <see cref="Double4" />.</returns>
        public static Double3 Transform(Double3 vector, Matrix transform)
        {
            Transform(ref vector, ref transform, out Double3 result);
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
        public static void TransformCoordinate(ref Double3 coordinate, ref Matrix transform, out Double3 result)
        {
            var vector = new Double4
            {
                X = coordinate.X * transform.M11 + coordinate.Y * transform.M21 + coordinate.Z * transform.M31 + transform.M41,
                Y = coordinate.X * transform.M12 + coordinate.Y * transform.M22 + coordinate.Z * transform.M32 + transform.M42,
                Z = coordinate.X * transform.M13 + coordinate.Y * transform.M23 + coordinate.Z * transform.M33 + transform.M43,
                W = 1f / (coordinate.X * transform.M14 + coordinate.Y * transform.M24 + coordinate.Z * transform.M34 + transform.M44)
            };
            result = new Double3(vector.X * vector.W, vector.Y * vector.W, vector.Z * vector.W);
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
        public static Double3 TransformCoordinate(Double3 coordinate, Matrix transform)
        {
            TransformCoordinate(ref coordinate, ref transform, out var result);
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
        public static void TransformNormal(ref Double3 normal, ref Matrix transform, out Double3 result)
        {
            result = new Double3(normal.X * transform.M11 + normal.Y * transform.M21 + normal.Z * transform.M31,
                                 normal.X * transform.M12 + normal.Y * transform.M22 + normal.Z * transform.M32,
                                 normal.X * transform.M13 + normal.Y * transform.M23 + normal.Z * transform.M33);
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
        public static Double3 TransformNormal(Double3 normal, Matrix transform)
        {
            TransformNormal(ref normal, ref transform, out var result);
            return result;
        }

        /// <summary>
        /// Snaps the input position into the grid.
        /// </summary>
        /// <param name="pos">The position to snap.</param>
        /// <param name="gridSize">The size of the grid.</param>
        /// <returns>The position snapped to the grid.</returns>
        public static Double3 SnapToGrid(Double3 pos, Double3 gridSize)
        {
            if (Mathd.Abs(gridSize.X) > Mathd.Epsilon)
                pos.X = Mathd.Ceil((pos.X - (gridSize.X * 0.5)) / gridSize.X) * gridSize.X;
            if (Mathd.Abs(gridSize.Y) > Mathd.Epsilon)
                pos.Y = Mathd.Ceil((pos.Y - (gridSize.Y * 0.5)) / gridSize.Y) * gridSize.Y;
            if (Mathd.Abs(gridSize.Z) > Mathd.Epsilon)
                pos.Z = Mathd.Ceil((pos.Z - (gridSize.Z * 0.5)) / gridSize.Z) * gridSize.Z;
            return pos;
        }

        /// <summary>
        /// Adds two vectors.
        /// </summary>
        /// <param name="left">The first vector to add.</param>
        /// <param name="right">The second vector to add.</param>
        /// <returns>The sum of the two vectors.</returns>
        public static Double3 operator +(Double3 left, Double3 right)
        {
            return new Double3(left.X + right.X, left.Y + right.Y, left.Z + right.Z);
        }

        /// <summary>
        /// Multiplies a vector with another by performing component-wise multiplication equivalent to <see cref="Multiply(ref Double3,ref Double3,out Double3)" />.
        /// </summary>
        /// <param name="left">The first vector to multiply.</param>
        /// <param name="right">The second vector to multiply.</param>
        /// <returns>The multiplication of the two vectors.</returns>
        public static Double3 operator *(Double3 left, Double3 right)
        {
            return new Double3(left.X * right.X, left.Y * right.Y, left.Z * right.Z);
        }

        /// <summary>
        /// Assert a vector (return it unchanged).
        /// </summary>
        /// <param name="value">The vector to assert (unchanged).</param>
        /// <returns>The asserted (unchanged) vector.</returns>
        public static Double3 operator +(Double3 value)
        {
            return value;
        }

        /// <summary>
        /// Subtracts two vectors.
        /// </summary>
        /// <param name="left">The first vector to subtract.</param>
        /// <param name="right">The second vector to subtract.</param>
        /// <returns>The difference of the two vectors.</returns>
        public static Double3 operator -(Double3 left, Double3 right)
        {
            return new Double3(left.X - right.X, left.Y - right.Y, left.Z - right.Z);
        }

        /// <summary>
        /// Reverses the direction of a given vector.
        /// </summary>
        /// <param name="value">The vector to negate.</param>
        /// <returns>A vector facing in the opposite direction.</returns>
        public static Double3 operator -(Double3 value)
        {
            return new Double3(-value.X, -value.Y, -value.Z);
        }

        /// <summary>
        /// Transforms a vector by the given rotation.
        /// </summary>
        /// <param name="vector">The vector to transform.</param>
        /// <param name="rotation">The quaternion.</param>
        /// <returns>The scaled vector.</returns>
        public static Double3 operator *(Double3 vector, Quaternion rotation)
        {
            Transform(ref vector, ref rotation, out var result);
            return result;
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Double3 operator *(double scale, Double3 value)
        {
            return new Double3(value.X * scale, value.Y * scale, value.Z * scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Double3 operator *(Double3 value, double scale)
        {
            return new Double3(value.X * scale, value.Y * scale, value.Z * scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Double3 operator /(Double3 value, double scale)
        {
            return new Double3(value.X / scale, value.Y / scale, value.Z / scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="value">The vector to scale.</param>
        /// <returns>The scaled vector.</returns>
        public static Double3 operator /(double scale, Double3 value)
        {
            return new Double3(scale / value.X, scale / value.Y, scale / value.Z);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Double3 operator /(Double3 value, Double3 scale)
        {
            return new Double3(value.X / scale.X, value.Y / scale.Y, value.Z / scale.Z);
        }

        /// <summary>
        /// Remainder of value divided by scale.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The remained vector.</returns>
        public static Double3 operator %(Double3 value, double scale)
        {
            return new Double3(value.X % scale, value.Y % scale, value.Z % scale);
        }

        /// <summary>
        /// Remainder of value divided by scale.
        /// </summary>
        /// <param name="value">The amount by which to scale the vector.</param>
        /// <param name="scale">The vector to scale.</param>
        /// <returns>The remained vector.</returns>
        public static Double3 operator %(double value, Double3 scale)
        {
            return new Double3(value % scale.X, value % scale.Y, value % scale.Z);
        }

        /// <summary>
        /// Remainder of value divided by scale.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The remained vector.</returns>
        public static Double3 operator %(Double3 value, Double3 scale)
        {
            return new Double3(value.X % scale.X, value.Y % scale.Y, value.Z % scale.Z);
        }

        /// <summary>
        /// Performs a component-wise addition.
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be added on elements</param>
        /// <returns>The vector with added scalar for each element.</returns>
        public static Double3 operator +(Double3 value, double scalar)
        {
            return new Double3(value.X + scalar, value.Y + scalar, value.Z + scalar);
        }

        /// <summary>
        /// Performs a component-wise addition.
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be added on elements</param>
        /// <returns>The vector with added scalar for each element.</returns>
        public static Double3 operator +(double scalar, Double3 value)
        {
            return new Double3(scalar + value.X, scalar + value.Y, scalar + value.Z);
        }

        /// <summary>
        /// Performs a component-wise subtraction.
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be subtracted from elements</param>
        /// <returns>The vector with added scalar from each element.</returns>
        public static Double3 operator -(Double3 value, double scalar)
        {
            return new Double3(value.X - scalar, value.Y - scalar, value.Z - scalar);
        }

        /// <summary>
        /// Performs a component-wise subtraction.
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be subtracted from elements</param>
        /// <returns>The vector with subtracted scalar from each element.</returns>
        public static Double3 operator -(double scalar, Double3 value)
        {
            return new Double3(scalar - value.X, scalar - value.Y, scalar - value.Z);
        }

        /// <summary>
        /// Tests for equality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left" /> has the same value as <paramref name="right" />; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator ==(Double3 left, Double3 right)
        {
            return Mathd.NearEqual(left.X, right.X) && Mathd.NearEqual(left.Y, right.Y) && Mathd.NearEqual(left.Z, right.Z);
        }

        /// <summary>
        /// Tests for inequality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left" /> has a different value than <paramref name="right" />; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator !=(Double3 left, Double3 right)
        {
            return !Mathd.NearEqual(left.X, right.X) || !Mathd.NearEqual(left.Y, right.Y) || !Mathd.NearEqual(left.Z, right.Z);
        }

        /// <summary>
        /// Performs an implicit conversion from <see cref="Double3" /> to <see cref="Float3" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static implicit operator Float3(Double3 value)
        {
            return new Float3((float)value.X, (float)value.Y, (float)value.Z);
        }

        /// <summary>
        /// Performs an implicit conversion from <see cref="Double3" /> to <see cref="Vector3" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static implicit operator Vector3(Double3 value)
        {
            return new Vector3((Real)value.X, (Real)value.Y, (Real)value.Z);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Double3" /> to <see cref="Double2" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Double2(Double3 value)
        {
            return new Double2(value.X, value.Y);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Double3" /> to <see cref="Double4" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Double4(Double3 value)
        {
            return new Double4(value, 0.0);
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public override string ToString()
        {
            return string.Format(CultureInfo.CurrentCulture, _formatString, X, Y, Z);
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
            return string.Format(CultureInfo.CurrentCulture, _formatString, X.ToString(format, CultureInfo.CurrentCulture), Y.ToString(format, CultureInfo.CurrentCulture), Z.ToString(format, CultureInfo.CurrentCulture));
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <param name="formatProvider">The format provider.</param>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public string ToString(IFormatProvider formatProvider)
        {
            return string.Format(formatProvider, _formatString, X, Y, Z);
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
            return string.Format(formatProvider, "X:{0} Y:{1} Z:{2}", X.ToString(format, formatProvider), Y.ToString(format, formatProvider), Z.ToString(format, formatProvider));
        }

        /// <summary>
        /// Returns a hash code for this instance.
        /// </summary>
        public override int GetHashCode()
        {
            unchecked
            {
                int hashCode = X.GetHashCode();
                hashCode = (hashCode * 397) ^ Y.GetHashCode();
                hashCode = (hashCode * 397) ^ Z.GetHashCode();
                return hashCode;
            }
        }

        /// <summary>
        /// Determines whether the specified <see cref="Double3" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="Double3" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="Double3" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(ref Double3 other)
        {
            return Mathd.NearEqual(other.X, X) && Mathd.NearEqual(other.Y, Y) && Mathd.NearEqual(other.Z, Z);
        }

        /// <summary>
        /// Determines whether the specified <see cref="Double3" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="Double3" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="Double3" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(Double3 other)
        {
            return Mathd.NearEqual(other.X, X) && Mathd.NearEqual(other.Y, Y) && Mathd.NearEqual(other.Z, Z);
        }

        /// <summary>
        /// Determines whether the specified <see cref="System.Object" /> is equal to this instance.
        /// </summary>
        /// <param name="value">The <see cref="System.Object" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="System.Object" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        public override bool Equals(object value)
        {
            return value is Double3 other && Mathd.NearEqual(other.X, X) && Mathd.NearEqual(other.Y, Y) && Mathd.NearEqual(other.Z, Z);
        }
    }
}
