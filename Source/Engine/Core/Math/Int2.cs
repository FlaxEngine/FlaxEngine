// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Globalization;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace FlaxEngine
{
    /// <summary>
    /// Represents a two dimensional mathematical vector (signed integers).
    /// </summary>
    [Serializable]
#if FLAX_EDITOR
    [System.ComponentModel.TypeConverter(typeof(TypeConverters.Int2Converter))]
#endif
    partial struct Int2 : IEquatable<Int2>, IFormattable
    {
        private static readonly string _formatString = "X:{0} Y:{1}";

        /// <summary>
        /// The size of the <see cref="Int2" /> type, in bytes.
        /// </summary>
        public static readonly int SizeInBytes = Marshal.SizeOf(typeof(Int2));

        /// <summary>
        /// A <see cref="Int2" /> with all of its components set to zero.
        /// </summary>
        public static readonly Int2 Zero;

        /// <summary>
        /// The X unit <see cref="Int2" /> (1, 0).
        /// </summary>
        public static readonly Int2 UnitX = new Int2(1, 0);

        /// <summary>
        /// The Y unit <see cref="Int2" /> (0, 1).
        /// </summary>
        public static readonly Int2 UnitY = new Int2(0, 1);

        /// <summary>
        /// A <see cref="Int2" /> with all of its components set to one.
        /// </summary>
        public static readonly Int2 One = new Int2(1, 1);

        /// <summary>
        /// A <see cref="Int2" /> with all components equal to <see cref="int.MinValue"/>.
        /// </summary>
        public static readonly Int2 Minimum = new Int2(int.MinValue);

        /// <summary>
        /// A <see cref="Int2" /> with all components equal to <see cref="int.MaxValue"/>.
        /// </summary>
        public static readonly Int2 Maximum = new Int2(int.MaxValue);

        /// <summary>
        /// Initializes a new instance of the <see cref="Int2" /> struct.
        /// </summary>
        /// <param name="value">The value that will be assigned to all components.</param>
        public Int2(int value)
        {
            X = value;
            Y = value;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Int2" /> struct.
        /// </summary>
        /// <param name="x">Initial value for the X component of the vector.</param>
        /// <param name="y">Initial value for the Y component of the vector.</param>
        public Int2(int x, int y)
        {
            X = x;
            Y = y;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Int2" /> struct.
        /// </summary>
        /// <param name="value">A vector containing the values with which to initialize the X and Y components.</param>
        public Int2(Int3 value)
        {
            X = value.X;
            Y = value.Y;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Int2" /> struct.
        /// </summary>
        /// <param name="value">A vector containing the values with which to initialize the X and Y components.</param>
        public Int2(Int4 value)
        {
            X = value.X;
            Y = value.Y;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Int2" /> struct.
        /// </summary>
        /// <param name="values">The values to assign to the X and Y components of the vector. This must be an array with two elements.</param>
        /// <exception cref="ArgumentNullException">Thrown when <paramref name="values" /> is <c>null</c>.</exception>
        /// <exception cref="ArgumentOutOfRangeException">Thrown when <paramref name="values" /> contains more or less than two elements.
        /// </exception>
        public Int2(int[] values)
        {
            if (values == null)
                throw new ArgumentNullException(nameof(values));
            if (values.Length != 2)
                throw new ArgumentOutOfRangeException(nameof(values), "There must be two and only two input values for Int2.");
            X = values[0];
            Y = values[1];
        }

        /// <summary>
        /// Gets a value indicting whether this vector is zero
        /// </summary>
        public bool IsZero => X == 0 && Y == 0;

        /// <summary>
        /// Gets a minimum component value
        /// </summary>
        public int MinValue => Mathf.Min(X, Y);

        /// <summary>
        /// Gets a maximum component value
        /// </summary>
        public int MaxValue => Mathf.Max(X, Y);

        /// <summary>
        /// Gets an arithmetic average value of all vector components.
        /// </summary>
        public float AvgValue => (X + Y) * (1.0f / 2.0f);

        /// <summary>
        /// Gets a sum of the component values.
        /// </summary>
        public int ValuesSum => X + Y;

        /// <summary>
        /// Gets or sets the component at the specified index.
        /// </summary>
        /// <value>The value of the X or Y component, depending on the index.</value>
        /// <param name="index">The index of the component to access. Use 0 for the X component and 1 for the Y component.</param>
        /// <returns>The value of the component at the specified index.</returns>
        /// <exception cref="System.ArgumentOutOfRangeException">Thrown when the <paramref name="index" /> is out of the range [0, 1].</exception>
        public int this[int index]
        {
            get
            {
                switch (index)
                {
                case 0: return X;
                case 1: return Y;
                }
                throw new ArgumentOutOfRangeException(nameof(index), "Indices for Int2 run from 0 to 1, inclusive.");
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
                default: throw new ArgumentOutOfRangeException(nameof(index), "Indices for Int2 run from 0 to 1, inclusive.");
                }
            }
        }

        /// <summary>
        /// Calculates the length of the vector.
        /// </summary>
        /// <returns>The length of the vector.</returns>
        /// <remarks><see cref="Int2.LengthSquared" /> may be preferred when only the relative length is needed and speed is of the essence.</remarks>
        public float Length => (float)Math.Sqrt(X * X + Y * Y);

        /// <summary>
        /// Calculates the squared length of the vector.
        /// </summary>
        /// <returns>The squared length of the vector.</returns>
        /// <remarks>This method may be preferred to <see cref="Int2.Length" /> when only a relative length is needed and speed is of the essence.</remarks>
        public int LengthSquared => X * X + Y * Y;

        /// <summary>
        /// Creates an array containing the elements of the vector.
        /// </summary>
        /// <returns>A two-element array containing the components of the vector.</returns>
        public int[] ToArray()
        {
            return new[] { X, Y };
        }

        /// <summary>
        /// Adds two vectors.
        /// </summary>
        /// <param name="left">The first vector to add.</param>
        /// <param name="right">The second vector to add.</param>
        /// <param name="result">When the method completes, contains the sum of the two vectors.</param>
        public static void Add(ref Int2 left, ref Int2 right, out Int2 result)
        {
            result = new Int2(left.X + right.X, left.Y + right.Y);
        }

        /// <summary>
        /// Adds two vectors.
        /// </summary>
        /// <param name="left">The first vector to add.</param>
        /// <param name="right">The second vector to add.</param>
        /// <returns>The sum of the two vectors.</returns>
        public static Int2 Add(Int2 left, Int2 right)
        {
            return new Int2(left.X + right.X, left.Y + right.Y);
        }

        /// <summary>
        /// Perform a component-wise addition
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be added to elements</param>
        /// <param name="result">The vector with added scalar for each element.</param>
        public static void Add(ref Int2 left, ref int right, out Int2 result)
        {
            result = new Int2(left.X + right, left.Y + right);
        }

        /// <summary>
        /// Perform a component-wise addition
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be added to elements</param>
        /// <returns>The vector with added scalar for each element.</returns>
        public static Int2 Add(Int2 left, int right)
        {
            return new Int2(left.X + right, left.Y + right);
        }

        /// <summary>
        /// Subtracts two vectors.
        /// </summary>
        /// <param name="left">The first vector to subtract.</param>
        /// <param name="right">The second vector to subtract.</param>
        /// <param name="result">When the method completes, contains the difference of the two vectors.</param>
        public static void Subtract(ref Int2 left, ref Int2 right, out Int2 result)
        {
            result = new Int2(left.X - right.X, left.Y - right.Y);
        }

        /// <summary>
        /// Subtracts two vectors.
        /// </summary>
        /// <param name="left">The first vector to subtract.</param>
        /// <param name="right">The second vector to subtract.</param>
        /// <returns>The difference of the two vectors.</returns>
        public static Int2 Subtract(Int2 left, Int2 right)
        {
            return new Int2(left.X - right.X, left.Y - right.Y);
        }

        /// <summary>
        /// Perform a component-wise subtraction
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be subtracted from elements</param>
        /// <param name="result">The vector with subtracted scalar for each element.</param>
        public static void Subtract(ref Int2 left, ref int right, out Int2 result)
        {
            result = new Int2(left.X - right, left.Y - right);
        }

        /// <summary>
        /// Perform a component-wise subtraction
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be subtracted from elements</param>
        /// <returns>The vector with subtracted scalar for each element.</returns>
        public static Int2 Subtract(Int2 left, int right)
        {
            return new Int2(left.X - right, left.Y - right);
        }

        /// <summary>
        /// Perform a component-wise subtraction
        /// </summary>
        /// <param name="left">The scalar value to be subtracted from elements</param>
        /// <param name="right">The input vector</param>
        /// <param name="result">The vector with subtracted scalar for each element.</param>
        public static void Subtract(ref int left, ref Int2 right, out Int2 result)
        {
            result = new Int2(left - right.X, left - right.Y);
        }

        /// <summary>
        /// Perform a component-wise subtraction
        /// </summary>
        /// <param name="left">The scalar value to be subtracted from elements</param>
        /// <param name="right">The input vector</param>
        /// <returns>The vector with subtracted scalar for each element.</returns>
        public static Int2 Subtract(int left, Int2 right)
        {
            return new Int2(left - right.X, left - right.Y);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="result">When the method completes, contains the scaled vector.</param>
        public static void Multiply(ref Int2 value, int scale, out Int2 result)
        {
            result = new Int2(value.X * scale, value.Y * scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Int2 Multiply(Int2 value, int scale)
        {
            return new Int2(value.X * scale, value.Y * scale);
        }

        /// <summary>
        /// Multiplies a vector with another by performing component-wise multiplication.
        /// </summary>
        /// <param name="left">The first vector to multiply.</param>
        /// <param name="right">The second vector to multiply.</param>
        /// <param name="result">When the method completes, contains the multiplied vector.</param>
        public static void Multiply(ref Int2 left, ref Int2 right, out Int2 result)
        {
            result = new Int2(left.X * right.X, left.Y * right.Y);
        }

        /// <summary>
        /// Multiplies a vector with another by performing component-wise multiplication.
        /// </summary>
        /// <param name="left">The first vector to multiply.</param>
        /// <param name="right">The second vector to multiply.</param>
        /// <returns>The multiplied vector.</returns>
        public static Int2 Multiply(Int2 left, Int2 right)
        {
            return new Int2(left.X * right.X, left.Y * right.Y);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="result">When the method completes, contains the scaled vector.</param>
        public static void Divide(ref Int2 value, int scale, out Int2 result)
        {
            result = new Int2(value.X / scale, value.Y / scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Int2 Divide(Int2 value, int scale)
        {
            return new Int2(value.X / scale, value.Y / scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="value">The vector to scale.</param>
        /// <param name="result">When the method completes, contains the scaled vector.</param>
        public static void Divide(int scale, ref Int2 value, out Int2 result)
        {
            result = new Int2(scale / value.X, scale / value.Y);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Int2 Divide(int scale, Int2 value)
        {
            return new Int2(scale / value.X, scale / value.Y);
        }

        /// <summary>
        /// Reverses the direction of a given vector.
        /// </summary>
        /// <param name="value">The vector to negate.</param>
        /// <param name="result">When the method completes, contains a vector facing in the opposite direction.</param>
        public static void Negate(ref Int2 value, out Int2 result)
        {
            result = new Int2(-value.X, -value.Y);
        }

        /// <summary>
        /// Reverses the direction of a given vector.
        /// </summary>
        /// <param name="value">The vector to negate.</param>
        /// <returns>A vector facing in the opposite direction.</returns>
        public static Int2 Negate(Int2 value)
        {
            return new Int2(-value.X, -value.Y);
        }

        /// <summary>
        /// Restricts a value to be within a specified range.
        /// </summary>
        /// <param name="value">The value to clamp.</param>
        /// <param name="min">The minimum value.</param>
        /// <param name="max">The maximum value.</param>
        /// <param name="result">When the method completes, contains the clamped value.</param>
        public static void Clamp(ref Int2 value, ref Int2 min, ref Int2 max, out Int2 result)
        {
            int x = value.X;
            x = x > max.X ? max.X : x;
            x = x < min.X ? min.X : x;

            int y = value.Y;
            y = y > max.Y ? max.Y : y;
            y = y < min.Y ? min.Y : y;

            result = new Int2(x, y);
        }

        /// <summary>
        /// Restricts a value to be within a specified range.
        /// </summary>
        /// <param name="value">The value to clamp.</param>
        /// <param name="min">The minimum value.</param>
        /// <param name="max">The maximum value.</param>
        /// <returns>The clamped value.</returns>
        public static Int2 Clamp(Int2 value, Int2 min, Int2 max)
        {
            Clamp(ref value, ref min, ref max, out Int2 result);
            return result;
        }

        /// <summary>
        /// Calculates the distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <param name="result">When the method completes, contains the distance between the two vectors.</param>
        /// <remarks><see cref="Int2.DistanceSquared(ref Int2, ref Int2, out int)" /> may be preferred when only the relative distance is needed and speed is of the essence.</remarks>
        public static void Distance(ref Int2 value1, ref Int2 value2, out float result)
        {
            int x = value1.X - value2.X;
            int y = value1.Y - value2.Y;
            result = (float)Math.Sqrt(x * x + y * y);
        }

        /// <summary>
        /// Calculates the distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <returns>The distance between the two vectors.</returns>
        /// <remarks><see cref="Int2.DistanceSquared(Int2, Int2)" /> may be preferred when only the relative distance is needed and speed is of the essence.</remarks>
        public static float Distance(Int2 value1, Int2 value2)
        {
            int x = value1.X - value2.X;
            int y = value1.Y - value2.Y;
            return (float)Math.Sqrt(x * x + y * y);
        }

        /// <summary>
        /// Calculates the squared distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector</param>
        /// <param name="result">When the method completes, contains the squared distance between the two vectors.</param>
        /// <remarks>
        /// Distance squared is the value before taking the square root.
        /// Distance squared can often be used in place of distance if relative comparisons are being made.
        /// For example, consider three points A, B, and C. To determine whether B or C is further from A,
        /// compare the distance between A and B to the distance between A and C. Calculating the two distances
        /// involves two square roots, which are computationally expensive. However, using distance squared
        /// provides the same information and avoids calculating two square roots.
        /// </remarks>
        public static void DistanceSquared(ref Int2 value1, ref Int2 value2, out int result)
        {
            int x = value1.X - value2.X;
            int y = value1.Y - value2.Y;
            result = x * x + y * y;
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
        public static int DistanceSquared(Int2 value1, Int2 value2)
        {
            int x = value1.X - value2.X;
            int y = value1.Y - value2.Y;

            return x * x + y * y;
        }

        /// <summary>
        /// Calculates the dot product of two vectors.
        /// </summary>
        /// <param name="left">First source vector.</param>
        /// <param name="right">Second source vector.</param>
        /// <param name="result">When the method completes, contains the dot product of the two vectors.</param>
        public static void Dot(ref Int2 left, ref Int2 right, out int result)
        {
            result = left.X * right.X + left.Y * right.Y;
        }

        /// <summary>
        /// Calculates the dot product of two vectors.
        /// </summary>
        /// <param name="left">First source vector.</param>
        /// <param name="right">Second source vector.</param>
        /// <returns>The dot product of the two vectors.</returns>
        public static int Dot(Int2 left, Int2 right)
        {
            return left.X * right.X + left.Y * right.Y;
        }

        /// <summary>
        /// Returns a vector containing the largest components of the specified vectors.
        /// </summary>
        /// <param name="left">The first source vector.</param>
        /// <param name="right">The second source vector.</param>
        /// <param name="result">When the method completes, contains an new vector composed of the largest components of the source vectors.</param>
        public static void Max(ref Int2 left, ref Int2 right, out Int2 result)
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
        public static Int2 Max(Int2 left, Int2 right)
        {
            Max(ref left, ref right, out Int2 result);
            return result;
        }

        /// <summary>
        /// Returns a vector containing the smallest components of the specified vectors.
        /// </summary>
        /// <param name="left">The first source vector.</param>
        /// <param name="right">The second source vector.</param>
        /// <param name="result">When the method completes, contains an new vector composed of the smallest components of the source vectors.</param>
        public static void Min(ref Int2 left, ref Int2 right, out Int2 result)
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
        public static Int2 Min(Int2 left, Int2 right)
        {
            Min(ref left, ref right, out Int2 result);
            return result;
        }

        /// <summary>
        /// Returns the absolute value of a vector.
        /// </summary>
        /// <param name="v">The value.</param>
        /// <returns> A vector which components are less or equal to 0.</returns>
        public static Int2 Abs(Int2 v)
        {
            return new Int2(Math.Abs(v.X), Math.Abs(v.Y));
        }

        /// <summary>
        /// Adds two vectors.
        /// </summary>
        /// <param name="left">The first vector to add.</param>
        /// <param name="right">The second vector to add.</param>
        /// <returns>The sum of the two vectors.</returns>
        public static Int2 operator +(Int2 left, Int2 right)
        {
            return new Int2(left.X + right.X, left.Y + right.Y);
        }

        /// <summary>
        /// Multiplies a vector with another by performing component-wise multiplication equivalent to
        /// <see cref="Multiply(ref Int2,ref Int2,out Int2)" />.
        /// </summary>
        /// <param name="left">The first vector to multiply.</param>
        /// <param name="right">The second vector to multiply.</param>
        /// <returns>The multiplication of the two vectors.</returns>
        public static Int2 operator *(Int2 left, Int2 right)
        {
            return new Int2(left.X * right.X, left.Y * right.Y);
        }

        /// <summary>
        /// Assert a vector (return it unchanged).
        /// </summary>
        /// <param name="value">The vector to assert (unchanged).</param>
        /// <returns>The asserted (unchanged) vector.</returns>
        public static Int2 operator +(Int2 value)
        {
            return value;
        }

        /// <summary>
        /// Subtracts two vectors.
        /// </summary>
        /// <param name="left">The first vector to subtract.</param>
        /// <param name="right">The second vector to subtract.</param>
        /// <returns>The difference of the two vectors.</returns>
        public static Int2 operator -(Int2 left, Int2 right)
        {
            return new Int2(left.X - right.X, left.Y - right.Y);
        }

        /// <summary>
        /// Reverses the direction of a given vector.
        /// </summary>
        /// <param name="value">The vector to negate.</param>
        /// <returns>A vector facing in the opposite direction.</returns>
        public static Int2 operator -(Int2 value)
        {
            return new Int2(-value.X, -value.Y);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Int2 operator *(int scale, Int2 value)
        {
            return new Int2(value.X * scale, value.Y * scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Int2 operator *(Int2 value, int scale)
        {
            return new Int2(value.X * scale, value.Y * scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Int2 operator /(Int2 value, int scale)
        {
            return new Int2(value.X / scale, value.Y / scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="value">The vector to scale.</param>
        /// <returns>The scaled vector.</returns>
        public static Int2 operator /(int scale, Int2 value)
        {
            return new Int2(scale / value.X, scale / value.Y);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Int2 operator /(Int2 value, Int2 scale)
        {
            return new Int2(value.X / scale.X, value.Y / scale.Y);
        }

        /// <summary>
        /// Remainder of value divided by scale.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The remained vector.</returns>
        public static Int2 operator %(Int2 value, float scale)
        {
            return new Int2((int)(value.X % scale), (int)(value.Y % scale));
        }

        /// <summary>
        /// Remainder of value divided by scale.
        /// </summary>
        /// <param name="value">The amount by which to scale the vector.</param>
        /// <param name="scale">The vector to scale.</param>
        /// <returns>The remained vector.</returns>
        public static Int2 operator %(float value, Int2 scale)
        {
            return new Int2((int)(value % scale.X), (int)(value % scale.Y));
        }

        /// <summary>
        /// Remainder of value divided by scale.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The remained vector.</returns>
        public static Int2 operator %(Int2 value, Int2 scale)
        {
            return new Int2(value.X % scale.X, value.Y % scale.Y);
        }

        /// <summary>
        /// Perform a component-wise addition
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be added on elements</param>
        /// <returns>The vector with added scalar for each element.</returns>
        public static Int2 operator +(Int2 value, int scalar)
        {
            return new Int2(value.X + scalar, value.Y + scalar);
        }

        /// <summary>
        /// Perform a component-wise addition
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be added on elements</param>
        /// <returns>The vector with added scalar for each element.</returns>
        public static Int2 operator +(int scalar, Int2 value)
        {
            return new Int2(scalar + value.X, scalar + value.Y);
        }

        /// <summary>
        /// Perform a component-wise subtraction
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be subtracted from elements</param>
        /// <returns>The vector with subtracted scalar from each element.</returns>
        public static Int2 operator -(Int2 value, int scalar)
        {
            return new Int2(value.X - scalar, value.Y - scalar);
        }

        /// <summary>
        /// Perform a component-wise subtraction
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be subtracted from elements</param>
        /// <returns>The vector with subtracted scalar from each element.</returns>
        public static Int2 operator -(int scalar, Int2 value)
        {
            return new Int2(scalar - value.X, scalar - value.Y);
        }

        /// <summary>
        /// Tests for equality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left" /> has the same value as <paramref name="right" />; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator ==(Int2 left, Int2 right)
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
        public static bool operator !=(Int2 left, Int2 right)
        {
            return !left.Equals(ref right);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Int2" /> to <see cref="Int3" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Int3(Int2 value)
        {
            return new Int3(value, 0);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Int2" /> to <see cref="Int4" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Int4(Int2 value)
        {
            return new Int4(value, 0, 0);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Int2" /> to <see cref="Float2" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Float2(Int2 value)
        {
            return new Float2(value.X, value.Y);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Int2" /> to <see cref="Float3" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Float3(Int2 value)
        {
            return new Float3(value.X, value.Y, 0);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Int2" /> to <see cref="Float4" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Float4(Int2 value)
        {
            return new Float4(value.X, value.Y, 0, 0);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Int2" /> to <see cref="Vector2" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Vector2(Int2 value)
        {
            return new Vector2(value.X, value.Y);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Int2" /> to <see cref="Vector3" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Vector3(Int2 value)
        {
            return new Vector3(value.X, value.Y, 0);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Int2" /> to <see cref="Vector4" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Vector4(Int2 value)
        {
            return new Vector4(value.X, value.Y, 0, 0);
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public override string ToString()
        {
            return string.Format(CultureInfo.CurrentCulture, _formatString, X, Y);
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
        /// <returns>A hash code for this instance, suitable for use in hashing algorithms and data structures like a hash table.</returns>
        public override int GetHashCode()
        {
            unchecked
            {
                return (X.GetHashCode() * 397) ^ Y.GetHashCode();
            }
        }

        /// <summary>
        /// Determines whether the specified <see cref="Int2" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="Int2" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="Int2" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(ref Int2 other)
        {
            return other.X == X && other.Y == Y;
        }

        /// <summary>
        /// Determines whether the specified <see cref="Int2"/> are equal.
        /// </summary>
        public static bool Equals(ref Int2 a, ref Int2 b)
        {
            return a.X == b.X && a.Y == b.Y;
        }

        /// <summary>
        /// Determines whether the specified <see cref="Int2" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="Int2" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="Int2" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(Int2 other)
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
            return value is Int2 other && Equals(ref other);
        }
    }
}
