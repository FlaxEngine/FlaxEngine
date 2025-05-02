// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Globalization;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace FlaxEngine
{
    /// <summary>
    /// Represents a three dimensional mathematical vector (signed integers).
    /// </summary>
    [Serializable]
#if FLAX_EDITOR
    [System.ComponentModel.TypeConverter(typeof(TypeConverters.Int3Converter))]
#endif
    partial struct Int3 : IEquatable<Int3>, IFormattable
    {
        private static readonly string _formatString = "X:{0} Y:{1} Z:{2}";

        /// <summary>
        /// The size of the <see cref="Int3" /> type, in bytes.
        /// </summary>
        public static readonly int SizeInBytes = Marshal.SizeOf(typeof(Int3));

        /// <summary>
        /// A <see cref="Int3" /> with all of its components set to zero.
        /// </summary>
        public static readonly Int3 Zero;

        /// <summary>
        /// The X unit <see cref="Int3" /> (1, 0, 0).
        /// </summary>
        public static readonly Int3 UnitX = new Int3(1, 0, 0);

        /// <summary>
        /// The Y unit <see cref="Int3" /> (0, 1, 0).
        /// </summary>
        public static readonly Int3 UnitY = new Int3(0, 1, 0);

        /// <summary>
        /// The Z unit <see cref="Int3" /> (0, 0, 1).
        /// </summary>
        public static readonly Int3 UnitZ = new Int3(0, 0, 1);

        /// <summary>
        /// A <see cref="Int3" /> with all of its components set to one.
        /// </summary>
        public static readonly Int3 One = new Int3(1, 1, 1);

        /// <summary>
        /// A <see cref="Int3" /> with all components equal to <see cref="int.MinValue"/>.
        /// </summary>
        public static readonly Int3 Minimum = new Int3(int.MinValue);

        /// <summary>
        /// A <see cref="Int3" /> with all components equal to <see cref="int.MaxValue"/>.
        /// </summary>
        public static readonly Int3 Maximum = new Int3(int.MaxValue);

        /// <summary>
        /// Initializes a new instance of the <see cref="Int3" /> struct.
        /// </summary>
        /// <param name="value">The value that will be assigned to all components.</param>
        public Int3(int value)
        {
            X = value;
            Y = value;
            Z = value;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Int3" /> struct.
        /// </summary>
        /// <param name="x">Initial value for the X component of the vector.</param>
        /// <param name="y">Initial value for the Y component of the vector.</param>
        /// <param name="z">Initial value for the Z component of the vector.</param>
        public Int3(int x, int y, int z)
        {
            X = x;
            Y = y;
            Z = z;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Int3" /> struct.
        /// </summary>
        /// <param name="value">A vector containing the values with which to initialize the X and Y components.</param>
        /// <param name="z">Initial value for the Z component of the vector.</param>
        public Int3(Int2 value, int z)
        {
            X = value.X;
            Y = value.Y;
            Z = z;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Int3" /> struct.
        /// </summary>
        /// <param name="value">A vector containing the values with which to initialize the X, Y and Z components.</param>
        public Int3(Int4 value)
        {
            X = value.X;
            Y = value.Y;
            Z = value.Z;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Int3" /> struct.
        /// </summary>
        /// <param name="values">The values to assign to the X, Y, and Z components of the vector. This must be an array with three elements.
        /// </param>
        /// <exception cref="ArgumentNullException">Thrown when <paramref name="values" /> is <c>null</c>.</exception>
        /// <exception cref="ArgumentOutOfRangeException">Thrown when <paramref name="values" /> contains more or less than three elements.</exception>
        public Int3(int[] values)
        {
            if (values == null)
                throw new ArgumentNullException(nameof(values));
            if (values.Length != 3)
                throw new ArgumentOutOfRangeException(nameof(values), "There must be three and only three input values for Int3.");
            X = values[0];
            Y = values[1];
            Z = values[2];
        }

        /// <summary>
        /// Gets a value indicting whether this vector is zero
        /// </summary>
        public bool IsZero => X == 0 && Y == 0 && Z == 0;

        /// <summary>
        /// Gets a minimum component value
        /// </summary>
        public int MinValue => Mathf.Min(X, Mathf.Min(Y, Z));

        /// <summary>
        /// Gets a maximum component value
        /// </summary>
        public int MaxValue => Mathf.Max(X, Mathf.Max(Y, Z));

        /// <summary>
        /// Gets an arithmetic average value of all vector components.
        /// </summary>
        public float AvgValue => (X + Y + Z) * (1.0f / 3.0f);

        /// <summary>
        /// Gets a sum of the component values.
        /// </summary>
        public int ValuesSum => X + Y + Z;

        /// <summary>
        /// Gets or sets the component at the specified index.
        /// </summary>
        /// <value>The value of the X, Y, or Z component, depending on the index.</value>
        /// <param name="index">The index of the component to access. Use 0 for the X component, 1 for the Y component, and 2 for the Z component.</param>
        /// <returns>The value of the component at the specified index.</returns>
        /// <exception cref="System.ArgumentOutOfRangeException">Thrown when the <paramref name="index" /> is out of the range [0, 2].</exception>
        public int this[int index]
        {
            get
            {
                switch (index)
                {
                case 0: return X;
                case 1: return Y;
                case 2: return Z;
                }

                throw new ArgumentOutOfRangeException(nameof(index), "Indices for Int3 run from 0 to 2, inclusive.");
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
                default: throw new ArgumentOutOfRangeException(nameof(index), "Indices for Int3 run from 0 to 2, inclusive.");
                }
            }
        }

        /// <summary>
        /// Calculates the length of the vector.
        /// </summary>
        /// <returns>The length of the vector.</returns>
        /// <remarks><see cref="Int3.LengthSquared" /> may be preferred when only the relative length is needed and speed is of the essence.</remarks>
        public float Length => (float)Math.Sqrt(X * X + Y * Y + Z * Z);

        /// <summary>
        /// Calculates the squared length of the vector.
        /// </summary>
        /// <returns>The squared length of the vector.</returns>
        /// <remarks>This method may be preferred to <see cref="Int3.Length" /> when only a relative length is needed and speed is of the essence.</remarks>
        public int LengthSquared => X * X + Y * Y + Z * Z;

        /// <summary>
        /// Creates an array containing the elements of the vector.
        /// </summary>
        /// <returns>A three-element array containing the components of the vector.</returns>
        public int[] ToArray()
        {
            return new[] { X, Y, Z };
        }

        /// <summary>
        /// Adds two vectors.
        /// </summary>
        /// <param name="left">The first vector to add.</param>
        /// <param name="right">The second vector to add.</param>
        /// <param name="result">When the method completes, contains the sum of the two vectors.</param>
        public static void Add(ref Int3 left, ref Int3 right, out Int3 result)
        {
            result = new Int3(left.X + right.X, left.Y + right.Y, left.Z + right.Z);
        }

        /// <summary>
        /// Adds two vectors.
        /// </summary>
        /// <param name="left">The first vector to add.</param>
        /// <param name="right">The second vector to add.</param>
        /// <returns>The sum of the two vectors.</returns>
        public static Int3 Add(Int3 left, Int3 right)
        {
            return new Int3(left.X + right.X, left.Y + right.Y, left.Z + right.Z);
        }

        /// <summary>
        /// Perform a component-wise addition
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be added to elements</param>
        /// <param name="result">The vector with added scalar for each element.</param>
        public static void Add(ref Int3 left, ref int right, out Int3 result)
        {
            result = new Int3(left.X + right, left.Y + right, left.Z + right);
        }

        /// <summary>
        /// Perform a component-wise addition
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be added to elements</param>
        /// <returns>The vector with added scalar for each element.</returns>
        public static Int3 Add(Int3 left, int right)
        {
            return new Int3(left.X + right, left.Y + right, left.Z + right);
        }

        /// <summary>
        /// Subtracts two vectors.
        /// </summary>
        /// <param name="left">The first vector to subtract.</param>
        /// <param name="right">The second vector to subtract.</param>
        /// <param name="result">When the method completes, contains the difference of the two vectors.</param>
        public static void Subtract(ref Int3 left, ref Int3 right, out Int3 result)
        {
            result = new Int3(left.X - right.X, left.Y - right.Y, left.Z - right.Z);
        }

        /// <summary>
        /// Subtracts two vectors.
        /// </summary>
        /// <param name="left">The first vector to subtract.</param>
        /// <param name="right">The second vector to subtract.</param>
        /// <returns>The difference of the two vectors.</returns>
        public static Int3 Subtract(Int3 left, Int3 right)
        {
            return new Int3(left.X - right.X, left.Y - right.Y, left.Z - right.Z);
        }

        /// <summary>
        /// Perform a component-wise subtraction
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be subtracted from elements</param>
        /// <param name="result">The vector with subtracted scalar for each element.</param>
        public static void Subtract(ref Int3 left, ref int right, out Int3 result)
        {
            result = new Int3(left.X - right, left.Y - right, left.Z - right);
        }

        /// <summary>
        /// Perform a component-wise subtraction
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be subtracted from elements</param>
        /// <returns>The vector with subtracted scalar for each element.</returns>
        public static Int3 Subtract(Int3 left, int right)
        {
            return new Int3(left.X - right, left.Y - right, left.Z - right);
        }

        /// <summary>
        /// Perform a component-wise subtraction
        /// </summary>
        /// <param name="left">The scalar value to be subtracted from elements</param>
        /// <param name="right">The input vector.</param>
        /// <param name="result">The vector with subtracted scalar for each element.</param>
        public static void Subtract(ref int left, ref Int3 right, out Int3 result)
        {
            result = new Int3(left - right.X, left - right.Y, left - right.Z);
        }

        /// <summary>
        /// Perform a component-wise subtraction
        /// </summary>
        /// <param name="left">The scalar value to be subtracted from elements</param>
        /// <param name="right">The input vector.</param>
        /// <returns>The vector with subtracted scalar for each element.</returns>
        public static Int3 Subtract(int left, Int3 right)
        {
            return new Int3(left - right.X, left - right.Y, left - right.Z);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="result">When the method completes, contains the scaled vector.</param>
        public static void Multiply(ref Int3 value, int scale, out Int3 result)
        {
            result = new Int3(value.X * scale, value.Y * scale, value.Z * scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Int3 Multiply(Int3 value, int scale)
        {
            return new Int3(value.X * scale, value.Y * scale, value.Z * scale);
        }

        /// <summary>
        /// Multiply a vector with another by performing component-wise multiplication.
        /// </summary>
        /// <param name="left">The first vector to multiply.</param>
        /// <param name="right">The second vector to multiply.</param>
        /// <param name="result">When the method completes, contains the multiplied vector.</param>
        public static void Multiply(ref Int3 left, ref Int3 right, out Int3 result)
        {
            result = new Int3(left.X * right.X, left.Y * right.Y, left.Z * right.Z);
        }

        /// <summary>
        /// Multiply a vector with another by performing component-wise multiplication.
        /// </summary>
        /// <param name="left">The first vector to Multiply.</param>
        /// <param name="right">The second vector to multiply.</param>
        /// <returns>The multiplied vector.</returns>
        public static Int3 Multiply(Int3 left, Int3 right)
        {
            return new Int3(left.X * right.X, left.Y * right.Y, left.Z * right.Z);
        }

        /// <summary>
        /// Divides a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector (per component).</param>
        /// <param name="result">When the method completes, contains the divided vector.</param>
        public static void Divide(ref Int3 value, ref Int3 scale, out Int3 result)
        {
            result = new Int3(value.X / scale.X, value.Y / scale.Y, value.Z / scale.Z);
        }

        /// <summary>
        /// Divides a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector (per component).</param>
        /// <returns>The divided vector.</returns>
        public static Int3 Divide(Int3 value, Int3 scale)
        {
            return new Int3(value.X / scale.X, value.Y / scale.Y, value.Z / scale.Z);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="result">When the method completes, contains the scaled vector.</param>
        public static void Divide(ref Int3 value, int scale, out Int3 result)
        {
            result = new Int3(value.X / scale, value.Y / scale, value.Z / scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Int3 Divide(Int3 value, int scale)
        {
            return new Int3(value.X / scale, value.Y / scale, value.Z / scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="value">The vector to scale.</param>
        /// <param name="result">When the method completes, contains the scaled vector.</param>
        public static void Divide(int scale, ref Int3 value, out Int3 result)
        {
            result = new Int3(scale / value.X, scale / value.Y, scale / value.Z);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Int3 Divide(int scale, Int3 value)
        {
            return new Int3(scale / value.X, scale / value.Y, scale / value.Z);
        }

        /// <summary>
        /// Reverses the direction of a given vector.
        /// </summary>
        /// <param name="value">The vector to negate.</param>
        /// <param name="result">When the method completes, contains a vector facing in the opposite direction.</param>
        public static void Negate(ref Int3 value, out Int3 result)
        {
            result = new Int3(-value.X, -value.Y, -value.Z);
        }

        /// <summary>
        /// Reverses the direction of a given vector.
        /// </summary>
        /// <param name="value">The vector to negate.</param>
        /// <returns>A vector facing in the opposite direction.</returns>
        public static Int3 Negate(Int3 value)
        {
            return new Int3(-value.X, -value.Y, -value.Z);
        }

        /// <summary>
        /// Restricts a value to be within a specified range.
        /// </summary>
        /// <param name="value">The value to clamp.</param>
        /// <param name="min">The minimum value.</param>
        /// <param name="max">The maximum value.</param>
        /// <param name="result">When the method completes, contains the clamped value.</param>
        public static void Clamp(ref Int3 value, ref Int3 min, ref Int3 max, out Int3 result)
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

            result = new Int3(x, y, z);
        }

        /// <summary>
        /// Restricts a value to be within a specified range.
        /// </summary>
        /// <param name="value">The value to clamp.</param>
        /// <param name="min">The minimum value.</param>
        /// <param name="max">The maximum value.</param>
        /// <returns>The clamped value.</returns>
        public static Int3 Clamp(Int3 value, Int3 min, Int3 max)
        {
            Clamp(ref value, ref min, ref max, out Int3 result);
            return result;
        }

        /// <summary>
        /// Calculates the distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <param name="result">When the method completes, contains the distance between the two vectors.</param>
        /// <remarks><see cref="Int3.DistanceSquared(ref Int3, ref Int3, out int)" /> may be preferred when only the relative distance is needed and speed is of the essence.</remarks>
        public static void Distance(ref Int3 value1, ref Int3 value2, out float result)
        {
            int x = value1.X - value2.X;
            int y = value1.Y - value2.Y;
            int z = value1.Z - value2.Z;
            result = (float)Math.Sqrt(x * x + y * y + z * z);
        }

        /// <summary>
        /// Calculates the distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <returns>The distance between the two vectors.</returns>
        /// <remarks><see cref="Int3.DistanceSquared(Int3, Int3)" /> may be preferred when only the relative distance is needed and speed is of the essence.</remarks>
        public static float Distance(Int3 value1, Int3 value2)
        {
            int x = value1.X - value2.X;
            int y = value1.Y - value2.Y;
            int z = value1.Z - value2.Z;
            return (float)Math.Sqrt(x * x + y * y + z * z);
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
        public static void DistanceSquared(ref Int3 value1, ref Int3 value2, out int result)
        {
            int x = value1.X - value2.X;
            int y = value1.Y - value2.Y;
            int z = value1.Z - value2.Z;
            result = x * x + y * y + z * z;
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
        public static int DistanceSquared(Int3 value1, Int3 value2)
        {
            int x = value1.X - value2.X;
            int y = value1.Y - value2.Y;
            int z = value1.Z - value2.Z;
            return x * x + y * y + z * z;
        }

        /// <summary>
        /// Calculates the distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <param name="result">When the method completes, contains the distance between the two vectors in the XZ plane.</param>
        public static void Distance2D(ref Int2 value1, ref Int2 value2, out float result)
        {
            int x = value1.X - value2.X;
            int y = value1.Y - value2.Y;
            result = (float)Math.Sqrt(x * x + y * y);
        }

        /// <summary>
        /// Calculates the distance between two vectors in the XZ plane (ignoring Y).
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <returns>The distance between the two vectors in the XZ plane.</returns>
        public static float Distance2D(Int2 value1, Int2 value2)
        {
            int x = value1.X - value2.X;
            int y = value1.Y - value2.Y;
            return (float)Math.Sqrt(x * x + y * y);
        }

        /// <summary>
        /// Calculates the squared distance between two vectors in the XZ plane (ignoring Y).
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector</param>
        /// <param name="result">When the method completes, contains the squared distance between the two vectors in the XZ plane.</param>
        public static void Distance2DSquared(ref Int2 value1, ref Int2 value2, out int result)
        {
            int x = value1.X - value2.X;
            int y = value1.Y - value2.Y;
            result = x * x + y * y;
        }

        /// <summary>
        /// Calculates the squared distance between two vectors in the XZ plane (ignoring Y).
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <returns>The squared distance between the two vectors in the XZ plane.</returns>
        public static int Distance2DSquared(Int2 value1, Int2 value2)
        {
            int x = value1.X - value2.X;
            int y = value1.Y - value2.Y;

            return x * x + y * y;
        }

        /// <summary>
        /// Returns a vector containing the largest components of the specified vectors.
        /// </summary>
        /// <param name="left">The first source vector.</param>
        /// <param name="right">The second source vector.</param>
        /// <param name="result">When the method completes, contains an new vector composed of the largest components of the source vectors.
        /// </param>
        public static void Max(ref Int3 left, ref Int3 right, out Int3 result)
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
        public static Int3 Max(Int3 left, Int3 right)
        {
            Max(ref left, ref right, out Int3 result);
            return result;
        }

        /// <summary>
        /// Returns a vector containing the smallest components of the specified vectors.
        /// </summary>
        /// <param name="left">The first source vector.</param>
        /// <param name="right">The second source vector.</param>
        /// <param name="result">When the method completes, contains an new vector composed of the smallest components of the source vectors.</param>
        public static void Min(ref Int3 left, ref Int3 right, out Int3 result)
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
        public static Int3 Min(Int3 left, Int3 right)
        {
            Min(ref left, ref right, out Int3 result);
            return result;
        }

        /// <summary>
        /// Returns the absolute value of a vector.
        /// </summary>
        /// <param name="v">The value.</param>
        /// <returns> A vector which components are less or equal to 0.</returns>
        public static Int3 Abs(Int3 v)
        {
            return new Int3(Math.Abs(v.X), Math.Abs(v.Y), Math.Abs(v.Z));
        }

        /// <summary>
        /// Adds two vectors.
        /// </summary>
        /// <param name="left">The first vector to add.</param>
        /// <param name="right">The second vector to add.</param>
        /// <returns>The sum of the two vectors.</returns>
        public static Int3 operator +(Int3 left, Int3 right)
        {
            return new Int3(left.X + right.X, left.Y + right.Y, left.Z + right.Z);
        }

        /// <summary>
        /// Multiplies a vector with another by performing component-wise multiplication equivalent to
        /// <see cref="Multiply(ref Int3,ref Int3,out Int3)" />.
        /// </summary>
        /// <param name="left">The first vector to multiply.</param>
        /// <param name="right">The second vector to multiply.</param>
        /// <returns>The multiplication of the two vectors.</returns>
        public static Int3 operator *(Int3 left, Int3 right)
        {
            return new Int3(left.X * right.X, left.Y * right.Y, left.Z * right.Z);
        }

        /// <summary>
        /// Assert a vector (return it unchanged).
        /// </summary>
        /// <param name="value">The vector to assert (unchanged).</param>
        /// <returns>The asserted (unchanged) vector.</returns>
        public static Int3 operator +(Int3 value)
        {
            return value;
        }

        /// <summary>
        /// Subtracts two vectors.
        /// </summary>
        /// <param name="left">The first vector to subtract.</param>
        /// <param name="right">The second vector to subtract.</param>
        /// <returns>The difference of the two vectors.</returns>
        public static Int3 operator -(Int3 left, Int3 right)
        {
            return new Int3(left.X - right.X, left.Y - right.Y, left.Z - right.Z);
        }

        /// <summary>
        /// Reverses the direction of a given vector.
        /// </summary>
        /// <param name="value">The vector to negate.</param>
        /// <returns>A vector facing in the opposite direction.</returns>
        public static Int3 operator -(Int3 value)
        {
            return new Int3(-value.X, -value.Y, -value.Z);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Int3 operator *(int scale, Int3 value)
        {
            return new Int3(value.X * scale, value.Y * scale, value.Z * scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Int3 operator *(Int3 value, int scale)
        {
            return new Int3(value.X * scale, value.Y * scale, value.Z * scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Int3 operator /(Int3 value, int scale)
        {
            return new Int3(value.X / scale, value.Y / scale, value.Z / scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="value">The vector to scale.</param>
        /// <returns>The scaled vector.</returns>
        public static Int3 operator /(int scale, Int3 value)
        {
            return new Int3(scale / value.X, scale / value.Y, scale / value.Z);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Int3 operator /(Int3 value, Int3 scale)
        {
            return new Int3(value.X / scale.X, value.Y / scale.Y, value.Z / scale.Z);
        }

        /// <summary>
        /// Remainder of value divided by scale.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The remained vector.</returns>
        public static Int3 operator %(Int3 value, float scale)
        {
            return new Int3((int)(value.X % scale), (int)(value.Y % scale), (int)(value.Z % scale));
        }

        /// <summary>
        /// Remainder of value divided by scale.
        /// </summary>
        /// <param name="value">The amount by which to scale the vector.</param>
        /// <param name="scale">The vector to scale.</param>
        /// <returns>The remained vector.</returns>
        public static Int3 operator %(float value, Int3 scale)
        {
            return new Int3((int)(value % scale.X), (int)(value % scale.Y), (int)(value % scale.Z));
        }

        /// <summary>
        /// Remainder of value divided by scale.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The remained vector.</returns>
        public static Int3 operator %(Int3 value, Int3 scale)
        {
            return new Int3(value.X % scale.X, value.Y % scale.Y, value.Z % scale.Z);
        }

        /// <summary>
        /// Perform a component-wise addition
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be added on elements</param>
        /// <returns>The vector with added scalar for each element.</returns>
        public static Int3 operator +(Int3 value, int scalar)
        {
            return new Int3(value.X + scalar, value.Y + scalar, value.Z + scalar);
        }

        /// <summary>
        /// Perform a component-wise addition
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be added on elements</param>
        /// <returns>The vector with added scalar for each element.</returns>
        public static Int3 operator +(int scalar, Int3 value)
        {
            return new Int3(scalar + value.X, scalar + value.Y, scalar + value.Z);
        }

        /// <summary>
        /// Perform a component-wise subtraction
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be subtracted from elements</param>
        /// <returns>The vector with added scalar from each element.</returns>
        public static Int3 operator -(Int3 value, int scalar)
        {
            return new Int3(value.X - scalar, value.Y - scalar, value.Z - scalar);
        }

        /// <summary>
        /// Perform a component-wise subtraction
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be subtracted from elements</param>
        /// <returns>The vector with subtracted scalar from each element.</returns>
        public static Int3 operator -(int scalar, Int3 value)
        {
            return new Int3(scalar - value.X, scalar - value.Y, scalar - value.Z);
        }

        /// <summary>
        /// Tests for equality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left" /> has the same value as <paramref name="right" />; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator ==(Int3 left, Int3 right)
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
        public static bool operator !=(Int3 left, Int3 right)
        {
            return !left.Equals(ref right);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Int3" /> to <see cref="Int2" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Int2(Int3 value)
        {
            return new Int2(value.X, value.Y);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Int3" /> to <see cref="Int4" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Int4(Int3 value)
        {
            return new Int4(value, 0);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Int3" /> to <see cref="Float2" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Float2(Int3 value)
        {
            return new Float2(value.X, value.Y);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Int3" /> to <see cref="Float3" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Float3(Int3 value)
        {
            return new Float3(value.X, value.Y, value.Z);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Int3" /> to <see cref="Float4" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Float4(Int3 value)
        {
            return new Float4(value.X, value.Y, value.Z, 0);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Int3" /> to <see cref="Vector2" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Vector2(Int3 value)
        {
            return new Vector2(value.X, value.Y);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Int3" /> to <see cref="Vector3" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Vector3(Int3 value)
        {
            return new Vector3(value.X, value.Y, value.Z);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Int3" /> to <see cref="Vector4" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Vector4(Int3 value)
        {
            return new Vector4(value.X, value.Y, value.Z, 0);
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
            return string.Format(formatProvider, _formatString, X.ToString(format, formatProvider), Y.ToString(format, formatProvider), Z.ToString(format, formatProvider));
        }

        /// <summary>
        /// Returns a hash code for this instance.
        /// </summary>
        /// <returns>A hash code for this instance, suitable for use in hashing algorithms and data structures like a hash table.</returns>
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
        /// Determines whether the specified <see cref="Int3" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="Int3" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="Int3" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(ref Int3 other)
        {
            return other.X == X && other.Y == Y && other.Z == Z;
        }

        /// <summary>
        /// Determines whether the specified <see cref="Int3" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="Int3" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="Int3" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(Int3 other)
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
            return value is Int3 other && Equals(ref other);
        }
    }
}
