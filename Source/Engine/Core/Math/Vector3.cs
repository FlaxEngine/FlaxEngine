// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace FlaxEngine
{
    /// <summary>
    /// Represents a three dimensional mathematical vector.
    /// </summary>
    [Unmanaged]
    [Serializable]
    [StructLayout(LayoutKind.Sequential)]
#if FLAX_EDITOR
    [System.ComponentModel.TypeConverter(typeof(TypeConverters.Vector3Converter))]
#endif
    public unsafe partial struct Vector3 : IEquatable<Vector3>, IFormattable
    {
        private static readonly string _formatString = "X:{0:F2} Y:{1:F2} Z:{2:F2}";

        /// <summary>
        /// The X component.
        /// </summary>
        public Real X;

        /// <summary>
        /// The Y component.
        /// </summary>
        public Real Y;

        /// <summary>
        /// The Z component.
        /// </summary>
        public Real Z;

        /// <summary>
        /// The size of the <see cref="Vector3" /> type, in bytes.
        /// </summary>
        public static readonly int SizeInBytes = Marshal.SizeOf(typeof(Vector3));

        /// <summary>
        /// A <see cref="Vector3" /> with all of its components set to zero.
        /// </summary>
        public static readonly Vector3 Zero;

        /// <summary>
        /// The X unit <see cref="Vector3" /> (1, 0, 0).
        /// </summary>
        public static readonly Vector3 UnitX = new Vector3(1.0f, 0.0f, 0.0f);

        /// <summary>
        /// The Y unit <see cref="Vector3" /> (0, 1, 0).
        /// </summary>
        public static readonly Vector3 UnitY = new Vector3(0.0f, 1.0f, 0.0f);

        /// <summary>
        /// The Z unit <see cref="Vector3" /> (0, 0, 1).
        /// </summary>
        public static readonly Vector3 UnitZ = new Vector3(0.0f, 0.0f, 1.0f);

        /// <summary>
        /// A <see cref="Vector3" /> with all of its components set to one.
        /// </summary>
        public static readonly Vector3 One = new Vector3(1.0f, 1.0f, 1.0f);

        /// <summary>
        /// A <see cref="Vector3" /> with all of its components set to half.
        /// </summary>
        public static readonly Vector3 Half = new Vector3(0.5f, 0.5f, 0.5f);

        /// <summary>
        /// A unit <see cref="Vector3" /> designating up (0, 1, 0).
        /// </summary>
        public static readonly Vector3 Up = new Vector3(0.0f, 1.0f, 0.0f);

        /// <summary>
        /// A unit <see cref="Vector3" /> designating down (0, -1, 0).
        /// </summary>
        public static readonly Vector3 Down = new Vector3(0.0f, -1.0f, 0.0f);

        /// <summary>
        /// A unit <see cref="Vector3" /> designating left (-1, 0, 0).
        /// </summary>
        public static readonly Vector3 Left = new Vector3(-1.0f, 0.0f, 0.0f);

        /// <summary>
        /// A unit <see cref="Vector3" /> designating right (1, 0, 0).
        /// </summary>
        public static readonly Vector3 Right = new Vector3(1.0f, 0.0f, 0.0f);

        /// <summary>
        /// A unit <see cref="Vector3" /> designating forward in a left-handed coordinate system (0, 0, 1).
        /// </summary>
        public static readonly Vector3 Forward = new Vector3(0.0f, 0.0f, 1.0f);

        /// <summary>
        /// A unit <see cref="Vector3" /> designating backward in a left-handed coordinate system (0, 0, -1).
        /// </summary>
        public static readonly Vector3 Backward = new Vector3(0.0f, 0.0f, -1.0f);

        /// <summary>
        /// A <see cref="Vector3" /> with all components equal to <see cref="double.MinValue"/> (or <see cref="float.MinValue"/> if using 32-bit precision).
        /// </summary>
        public static readonly Vector3 Minimum = new Vector3(float.MinValue);

        /// <summary>
        /// A <see cref="Vector3" /> with all components equal to <see cref="double.MaxValue"/> (or <see cref="float.MaxValue"/> if using 32-bit precision).
        /// </summary>
        public static readonly Vector3 Maximum = new Vector3(float.MaxValue);

        /// <summary>
        /// Initializes a new instance of the <see cref="Vector3" /> struct.
        /// </summary>
        /// <param name="value">The value that will be assigned to all components.</param>
        public Vector3(float value)
        {
            X = value;
            Y = value;
            Z = value;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Vector3" /> struct.
        /// </summary>
        /// <param name="x">Initial value for the X component of the vector.</param>
        /// <param name="y">Initial value for the Y component of the vector.</param>
        /// <param name="z">Initial value for the Z component of the vector.</param>
        public Vector3(float x, float y, float z)
        {
            X = x;
            Y = y;
            Z = z;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Vector3" /> struct.
        /// </summary>
        /// <param name="value">The value that will be assigned to all components.</param>
        public Vector3(double value)
        {
            X = Y = Z = (Real)value;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Vector3" /> struct.
        /// </summary>
        /// <param name="x">Initial value for the X component of the vector.</param>
        /// <param name="y">Initial value for the Y component of the vector.</param>
        /// <param name="z">Initial value for the Z component of the vector.</param>
        public Vector3(double x, double y, double z)
        {
            X = (Real)x;
            Y = (Real)y;
            Z = (Real)z;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Vector3" /> struct.
        /// </summary>
        /// <param name="value">A vector containing the values with which to initialize the X and Y components.</param>
        /// <param name="z">Initial value for the Z component of the vector.</param>
        public Vector3(Vector2 value, Real z)
        {
            X = value.X;
            Y = value.Y;
            Z = z;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Vector3" /> struct.
        /// </summary>
        /// <param name="value">A vector containing the values with which to initialize the X, Y and Z components.</param>
        public Vector3(Vector4 value)
        {
            X = value.X;
            Y = value.Y;
            Z = value.Z;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Vector3" /> struct.
        /// </summary>
        /// <param name="values">The values to assign to the X, Y, and Z components of the vector. This must be an array with three elements.</param>
        /// <exception cref="ArgumentNullException">Thrown when <paramref name="values" /> is <c>null</c>.</exception>
        /// <exception cref="ArgumentOutOfRangeException"> Thrown when <paramref name="values" /> contains more or less than three elements.</exception>
        public Vector3(Real[] values)
        {
            if (values == null)
                throw new ArgumentNullException(nameof(values));
            if (values.Length != 3)
                throw new ArgumentOutOfRangeException(nameof(values), "There must be three and only three input values for Vector3.");
            X = values[0];
            Y = values[1];
            Z = values[2];
        }

        /// <summary>
        /// Gets a value indicting whether this instance is normalized.
        /// </summary>
        public bool IsNormalized => Mathr.Abs((X * X + Y * Y + Z * Z) - 1.0f) < 1e-4f;

        /// <summary>
        /// Gets the normalized vector. Returned vector has length equal 1.
        /// </summary>
        public Vector3 Normalized
        {
            get
            {
                Vector3 result = this;
                result.Normalize();
                return result;
            }
        }

        /// <summary>
        /// Gets a value indicting whether this vector is zero
        /// </summary>
        public bool IsZero => Mathr.IsZero(X) && Mathr.IsZero(Y) && Mathr.IsZero(Z);

        /// <summary>
        /// Gets a value indicting whether this vector is one
        /// </summary>
        public bool IsOne => Mathr.IsOne(X) && Mathr.IsOne(Y) && Mathr.IsOne(Z);

        /// <summary>
        /// Gets a minimum component value
        /// </summary>
        public Real MinValue => Mathr.Min(X, Mathr.Min(Y, Z));

        /// <summary>
        /// Gets a maximum component value
        /// </summary>
        public Real MaxValue => Mathr.Max(X, Mathr.Max(Y, Z));

        /// <summary>
        /// Gets an arithmetic average value of all vector components.
        /// </summary>
        public Real AvgValue => (X + Y + Z) * (1.0f / 3.0f);

        /// <summary>
        /// Gets a sum of the component values.
        /// </summary>
        public Real ValuesSum => X + Y + Z;

        /// <summary>
        /// Gets a vector with values being absolute values of that vector.
        /// </summary>
        public Vector3 Absolute => new Vector3(Mathr.Abs(X), Mathr.Abs(Y), Mathr.Abs(Z));

        /// <summary>
        /// Gets a vector with values being opposite to values of that vector.
        /// </summary>
        public Vector3 Negative => new Vector3(-X, -Y, -Z);

        /// <summary>
        /// Gets or sets the component at the specified index.
        /// </summary>
        /// <value>The value of the X, Y, or Z component, depending on the index.</value>
        /// <param name="index">The index of the component to access. Use 0 for the X component, 1 for the Y component, and 2 for the Z component.</param>
        /// <returns>The value of the component at the specified index.</returns>
        /// <exception cref="System.ArgumentOutOfRangeException">Thrown when the <paramref name="index" /> is out of the range [0, 2].</exception>
        public Real this[int index]
        {
            get
            {
                switch (index)
                {
                case 0: return X;
                case 1: return Y;
                case 2: return Z;
                }
                throw new ArgumentOutOfRangeException(nameof(index), "Indices for Vector3 run from 0 to 2, inclusive.");
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
                default: throw new ArgumentOutOfRangeException(nameof(index), "Indices for Vector3 run from 0 to 2, inclusive.");
                }
            }
        }

        /// <summary>
        /// Calculates the length of the vector.
        /// </summary>
        /// <returns>The length of the vector.</returns>
        /// <remarks><see cref="Vector3.LengthSquared" /> may be preferred when only the relative length is needed and speed is of the essence.</remarks>
        public Real Length => (Real)Math.Sqrt(X * X + Y * Y + Z * Z);

        /// <summary>
        /// Calculates the squared length of the vector.
        /// </summary>
        /// <returns>The squared length of the vector.</returns>
        /// <remarks>This method may be preferred to <see cref="Vector3.Length" /> when only a relative length is needed and speed is of the essence.</remarks>
        public Real LengthSquared => X * X + Y * Y + Z * Z;

        /// <summary>
        /// Converts the vector into a unit vector.
        /// </summary>
        public void Normalize()
        {
            Real length = (Real)Math.Sqrt(X * X + Y * Y + Z * Z);
            if (length >= Mathr.Epsilon)
            {
                Real inv = 1.0f / length;
                X *= inv;
                Y *= inv;
                Z *= inv;
            }
        }

        /// <summary>
        /// Creates an array containing the elements of the vector.
        /// </summary>
        /// <returns>A three-element array containing the components of the vector.</returns>
        public Real[] ToArray()
        {
            return new[] { X, Y, Z };
        }

        /// <summary>
        /// Adds two vectors.
        /// </summary>
        /// <param name="left">The first vector to add.</param>
        /// <param name="right">The second vector to add.</param>
        /// <param name="result">When the method completes, contains the sum of the two vectors.</param>
        public static void Add(ref Vector3 left, ref Vector3 right, out Vector3 result)
        {
            result = new Vector3(left.X + right.X, left.Y + right.Y, left.Z + right.Z);
        }

        /// <summary>
        /// Adds two vectors.
        /// </summary>
        /// <param name="left">The first vector to add.</param>
        /// <param name="right">The second vector to add.</param>
        /// <returns>The sum of the two vectors.</returns>
        public static Vector3 Add(Vector3 left, Vector3 right)
        {
            return new Vector3(left.X + right.X, left.Y + right.Y, left.Z + right.Z);
        }

        /// <summary>
        /// Performs a component-wise addition.
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be added to elements</param>
        /// <param name="result">The vector with added scalar for each element.</param>
        public static void Add(ref Vector3 left, ref float right, out Vector3 result)
        {
            result = new Vector3(left.X + right, left.Y + right, left.Z + right);
        }

        /// <summary>
        /// Performs a component-wise addition.
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be added to elements</param>
        /// <returns>The vector with added scalar for each element.</returns>
        public static Vector3 Add(Vector3 left, float right)
        {
            return new Vector3(left.X + right, left.Y + right, left.Z + right);
        }

        /// <summary>
        /// Subtracts two vectors.
        /// </summary>
        /// <param name="left">The first vector to subtract.</param>
        /// <param name="right">The second vector to subtract.</param>
        /// <param name="result">When the method completes, contains the difference of the two vectors.</param>
        public static void Subtract(ref Vector3 left, ref Vector3 right, out Vector3 result)
        {
            result = new Vector3(left.X - right.X, left.Y - right.Y, left.Z - right.Z);
        }

        /// <summary>
        /// Subtracts two vectors.
        /// </summary>
        /// <param name="left">The first vector to subtract.</param>
        /// <param name="right">The second vector to subtract.</param>
        /// <returns>The difference of the two vectors.</returns>
        public static Vector3 Subtract(Vector3 left, Vector3 right)
        {
            return new Vector3(left.X - right.X, left.Y - right.Y, left.Z - right.Z);
        }

        /// <summary>
        /// Performs a component-wise subtraction.
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be subtracted from elements</param>
        /// <param name="result">The vector with subtracted scalar for each element.</param>
        public static void Subtract(ref Vector3 left, ref float right, out Vector3 result)
        {
            result = new Vector3(left.X - right, left.Y - right, left.Z - right);
        }

        /// <summary>
        /// Performs a component-wise subtraction.
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be subtracted from elements</param>
        /// <returns>The vector with subtracted scalar for each element.</returns>
        public static Vector3 Subtract(Vector3 left, float right)
        {
            return new Vector3(left.X - right, left.Y - right, left.Z - right);
        }

        /// <summary>
        /// Performs a component-wise subtraction.
        /// </summary>
        /// <param name="left">The scalar value to be subtracted from elements</param>
        /// <param name="right">The input vector.</param>
        /// <param name="result">The vector with subtracted scalar for each element.</param>
        public static void Subtract(ref float left, ref Vector3 right, out Vector3 result)
        {
            result = new Vector3(left - right.X, left - right.Y, left - right.Z);
        }

        /// <summary>
        /// Performs a component-wise subtraction.
        /// </summary>
        /// <param name="left">The scalar value to be subtracted from elements</param>
        /// <param name="right">The input vector.</param>
        /// <returns>The vector with subtracted scalar for each element.</returns>
        public static Vector3 Subtract(float left, Vector3 right)
        {
            return new Vector3(left - right.X, left - right.Y, left - right.Z);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="result">When the method completes, contains the scaled vector.</param>
        public static void Multiply(ref Vector3 value, float scale, out Vector3 result)
        {
            result = new Vector3(value.X * scale, value.Y * scale, value.Z * scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Vector3 Multiply(Vector3 value, float scale)
        {
            return new Vector3(value.X * scale, value.Y * scale, value.Z * scale);
        }

        /// <summary>
        /// Multiply a vector with another by performing component-wise multiplication.
        /// </summary>
        /// <param name="left">The first vector to multiply.</param>
        /// <param name="right">The second vector to multiply.</param>
        /// <param name="result">When the method completes, contains the multiplied vector.</param>
        public static void Multiply(ref Vector3 left, ref Vector3 right, out Vector3 result)
        {
            result = new Vector3(left.X * right.X, left.Y * right.Y, left.Z * right.Z);
        }

        /// <summary>
        /// Multiply a vector with another by performing component-wise multiplication.
        /// </summary>
        /// <param name="left">The first vector to Multiply.</param>
        /// <param name="right">The second vector to multiply.</param>
        /// <returns>The multiplied vector.</returns>
        public static Vector3 Multiply(Vector3 left, Vector3 right)
        {
            return new Vector3(left.X * right.X, left.Y * right.Y, left.Z * right.Z);
        }

        /// <summary>
        /// Divides a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector (per component).</param>
        /// <param name="result">When the method completes, contains the divided vector.</param>
        public static void Divide(ref Vector3 value, ref Vector3 scale, out Vector3 result)
        {
            result = new Vector3(value.X / scale.X, value.Y / scale.Y, value.Z / scale.Z);
        }

        /// <summary>
        /// Divides a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector (per component).</param>
        /// <returns>The divided vector.</returns>
        public static Vector3 Divide(Vector3 value, Vector3 scale)
        {
            return new Vector3(value.X / scale.X, value.Y / scale.Y, value.Z / scale.Z);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="result">When the method completes, contains the scaled vector.</param>
        public static void Divide(ref Vector3 value, float scale, out Vector3 result)
        {
            result = new Vector3(value.X / scale, value.Y / scale, value.Z / scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Vector3 Divide(Vector3 value, float scale)
        {
            return new Vector3(value.X / scale, value.Y / scale, value.Z / scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="value">The vector to scale.</param>
        /// <param name="result">When the method completes, contains the scaled vector.</param>
        public static void Divide(float scale, ref Vector3 value, out Vector3 result)
        {
            result = new Vector3(scale / value.X, scale / value.Y, scale / value.Z);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Vector3 Divide(float scale, Vector3 value)
        {
            return new Vector3(scale / value.X, scale / value.Y, scale / value.Z);
        }

        /// <summary>
        /// Reverses the direction of a given vector.
        /// </summary>
        /// <param name="value">The vector to negate.</param>
        /// <param name="result">When the method completes, contains a vector facing in the opposite direction.</param>
        public static void Negate(ref Vector3 value, out Vector3 result)
        {
            result = new Vector3(-value.X, -value.Y, -value.Z);
        }

        /// <summary>
        /// Reverses the direction of a given vector.
        /// </summary>
        /// <param name="value">The vector to negate.</param>
        /// <returns>A vector facing in the opposite direction.</returns>
        public static Vector3 Negate(Vector3 value)
        {
            return new Vector3(-value.X, -value.Y, -value.Z);
        }

        /// <summary>
        /// Returns a <see cref="Vector3" /> containing the 3D Cartesian coordinates of a point specified in Barycentric coordinates relative to a 3D triangle.
        /// </summary>
        /// <param name="value1">A <see cref="Vector3" /> containing the 3D Cartesian coordinates of vertex 1 of the triangle.</param>
        /// <param name="value2">A <see cref="Vector3" /> containing the 3D Cartesian coordinates of vertex 2 of the triangle.</param>
        /// <param name="value3">A <see cref="Vector3" /> containing the 3D Cartesian coordinates of vertex 3 of the triangle.</param>
        /// <param name="amount1">Barycentric coordinate b2, which expresses the weighting factor toward vertex 2 (specified in <paramref name="value2" />).</param>
        /// <param name="amount2">Barycentric coordinate b3, which expresses the weighting factor toward vertex 3 (specified in <paramref name="value3" />).</param>
        /// <param name="result">When the method completes, contains the 3D Cartesian coordinates of the specified point.</param>
        public static void Barycentric(ref Vector3 value1, ref Vector3 value2, ref Vector3 value3, float amount1, float amount2, out Vector3 result)
        {
            result = new Vector3(value1.X + amount1 * (value2.X - value1.X) + amount2 * (value3.X - value1.X),
                                 value1.Y + amount1 * (value2.Y - value1.Y) + amount2 * (value3.Y - value1.Y),
                                 value1.Z + amount1 * (value2.Z - value1.Z) + amount2 * (value3.Z - value1.Z));
        }

        /// <summary>
        /// Returns a <see cref="Vector3" /> containing the 3D Cartesian coordinates of a point specified in Barycentric coordinates relative to a 3D triangle.
        /// </summary>
        /// <param name="value1">A <see cref="Vector3" /> containing the 3D Cartesian coordinates of vertex 1 of the triangle.</param>
        /// <param name="value2">A <see cref="Vector3" /> containing the 3D Cartesian coordinates of vertex 2 of the triangle.</param>
        /// <param name="value3">A <see cref="Vector3" /> containing the 3D Cartesian coordinates of vertex 3 of the triangle.</param>
        /// <param name="amount1">Barycentric coordinate b2, which expresses the weighting factor toward vertex 2 (specified in <paramref name="value2" />).</param>
        /// <param name="amount2">Barycentric coordinate b3, which expresses the weighting factor toward vertex 3 (specified in <paramref name="value3" />).</param>
        /// <returns>A new <see cref="Vector3" /> containing the 3D Cartesian coordinates of the specified point.</returns>
        public static Vector3 Barycentric(Vector3 value1, Vector3 value2, Vector3 value3, float amount1, float amount2)
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
        public static void Clamp(ref Vector3 value, ref Vector3 min, ref Vector3 max, out Vector3 result)
        {
            Real x = value.X;
            x = x > max.X ? max.X : x;
            x = x < min.X ? min.X : x;
            Real y = value.Y;
            y = y > max.Y ? max.Y : y;
            y = y < min.Y ? min.Y : y;
            Real z = value.Z;
            z = z > max.Z ? max.Z : z;
            z = z < min.Z ? min.Z : z;
            result = new Vector3(x, y, z);
        }

        /// <summary>
        /// Restricts a value to be within a specified range.
        /// </summary>
        /// <param name="value">The value to clamp.</param>
        /// <param name="min">The minimum value.</param>
        /// <param name="max">The maximum value.</param>
        /// <returns>The clamped value.</returns>
        public static Vector3 Clamp(Vector3 value, Vector3 min, Vector3 max)
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
        public static void Cross(ref Vector3 left, ref Vector3 right, out Vector3 result)
        {
            result = new Vector3(left.Y * right.Z - left.Z * right.Y,
                                 left.Z * right.X - left.X * right.Z,
                                 left.X * right.Y - left.Y * right.X);
        }

        /// <summary>
        /// Calculates the cross product of two vectors.
        /// </summary>
        /// <param name="left">First source vector.</param>
        /// <param name="right">Second source vector.</param>
        /// <returns>The cross product of the two vectors.</returns>
        public static Vector3 Cross(Vector3 left, Vector3 right)
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
        /// <remarks><see cref="Vector3.DistanceSquared(ref Vector3, ref Vector3, out Real)" /> may be preferred when only the relative distance is needed and speed is of the essence.</remarks>
        public static void Distance(ref Vector3 value1, ref Vector3 value2, out Real result)
        {
            Real x = value1.X - value2.X;
            Real y = value1.Y - value2.Y;
            Real z = value1.Z - value2.Z;
            result = (Real)Math.Sqrt(x * x + y * y + z * z);
        }

        /// <summary>
        /// Calculates the distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <returns>The distance between the two vectors.</returns>
        /// <remarks><see cref="Vector3.DistanceSquared(ref Vector3, ref Vector3, out Real)" /> may be preferred when only the relative distance is needed and speed is of the essence.</remarks>
        public static Real Distance(ref Vector3 value1, ref Vector3 value2)
        {
            Real x = value1.X - value2.X;
            Real y = value1.Y - value2.Y;
            Real z = value1.Z - value2.Z;
            return (Real)Math.Sqrt(x * x + y * y + z * z);
        }

        /// <summary>
        /// Calculates the distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <returns>The distance between the two vectors.</returns>
        /// <remarks><see cref="Vector3.DistanceSquared(Vector3, Vector3)" /> may be preferred when only the relative distance is needed and speed is of the essence.</remarks>
        public static Real Distance(Vector3 value1, Vector3 value2)
        {
            Real x = value1.X - value2.X;
            Real y = value1.Y - value2.Y;
            Real z = value1.Z - value2.Z;
            return (Real)Math.Sqrt(x * x + y * y + z * z);
        }

        /// <summary>
        /// Calculates the squared distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <param name="result">When the method completes, contains the squared distance between the two vectors.</param>
        public static void DistanceSquared(ref Vector3 value1, ref Vector3 value2, out Real result)
        {
            Real x = value1.X - value2.X;
            Real y = value1.Y - value2.Y;
            Real z = value1.Z - value2.Z;
            result = x * x + y * y + z * z;
        }

        /// <summary>
        /// Calculates the squared distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <returns>The squared distance between the two vectors.</returns>
        public static Real DistanceSquared(ref Vector3 value1, ref Vector3 value2)
        {
            Real x = value1.X - value2.X;
            Real y = value1.Y - value2.Y;
            Real z = value1.Z - value2.Z;
            return x * x + y * y + z * z;
        }

        /// <summary>
        /// Calculates the squared distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <returns>The squared distance between the two vectors.</returns>
        public static Real DistanceSquared(Vector3 value1, Vector3 value2)
        {
            Real x = value1.X - value2.X;
            Real y = value1.Y - value2.Y;
            Real z = value1.Z - value2.Z;
            return x * x + y * y + z * z;
        }

        /// <summary>
        /// Calculates the distance between two vectors on the XY plane (ignoring Z).
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <param name="result">When the method completes, contains the distance between the two vectors in the XY plane.</param>
        public static void DistanceXY(ref Vector3 value1, ref Vector3 value2, out Real result)
        {
            Real x = value1.X - value2.X;
            Real y = value1.Y - value2.Y;
            result = (Real)Math.Sqrt(x * x + y * y);
        }

        /// <summary>
        /// Calculates the squared distance between two vectors on the XY plane (ignoring Z).
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector</param>
        /// <param name="result">When the method completes, contains the squared distance between the two vectors in the XY plane.</param>
        public static void DistanceXYSquared(ref Vector3 value1, ref Vector3 value2, out Real result)
        {
            Real x = value1.X - value2.X;
            Real y = value1.Y - value2.Y;
            result = x * x + y * y;
        }

        /// <summary>
        /// Calculates the distance between two vectors on the XZ plane (ignoring Y).
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <param name="result">When the method completes, contains the distance between the two vectors in the XY plane.</param>
        public static void DistanceXZ(ref Vector3 value1, ref Vector3 value2, out Real result)
        {
            Real x = value1.X - value2.X;
            Real z = value1.Z - value2.Z;
            result = (Real)Math.Sqrt(x * x + z * z);
        }

        /// <summary>
        /// Calculates the squared distance between two vectors on the XZ plane (ignoring Y).
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector</param>
        /// <param name="result">When the method completes, contains the squared distance between the two vectors in the XY plane.</param>
        public static void DistanceXZSquared(ref Vector3 value1, ref Vector3 value2, out Real result)
        {
            Real x = value1.X - value2.X;
            Real z = value1.Z - value2.Z;
            result = x * x + z * z;
        }

        /// <summary>
        /// Calculates the distance between two vectors on the YZ plane (ignoring X).
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <param name="result">When the method completes, contains the distance between the two vectors in the YZ plane.</param>
        public static void DistanceYZ(ref Vector3 value1, ref Vector3 value2, out Real result)
        {
            Real y = value1.Y - value2.Y;
            Real z = value1.Z - value2.Z;
            result = (Real)Math.Sqrt(y * y + z * z);
        }

        /// <summary>
        /// Calculates the squared distance between two vectors on the YZ plane (ignoring X).
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector</param>
        /// <param name="result">When the method completes, contains the squared distance between the two vectors in the YZ plane.</param>
        public static void DistanceYZSquared(ref Vector3 value1, ref Vector3 value2, out Real result)
        {
            Real y = value1.Y - value2.Y;
            Real z = value1.Z - value2.Z;
            result = y * y + z * z;
        }

        /// <summary>
        /// Tests whether one vector is near another vector.
        /// </summary>
        /// <param name="left">The left vector.</param>
        /// <param name="right">The right vector.</param>
        /// <param name="epsilon">The epsilon.</param>
        /// <returns><c>true</c> if left and right are near another, <c>false</c> otherwise</returns>
        public static bool NearEqual(Vector3 left, Vector3 right, float epsilon = Mathf.Epsilon)
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
        public static bool NearEqual(ref Vector3 left, ref Vector3 right, float epsilon = Mathf.Epsilon)
        {
            return Mathf.WithinEpsilon(left.X, right.X, epsilon) && Mathf.WithinEpsilon(left.Y, right.Y, epsilon) && Mathf.WithinEpsilon(left.Z, right.Z, epsilon);
        }

        /// <summary>
        /// Calculates the dot product of two vectors.
        /// </summary>
        /// <param name="left">First source vector.</param>
        /// <param name="right">Second source vector.</param>
        /// <param name="result">When the method completes, contains the dot product of the two vectors.</param>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void Dot(ref Vector3 left, ref Vector3 right, out Real result)
        {
            result = left.X * right.X + left.Y * right.Y + left.Z * right.Z;
        }

        /// <summary>
        /// Calculates the dot product of two vectors.
        /// </summary>
        /// <param name="left">First source vector.</param>
        /// <param name="right">Second source vector.</param>
        /// <returns>The dot product of the two vectors.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Real Dot(ref Vector3 left, ref Vector3 right)
        {
            return left.X * right.X + left.Y * right.Y + left.Z * right.Z;
        }

        /// <summary>
        /// Calculates the dot product of two vectors.
        /// </summary>
        /// <param name="left">First source vector.</param>
        /// <param name="right">Second source vector.</param>
        /// <returns>The dot product of the two vectors.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Real Dot(Vector3 left, Vector3 right)
        {
            return left.X * right.X + left.Y * right.Y + left.Z * right.Z;
        }

        /// <summary>
        /// Converts the vector into a unit vector.
        /// </summary>
        /// <param name="value">The vector to normalize.</param>
        /// <param name="result">When the method completes, contains the normalized vector.</param>
        public static void Normalize(ref Vector3 value, out Vector3 result)
        {
            result = value;
            result.Normalize();
        }

        /// <summary>
        /// Converts the vector into a unit vector.
        /// </summary>
        /// <param name="value">The vector to normalize.</param>
        /// <returns>The normalized vector.</returns>
        public static Vector3 Normalize(Vector3 value)
        {
            value.Normalize();
            return value;
        }

        /// <summary>
        /// Makes sure that Length of the output vector is always below max and above 0.
        /// </summary>
        /// <param name="vector">Input Vector.</param>
        /// <param name="max">Max Length</param>
        public static Vector3 ClampLength(Vector3 vector, Real max)
        {
            return ClampLength(vector, 0, max);
        }

        /// <summary>
        /// Makes sure that Length of the output vector is always below max and above min.
        /// </summary>
        /// <param name="vector">Input Vector.</param>
        /// <param name="min">Min Length</param>
        /// <param name="max">Max Length</param>
        public static Vector3 ClampLength(Vector3 vector, Real min, Real max)
        {
            ClampLength(vector, min, max, out Vector3 result);
            return result;
        }

        /// <summary>
        /// Makes sure that Length of the output vector is always below max and above min.
        /// </summary>
        /// <param name="vector">Input Vector.</param>
        /// <param name="min">Min Length</param>
        /// <param name="max">Max Length</param>
        /// <param name="result">The result vector.</param>
        public static void ClampLength(Vector3 vector, Real min, Real max, out Vector3 result)
        {
            result.X = vector.X;
            result.Y = vector.Y;
            result.Z = vector.Z;
            var lenSq = result.LengthSquared;
            if (lenSq > max * max)
            {
                var scaleFactor = max / (Real)Math.Sqrt(lenSq);
                result.X *= scaleFactor;
                result.Y *= scaleFactor;
                result.Z *= scaleFactor;
            }
            if (lenSq < min * min)
            {
                var scaleFactor = min / (Real)Math.Sqrt(lenSq);
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
        public static void Lerp(ref Vector3 start, ref Vector3 end, float amount, out Vector3 result)
        {
            result.X = Mathr.Lerp(start.X, end.X, amount);
            result.Y = Mathr.Lerp(start.Y, end.Y, amount);
            result.Z = Mathr.Lerp(start.Z, end.Z, amount);
        }

        /// <summary>
        /// Performs a linear interpolation between two vectors.
        /// </summary>
        /// <param name="start">Start vector.</param>
        /// <param name="end">End vector.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end" />.</param>
        /// <returns>The linear interpolation of the two vectors.</returns>
        /// <remarks>Passing <paramref name="amount" /> a value of 0 will cause <paramref name="start" /> to be returned; a value of 1 will cause <paramref name="end" /> to be returned.</remarks>
        public static Vector3 Lerp(Vector3 start, Vector3 end, float amount)
        {
            Lerp(ref start, ref end, amount, out var result);
            return result;
        }

        /// <summary>
        /// Performs a gradual change of a vector towards a specified target over time
        /// </summary>
        /// <param name="current">Current vector.</param>
        /// <param name="target">Target vector.</param>
        /// <param name="currentVelocity">Used to store the current velocity.</param>
        /// <param name="smoothTime">Determines the approximate time it should take to reach the target vector.</param>
        /// <param name="maxSpeed">Defines the upper limit on the speed of the Smooth Damp.</param>
        public static Vector3 SmoothDamp(Vector3 current, Vector3 target, ref Vector3 currentVelocity, float smoothTime, float maxSpeed)
        {
            return SmoothDamp(current, target, ref currentVelocity, smoothTime, maxSpeed, Time.DeltaTime);
        }

        /// <summary>
        /// Performs a gradual change of a vector towards a specified target over time
        /// </summary>
        /// <param name="current">Current vector.</param>
        /// <param name="target">Target vector.</param>
        /// <param name="currentVelocity">Used to store the current velocity.</param>
        /// <param name="smoothTime">Determines the approximate time it should take to reach the target vector.</param>
        public static Vector3 SmoothDamp(Vector3 current, Vector3 target, ref Vector3 currentVelocity, float smoothTime)
        {
            return SmoothDamp(current, target, ref currentVelocity, smoothTime, float.PositiveInfinity, Time.DeltaTime);
        }

        /// <summary>
        /// Performs a gradual change of a vector towards a specified target over time
        /// </summary>
        /// <param name="current">Current vector.</param>
        /// <param name="target">Target vector.</param>
        /// <param name="currentVelocity">Used to store the current velocity.</param>
        /// <param name="smoothTime">Determines the approximate time it should take to reach the target vector.</param>
        /// <param name="maxSpeed">Defines the upper limit on the speed of the Smooth Damp.</param>
        /// <param name="deltaTime">Delta Time, represents the time elapsed since last frame.</param>
        public static Vector3 SmoothDamp(Vector3 current, Vector3 target, ref Vector3 currentVelocity, float smoothTime, [DefaultValue("float.PositiveInfinity")] float maxSpeed, [DefaultValue("Time.DeltaTime")] float deltaTime)
        {
            smoothTime = Mathf.Max(0.0001f, smoothTime);
            Real a = 2f / smoothTime;
            Real b = a * deltaTime;
            Real e = 1f / (1f + b + 0.48f * b * b + 0.235f * b * b * b);

            Real change_x = current.X - target.X;
            Real change_y = current.Y - target.Y;
            Real change_z = current.Z - target.Z;

            Vector3 originalTo = target;

            Real maxChangeSpeed = maxSpeed * smoothTime;
            Real changeSq = maxChangeSpeed * maxChangeSpeed;
            Real sqrLen = change_x * change_x + change_y * change_y + change_z * change_z;
            if (sqrLen > changeSq)
            {
                var len = (Real)Math.Sqrt(sqrLen);
                change_x = change_x / len * maxChangeSpeed;
                change_y = change_y / len * maxChangeSpeed;
                change_z = change_z / len * maxChangeSpeed;
            }

            target.X = current.X - change_x;
            target.Y = current.Y - change_y;
            target.Z = current.Z - change_z;

            Real temp_x = (currentVelocity.X + a * change_x) * deltaTime;
            Real temp_y = (currentVelocity.Y + a * change_y) * deltaTime;
            Real temp_z = (currentVelocity.Z + a * change_z) * deltaTime;

            currentVelocity.X = (currentVelocity.X - a * temp_x) * e;
            currentVelocity.Y = (currentVelocity.Y - a * temp_y) * e;
            currentVelocity.Z = (currentVelocity.Z - a * temp_z) * e;

            Real output_x = target.X + (change_x + temp_x) * e;
            Real output_y = target.Y + (change_y + temp_y) * e;
            Real output_z = target.Z + (change_z + temp_z) * e;

            Real x1 = originalTo.X - current.X;
            Real y1 = originalTo.Y - current.Y;
            Real z1 = originalTo.Z - current.Z;

            Real x2 = output_x - originalTo.X;
            Real y2 = output_y - originalTo.Y;
            Real z2 = output_z - originalTo.Z;

            if (x1 * x2 + y1 * y2 + z1 * z2 > 0)
            {
                output_x = originalTo.X;
                output_y = originalTo.Y;
                output_z = originalTo.Z;

                currentVelocity.X = (output_x - originalTo.X) / deltaTime;
                currentVelocity.Y = (output_y - originalTo.Y) / deltaTime;
                currentVelocity.Z = (output_z - originalTo.Z) / deltaTime;
            }

            return new Vector3(output_x, output_y, output_z);
        }


        /// <summary>
        /// Performs a cubic interpolation between two vectors.
        /// </summary>
        /// <param name="start">Start vector.</param>
        /// <param name="end">End vector.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end" />.</param>
        /// <param name="result">When the method completes, contains the cubic interpolation of the two vectors.</param>
        public static void SmoothStep(ref Vector3 start, ref Vector3 end, float amount, out Vector3 result)
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
        public static Vector3 SmoothStep(Vector3 start, Vector3 end, float amount)
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
        public static Vector3 MoveTowards(Vector3 current, Vector3 target, float maxDistanceDelta)
        {
            var to = target - current;
            var distanceSq = to.LengthSquared;
            if (distanceSq == 0 || (maxDistanceDelta >= 0 && distanceSq <= maxDistanceDelta * maxDistanceDelta))
                return target;
            var scale = maxDistanceDelta / Mathr.Sqrt(distanceSq);
            return new Vector3(current.X + to.X * scale, current.Y + to.Y * scale, current.Z + to.Z * scale);
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
        public static void Hermite(ref Vector3 value1, ref Vector3 tangent1, ref Vector3 value2, ref Vector3 tangent2, float amount, out Vector3 result)
        {
            float squared = amount * amount;
            float cubed = amount * squared;
            float part1 = 2.0f * cubed - 3.0f * squared + 1.0f;
            float part2 = -2.0f * cubed + 3.0f * squared;
            float part3 = cubed - 2.0f * squared + amount;
            float part4 = cubed - squared;
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
        public static Vector3 Hermite(Vector3 value1, Vector3 tangent1, Vector3 value2, Vector3 tangent2, float amount)
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
        public static void CatmullRom(ref Vector3 value1, ref Vector3 value2, ref Vector3 value3, ref Vector3 value4, float amount, out Vector3 result)
        {
            float squared = amount * amount;
            float cubed = amount * squared;
            result.X = 0.5f * (2.0f * value2.X + (-value1.X + value3.X) * amount +
                               (2.0f * value1.X - 5.0f * value2.X + 4.0f * value3.X - value4.X) * squared +
                               (-value1.X + 3.0f * value2.X - 3.0f * value3.X + value4.X) * cubed);
            result.Y = 0.5f * (2.0f * value2.Y + (-value1.Y + value3.Y) * amount +
                               (2.0f * value1.Y - 5.0f * value2.Y + 4.0f * value3.Y - value4.Y) * squared +
                               (-value1.Y + 3.0f * value2.Y - 3.0f * value3.Y + value4.Y) * cubed);
            result.Z = 0.5f * (2.0f * value2.Z + (-value1.Z + value3.Z) * amount +
                               (2.0f * value1.Z - 5.0f * value2.Z + 4.0f * value3.Z - value4.Z) * squared +
                               (-value1.Z + 3.0f * value2.Z - 3.0f * value3.Z + value4.Z) * cubed);
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
        public static Vector3 CatmullRom(Vector3 value1, Vector3 value2, Vector3 value3, Vector3 value4, float amount)
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
        public static void Max(ref Vector3 left, ref Vector3 right, out Vector3 result)
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
        public static Vector3 Max(Vector3 left, Vector3 right)
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
        public static void Min(ref Vector3 left, ref Vector3 right, out Vector3 result)
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
        public static Vector3 Min(Vector3 left, Vector3 right)
        {
            Min(ref left, ref right, out var result);
            return result;
        }

        /// <summary>
        /// Returns the absolute value of a vector.
        /// </summary>
        /// <param name="v">The value.</param>
        /// <returns> A vector which components are less or equal to 0.</returns>
        public static Vector3 Abs(Vector3 v)
        {
            return new Vector3(Math.Abs(v.X), Math.Abs(v.Y), Math.Abs(v.Z));
        }

        /// <summary>
        /// Projects a vector onto another vector.
        /// </summary>
        /// <param name="vector">The vector to project.</param>
        /// <param name="onNormal">The projection normal vector.</param>
        /// <returns>The projected vector.</returns>
        public static Vector3 Project(Vector3 vector, Vector3 onNormal)
        {
            Real sqrMag = Dot(onNormal, onNormal);
            if (sqrMag < Mathr.Epsilon)
                return Zero;
            return onNormal * Dot(vector, onNormal) / sqrMag;
        }

        /// <summary>
        /// Projects a vector onto a plane defined by a normal orthogonal to the plane.
        /// </summary>
        /// <param name="vector">The vector to project.</param>
        /// <param name="planeNormal">The plane normal vector.</param>
        /// <returns>The projected vector.</returns>
        public static Vector3 ProjectOnPlane(Vector3 vector, Vector3 planeNormal)
        {
            return vector - Project(vector, planeNormal);
        }

        /// <summary>
        /// Calculates the angle (in degrees) between <paramref name="from"/> and <paramref name="to"/>. This is always the smallest value.
        /// </summary>
        /// <param name="from">The first vector.</param>
        /// <param name="to">The second vector.</param>
        /// <returns>The angle (in degrees).</returns>
        public static Real Angle(Vector3 from, Vector3 to)
        {
            Real dot = Mathr.Clamp(Dot(from.Normalized, to.Normalized), -1.0f, 1.0f);
            if (Mathr.Abs(dot) > (1.0f - Mathr.Epsilon))
                return dot > 0.0f ? 0.0f : 180.0f;
            return (Real)Math.Acos(dot) * Mathr.RadiansToDegrees;
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
        public static void Project(ref Vector3 vector, float x, float y, float width, float height, float minZ, float maxZ, ref Matrix worldViewProjection, out Vector3 result)
        {
            TransformCoordinate(ref vector, ref worldViewProjection, out var v);
            result = new Vector3((1.0f + v.X) * 0.5f * width + x, (1.0f - v.Y) * 0.5f * height + y, v.Z * (maxZ - minZ) + minZ);
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
        public static Vector3 Project(Vector3 vector, float x, float y, float width, float height, float minZ, float maxZ, Matrix worldViewProjection)
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
        public static void Unproject(ref Vector3 vector, float x, float y, float width, float height, float minZ, float maxZ, ref Matrix worldViewProjection, out Vector3 result)
        {
            Matrix.Invert(ref worldViewProjection, out var matrix);
            var v = new Vector3
            {
                X = (vector.X - x) / width * 2.0f - 1.0f,
                Y = -((vector.Y - y) / height * 2.0f - 1.0f),
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
        public static Vector3 Unproject(Vector3 vector, float x, float y, float width, float height, float minZ, float maxZ, Matrix worldViewProjection)
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
        public static void Reflect(ref Vector3 vector, ref Vector3 normal, out Vector3 result)
        {
            Real dot = vector.X * normal.X + vector.Y * normal.Y + vector.Z * normal.Z;
            result.X = vector.X - 2.0f * dot * normal.X;
            result.Y = vector.Y - 2.0f * dot * normal.Y;
            result.Z = vector.Z - 2.0f * dot * normal.Z;
        }

        /// <summary>
        /// Returns the reflection of a vector off a surface that has the specified normal.
        /// </summary>
        /// <param name="vector">The source vector.</param>
        /// <param name="normal">Normal of the surface.</param>
        /// <returns>The reflected vector.</returns>
        /// <remarks>Reflect only gives the direction of a reflection off a surface, it does not determine whether the original vector was close enough to the surface to hit it.</remarks>
        public static Vector3 Reflect(Vector3 vector, Vector3 normal)
        {
            Reflect(ref vector, ref normal, out var result);
            return result;
        }

        /// <summary>
        /// Transforms a 3D vector by the given <see cref="Quaternion" /> rotation.
        /// </summary>
        /// <param name="vector">The vector to rotate.</param>
        /// <param name="rotation">The <see cref="Quaternion" /> rotation to apply.</param>
        /// <param name="result">When the method completes, contains the transformed <see cref="Vector3" />.</param>
        public static void Transform(ref Vector3 vector, ref Quaternion rotation, out Vector3 result)
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
            result = new Vector3(vector.X * (1.0f - yy - zz) + vector.Y * (xy - wz) + vector.Z * (xz + wy),
                                 vector.X * (xy + wz) + vector.Y * (1.0f - xx - zz) + vector.Z * (yz - wx),
                                 vector.X * (xz - wy) + vector.Y * (yz + wx) + vector.Z * (1.0f - xx - yy));
        }

        /// <summary>
        /// Transforms a 3D vector by the given <see cref="Quaternion" /> rotation.
        /// </summary>
        /// <param name="vector">The vector to rotate.</param>
        /// <param name="rotation">The <see cref="Quaternion" /> rotation to apply.</param>
        /// <returns>The transformed <see cref="Vector3" />.</returns>
        public static Vector3 Transform(Vector3 vector, Quaternion rotation)
        {
            Transform(ref vector, ref rotation, out var result);
            return result;
        }

        /// <summary>
        /// Transforms a 3D vector by the given <see cref="Matrix3x3"/>.
        /// </summary>
        /// <param name="vector">The source vector.</param>
        /// <param name="transform">The transformation <see cref="Matrix3x3"/>.</param>
        /// <param name="result">When the method completes, contains the transformed <see cref="Vector3"/>.</param>
        public static void Transform(ref Vector3 vector, ref Matrix3x3 transform, out Vector3 result)
        {
            result = new Vector3((vector.X * transform.M11) + (vector.Y * transform.M21) + (vector.Z * transform.M31),
                                 (vector.X * transform.M12) + (vector.Y * transform.M22) + (vector.Z * transform.M32),
                                 (vector.X * transform.M13) + (vector.Y * transform.M23) + (vector.Z * transform.M33));
        }

        /// <summary>
        /// Transforms a 3D vector by the given <see cref="Matrix3x3"/>.
        /// </summary>
        /// <param name="vector">The source vector.</param>
        /// <param name="transform">The transformation <see cref="Matrix3x3"/>.</param>
        /// <returns>The transformed <see cref="Vector3"/>.</returns>
        public static Vector3 Transform(Vector3 vector, Matrix3x3 transform)
        {
            Transform(ref vector, ref transform, out var result);
            return result;
        }

        /// <summary>
        /// Transforms a 3D vector by the given <see cref="Matrix" />.
        /// </summary>
        /// <param name="vector">The source vector.</param>
        /// <param name="transform">The transformation <see cref="Matrix" />.</param>
        /// <param name="result">When the method completes, contains the transformed <see cref="Vector3" />.</param>
        public static void Transform(ref Vector3 vector, ref Matrix transform, out Vector3 result)
        {
            result = new Vector3(vector.X * transform.M11 + vector.Y * transform.M21 + vector.Z * transform.M31 + transform.M41,
                                 vector.X * transform.M12 + vector.Y * transform.M22 + vector.Z * transform.M32 + transform.M42,
                                 vector.X * transform.M13 + vector.Y * transform.M23 + vector.Z * transform.M33 + transform.M43);
        }

        /// <summary>
        /// Transforms a 3D vector by the given <see cref="Matrix" />.
        /// </summary>
        /// <param name="vector">The source vector.</param>
        /// <param name="transform">The transformation <see cref="Matrix" />.</param>
        /// <param name="result">When the method completes, contains the transformed <see cref="Vector4" />.</param>
        public static void Transform(ref Vector3 vector, ref Matrix transform, out Vector4 result)
        {
            result = new Vector4(vector.X * transform.M11 + vector.Y * transform.M21 + vector.Z * transform.M31 + transform.M41,
                                 vector.X * transform.M12 + vector.Y * transform.M22 + vector.Z * transform.M32 + transform.M42,
                                 vector.X * transform.M13 + vector.Y * transform.M23 + vector.Z * transform.M33 + transform.M43,
                                 vector.X * transform.M14 + vector.Y * transform.M24 + vector.Z * transform.M34 + transform.M44);
        }

        /// <summary>
        /// Transforms a 3D vector by the given <see cref="FlaxEngine.Transform" />.
        /// </summary>
        /// <param name="vector">The source vector.</param>
        /// <param name="transform">The transformation <see cref="FlaxEngine.Transform" />.</param>
        /// <param name="result">When the method completes, contains the transformed <see cref="Vector3" />.</param>
        public static void Transform(ref Vector3 vector, ref Transform transform, out Vector3 result)
        {
            transform.LocalToWorld(ref vector, out result);
        }

        /// <summary>
        /// Transforms a 3D vector by the given <see cref="Matrix" />.
        /// </summary>
        /// <param name="vector">The source vector.</param>
        /// <param name="transform">The transformation <see cref="Matrix" />.</param>
        /// <returns>The transformed <see cref="Vector4" />.</returns>
        public static Vector3 Transform(Vector3 vector, Matrix transform)
        {
            Transform(ref vector, ref transform, out Vector3 result);
            return result;
        }

        /// <summary>
        /// Transforms a 3D vector by the given <see cref="FlaxEngine.Transform" />.
        /// </summary>
        /// <param name="vector">The source vector.</param>
        /// <param name="transform">The transformation <see cref="FlaxEngine.Transform" />.</param>
        /// <returns>The transformed <see cref="Vector4" />.</returns>
        public static Vector3 Transform(Vector3 vector, Transform transform)
        {
            Transform(ref vector, ref transform, out Vector3 result);
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
        public static void TransformCoordinate(ref Vector3 coordinate, ref Matrix transform, out Vector3 result)
        {
            var vector = new Vector4
            {
                X = coordinate.X * transform.M11 + coordinate.Y * transform.M21 + coordinate.Z * transform.M31 + transform.M41,
                Y = coordinate.X * transform.M12 + coordinate.Y * transform.M22 + coordinate.Z * transform.M32 + transform.M42,
                Z = coordinate.X * transform.M13 + coordinate.Y * transform.M23 + coordinate.Z * transform.M33 + transform.M43,
                W = 1f / (coordinate.X * transform.M14 + coordinate.Y * transform.M24 + coordinate.Z * transform.M34 + transform.M44)
            };
            result = new Vector3(vector.X * vector.W, vector.Y * vector.W, vector.Z * vector.W);
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
        public static Vector3 TransformCoordinate(Vector3 coordinate, Matrix transform)
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
        public static void TransformNormal(ref Vector3 normal, ref Matrix transform, out Vector3 result)
        {
            result = new Vector3(normal.X * transform.M11 + normal.Y * transform.M21 + normal.Z * transform.M31,
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
        public static Vector3 TransformNormal(Vector3 normal, Matrix transform)
        {
            TransformNormal(ref normal, ref transform, out var result);
            return result;
        }

        /// <summary>
        /// Snaps the input position onto the grid.
        /// </summary>
        /// <param name="pos">The position to snap.</param>
        /// <param name="gridSize">The size of the grid.</param>
        /// <returns>The position snapped to the grid.</returns>
        public static Vector3 SnapToGrid(Vector3 pos, Vector3 gridSize)
        {
            pos.X = Mathr.Ceil((pos.X - (gridSize.X * 0.5f)) / gridSize.X) * gridSize.X;
            pos.Y = Mathr.Ceil((pos.Y - (gridSize.Y * 0.5f)) / gridSize.Y) * gridSize.Y;
            pos.Z = Mathr.Ceil((pos.Z - (gridSize.Z * 0.5f)) / gridSize.Z) * gridSize.Z;
            return pos;
        }

        /// <summary>
        /// Snaps the <paramref name="point"/> onto the rotated grid.<br/>
        /// For world aligned grid snapping use <b><see cref="SnapToGrid(FlaxEngine.Vector3,FlaxEngine.Vector3)"/></b> instead.
        /// <example><para><b>Example code:</b></para>
        /// <code>
        /// <see langword="public" /> <see langword="class" /> SnapToGridExample : <see cref="Script"/><br/>
        ///     <see langword="public" /> <see cref="Vector3"/> GridSize = <see cref="Vector3.One"/> * 20.0f;<br/>
        ///     <see langword="public" /> <see cref="Actor"/> RayOrigin;<br/>
        ///     <see langword="public" /> <see cref="Actor"/> SomeObject;<br/>
        ///     <see langword="public" /> <see langword="override" /> <see langword="void" /> <see cref="Script.OnFixedUpdate"/><br/>
        ///     {<br/>
        ///         <see langword="if" /> (<see cref="Physics"/>.RayCast(RayOrigin.Position, RayOrigin.Transform.Forward, out <see cref="RayCastHit"/> hit)
        ///         {<br/>
        ///             <see cref="Vector3"/> position = hit.Collider.Position;
        ///             <see cref="FlaxEngine.Transform"/> transform = hit.Collider.Transform;
        ///             <see cref="Vector3"/> point = hit.Point;
        ///             <see cref="Vector3"/> normal = hit.Normal;
        ///             //Get rotation from normal relative to collider transform
        ///             <see cref="Quaternion"/> rot = <see cref="Quaternion"/>.GetRotationFromNormal(normal, transform);
        ///             point = <see cref="Vector3"/>.SnapToGrid(point, GridSize, rot, position);
        ///             SomeObject.Position = point;
        ///         }
        ///     }
        /// }
        /// </code>
        /// </example>
        /// </summary>
        /// <param name="point">The position to snap.</param>
        /// <param name="gridSize">The size of the grid.</param>
        /// <param name="gridOrientation">The rotation of the grid.</param>
        /// <param name="gridOrigin">The center point of the grid.</param>
        /// <param name="offset">The local position offset applied to the snapped position before grid rotation.</param>
        /// <returns>The position snapped to the grid.</returns>
        public static Vector3 SnapToGrid(Vector3 point, Vector3 gridSize, Quaternion gridOrientation, Vector3 gridOrigin, Vector3 offset)
        {
            return ((SnapToGrid(point - gridOrigin, gridSize) * gridOrientation.Conjugated() + offset) * gridOrientation) + gridOrigin;
        }

        /// <summary>
        /// Adds two vectors.
        /// </summary>
        /// <param name="left">The first vector to add.</param>
        /// <param name="right">The second vector to add.</param>
        /// <returns>The sum of the two vectors.</returns>
        public static Vector3 operator +(Vector3 left, Vector3 right)
        {
            return new Vector3(left.X + right.X, left.Y + right.Y, left.Z + right.Z);
        }

        /// <summary>
        /// Multiplies a vector with another by performing component-wise multiplication equivalent to <see cref="Multiply(ref Vector3,ref Vector3,out Vector3)" />.
        /// </summary>
        /// <param name="left">The first vector to multiply.</param>
        /// <param name="right">The second vector to multiply.</param>
        /// <returns>The multiplication of the two vectors.</returns>
        public static Vector3 operator *(Vector3 left, Vector3 right)
        {
            return new Vector3(left.X * right.X, left.Y * right.Y, left.Z * right.Z);
        }

        /// <summary>
        /// Assert a vector (return it unchanged).
        /// </summary>
        /// <param name="value">The vector to assert (unchanged).</param>
        /// <returns>The asserted (unchanged) vector.</returns>
        public static Vector3 operator +(Vector3 value)
        {
            return value;
        }

        /// <summary>
        /// Subtracts two vectors.
        /// </summary>
        /// <param name="left">The first vector to subtract.</param>
        /// <param name="right">The second vector to subtract.</param>
        /// <returns>The difference of the two vectors.</returns>
        public static Vector3 operator -(Vector3 left, Vector3 right)
        {
            return new Vector3(left.X - right.X, left.Y - right.Y, left.Z - right.Z);
        }

        /// <summary>
        /// Reverses the direction of a given vector.
        /// </summary>
        /// <param name="value">The vector to negate.</param>
        /// <returns>A vector facing in the opposite direction.</returns>
        public static Vector3 operator -(Vector3 value)
        {
            return new Vector3(-value.X, -value.Y, -value.Z);
        }

        /// <summary>
        /// Transforms a vector by the given rotation.
        /// </summary>
        /// <param name="vector">The vector to transform.</param>
        /// <param name="rotation">The quaternion.</param>
        /// <returns>The scaled vector.</returns>
        public static Vector3 operator *(Vector3 vector, Quaternion rotation)
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
        public static Vector3 operator *(Real scale, Vector3 value)
        {
            return new Vector3(value.X * scale, value.Y * scale, value.Z * scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Vector3 operator *(Vector3 value, Real scale)
        {
            return new Vector3(value.X * scale, value.Y * scale, value.Z * scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Vector3 operator /(Vector3 value, Real scale)
        {
            return new Vector3(value.X / scale, value.Y / scale, value.Z / scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="value">The vector to scale.</param>
        /// <returns>The scaled vector.</returns>
        public static Vector3 operator /(Real scale, Vector3 value)
        {
            return new Vector3(scale / value.X, scale / value.Y, scale / value.Z);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Vector3 operator /(Vector3 value, Vector3 scale)
        {
            return new Vector3(value.X / scale.X, value.Y / scale.Y, value.Z / scale.Z);
        }

        /// <summary>
        /// Remainder of value divided by scale.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The remained vector.</returns>
        public static Vector3 operator %(Vector3 value, Real scale)
        {
            return new Vector3(value.X % scale, value.Y % scale, value.Z % scale);
        }

        /// <summary>
        /// Remainder of value divided by scale.
        /// </summary>
        /// <param name="value">The amount by which to scale the vector.</param>
        /// <param name="scale">The vector to scale.</param>
        /// <returns>The remained vector.</returns>
        public static Vector3 operator %(Real value, Vector3 scale)
        {
            return new Vector3(value % scale.X, value % scale.Y, value % scale.Z);
        }

        /// <summary>
        /// Remainder of value divided by scale.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The remained vector.</returns>
        public static Vector3 operator %(Vector3 value, Vector3 scale)
        {
            return new Vector3(value.X % scale.X, value.Y % scale.Y, value.Z % scale.Z);
        }

        /// <summary>
        /// Performs a component-wise addition.
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be added on elements</param>
        /// <returns>The vector with added scalar for each element.</returns>
        public static Vector3 operator +(Vector3 value, Real scalar)
        {
            return new Vector3(value.X + scalar, value.Y + scalar, value.Z + scalar);
        }

        /// <summary>
        /// Performs a component-wise addition.
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be added on elements</param>
        /// <returns>The vector with added scalar for each element.</returns>
        public static Vector3 operator +(Real scalar, Vector3 value)
        {
            return new Vector3(scalar + value.X, scalar + value.Y, scalar + value.Z);
        }

        /// <summary>
        /// Performs a component-wise subtraction.
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be subtracted from elements</param>
        /// <returns>The vector with added scalar from each element.</returns>
        public static Vector3 operator -(Vector3 value, Real scalar)
        {
            return new Vector3(value.X - scalar, value.Y - scalar, value.Z - scalar);
        }

        /// <summary>
        /// Performs a component-wise subtraction.
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be subtracted from elements</param>
        /// <returns>The vector with subtracted scalar from each element.</returns>
        public static Vector3 operator -(Real scalar, Vector3 value)
        {
            return new Vector3(scalar - value.X, scalar - value.Y, scalar - value.Z);
        }

        /// <summary>
        /// Adds a vector to another by performing component-wise addition.
        /// </summary>
        /// <param name="left">The first vector to add.</param>
        /// <param name="right">The second vector to add.</param>
        /// <returns>The sum of the two vectors.</returns>
        public static Float3 operator +(Float3 left, Vector3 right)
        {
            return new Float3(left.X + (float)right.X, left.Y + (float)right.Y, left.Z + (float)right.Z);
        }

        /// <summary>
        /// Subtracts a vector from another by performing component-wise subtraction.
        /// </summary>
        /// <param name="left">The first vector to add.</param>
        /// <param name="right">The second vector to add.</param>
        /// <returns>The sum of the two vectors.</returns>
        public static Float3 operator -(Float3 left, Vector3 right)
        {
            return new Float3(left.X - (float)right.X, left.Y - (float)right.Y, left.Z - (float)right.Z);
        }

        /// <summary>
        /// Multiplies a vector with another by performing component-wise multiplication.
        /// </summary>
        /// <param name="left">The first vector to multiply.</param>
        /// <param name="right">The second vector to multiply.</param>
        /// <returns>The multiplication of the two vectors.</returns>
        public static Float3 operator *(Float3 left, Vector3 right)
        {
            return new Float3(left.X * (float)right.X, left.Y * (float)right.Y, left.Z * (float)right.Z);
        }

        /// <summary>
        /// Adds a vector to another by performing component-wise addition.
        /// </summary>
        /// <param name="left">The first vector to add.</param>
        /// <param name="right">The second vector to add.</param>
        /// <returns>The sum of the two vectors.</returns>
        public static Vector3 operator +(Vector3 left, Float3 right)
        {
            return new Vector3(left.X + (Real)right.X, left.Y + (Real)right.Y, left.Z + (Real)right.Z);
        }

        /// <summary>
        /// Subtracts a vector from another by performing component-wise subtraction.
        /// </summary>
        /// <param name="left">The first vector to add.</param>
        /// <param name="right">The second vector to add.</param>
        /// <returns>The sum of the two vectors.</returns>
        public static Vector3 operator -(Vector3 left, Float3 right)
        {
            return new Vector3(left.X - (Real)right.X, left.Y - (Real)right.Y, left.Z - (Real)right.Z);
        }

        /// <summary>
        /// Multiplies a vector with another by performing component-wise multiplication.
        /// </summary>
        /// <param name="left">The first vector to multiply.</param>
        /// <param name="right">The second vector to multiply.</param>
        /// <returns>The multiplication of the two vectors.</returns>
        public static Vector3 operator *(Vector3 left, Float3 right)
        {
            return new Vector3(left.X * (Real)right.X, left.Y * (Real)right.Y, left.Z * (Real)right.Z);
        }

        /// <summary>
        /// Tests for equality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left" /> has the same value as <paramref name="right" />; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator ==(Vector3 left, Vector3 right)
        {
            return Mathr.NearEqual(left.X, right.X) && Mathr.NearEqual(left.Y, right.Y) && Mathr.NearEqual(left.Z, right.Z);
        }

        /// <summary>
        /// Tests for inequality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left" /> has a different value than <paramref name="right" />; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator !=(Vector3 left, Vector3 right)
        {
            return !Mathr.NearEqual(left.X, right.X) || !Mathr.NearEqual(left.Y, right.Y) || !Mathr.NearEqual(left.Z, right.Z);
        }

        /// <summary>
        /// Performs an implicit conversion from <see cref="Vector3" /> to <see cref="Float3" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static implicit operator Float3(Vector3 value)
        {
            return new Float3((float)value.X, (float)value.Y, (float)value.Z);
        }

        /// <summary>
        /// Performs an implicit conversion from <see cref="Vector3" /> to <see cref="Double3" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static implicit operator Double3(Vector3 value)
        {
            return new Double3(value.X, value.Y, value.Z);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Vector3" /> to <see cref="Int3" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Int3(Vector3 value)
        {
            return new Int3((int)value.X, (int)value.Y, (int)value.Z);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Vector3" /> to <see cref="Vector2" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Vector2(Vector3 value)
        {
            return new Vector2(value.X, value.Y);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Vector3" /> to <see cref="Vector4" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Vector4(Vector3 value)
        {
            return new Vector4(value, 0.0f);
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
        /// Determines whether the specified <see cref="Vector3" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="Vector3" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="Vector3" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(ref Vector3 other)
        {
            return Mathr.NearEqual(other.X, X) && Mathr.NearEqual(other.Y, Y) && Mathr.NearEqual(other.Z, Z);
        }

        /// <summary>
        /// Determines whether the specified <see cref="Vector3" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="Vector3" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="Vector3" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(Vector3 other)
        {
            return Mathr.NearEqual(other.X, X) && Mathr.NearEqual(other.Y, Y) && Mathr.NearEqual(other.Z, Z);
        }

        /// <summary>
        /// Determines whether the specified <see cref="System.Object" /> is equal to this instance.
        /// </summary>
        /// <param name="value">The <see cref="System.Object" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="System.Object" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        public override bool Equals(object value)
        {
            return value is Vector3 other && Mathr.NearEqual(other.X, X) && Mathr.NearEqual(other.Y, Y) && Mathr.NearEqual(other.Z, Z);
        }
    }
}
