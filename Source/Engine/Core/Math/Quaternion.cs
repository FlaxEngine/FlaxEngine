// Copyright (c) Wojciech Figat. All rights reserved.

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
    [System.ComponentModel.TypeConverter(typeof(TypeConverters.QuaternionConverter))]
#endif
    partial struct Quaternion : IEquatable<Quaternion>, IFormattable
    {
        private static readonly string _formatString = "X:{0:F2} Y:{1:F2} Z:{2:F2} W:{3:F2}";

        /// <summary>
        /// The size of the <see cref="Quaternion" /> type, in bytes.
        /// </summary>
        public static readonly int SizeInBytes = Marshal.SizeOf(typeof(Quaternion));

        /// <summary>
        /// A <see cref="Quaternion" /> with all of its components set to zero.
        /// </summary>
        public static readonly Quaternion Zero;

        /// <summary>
        /// A <see cref="Quaternion" /> with all of its components set to one.
        /// </summary>
        public static readonly Quaternion One = new Quaternion(1.0f, 1.0f, 1.0f, 1.0f);

        /// <summary>
        /// The identity <see cref="Quaternion" /> (0, 0, 0, 1).
        /// </summary>
        public static readonly Quaternion Identity = new Quaternion(0.0f, 0.0f, 0.0f, 1.0f);

        /// <summary>
        /// Initializes a new instance of the <see cref="Quaternion" /> struct.
        /// </summary>
        /// <param name="value">A vector containing the values with which to initialize the components.</param>
        public Quaternion(Float4 value)
        {
            X = value.X;
            Y = value.Y;
            Z = value.Z;
            W = value.W;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Quaternion" /> struct.
        /// </summary>
        /// <param name="value">A vector containing the values with which to initialize the components.</param>
        public Quaternion(Vector4 value)
        {
            X = (float)value.X;
            Y = (float)value.Y;
            Z = (float)value.Z;
            W = (float)value.W;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Quaternion" /> struct.
        /// </summary>
        /// <param name="value">A vector containing the values with which to initialize the X, Y, and Z components.</param>
        /// <param name="w">Initial value for the W component of the quaternion.</param>
        public Quaternion(Float3 value, float w)
        {
            X = value.X;
            Y = value.Y;
            Z = value.Z;
            W = w;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Quaternion" /> struct.
        /// </summary>
        /// <param name="x">Initial value for the X component of the quaternion.</param>
        /// <param name="y">Initial value for the Y component of the quaternion.</param>
        /// <param name="z">Initial value for the Z component of the quaternion.</param>
        /// <param name="w">Initial value for the W component of the quaternion.</param>
        public Quaternion(float x, float y, float z, float w)
        {
            X = x;
            Y = y;
            Z = z;
            W = w;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Quaternion" /> struct.
        /// </summary>
        /// <param name="values">The values to assign to the X, Y, Z, and W components of the quaternion. This must be an array with four elements.</param>
        /// <exception cref="ArgumentNullException">Thrown when <paramref name="values" /> is <c>null</c>.</exception>
        /// <exception cref="ArgumentOutOfRangeException">Thrown when <paramref name="values" /> contains more or less than four elements.</exception>
        public Quaternion(float[] values)
        {
            if (values == null)
                throw new ArgumentNullException(nameof(values));
            if (values.Length != 4)
                throw new ArgumentOutOfRangeException(nameof(values), "There must be four and only four input values for Quaternion.");

            X = values[0];
            Y = values[1];
            Z = values[2];
            W = values[3];
        }

        /// <summary>
        /// Gets a value indicating whether this instance is equivalent to the identity quaternion.
        /// </summary>
        public bool IsIdentity => Equals(Identity);

        /// <summary>
        /// Gets a value indicting whether this instance is normalized.
        /// </summary>
        public bool IsNormalized => Mathf.Abs((X * X + Y * Y + Z * Z + W * W) - 1.0f) < 1e-4f;

        /// <summary>
        /// Gets the euler angle (pitch, yaw, roll) in degrees.
        /// </summary>
        public Float3 EulerAngles
        {
            get
            {
                Float3 result;
                float sqw = W * W;
                float sqx = X * X;
                float sqy = Y * Y;
                float sqz = Z * Z;
                float unit = sqx + sqy + sqz + sqw; // if normalised is one, otherwise is correction factor
                float test = X * W - Y * Z;

                if (test > 0.499995f * unit)
                {
                    // singularity at north pole

                    // yaw pitch roll
                    result.Y = 2.0f * Mathf.Atan2(Y, X);
                    result.X = Mathf.PiOverTwo;
                    result.Z = 0;
                }
                else if (test < -0.499995f * unit)
                {
                    // singularity at south pole

                    // yaw pitch roll
                    result.Y = -2.0f * Mathf.Atan2(Y, X);
                    result.X = -Mathf.PiOverTwo;
                    result.Z = 0;
                }
                else
                {
                    // yaw pitch roll
                    var q = new Quaternion(W, Z, X, Y);
                    result.Y = Mathf.Atan2(2.0f * q.X * q.W + 2.0f * q.Y * q.Z, 1 - 2.0f * (q.Z * q.Z + q.W * q.W));
                    result.X = Mathf.Asin(2.0f * (q.X * q.Z - q.W * q.Y));
                    result.Z = Mathf.Atan2(2.0f * q.X * q.Y + 2.0f * q.Z * q.W, 1 - 2.0f * (q.Y * q.Y + q.Z * q.Z));
                }

                result *= Mathf.RadiansToDegrees;
                return new Float3(Mathf.UnwindDegrees(result.X), Mathf.UnwindDegrees(result.Y), Mathf.UnwindDegrees(result.Z));
            }
        }

        /// <summary>
        /// Gets the angle of the quaternion.
        /// </summary>
        public float Angle
        {
            get
            {
                float length = X * X + Y * Y + Z * Z;
                if (Mathf.IsZero(length))
                    return 0.0f;
                return (float)(2.0 * Math.Acos(Mathf.Clamp(W, -1f, 1f)));
            }
        }

        /// <summary>
        /// Gets the axis components of the quaternion.
        /// </summary>
        public Float3 Axis
        {
            get
            {
                float length = X * X + Y * Y + Z * Z;
                if (Mathf.IsZero(length))
                    return Float3.UnitX;
                float inv = 1.0f / (float)Math.Sqrt(length);
                return new Float3(X * inv, Y * inv, Z * inv);
            }
        }

        /// <summary>
        /// Gets or sets the component at the specified index.
        /// </summary>
        /// <value>The value of the X, Y, Z, or W component, depending on the index.</value>
        /// <param name="index">The index of the component to access. Use 0 for the X component, 1 for the Y component, 2 for the Z component, and 3 for the W component.</param>
        /// <returns>The value of the component at the specified index.</returns>
        /// <exception cref="System.ArgumentOutOfRangeException">Thrown when the <paramref name="index" /> is out of the range [0, 3].</exception>
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
                throw new ArgumentOutOfRangeException(nameof(index), "Indices for Quaternion run from 0 to 3, inclusive.");
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
                default: throw new ArgumentOutOfRangeException(nameof(index), "Indices for Quaternion run from 0 to 3, inclusive.");
                }
            }
        }

        /// <summary>
        /// Conjugates the quaternion.
        /// </summary>
        public void Conjugate()
        {
            X = -X;
            Y = -Y;
            Z = -Z;
        }

        /// <summary>
        /// Gets the conjugated quaternion.
        /// </summary>
        public Quaternion Conjugated()
        {
            return new Quaternion(-X, -Y, -Z, W);
        }

        /// <summary>
        /// Conjugates and renormalizes the quaternion.
        /// </summary>
        public void Invert()
        {
            float lengthSq = LengthSquared;
            if (!Mathf.IsZero(lengthSq))
            {
                lengthSq = 1.0f / lengthSq;

                X = -X * lengthSq;
                Y = -Y * lengthSq;
                Z = -Z * lengthSq;
                W *= lengthSq;
            }
        }

        /// <summary>
        /// Calculates the length of the quaternion.
        /// </summary>
        /// <returns>The length of the quaternion.</returns>
        /// <remarks><see cref="Quaternion.LengthSquared" /> may be preferred when only the relative length is needed and speed is of the essence.</remarks>
        public float Length => (float)Math.Sqrt(X * X + Y * Y + Z * Z + W * W);

        /// <summary>
        /// Calculates the squared length of the quaternion.
        /// </summary>
        /// <returns>The squared length of the quaternion.</returns>
        /// <remarks>This method may be preferred to <see cref="Quaternion.Length" /> when only a relative length is needed and speed is of the essence.</remarks>
        public float LengthSquared => X * X + Y * Y + Z * Z + W * W;

        /// <summary>
        /// Converts the quaternion into a unit quaternion.
        /// </summary>
        public void Normalize()
        {
            float length = Length;
            if (length >= Mathf.Epsilon)
            {
                float inverse = 1.0f / length;
                X *= inverse;
                Y *= inverse;
                Z *= inverse;
                W *= inverse;
            }
        }

        /// <summary>
        /// Creates an array containing the elements of the quaternion.
        /// </summary>
        /// <returns>A four-element array containing the components of the quaternion.</returns>
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
        /// Adds two quaternions.
        /// </summary>
        /// <param name="left">The first quaternion to add.</param>
        /// <param name="right">The second quaternion to add.</param>
        /// <param name="result">When the method completes, contains the sum of the two quaternions.</param>
        public static void Add(ref Quaternion left, ref Quaternion right, out Quaternion result)
        {
            result.X = left.X + right.X;
            result.Y = left.Y + right.Y;
            result.Z = left.Z + right.Z;
            result.W = left.W + right.W;
        }

        /// <summary>
        /// Adds two quaternions.
        /// </summary>
        /// <param name="left">The first quaternion to add.</param>
        /// <param name="right">The second quaternion to add.</param>
        /// <returns>The sum of the two quaternions.</returns>
        public static Quaternion Add(Quaternion left, Quaternion right)
        {
            Add(ref left, ref right, out var result);
            return result;
        }

        /// <summary>
        /// Subtracts two quaternions.
        /// </summary>
        /// <param name="left">The first quaternion to subtract.</param>
        /// <param name="right">The second quaternion to subtract.</param>
        /// <param name="result">When the method completes, contains the difference of the two quaternions.</param>
        public static void Subtract(ref Quaternion left, ref Quaternion right, out Quaternion result)
        {
            result.X = left.X - right.X;
            result.Y = left.Y - right.Y;
            result.Z = left.Z - right.Z;
            result.W = left.W - right.W;
        }

        /// <summary>
        /// Subtracts two quaternions.
        /// </summary>
        /// <param name="left">The first quaternion to subtract.</param>
        /// <param name="right">The second quaternion to subtract.</param>
        /// <returns>The difference of the two quaternions.</returns>
        public static Quaternion Subtract(Quaternion left, Quaternion right)
        {
            Subtract(ref left, ref right, out var result);
            return result;
        }

        /// <summary>
        /// Scales a quaternion by the given value.
        /// </summary>
        /// <param name="value">The quaternion to scale.</param>
        /// <param name="scale">The amount by which to scale the quaternion.</param>
        /// <param name="result">When the method completes, contains the scaled quaternion.</param>
        public static void Multiply(ref Quaternion value, float scale, out Quaternion result)
        {
            result.X = value.X * scale;
            result.Y = value.Y * scale;
            result.Z = value.Z * scale;
            result.W = value.W * scale;
        }

        /// <summary>
        /// Scales a quaternion by the given value.
        /// </summary>
        /// <param name="value">The quaternion to scale.</param>
        /// <param name="scale">The amount by which to scale the quaternion.</param>
        /// <returns>The scaled quaternion.</returns>
        public static Quaternion Multiply(Quaternion value, float scale)
        {
            Multiply(ref value, scale, out var result);
            return result;
        }

        /// <summary>
        /// Multiplies a quaternion by another.
        /// </summary>
        /// <param name="left">The first quaternion to multiply.</param>
        /// <param name="right">The second quaternion to multiply.</param>
        /// <param name="result">When the method completes, contains the multiplied quaternion.</param>
        public static void Multiply(ref Quaternion left, ref Quaternion right, out Quaternion result)
        {
            float a = left.Y * right.Z - left.Z * right.Y;
            float b = left.Z * right.X - left.X * right.Z;
            float c = left.X * right.Y - left.Y * right.X;
            float d = left.X * right.X + left.Y * right.Y + left.Z * right.Z;
            result.X = left.X * right.W + right.X * left.W + a;
            result.Y = left.Y * right.W + right.Y * left.W + b;
            result.Z = left.Z * right.W + right.Z * left.W + c;
            result.W = left.W * right.W - d;
        }

        /// <summary>
        /// Multiplies a quaternion by another.
        /// </summary>
        /// <param name="left">The first quaternion to multiply.</param>
        /// <param name="right">The second quaternion to multiply.</param>
        /// <returns>The multiplied quaternion.</returns>
        public static Quaternion Multiply(Quaternion left, Quaternion right)
        {
            Multiply(ref left, ref right, out var result);
            return result;
        }

        /// <summary>
        /// Reverses the direction of a given quaternion.
        /// </summary>
        /// <param name="value">The quaternion to negate.</param>
        /// <param name="result">When the method completes, contains a quaternion facing in the opposite direction.</param>
        public static void Negate(ref Quaternion value, out Quaternion result)
        {
            result.X = -value.X;
            result.Y = -value.Y;
            result.Z = -value.Z;
            result.W = -value.W;
        }

        /// <summary>
        /// Reverses the direction of a given quaternion.
        /// </summary>
        /// <param name="value">The quaternion to negate.</param>
        /// <returns>A quaternion facing in the opposite direction.</returns>
        public static Quaternion Negate(Quaternion value)
        {
            Negate(ref value, out var result);
            return result;
        }

        /// <summary>
        /// Returns a <see cref="Quaternion" /> containing the 4D Cartesian coordinates of a point specified in Barycentric coordinates relative to a 2D triangle.
        /// </summary>
        /// <param name="value1">A <see cref="Quaternion" /> containing the 4D Cartesian coordinates of vertex 1 of the triangle.</param>
        /// <param name="value2">A <see cref="Quaternion" /> containing the 4D Cartesian coordinates of vertex 2 of the triangle.</param>
        /// <param name="value3">A <see cref="Quaternion" /> containing the 4D Cartesian coordinates of vertex 3 of the triangle.</param>
        /// <param name="amount1">Barycentric coordinate b2, which expresses the weighting factor toward vertex 2 (specified in <paramref name="value2" />).</param>
        /// <param name="amount2">Barycentric coordinate b3, which expresses the weighting factor toward vertex 3 (specified in <paramref name="value3" />).</param>
        /// <param name="result">When the method completes, contains a new <see cref="Quaternion" /> containing the 4D Cartesian coordinates of the specified point.</param>
        public static void Barycentric(ref Quaternion value1, ref Quaternion value2, ref Quaternion value3, float amount1, float amount2, out Quaternion result)
        {
            Slerp(ref value1, ref value2, amount1 + amount2, out var start);
            Slerp(ref value1, ref value3, amount1 + amount2, out var end);
            Slerp(ref start, ref end, amount2 / (amount1 + amount2), out result);
        }

        /// <summary>
        /// Returns a <see cref="Quaternion" /> containing the 4D Cartesian coordinates of a point specified in Barycentric coordinates relative to a 2D triangle.
        /// </summary>
        /// <param name="value1">A <see cref="Quaternion" /> containing the 4D Cartesian coordinates of vertex 1 of the triangle.</param>
        /// <param name="value2">A <see cref="Quaternion" /> containing the 4D Cartesian coordinates of vertex 2 of the triangle.</param>
        /// <param name="value3">A <see cref="Quaternion" /> containing the 4D Cartesian coordinates of vertex 3 of the triangle.</param>
        /// <param name="amount1">Barycentric coordinate b2, which expresses the weighting factor toward vertex 2 (specified in <paramref name="value2" />).</param>
        /// <param name="amount2">Barycentric coordinate b3, which expresses the weighting factor toward vertex 3 (specified in <paramref name="value3" />).</param>
        /// <returns>A new <see cref="Quaternion" /> containing the 4D Cartesian coordinates of the specified point.</returns>
        public static Quaternion Barycentric(Quaternion value1, Quaternion value2, Quaternion value3, float amount1, float amount2)
        {
            Barycentric(ref value1, ref value2, ref value3, amount1, amount2, out var result);
            return result;
        }

        /// <summary>
        /// Conjugates a quaternion.
        /// </summary>
        /// <param name="value">The quaternion to conjugate.</param>
        /// <param name="result">When the method completes, contains the conjugated quaternion.</param>
        public static void Conjugate(ref Quaternion value, out Quaternion result)
        {
            result.X = -value.X;
            result.Y = -value.Y;
            result.Z = -value.Z;
            result.W = value.W;
        }

        /// <summary>
        /// Conjugates a quaternion.
        /// </summary>
        /// <param name="value">The quaternion to conjugate.</param>
        /// <returns>The conjugated quaternion.</returns>
        public static Quaternion Conjugate(Quaternion value)
        {
            Conjugate(ref value, out var result);
            return result;
        }

        /// <summary>
        /// Calculates the dot product of two quaternions.
        /// </summary>
        /// <param name="left">First source quaternion.</param>
        /// <param name="right">Second source quaternion.</param>
        /// <returns>The dot product of the two quaternions.</returns>
        public static float Dot(ref Quaternion left, ref Quaternion right)
        {
            return left.X * right.X + left.Y * right.Y + left.Z * right.Z + left.W * right.W;
        }

        /// <summary>
        /// Calculates the dot product of two quaternions.
        /// </summary>
        /// <param name="left">First source quaternion.</param>
        /// <param name="right">Second source quaternion.</param>
        /// <returns>The dot product of the two quaternions.</returns>
        public static float Dot(Quaternion left, Quaternion right)
        {
            return left.X * right.X + left.Y * right.Y + left.Z * right.Z + left.W * right.W;
        }

        /// <summary>
        /// Calculates the angle between two quaternions.
        /// </summary>
        /// <param name="a">First source quaternion.</param>
        /// <param name="b">Second source quaternion.</param>
        /// <returns>Returns the angle in degrees between two rotations a and b.</returns>
        public static float AngleBetween(Quaternion a, Quaternion b)
        {
            float num = Dot(a, b);
            return num > Tolerance ? 0 : Mathf.Acos(Mathf.Min(Mathf.Abs(num), 1f)) * 2f * 57.29578f;
        }

        /// <summary>
        /// Exponentiates a quaternion.
        /// </summary>
        /// <param name="value">The quaternion to exponentiate.</param>
        /// <param name="result">When the method completes, contains the exponentiated quaternion.</param>
        public static void Exponential(ref Quaternion value, out Quaternion result)
        {
            var angle = (float)Math.Sqrt(value.X * value.X + value.Y * value.Y + value.Z * value.Z);
            var sin = (float)Math.Sin(angle);

            if (!Mathf.IsZero(sin))
            {
                float coeff = sin / angle;
                result.X = coeff * value.X;
                result.Y = coeff * value.Y;
                result.Z = coeff * value.Z;
            }
            else
            {
                result = value;
            }

            result.W = (float)Math.Cos(angle);
        }

        /// <summary>
        /// Exponentiates a quaternion.
        /// </summary>
        /// <param name="value">The quaternion to exponentiate.</param>
        /// <returns>The exponentiated quaternion.</returns>
        public static Quaternion Exponential(Quaternion value)
        {
            Exponential(ref value, out var result);
            return result;
        }

        /// <summary>
        /// Conjugates and renormalizes the quaternion.
        /// </summary>
        /// <param name="value">The quaternion to conjugate and renormalize.</param>
        /// <param name="result">When the method completes, contains the conjugated and renormalized quaternion.</param>
        public static void Invert(ref Quaternion value, out Quaternion result)
        {
            result = value;
            result.Invert();
        }

        /// <summary>
        /// Conjugates and renormalizes the quaternion.
        /// </summary>
        /// <param name="value">The quaternion to conjugate and renormalize.</param>
        /// <returns>The conjugated and renormalized quaternion.</returns>
        public static Quaternion Invert(Quaternion value)
        {
            var result = value;
            result.Invert();
            return result;
        }

        /// <summary>
        /// Calculates the orientation from the direction vector.
        /// </summary>
        /// <param name="direction">The direction vector (normalized).</param>
        /// <returns>The orientation.</returns>
        public static Quaternion FromDirection(Float3 direction)
        {
            Quaternion orientation;
            if (Float3.Dot(direction, Float3.Up) >= 0.999f)
            {
                orientation = RotationAxis(Float3.Left, Mathf.PiOverTwo);
            }
            else if (Float3.Dot(direction, Float3.Down) >= 0.999f)
            {
                orientation = RotationAxis(Float3.Right, Mathf.PiOverTwo);
            }
            else
            {
                var right = Float3.Cross(direction, Float3.Up);
                var up = Float3.Cross(right, direction);
                orientation = LookRotation(direction, up);
            }
            return orientation;
        }

        /// <summary>
        /// Performs a linear interpolation between two quaternions.
        /// </summary>
        /// <param name="start">Start quaternion.</param>
        /// <param name="end">End quaternion.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end" />.</param>
        /// <param name="result">When the method completes, contains the linear interpolation of the two quaternions.</param>
        /// <remarks>
        /// This method performs the linear interpolation based on the following formula.
        /// <code>start + (end - start) * amount</code>
        /// Passing <paramref name="amount" /> a value of 0 will cause <paramref name="start" /> to be returned; a value of 1
        /// will cause <paramref name="end" /> to be returned.
        /// </remarks>
        public static void Lerp(ref Quaternion start, ref Quaternion end, float amount, out Quaternion result)
        {
            float inverse = 1.0f - amount;

            if (Dot(start, end) >= 0.0f)
            {
                result.X = inverse * start.X + amount * end.X;
                result.Y = inverse * start.Y + amount * end.Y;
                result.Z = inverse * start.Z + amount * end.Z;
                result.W = inverse * start.W + amount * end.W;
            }
            else
            {
                result.X = inverse * start.X - amount * end.X;
                result.Y = inverse * start.Y - amount * end.Y;
                result.Z = inverse * start.Z - amount * end.Z;
                result.W = inverse * start.W - amount * end.W;
            }

            result.Normalize();
        }

        /// <summary>
        /// Performs a linear interpolation between two quaternion.
        /// </summary>
        /// <param name="start">Start quaternion.</param>
        /// <param name="end">End quaternion.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end" />.</param>
        /// <returns>The linear interpolation of the two quaternions.</returns>
        /// <remarks>
        /// This method performs the linear interpolation based on the following formula.
        /// <code>start + (end - start) * amount</code>
        /// Passing <paramref name="amount" /> a value of 0 will cause <paramref name="start" /> to be returned; a value of 1
        /// will cause <paramref name="end" /> to be returned.
        /// </remarks>
        public static Quaternion Lerp(Quaternion start, Quaternion end, float amount)
        {
            Lerp(ref start, ref end, amount, out var result);
            return result;
        }

        /// <summary>
        /// Calculates the natural logarithm of the specified quaternion.
        /// </summary>
        /// <param name="value">The quaternion whose logarithm will be calculated.</param>
        /// <param name="result">When the method completes, contains the natural logarithm of the quaternion.</param>
        public static void Logarithm(ref Quaternion value, out Quaternion result)
        {
            if (Math.Abs(value.W) < 1.0)
            {
                var angle = (float)Math.Acos(value.W);
                var sin = (float)Math.Sin(angle);

                if (!Mathf.IsZero(sin))
                {
                    float coeff = angle / sin;
                    result.X = value.X * coeff;
                    result.Y = value.Y * coeff;
                    result.Z = value.Z * coeff;
                }
                else
                {
                    result = value;
                }
            }
            else
            {
                result = value;
            }

            result.W = 0.0f;
        }

        /// <summary>
        /// Calculates the natural logarithm of the specified quaternion.
        /// </summary>
        /// <param name="value">The quaternion whose logarithm will be calculated.</param>
        /// <returns>The natural logarithm of the quaternion.</returns>
        public static Quaternion Logarithm(Quaternion value)
        {
            Logarithm(ref value, out var result);
            return result;
        }

        /// <summary>
        /// Converts the quaternion into a unit quaternion.
        /// </summary>
        /// <param name="value">The quaternion to normalize.</param>
        /// <param name="result">When the method completes, contains the normalized quaternion.</param>
        public static void Normalize(ref Quaternion value, out Quaternion result)
        {
            Quaternion temp = value;
            result = temp;
            result.Normalize();
        }

        /// <summary>
        /// Converts the quaternion into a unit quaternion.
        /// </summary>
        /// <param name="value">The quaternion to normalize.</param>
        /// <returns>The normalized quaternion.</returns>
        public static Quaternion Normalize(Quaternion value)
        {
            value.Normalize();
            return value;
        }

        /// <summary>
        /// Creates a quaternion given a rotation and an axis.
        /// </summary>
        /// <param name="axis">The axis of rotation.</param>
        /// <param name="angle">The angle of rotation (in radians).</param>
        /// <param name="result">When the method completes, contains the newly created quaternion.</param>
        public static void RotationAxis(ref Float3 axis, float angle, out Quaternion result)
        {
            Float3.Normalize(ref axis, out var normalized);

            float half = angle * 0.5f;
            var sin = (float)Math.Sin(half);
            var cos = (float)Math.Cos(half);

            result.X = normalized.X * sin;
            result.Y = normalized.Y * sin;
            result.Z = normalized.Z * sin;
            result.W = cos;
        }

        /// <summary>
        /// Creates a quaternion given a rotation and an axis.
        /// </summary>
        /// <param name="axis">The axis of rotation.</param>
        /// <param name="angle">The angle of rotation (in radians).</param>
        /// <returns>The newly created quaternion.</returns>
        public static Quaternion RotationAxis(Float3 axis, float angle)
        {
            RotationAxis(ref axis, angle, out var result);
            return result;
        }

        /// <summary>
        /// Creates a quaternion given a rotation matrix.
        /// </summary>
        /// <param name="matrix">The rotation matrix.</param>
        /// <param name="result">When the method completes, contains the newly created quaternion.</param>
        public static void RotationMatrix(ref Matrix matrix, out Quaternion result)
        {
            float sqrt;
            float half;
            float scale = matrix.M11 + matrix.M22 + matrix.M33;

            if (scale > 0.0f)
            {
                sqrt = (float)Math.Sqrt(scale + 1.0f);
                result.W = sqrt * 0.5f;
                sqrt = 0.5f / sqrt;

                result.X = (matrix.M23 - matrix.M32) * sqrt;
                result.Y = (matrix.M31 - matrix.M13) * sqrt;
                result.Z = (matrix.M12 - matrix.M21) * sqrt;
            }
            else if (matrix.M11 >= matrix.M22 && matrix.M11 >= matrix.M33)
            {
                sqrt = (float)Math.Sqrt(1.0f + matrix.M11 - matrix.M22 - matrix.M33);
                half = 0.5f / sqrt;

                result.X = 0.5f * sqrt;
                result.Y = (matrix.M12 + matrix.M21) * half;
                result.Z = (matrix.M13 + matrix.M31) * half;
                result.W = (matrix.M23 - matrix.M32) * half;
            }
            else if (matrix.M22 > matrix.M33)
            {
                sqrt = (float)Math.Sqrt(1.0f + matrix.M22 - matrix.M11 - matrix.M33);
                half = 0.5f / sqrt;

                result.X = (matrix.M21 + matrix.M12) * half;
                result.Y = 0.5f * sqrt;
                result.Z = (matrix.M32 + matrix.M23) * half;
                result.W = (matrix.M31 - matrix.M13) * half;
            }
            else
            {
                sqrt = (float)Math.Sqrt(1.0f + matrix.M33 - matrix.M11 - matrix.M22);
                half = 0.5f / sqrt;

                result.X = (matrix.M31 + matrix.M13) * half;
                result.Y = (matrix.M32 + matrix.M23) * half;
                result.Z = 0.5f * sqrt;
                result.W = (matrix.M12 - matrix.M21) * half;
            }
        }

        /// <summary>
        /// Creates a quaternion given a rotation matrix.
        /// </summary>
        /// <param name="matrix">The rotation matrix.</param>
        /// <param name="result">When the method completes, contains the newly created quaternion.</param>
        public static void RotationMatrix(ref Matrix3x3 matrix, out Quaternion result)
        {
            float sqrt;
            float half;
            float scale = matrix.M11 + matrix.M22 + matrix.M33;

            if (scale > 0.0f)
            {
                sqrt = (float)Math.Sqrt(scale + 1.0f);
                result.W = sqrt * 0.5f;
                sqrt = 0.5f / sqrt;

                result.X = (matrix.M23 - matrix.M32) * sqrt;
                result.Y = (matrix.M31 - matrix.M13) * sqrt;
                result.Z = (matrix.M12 - matrix.M21) * sqrt;
            }
            else if (matrix.M11 >= matrix.M22 && matrix.M11 >= matrix.M33)
            {
                sqrt = (float)Math.Sqrt(1.0f + matrix.M11 - matrix.M22 - matrix.M33);
                half = 0.5f / sqrt;

                result.X = 0.5f * sqrt;
                result.Y = (matrix.M12 + matrix.M21) * half;
                result.Z = (matrix.M13 + matrix.M31) * half;
                result.W = (matrix.M23 - matrix.M32) * half;
            }
            else if (matrix.M22 > matrix.M33)
            {
                sqrt = (float)Math.Sqrt(1.0f + matrix.M22 - matrix.M11 - matrix.M33);
                half = 0.5f / sqrt;

                result.X = (matrix.M21 + matrix.M12) * half;
                result.Y = 0.5f * sqrt;
                result.Z = (matrix.M32 + matrix.M23) * half;
                result.W = (matrix.M31 - matrix.M13) * half;
            }
            else
            {
                sqrt = (float)Math.Sqrt(1.0f + matrix.M33 - matrix.M11 - matrix.M22);
                half = 0.5f / sqrt;

                result.X = (matrix.M31 + matrix.M13) * half;
                result.Y = (matrix.M32 + matrix.M23) * half;
                result.Z = 0.5f * sqrt;
                result.W = (matrix.M12 - matrix.M21) * half;
            }
        }

        /// <summary>
        /// Creates a left-handed, look-at quaternion.
        /// </summary>
        /// <param name="eye">The position of the viewer's eye.</param>
        /// <param name="target">The camera look-at target.</param>
        /// <param name="up">The camera's up vector.</param>
        /// <param name="result">When the method completes, contains the created look-at quaternion.</param>
        public static void LookAt(ref Float3 eye, ref Float3 target, ref Float3 up, out Quaternion result)
        {
            Matrix3x3.LookAt(ref eye, ref target, ref up, out var matrix);
            RotationMatrix(ref matrix, out result);
        }

        /// <summary>
        /// Creates a left-handed, look-at quaternion.
        /// </summary>
        /// <param name="eye">The position of the viewer's eye.</param>
        /// <param name="target">The camera look-at target.</param>
        /// <returns>The created look-at quaternion.</returns>
        public static Quaternion LookAt(Float3 eye, Float3 target)
        {
            return LookAt(eye, target, Float3.Up);
        }

        /// <summary>
        /// Creates a left-handed, look-at quaternion.
        /// </summary>
        /// <param name="eye">The position of the viewer's eye.</param>
        /// <param name="target">The camera look-at target.</param>
        /// <param name="up">The camera's up vector.</param>
        /// <returns>The created look-at quaternion.</returns>
        public static Quaternion LookAt(Float3 eye, Float3 target, Float3 up)
        {
            LookAt(ref eye, ref target, ref up, out var result);
            return result;
        }

        /// <summary>
        /// Creates a left-handed, look-at quaternion.
        /// </summary>
        /// <param name="forward">The camera's forward direction.</param>
        /// <param name="up">The camera's up vector.</param>
        /// <param name="result">When the method completes, contains the created look-at quaternion.</param>
        public static void RotationLookAt(ref Float3 forward, ref Float3 up, out Quaternion result)
        {
            var eye = Float3.Zero;
            LookAt(ref eye, ref forward, ref up, out result);
        }

        /// <summary>
        /// Creates a left-handed, look-at quaternion.
        /// </summary>
        /// <param name="forward">The camera's forward direction.</param>
        /// <returns>The created look-at quaternion.</returns>
        public static Quaternion RotationLookAt(Float3 forward)
        {
            return RotationLookAt(forward, Float3.Up);
        }

        /// <summary>
        /// Creates a left-handed, look-at quaternion.
        /// </summary>
        /// <param name="forward">The camera's forward direction.</param>
        /// <param name="up">The camera's up vector.</param>
        /// <returns>The created look-at quaternion.</returns>
        public static Quaternion RotationLookAt(Float3 forward, Float3 up)
        {
            RotationLookAt(ref forward, ref up, out var result);
            return result;
        }

        /// <summary>
        /// Creates a rotation with the specified forward and upwards directions.
        /// </summary>
        /// <param name="forward">The forward direction. Direction to orient towards.</param>
        /// <returns>The calculated quaternion.</returns>
        public static Quaternion LookRotation(Float3 forward)
        {
            return LookRotation(forward, Float3.Up);
        }

        /// <summary>
        /// Creates a rotation with the specified forward and upwards directions.
        /// </summary>
        /// <param name="forward">The forward direction. Direction to orient towards.</param>
        /// <param name="up">Up direction. Constrains y axis orientation to a plane this vector lies on. This rule might be broken if forward and up direction are nearly parallel.</param>
        /// <returns>The calculated quaternion.</returns>
        public static Quaternion LookRotation(Float3 forward, Float3 up)
        {
            LookRotation(ref forward, ref up, out var result);
            return result;
        }

        /// <summary>
        /// Creates a rotation with the specified forward and upwards directions.
        /// </summary>
        /// <param name="forward">The forward direction. Direction to orient towards.</param>
        /// <param name="up">The up direction. Constrains y axis orientation to a plane this vector lies on. This rule might be broken if forward and up direction are nearly parallel.</param>
        /// <param name="result">The calculated quaternion.</param>
        public static void LookRotation(ref Float3 forward, ref Float3 up, out Quaternion result)
        {
            Float3 forwardNorm = forward;
            forwardNorm.Normalize();
            Float3.Cross(ref up, ref forwardNorm, out var rightNorm);
            rightNorm.Normalize();
            Float3.Cross(ref forwardNorm, ref rightNorm, out var upNorm);

            float m00 = rightNorm.X;
            float m01 = rightNorm.Y;
            float m02 = rightNorm.Z;
            float m10 = upNorm.X;
            float m11 = upNorm.Y;
            float m12 = upNorm.Z;
            float m20 = forwardNorm.X;
            float m21 = forwardNorm.Y;
            float m22 = forwardNorm.Z;

            float sum = m00 + m11 + m22;
            if (sum > 0)
            {
                float num = Mathf.Sqrt(sum + 1);
                float invNumHalf = 0.5f / num;
                result.X = (m12 - m21) * invNumHalf;
                result.Y = (m20 - m02) * invNumHalf;
                result.Z = (m01 - m10) * invNumHalf;
                result.W = num * 0.5f;
            }
            else if (m00 >= m11 && m00 >= m22)
            {
                float num = Mathf.Sqrt(1 + m00 - m11 - m22);
                float invNumHalf = 0.5f / num;
                result.X = 0.5f * num;
                result.Y = (m01 + m10) * invNumHalf;
                result.Z = (m02 + m20) * invNumHalf;
                result.W = (m12 - m21) * invNumHalf;
            }
            else if (m11 > m22)
            {
                float num = Mathf.Sqrt(1 + m11 - m00 - m22);
                float invNumHalf = 0.5f / num;
                result.X = (m10 + m01) * invNumHalf;
                result.Y = 0.5f * num;
                result.Z = (m21 + m12) * invNumHalf;
                result.W = (m20 - m02) * invNumHalf;
            }
            else
            {
                float num = Mathf.Sqrt(1 + m22 - m00 - m11);
                float invNumHalf = 0.5f / num;
                result.X = (m20 + m02) * invNumHalf;
                result.Y = (m21 + m12) * invNumHalf;
                result.Z = 0.5f * num;
                result.W = (m01 - m10) * invNumHalf;
            }
        }

        /// <summary>
        /// Gets the shortest arc quaternion to rotate this vector to the destination vector.
        /// </summary>
        /// <param name="from">The source vector.</param>
        /// <param name="to">The destination vector.</param>
        /// <param name="result">The result.</param>
        /// <param name="fallbackAxis">The fallback axis.</param>
        public static void GetRotationFromTo(ref Float3 from, ref Float3 to, out Quaternion result, ref Float3 fallbackAxis)
        {
            // Based on Stan Melax's article in Game Programming Gems

            Float3 v0 = from;
            Float3 v1 = to;
            v0.Normalize();
            v1.Normalize();

            // If dot == 1, vectors are the same
            float d = Float3.Dot(ref v0, ref v1);
            if (d >= 1.0f)
            {
                result = Identity;
                return;
            }

            if (d < 1e-6f - 1.0f)
            {
                if (fallbackAxis != Float3.Zero)
                {
                    // Rotate 180 degrees about the fallback axis
                    RotationAxis(ref fallbackAxis, Mathf.Pi, out result);
                }
                else
                {
                    // Generate an axis
                    Float3 axis = Float3.Cross(Float3.UnitX, from);
                    if (axis.LengthSquared < Mathf.Epsilon) // Pick another if colinear
                        axis = Float3.Cross(Float3.UnitY, from);
                    axis.Normalize();
                    RotationAxis(ref axis, Mathf.Pi, out result);
                }
            }
            else
            {
                float s = Mathf.Sqrt((1 + d) * 2);
                float invS = 1 / s;
                Float3.Cross(ref v0, ref v1, out var c);
                result.X = c.X * invS;
                result.Y = c.Y * invS;
                result.Z = c.Z * invS;
                result.W = s * 0.5f;
                result.Normalize();
            }
        }

        /// <summary>
        /// Gets the shortest arc quaternion to rotate this vector to the destination vector.
        /// </summary>
        /// <param name="from">The source vector.</param>
        /// <param name="to">The destination vector.</param>
        /// <param name="fallbackAxis">The fallback axis.</param>
        /// <returns>The rotation.</returns>
        public static Quaternion GetRotationFromTo(Float3 from, Float3 to, Float3 fallbackAxis)
        {
            GetRotationFromTo(ref from, ref to, out var result, ref fallbackAxis);
            return result;
        }

        /// <summary>
        /// Gets the quaternion that will rotate vector from into vector to, around their plan perpendicular axis. The input vectors don't need to be normalized.
        /// </summary>
        /// <param name="from">The source vector.</param>
        /// <param name="to">The destination vector.</param>
        /// <param name="result">The result.</param>
        public static void FindBetween(ref Float3 from, ref Float3 to, out Quaternion result)
        {
            // http://lolengine.net/blog/2014/02/24/quaternion-from-two-vectors-final
            float normFromNormTo = Mathf.Sqrt(from.LengthSquared * to.LengthSquared);
            if (normFromNormTo < Mathf.Epsilon)
            {
                result = Identity;
                return;
            }
            float w = normFromNormTo + Float3.Dot(from, to);
            if (w < 1.0-6f * normFromNormTo)
            {
                result = Mathf.Abs(from.X) > Mathf.Abs(from.Z)
                         ? new Quaternion(-from.Y, from.X, 0.0f, 0.0f)
                         : new Quaternion(0.0f, -from.Z, from.Y, 0.0f);
            }
            else
            {
                Float3 cross = Float3.Cross(from, to);
                result = new Quaternion(cross.X, cross.Y, cross.Z, w);
            }
            result.Normalize();
        }

        /// <summary>
        /// Gets the quaternion that will rotate the from into vector to, around their plan perpendicular axis. The input vectors don't need to be normalized.
        /// </summary>
        /// <param name="from">The source vector.</param>
        /// <param name="to">The destination vector.</param>
        /// <returns>The rotation.</returns>
        public static Quaternion FindBetween(Float3 from, Float3 to)
        {
            FindBetween(ref from, ref to, out var result);
            return result;
        }

        /// <summary>
        /// Creates a left-handed spherical billboard that rotates around a specified object position.
        /// </summary>
        /// <param name="objectPosition">The position of the object around which the billboard will rotate.</param>
        /// <param name="cameraPosition">The position of the camera.</param>
        /// <param name="cameraUpVector">The up vector of the camera.</param>
        /// <param name="cameraForwardVector">The forward vector of the camera.</param>
        /// <param name="result">When the method completes, contains the created billboard quaternion.</param>
        public static void Billboard(ref Float3 objectPosition, ref Float3 cameraPosition, ref Float3 cameraUpVector, ref Float3 cameraForwardVector, out Quaternion result)
        {
            Matrix3x3.Billboard(ref objectPosition, ref cameraPosition, ref cameraUpVector, ref cameraForwardVector, out var matrix);
            RotationMatrix(ref matrix, out result);
        }

        /// <summary>
        /// Creates a left-handed spherical billboard that rotates around a specified object position.
        /// </summary>
        /// <param name="objectPosition">The position of the object around which the billboard will rotate.</param>
        /// <param name="cameraPosition">The position of the camera.</param>
        /// <param name="cameraUpVector">The up vector of the camera.</param>
        /// <param name="cameraForwardVector">The forward vector of the camera.</param>
        /// <returns>The created billboard quaternion.</returns>
        public static Quaternion Billboard(Float3 objectPosition, Float3 cameraPosition, Float3 cameraUpVector, Float3 cameraForwardVector)
        {
            Billboard(ref objectPosition, ref cameraPosition, ref cameraUpVector, ref cameraForwardVector, out var result);
            return result;
        }

        /// <summary>
        /// Creates a quaternion given a rotation matrix.
        /// </summary>
        /// <param name="matrix">The rotation matrix.</param>
        /// <returns>The newly created quaternion.</returns>
        public static Quaternion RotationMatrix(Matrix matrix)
        {
            RotationMatrix(ref matrix, out var result);
            return result;
        }

        /// <summary>
        /// Creates a quaternion that rotates around the x-axis.
        /// </summary>
        /// <param name="angle">Angle of rotation in radians.</param>
        /// <param name="result">When the method completes, contains the newly created quaternion.</param>
        public static void RotationX(float angle, out Quaternion result)
        {
            float halfAngle = angle * 0.5f;
            result = new Quaternion((float)Math.Sin(halfAngle), 0.0f, 0.0f, (float)Math.Cos(halfAngle));
        }

        /// <summary>
        /// Creates a quaternion that rotates around the x-axis.
        /// </summary>
        /// <param name="angle">Angle of rotation in radians.</param>
        /// <returns>The created rotation quaternion.</returns>
        public static Quaternion RotationX(float angle)
        {
            RotationX(angle, out var result);
            return result;
        }

        /// <summary>
        /// Creates a quaternion that rotates around the y-axis.
        /// </summary>
        /// <param name="angle">Angle of rotation in radians.</param>
        /// <param name="result">When the method completes, contains the newly created quaternion.</param>
        public static void RotationY(float angle, out Quaternion result)
        {
            float halfAngle = angle * 0.5f;
            result = new Quaternion(0.0f, (float)Math.Sin(halfAngle), 0.0f, (float)Math.Cos(halfAngle));
        }

        /// <summary>
        /// Creates a quaternion that rotates around the y-axis.
        /// </summary>
        /// <param name="angle">Angle of rotation in radians.</param>
        /// <returns>The created rotation quaternion.</returns>
        public static Quaternion RotationY(float angle)
        {
            RotationY(angle, out var result);
            return result;
        }

        /// <summary>
        /// Creates a quaternion that rotates around the z-axis.
        /// </summary>
        /// <param name="angle">Angle of rotation in radians.</param>
        /// <param name="result">When the method completes, contains the newly created quaternion.</param>
        public static void RotationZ(float angle, out Quaternion result)
        {
            float halfAngle = angle * 0.5f;
            result = new Quaternion(0.0f, 0.0f, (float)Math.Sin(halfAngle), (float)Math.Cos(halfAngle));
        }

        /// <summary>
        /// Creates a quaternion that rotates around the z-axis.
        /// </summary>
        /// <param name="angle">Angle of rotation in radians.</param>
        /// <returns>The created rotation quaternion.</returns>
        public static Quaternion RotationZ(float angle)
        {
            RotationZ(angle, out var result);
            return result;
        }

        /// <summary>
        /// Creates a quaternion given a pitch, yaw and roll values. Angles are in degrees.
        /// </summary>
        /// <param name="eulerAngles">The pitch, yaw and roll angles of rotation.</param>
        /// <returns>When the method completes, contains the newly created quaternion.</returns>
        public static Quaternion Euler(Float3 eulerAngles)
        {
            RotationYawPitchRoll(eulerAngles.Y * Mathf.DegreesToRadians, eulerAngles.X * Mathf.DegreesToRadians, eulerAngles.Z * Mathf.DegreesToRadians, out var result);
            return result;
        }

        /// <summary>
        /// Creates a quaternion given a pitch, yaw and roll values. Angles are in degrees.
        /// </summary>
        /// <param name="eulerAngles">The pitch, yaw and roll angles of rotation.</param>
        /// <param name="result">When the method completes, contains the newly created quaternion.</param>
        public static void Euler(ref Float3 eulerAngles, out Quaternion result)
        {
            RotationYawPitchRoll(eulerAngles.Y * Mathf.DegreesToRadians, eulerAngles.X * Mathf.DegreesToRadians, eulerAngles.Z * Mathf.DegreesToRadians, out result);
        }

        /// <summary>
        /// Creates a quaternion given a pitch, yaw and roll values. Angles are in degrees.
        /// </summary>
        /// <param name="x">The pitch of rotation (in degrees).</param>
        /// <param name="y">The yaw of rotation (in degrees).</param>
        /// <param name="z">The roll of rotation (in degrees).</param>
        /// <returns>When the method completes, contains the newly created quaternion.</returns>
        public static Quaternion Euler(float x, float y, float z)
        {
            RotationYawPitchRoll(y * Mathf.DegreesToRadians, x * Mathf.DegreesToRadians, z * Mathf.DegreesToRadians, out var result);
            return result;
        }

        /// <summary>
        /// Creates a quaternion given a pitch, yaw and roll values. Angles are in degrees.
        /// </summary>
        /// <param name="x">The pitch of rotation (in degrees).</param>
        /// <param name="y">The yaw of rotation (in degrees).</param>
        /// <param name="z">The roll of rotation (in degrees).</param>
        /// <param name="result">When the method completes, contains the newly created quaternion.</param>
        public static void Euler(float x, float y, float z, out Quaternion result)
        {
            RotationYawPitchRoll(y * Mathf.DegreesToRadians, x * Mathf.DegreesToRadians, z * Mathf.DegreesToRadians, out result);
        }

        /// <summary>
        /// Creates a quaternion given a yaw, pitch, and roll value. Angles are in radians. Use <see cref="Mathf.RadiansToDegrees"/> to convert degrees to radians.
        /// </summary>
        /// <param name="yaw">The yaw of rotation (in radians).</param>
        /// <param name="pitch">The pitch of rotation (in radians).</param>
        /// <param name="roll">The roll of rotation (in radians).</param>
        /// <param name="result">When the method completes, contains the newly created quaternion.</param>
        public static void RotationYawPitchRoll(float yaw, float pitch, float roll, out Quaternion result)
        {
            float halfRoll = roll * 0.5f;
            float halfPitch = pitch * 0.5f;
            float halfYaw = yaw * 0.5f;

            var sinRoll = (float)Math.Sin(halfRoll);
            var cosRoll = (float)Math.Cos(halfRoll);
            var sinPitch = (float)Math.Sin(halfPitch);
            var cosPitch = (float)Math.Cos(halfPitch);
            var sinYaw = (float)Math.Sin(halfYaw);
            var cosYaw = (float)Math.Cos(halfYaw);

            result.X = cosYaw * sinPitch * cosRoll + sinYaw * cosPitch * sinRoll;
            result.Y = sinYaw * cosPitch * cosRoll - cosYaw * sinPitch * sinRoll;
            result.Z = cosYaw * cosPitch * sinRoll - sinYaw * sinPitch * cosRoll;
            result.W = cosYaw * cosPitch * cosRoll + sinYaw * sinPitch * sinRoll;
        }

        /// <summary>
        /// Creates a quaternion given a yaw, pitch, and roll value. Angles are in radians.
        /// </summary>
        /// <param name="yaw">The yaw of rotation (in radians).</param>
        /// <param name="pitch">The pitch of rotation (in radians).</param>
        /// <param name="roll">The roll of rotation (in radians).</param>
        /// <returns>The newly created quaternion (in radians).</returns>
        public static Quaternion RotationYawPitchRoll(float yaw, float pitch, float roll)
        {
            RotationYawPitchRoll(yaw, pitch, roll, out var result);
            return result;
        }

        /// <summary>
        /// Interpolates between two quaternions, using spherical linear interpolation.
        /// </summary>
        /// <param name="start">Start quaternion.</param>
        /// <param name="end">End quaternion.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end" />.</param>
        /// <param name="result">When the method completes, contains the spherical linear interpolation of the two quaternions.</param>
        public static void Slerp(ref Quaternion start, ref Quaternion end, float amount, out Quaternion result)
        {
            float opposite;
            float inverse;
            float dot = Dot(start, end);

            if (Math.Abs(dot) > 1.0f - Mathf.Epsilon)
            {
                inverse = 1.0f - amount;
                opposite = amount * Math.Sign(dot);
            }
            else
            {
                var acos = (float)Math.Acos(Math.Abs(dot));
                var invSin = (float)(1.0 / Math.Sin(acos));

                inverse = (float)Math.Sin((1.0f - amount) * acos) * invSin;
                opposite = (float)Math.Sin(amount * acos) * invSin * Math.Sign(dot);
            }

            result.X = inverse * start.X + opposite * end.X;
            result.Y = inverse * start.Y + opposite * end.Y;
            result.Z = inverse * start.Z + opposite * end.Z;
            result.W = inverse * start.W + opposite * end.W;
        }

        /// <summary>
        /// Interpolates between two quaternions, using spherical linear interpolation.
        /// </summary>
        /// <param name="start">Start quaternion.</param>
        /// <param name="end">End quaternion.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end" />.</param>
        /// <returns>The spherical linear interpolation of the two quaternions.</returns>
        public static Quaternion Slerp(Quaternion start, Quaternion end, float amount)
        {
            Slerp(ref start, ref end, amount, out var result);
            return result;
        }

        /// <summary>
        /// Interpolates between quaternions, using spherical quadrangle interpolation.
        /// </summary>
        /// <param name="value1">First source quaternion.</param>
        /// <param name="value2">Second source quaternion.</param>
        /// <param name="value3">Third source quaternion.</param>
        /// <param name="value4">Fourth source quaternion.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of interpolation.</param>
        /// <param name="result">When the method completes, contains the spherical quadrangle interpolation of the quaternions.</param>
        public static void Squad(ref Quaternion value1, ref Quaternion value2, ref Quaternion value3, ref Quaternion value4, float amount, out Quaternion result)
        {
            Slerp(ref value1, ref value4, amount, out var start);
            Slerp(ref value2, ref value3, amount, out var end);
            Slerp(ref start, ref end, 2.0f * amount * (1.0f - amount), out result);
        }

        /// <summary>
        /// Interpolates between quaternions, using spherical quadrangle interpolation.
        /// </summary>
        /// <param name="value1">First source quaternion.</param>
        /// <param name="value2">Second source quaternion.</param>
        /// <param name="value3">Third source quaternion.</param>
        /// <param name="value4">Fourth source quaternion.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of interpolation.</param>
        /// <returns>The spherical quadrangle interpolation of the quaternions.</returns>
        public static Quaternion Squad(Quaternion value1, Quaternion value2, Quaternion value3, Quaternion value4, float amount)
        {
            Squad(ref value1, ref value2, ref value3, ref value4, amount, out var result);
            return result;
        }

        /// <summary>
        /// Sets up control points for spherical quadrangle interpolation.
        /// </summary>
        /// <param name="value1">First source quaternion.</param>
        /// <param name="value2">Second source quaternion.</param>
        /// <param name="value3">Third source quaternion.</param>
        /// <param name="value4">Fourth source quaternion.</param>
        /// <returns>An array of three quaternions that represent control points for spherical quadrangle interpolation.</returns>
        public static Quaternion[] SquadSetup(Quaternion value1, Quaternion value2, Quaternion value3, Quaternion value4)
        {
            Quaternion q0 = (value1 + value2).LengthSquared < (value1 - value2).LengthSquared ? -value1 : value1;
            Quaternion q2 = (value2 + value3).LengthSquared < (value2 - value3).LengthSquared ? -value3 : value3;
            Quaternion q3 = (value3 + value4).LengthSquared < (value3 - value4).LengthSquared ? -value4 : value4;
            Quaternion q1 = value2;

            Exponential(ref q1, out var q1Exp);
            Exponential(ref q2, out var q2Exp);

            var results = new Quaternion[3];
            results[0] = q1 * Exponential(-0.25f * (Logarithm(q1Exp * q2) + Logarithm(q1Exp * q0)));
            results[1] = q2 * Exponential(-0.25f * (Logarithm(q2Exp * q3) + Logarithm(q2Exp * q1)));
            results[2] = q2;

            return results;
        }

        /// <summary>
        /// Gets rotation from a normal in relation to a transform.<br/>
        /// This function is especially useful for axis aligned faces,
        /// and with <seealso cref="Physics.RayCast(Vector3, Vector3, out RayCastHit, float, uint, bool)"/>.
        /// 
        /// <example><para><b>Example code:</b></para>
        /// <code>
        /// <see langword="public" /> <see langword="class" /> GetRotationFromNormalExample : <see cref="Script"/><br/>
        ///     <see langword="public" /> <see cref="Actor"/> RayOrigin;<br/>
        ///     <see langword="public" /> <see cref="Actor"/> SomeObject;<br/>
        ///     <see langword="public" /> <see langword="override" /> <see langword="void" /> <see cref="Script.OnFixedUpdate"/><br/>
        ///     {<br/>
        ///         <see langword="if" /> (<see cref="Physics"/>.RayCast(RayOrigin.Position, RayOrigin.Transform.Forward, out <see cref="RayCastHit"/> hit)
        ///         {<br/>
        ///             <see cref="Vector3"/> position = hit.Collider.Position;
        ///             <see cref="Transform"/> transform = hit.Collider.Transform;
        ///             <see cref="Vector3"/> point = hit.Point;
        ///             <see cref="Vector3"/> normal = hit.Normal;
        ///             <see cref="Quaternion"/> rot = <see cref="Quaternion"/>.GetRotationFromNormal(normal,transform);
        ///             SomeObject.Position = point;
        ///             SomeObject.Orientation = rot;
        ///         }
        ///     }
        /// }
        /// </code>
        /// </example>
        /// </summary>
        /// <param name="normal">The normal vector.</param>
        /// <param name="reference">The reference transform.</param>
        /// <returns>The rotation from the normal vector.</returns>
        public static Quaternion GetRotationFromNormal(Vector3 normal, Transform reference)
        {
            Float3 up = reference.Up;
            var dot = Vector3.Dot(normal, up);
            if (Mathf.NearEqual(Math.Abs(dot), 1))
                up = reference.Right;
            return LookRotation(normal, up);
        }

        /// <summary>
        /// Adds two quaternions.
        /// </summary>
        /// <param name="left">The first quaternion to add.</param>
        /// <param name="right">The second quaternion to add.</param>
        /// <returns>The sum of the two quaternions.</returns>
        public static Quaternion operator +(Quaternion left, Quaternion right)
        {
            Add(ref left, ref right, out var result);
            return result;
        }

        /// <summary>
        /// Subtracts two quaternions.
        /// </summary>
        /// <param name="left">The first quaternion to subtract.</param>
        /// <param name="right">The second quaternion to subtract.</param>
        /// <returns>The difference of the two quaternions.</returns>
        public static Quaternion operator -(Quaternion left, Quaternion right)
        {
            Subtract(ref left, ref right, out var result);
            return result;
        }

        /// <summary>
        /// Reverses the direction of a given quaternion.
        /// </summary>
        /// <param name="value">The quaternion to negate.</param>
        /// <returns>A quaternion facing in the opposite direction.</returns>
        public static Quaternion operator -(Quaternion value)
        {
            Negate(ref value, out var result);
            return result;
        }

        /// <summary>
        /// Scales a quaternion by the given value.
        /// </summary>
        /// <param name="value">The quaternion to scale.</param>
        /// <param name="scale">The amount by which to scale the quaternion.</param>
        /// <returns>The scaled quaternion.</returns>
        public static Quaternion operator *(float scale, Quaternion value)
        {
            Multiply(ref value, scale, out var result);
            return result;
        }

        /// <summary>
        /// Scales a quaternion by the given value.
        /// </summary>
        /// <param name="value">The quaternion to scale.</param>
        /// <param name="scale">The amount by which to scale the quaternion.</param>
        /// <returns>The scaled quaternion.</returns>
        public static Quaternion operator *(Quaternion value, float scale)
        {
            Multiply(ref value, scale, out var result);
            return result;
        }

        /// <summary>
        /// Multiplies a quaternion by another.
        /// </summary>
        /// <param name="left">The first quaternion to multiply.</param>
        /// <param name="right">The second quaternion to multiply.</param>
        /// <returns>The multiplied quaternion.</returns>
        public static Quaternion operator *(Quaternion left, Quaternion right)
        {
            Multiply(ref left, ref right, out var result);
            return result;
        }

        /// <summary>
        /// Tests for equality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left" /> has the same value as <paramref name="right" />; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator ==(Quaternion left, Quaternion right)
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
        public static bool operator !=(Quaternion left, Quaternion right)
        {
            return !left.Equals(ref right);
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public override string ToString()
        {
            return string.Format(CultureInfo.CurrentCulture, _formatString, X, Y, Z, W);
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

            return string.Format(CultureInfo.CurrentCulture, _formatString, X.ToString(format, CultureInfo.CurrentCulture),
                                 Y.ToString(format, CultureInfo.CurrentCulture), Z.ToString(format, CultureInfo.CurrentCulture), W.ToString(format, CultureInfo.CurrentCulture));
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <param name="formatProvider">The format provider.</param>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public string ToString(IFormatProvider formatProvider)
        {
            return string.Format(formatProvider, _formatString, X, Y, Z, W);
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

            return string.Format(formatProvider, _formatString, X.ToString(format, formatProvider),
                                 Y.ToString(format, formatProvider), Z.ToString(format, formatProvider), W.ToString(format, formatProvider));
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
                hashCode = (hashCode * 397) ^ W.GetHashCode();
                return hashCode;
            }
        }

        /// <summary>
        /// Tests whether one quaternion is near another quaternion.
        /// </summary>
        /// <param name="left">The left quaternion.</param>
        /// <param name="right">The right quaternion.</param>
        /// <param name="epsilon">The epsilon.</param>
        /// <returns><c>true</c> if left and right are near another, <c>false</c> otherwise</returns>
        public static bool NearEqual(Quaternion left, Quaternion right, float epsilon = Mathf.Epsilon)
        {
            return NearEqual(ref left, ref right, epsilon);
        }

        /// <summary>
        /// Tests whether one quaternion is near another quaternion.
        /// </summary>
        /// <param name="left">The left quaternion.</param>
        /// <param name="right">The right quaternion.</param>
        /// <param name="epsilon">The epsilon.</param>
        /// <returns><c>true</c> if left and right are near another, <c>false</c> otherwise</returns>
        public static bool NearEqual(ref Quaternion left, ref Quaternion right, float epsilon = Mathf.Epsilon)
        {
            //return Dot(ref left, ref right) > 1.0f - epsilon;
            return Mathf.WithinEpsilon(left.X, right.X, epsilon) && Mathf.WithinEpsilon(left.Y, right.Y, epsilon) && Mathf.WithinEpsilon(left.Z, right.Z, epsilon) && Mathf.WithinEpsilon(left.W, right.W, epsilon);
        }

        /// <summary>
        /// Determines whether the specified <see cref="Quaternion" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="Quaternion" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="Quaternion" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(ref Quaternion other)
        {
            return X == other.X && Y == other.Y && Z == other.Z && W == other.W;
        }

        /// <summary>
        /// Determines whether the specified <see cref="Quaternion" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="Quaternion" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="Quaternion" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(Quaternion other)
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
            return value is Quaternion other && Equals(ref other);
        }
    }
}
