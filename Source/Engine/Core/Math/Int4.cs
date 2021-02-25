// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.ComponentModel;
using System.Globalization;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace FlaxEngine
{
    /// <summary>
    /// Represents a four dimensional mathematical vector (signed integers).
    /// </summary>
    [Serializable]
    [StructLayout(LayoutKind.Sequential, Pack = 4)]
    [TypeConverter(typeof(TypeConverters.Int4Converter))]
    public struct Int4 : IEquatable<Int4>, IFormattable
    {
        private static readonly string _formatString = "X:{0} Y:{1} Z:{2} W:{3}";

        /// <summary>
        /// The size of the <see cref="Int4" /> type, in bytes.
        /// </summary>
        public static readonly int SizeInBytes = Marshal.SizeOf(typeof(Int4));

        /// <summary>
        /// A <see cref="Int4" /> with all of its components set to zero.
        /// </summary>
        public static readonly Int4 Zero;

        /// <summary>
        /// The X unit <see cref="Int4" /> (1, 0, 0, 0).
        /// </summary>
        public static readonly Int4 UnitX = new Int4(1, 0, 0, 0);

        /// <summary>
        /// The Y unit <see cref="Int4" /> (0, 1, 0, 0).
        /// </summary>
        public static readonly Int4 UnitY = new Int4(0, 1, 0, 0);

        /// <summary>
        /// The Z unit <see cref="Int4" /> (0, 0, 1, 0).
        /// </summary>
        public static readonly Int4 UnitZ = new Int4(0, 0, 1, 0);

        /// <summary>
        /// The W unit <see cref="Int4" /> (0, 0, 0, 1).
        /// </summary>
        public static readonly Int4 UnitW = new Int4(0, 0, 0, 1);

        /// <summary>
        /// A <see cref="Int4" /> with all of its components set to one.
        /// </summary>
        public static readonly Int4 One = new Int4(1, 1, 1, 1);

        /// <summary>
        /// A <see cref="Int4" /> with all components equal to <see cref="int.MinValue"/>.
        /// </summary>
        public static readonly Int4 Minimum = new Int4(int.MinValue);

        /// <summary>
        /// A <see cref="Int4" /> with all components equal to <see cref="int.MaxValue"/>.
        /// </summary>
        public static readonly Int4 Maximum = new Int4(int.MaxValue);

        /// <summary>
        /// The X component of the vector.
        /// </summary>
        public int X;

        /// <summary>
        /// The Y component of the vector.
        /// </summary>
        public int Y;

        /// <summary>
        /// The Z component of the vector.
        /// </summary>
        public int Z;

        /// <summary>
        /// The W component of the vector.
        /// </summary>
        public int W;

        /// <summary>
        /// Initializes a new instance of the <see cref="Int4" /> struct.
        /// </summary>
        /// <param name="value">The value that will be assigned to all components.</param>
        public Int4(int value)
        {
            X = value;
            Y = value;
            Z = value;
            W = value;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Int4" /> struct.
        /// </summary>
        /// <param name="x">Initial value for the X component of the vector.</param>
        /// <param name="y">Initial value for the Y component of the vector.</param>
        /// <param name="z">Initial value for the Z component of the vector.</param>
        /// <param name="w">Initial value for the W component of the vector.</param>
        public Int4(int x, int y, int z, int w)
        {
            X = x;
            Y = y;
            Z = z;
            W = w;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Int4" /> struct.
        /// </summary>
        /// <param name="value">A vector containing the values with which to initialize the X, Y, and Z components.</param>
        /// <param name="w">Initial value for the W component of the vector.</param>
        public Int4(Int3 value, int w)
        {
            X = value.X;
            Y = value.Y;
            Z = value.Z;
            W = w;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Int4" /> struct.
        /// </summary>
        /// <param name="value">A vector containing the values with which to initialize the X and Y components.</param>
        /// <param name="z">Initial value for the Z component of the vector.</param>
        /// <param name="w">Initial value for the W component of the vector.</param>
        public Int4(Int2 value, int z, int w)
        {
            X = value.X;
            Y = value.Y;
            Z = z;
            W = w;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Int4" /> struct.
        /// </summary>
        /// <param name="values">
        /// The values to assign to the X, Y, Z, and W components of the vector. This must be an array with four elements.
        /// </param>
        /// <exception cref="ArgumentNullException">Thrown when <paramref name="values" /> is <c>null</c>.</exception>
        /// <exception cref="ArgumentOutOfRangeException">
        /// Thrown when <paramref name="values" /> contains more or less than four elements.
        /// </exception>
        public Int4(int[] values)
        {
            if (values == null)
                throw new ArgumentNullException(nameof(values));
            if (values.Length != 4)
                throw new ArgumentOutOfRangeException(nameof(values), "There must be four and only four input values for Int4.");

            X = values[0];
            Y = values[1];
            Z = values[2];
            W = values[3];
        }

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
        public int MinValue => Mathf.Min(X, Mathf.Min(Y, Mathf.Min(Z, W)));

        /// <summary>
        /// Gets a maximum component value
        /// </summary>
        public int MaxValue => Mathf.Max(X, Mathf.Max(Y, Mathf.Max(Z, W)));

        /// <summary>
        /// Gets a sum of the component values.
        /// </summary>
        public int ValuesSum => X + Y + Z + W;

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
        public int this[int index]
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

                throw new ArgumentOutOfRangeException(nameof(index), "Indices for Int4 run from 0 to 3, inclusive.");
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
                default: throw new ArgumentOutOfRangeException(nameof(index), "Indices for Int4 run from 0 to 3, inclusive.");
                }
            }
        }

        /// <summary>
        /// Creates an array containing the elements of the vector.
        /// </summary>
        /// <returns>A four-element array containing the components of the vector.</returns>
        public int[] ToArray()
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
        public static void Add(ref Int4 left, ref Int4 right, out Int4 result)
        {
            result = new Int4(left.X + right.X, left.Y + right.Y, left.Z + right.Z, left.W + right.W);
        }

        /// <summary>
        /// Adds two vectors.
        /// </summary>
        /// <param name="left">The first vector to add.</param>
        /// <param name="right">The second vector to add.</param>
        /// <returns>The sum of the two vectors.</returns>
        public static Int4 Add(Int4 left, Int4 right)
        {
            return new Int4(left.X + right.X, left.Y + right.Y, left.Z + right.Z, left.W + right.W);
        }

        /// <summary>
        /// Perform a component-wise addition
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be added to elements</param>
        /// <param name="result">The vector with added scalar for each element.</param>
        public static void Add(ref Int4 left, ref int right, out Int4 result)
        {
            result = new Int4(left.X + right, left.Y + right, left.Z + right, left.W + right);
        }

        /// <summary>
        /// Perform a component-wise addition
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be added to elements</param>
        /// <returns>The vector with added scalar for each element.</returns>
        public static Int4 Add(Int4 left, int right)
        {
            return new Int4(left.X + right, left.Y + right, left.Z + right, left.W + right);
        }

        /// <summary>
        /// Subtracts two vectors.
        /// </summary>
        /// <param name="left">The first vector to subtract.</param>
        /// <param name="right">The second vector to subtract.</param>
        /// <param name="result">When the method completes, contains the difference of the two vectors.</param>
        public static void Subtract(ref Int4 left, ref Int4 right, out Int4 result)
        {
            result = new Int4(left.X - right.X, left.Y - right.Y, left.Z - right.Z, left.W - right.W);
        }

        /// <summary>
        /// Subtracts two vectors.
        /// </summary>
        /// <param name="left">The first vector to subtract.</param>
        /// <param name="right">The second vector to subtract.</param>
        /// <returns>The difference of the two vectors.</returns>
        public static Int4 Subtract(Int4 left, Int4 right)
        {
            return new Int4(left.X - right.X, left.Y - right.Y, left.Z - right.Z, left.W - right.W);
        }

        /// <summary>
        /// Perform a component-wise subtraction
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be subtracted from elements</param>
        /// <param name="result">The vector with subtracted scalar for each element.</param>
        public static void Subtract(ref Int4 left, ref int right, out Int4 result)
        {
            result = new Int4(left.X - right, left.Y - right, left.Z - right, left.W - right);
        }

        /// <summary>
        /// Perform a component-wise subtraction
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be subtracted from elements</param>
        /// <returns>The vector with subtracted scalar for each element.</returns>
        public static Int4 Subtract(Int4 left, int right)
        {
            return new Int4(left.X - right, left.Y - right, left.Z - right, left.W - right);
        }

        /// <summary>
        /// Perform a component-wise subtraction
        /// </summary>
        /// <param name="left">The scalar value to be subtracted from elements</param>
        /// <param name="right">The input vector.</param>
        /// <param name="result">The vector with subtracted scalar for each element.</param>
        public static void Subtract(ref int left, ref Int4 right, out Int4 result)
        {
            result = new Int4(left - right.X, left - right.Y, left - right.Z, left - right.W);
        }

        /// <summary>
        /// Perform a component-wise subtraction
        /// </summary>
        /// <param name="left">The scalar value to be subtracted from elements</param>
        /// <param name="right">The input vector.</param>
        /// <returns>The vector with subtracted scalar for each element.</returns>
        public static Int4 Subtract(int left, Int4 right)
        {
            return new Int4(left - right.X, left - right.Y, left - right.Z, left - right.W);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="result">When the method completes, contains the scaled vector.</param>
        public static void Multiply(ref Int4 value, int scale, out Int4 result)
        {
            result = new Int4(value.X * scale, value.Y * scale, value.Z * scale, value.W * scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Int4 Multiply(Int4 value, int scale)
        {
            return new Int4(value.X * scale, value.Y * scale, value.Z * scale, value.W * scale);
        }

        /// <summary>
        /// Multiplies a vector with another by performing component-wise multiplication.
        /// </summary>
        /// <param name="left">The first vector to multiply.</param>
        /// <param name="right">The second vector to multiply.</param>
        /// <param name="result">When the method completes, contains the multiplied vector.</param>
        public static void Multiply(ref Int4 left, ref Int4 right, out Int4 result)
        {
            result = new Int4(left.X * right.X, left.Y * right.Y, left.Z * right.Z, left.W * right.W);
        }

        /// <summary>
        /// Multiplies a vector with another by performing component-wise multiplication.
        /// </summary>
        /// <param name="left">The first vector to multiply.</param>
        /// <param name="right">The second vector to multiply.</param>
        /// <returns>The multiplied vector.</returns>
        public static Int4 Multiply(Int4 left, Int4 right)
        {
            return new Int4(left.X * right.X, left.Y * right.Y, left.Z * right.Z, left.W * right.W);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="result">When the method completes, contains the scaled vector.</param>
        public static void Divide(ref Int4 value, int scale, out Int4 result)
        {
            result = new Int4(value.X / scale, value.Y / scale, value.Z / scale, value.W / scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Int4 Divide(Int4 value, int scale)
        {
            return new Int4(value.X / scale, value.Y / scale, value.Z / scale, value.W / scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="value">The vector to scale.</param>
        /// <param name="result">When the method completes, contains the scaled vector.</param>
        public static void Divide(int scale, ref Int4 value, out Int4 result)
        {
            result = new Int4(scale / value.X, scale / value.Y, scale / value.Z, scale / value.W);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Int4 Divide(int scale, Int4 value)
        {
            return new Int4(scale / value.X, scale / value.Y, scale / value.Z, scale / value.W);
        }

        /// <summary>
        /// Reverses the direction of a given vector.
        /// </summary>
        /// <param name="value">The vector to negate.</param>
        /// <param name="result">When the method completes, contains a vector facing in the opposite direction.</param>
        public static void Negate(ref Int4 value, out Int4 result)
        {
            result = new Int4(-value.X, -value.Y, -value.Z, -value.W);
        }

        /// <summary>
        /// Reverses the direction of a given vector.
        /// </summary>
        /// <param name="value">The vector to negate.</param>
        /// <returns>A vector facing in the opposite direction.</returns>
        public static Int4 Negate(Int4 value)
        {
            return new Int4(-value.X, -value.Y, -value.Z, -value.W);
        }

        /// <summary>
        /// Restricts a value to be within a specified range.
        /// </summary>
        /// <param name="value">The value to clamp.</param>
        /// <param name="min">The minimum value.</param>
        /// <param name="max">The maximum value.</param>
        /// <param name="result">When the method completes, contains the clamped value.</param>
        public static void Clamp(ref Int4 value, ref Int4 min, ref Int4 max, out Int4 result)
        {
            int x = value.X;
            x = x > max.X ? max.X : x;
            x = x < min.X ? min.X : x;

            int y = value.Y;
            y = y > max.Y ? max.Y : y;
            y = y < min.Y ? min.Y : y;

            int z = value.Z;
            z = z > max.Z ? max.Z : z;
            z = z < min.Z ? min.Z : z;

            int w = value.W;
            w = w > max.W ? max.W : w;
            w = w < min.W ? min.W : w;

            result = new Int4(x, y, z, w);
        }

        /// <summary>
        /// Restricts a value to be within a specified range.
        /// </summary>
        /// <param name="value">The value to clamp.</param>
        /// <param name="min">The minimum value.</param>
        /// <param name="max">The maximum value.</param>
        /// <returns>The clamped value.</returns>
        public static Int4 Clamp(Int4 value, Int4 min, Int4 max)
        {
            Clamp(ref value, ref min, ref max, out Int4 result);
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
        public static void Max(ref Int4 left, ref Int4 right, out Int4 result)
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
        public static Int4 Max(Int4 left, Int4 right)
        {
            Max(ref left, ref right, out Int4 result);
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
        public static void Min(ref Int4 left, ref Int4 right, out Int4 result)
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
        public static Int4 Min(Int4 left, Int4 right)
        {
            Min(ref left, ref right, out Int4 result);
            return result;
        }

        /// <summary>
        /// Returns the absolute value of a vector.
        /// </summary>
        /// <param name="v">The value.</param>
        /// <returns> A vector which components are less or equal to 0.</returns>
        public static Int4 Abs(Int4 v)
        {
            return new Int4(Math.Abs(v.X), Math.Abs(v.Y), Math.Abs(v.Z), Math.Abs(v.W));
        }

        /// <summary>
        /// Adds two vectors.
        /// </summary>
        /// <param name="left">The first vector to add.</param>
        /// <param name="right">The second vector to add.</param>
        /// <returns>The sum of the two vectors.</returns>
        public static Int4 operator +(Int4 left, Int4 right)
        {
            return new Int4(left.X + right.X, left.Y + right.Y, left.Z + right.Z, left.W + right.W);
        }

        /// <summary>
        /// Multiplies a vector with another by performing component-wise multiplication equivalent to
        /// <see cref="Multiply(ref Int4,ref Int4,out Int4)" />.
        /// </summary>
        /// <param name="left">The first vector to multiply.</param>
        /// <param name="right">The second vector to multiply.</param>
        /// <returns>The multiplication of the two vectors.</returns>
        public static Int4 operator *(Int4 left, Int4 right)
        {
            return new Int4(left.X * right.X, left.Y * right.Y, left.Z * right.Z, left.W * right.W);
        }

        /// <summary>
        /// Assert a vector (return it unchanged).
        /// </summary>
        /// <param name="value">The vector to assert (unchanged).</param>
        /// <returns>The asserted (unchanged) vector.</returns>
        public static Int4 operator +(Int4 value)
        {
            return value;
        }

        /// <summary>
        /// Subtracts two vectors.
        /// </summary>
        /// <param name="left">The first vector to subtract.</param>
        /// <param name="right">The second vector to subtract.</param>
        /// <returns>The difference of the two vectors.</returns>
        public static Int4 operator -(Int4 left, Int4 right)
        {
            return new Int4(left.X - right.X, left.Y - right.Y, left.Z - right.Z, left.W - right.W);
        }

        /// <summary>
        /// Reverses the direction of a given vector.
        /// </summary>
        /// <param name="value">The vector to negate.</param>
        /// <returns>A vector facing in the opposite direction.</returns>
        public static Int4 operator -(Int4 value)
        {
            return new Int4(-value.X, -value.Y, -value.Z, -value.W);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Int4 operator *(int scale, Int4 value)
        {
            return new Int4(value.X * scale, value.Y * scale, value.Z * scale, value.W * scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Int4 operator *(Int4 value, int scale)
        {
            return new Int4(value.X * scale, value.Y * scale, value.Z * scale, value.W * scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Int4 operator /(Int4 value, int scale)
        {
            return new Int4(value.X / scale, value.Y / scale, value.Z / scale, value.W / scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="value">The vector to scale.</param>
        /// <returns>The scaled vector.</returns>
        public static Int4 operator /(int scale, Int4 value)
        {
            return new Int4(scale / value.X, scale / value.Y, scale / value.Z, scale / value.W);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Int4 operator /(Int4 value, Int4 scale)
        {
            return new Int4(value.X / scale.X, value.Y / scale.Y, value.Z / scale.Z, value.W / scale.W);
        }

        /// <summary>
        /// Remainder of value divided by scale.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The remained vector.</returns>
        public static Int4 operator %(Int4 value, float scale)
        {
            return new Int4((int)(value.X % scale), (int)(value.Y % scale), (int)(value.Z % scale), (int)(value.W % scale));
        }
        
        /// <summary>
        /// Remainder of value divided by scale.
        /// </summary>
        /// <param name="value">The amount by which to scale the vector.</param>
        /// <param name="scale">The vector to scale.</param>
        /// <returns>The remained vector.</returns>
        public static Int4 operator %(float value, Int4 scale)
        {
            return new Int4((int)(value % scale.X), (int)(value % scale.Y), (int)(value % scale.Z), (int)(value % scale.W));
        }
        
        /// <summary>
        /// Remainder of value divided by scale.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The remained vector.</returns>
        public static Int4 operator %(Int4 value, Int4 scale)
        {
            return new Int4(value.X % scale.X, value.Y % scale.Y, value.Z % scale.Z, value.W % scale.W);
        }
        
        /// <summary>
        /// Perform a component-wise addition
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be added on elements</param>
        /// <returns>The vector with added scalar for each element.</returns>
        public static Int4 operator +(Int4 value, int scalar)
        {
            return new Int4(value.X + scalar, value.Y + scalar, value.Z + scalar, value.W + scalar);
        }

        /// <summary>
        /// Perform a component-wise addition
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be added on elements</param>
        /// <returns>The vector with added scalar for each element.</returns>
        public static Int4 operator +(int scalar, Int4 value)
        {
            return new Int4(scalar + value.X, scalar + value.Y, scalar + value.Z, scalar + value.W);
        }

        /// <summary>
        /// Perform a component-wise subtraction
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be subtracted from elements</param>
        /// <returns>The vector with subtracted scalar from each element.</returns>
        public static Int4 operator -(Int4 value, int scalar)
        {
            return new Int4(value.X - scalar, value.Y - scalar, value.Z - scalar, value.W - scalar);
        }

        /// <summary>
        /// Perform a component-wise subtraction
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be subtracted from elements</param>
        /// <returns>The vector with subtracted scalar from each element.</returns>
        public static Int4 operator -(int scalar, Int4 value)
        {
            return new Int4(scalar - value.X, scalar - value.Y, scalar - value.Z, scalar - value.W);
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
        public static bool operator ==(Int4 left, Int4 right)
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
        public static bool operator !=(Int4 left, Int4 right)
        {
            return !left.Equals(ref right);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Int4" /> to <see cref="Int2" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Int2(Int4 value)
        {
            return new Int2(value.X, value.Y);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Int4" /> to <see cref="Int3" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Int3(Int4 value)
        {
            return new Int3(value.X, value.Y, value.Z);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Int4" /> to <see cref="Vector2" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Vector2(Int4 value)
        {
            return new Vector2(value.X, value.Y);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Int4" /> to <see cref="Vector3" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Vector3(Int4 value)
        {
            return new Vector3(value.X, value.Y, value.Z);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Int4" /> to <see cref="Vector4" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Vector4(Int4 value)
        {
            return new Vector4(value.X, value.Y, value.Z, value.W);
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
        /// Determines whether the specified <see cref="Int4" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="Int4" /> to compare with this instance.</param>
        /// <returns>
        /// <c>true</c> if the specified <see cref="Int4" /> is equal to this instance; otherwise, <c>false</c>.
        /// </returns>
        public bool Equals(ref Int4 other)
        {
            return Mathf.NearEqual(other.X, X) &&
                   Mathf.NearEqual(other.Y, Y) &&
                   Mathf.NearEqual(other.Z, Z) &&
                   Mathf.NearEqual(other.W, W);
        }

        /// <summary>
        /// Determines whether the specified <see cref="Int4" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="Int4" /> to compare with this instance.</param>
        /// <returns>
        /// <c>true</c> if the specified <see cref="Int4" /> is equal to this instance; otherwise, <c>false</c>.
        /// </returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(Int4 other)
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
            if (!(value is Int4))
                return false;

            var strongValue = (Int4)value;
            return Equals(ref strongValue);
        }
    }
}
