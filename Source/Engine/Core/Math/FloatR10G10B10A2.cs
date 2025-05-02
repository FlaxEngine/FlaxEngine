// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Runtime.InteropServices;

namespace FlaxEngine
{
    /// <summary>
    /// Packed vector, layout: R:10 bytes, G:10 bytes, B:10 bytes, A:2 bytes, all values are stored as floats in range [0;1]
    /// </summary>
    [Serializable]
    [StructLayout(LayoutKind.Sequential, Pack = 4)]
    public struct FloatR10G10B10A2
    {
        private uint rawValue;

        /// <summary>
        /// Initializes a new instance of the <see cref = "T:FlaxEngine.FloatR10G10B10A2" /> structure.
        /// </summary>
        /// <param name="x">The floating point value that should be stored in R component (10 bit format).</param>
        /// <param name="y">The floating point value that should be stored in G component (10 bit format).</param>
        /// <param name="z">The floating point value that should be stored in B component (10 bit format).</param>
        /// <param name="w">The floating point value that should be stored in A component (2 bit format).</param>
        public FloatR10G10B10A2(float x, float y, float z, float w)
        {
            rawValue = Pack(x, y, z, w);
        }

        /// <summary>
        /// Initializes a new instance of the <see cref = "T:FlaxEngine.FloatR10G10B10A2" /> structure.
        /// </summary>
        /// <param name="value">The floating point value that should be stored in 10 bit format.</param>
        /// <param name="w">The floating point value that should be stored in alpha component (2 bit format).</param>
        public FloatR10G10B10A2(Float3 value, float w = 0)
        {
            rawValue = Pack(value.X, value.Y, value.Z, w);
        }

        /// <summary>
        /// Initializes a new instance of the <see cref = "T:FlaxEngine.FloatR10G10B10A2" /> structure.
        /// </summary>
        /// <param name = "value">The floating point value that should be stored in 10 bit format.</param>
        public FloatR10G10B10A2(Float4 value)
        {
            rawValue = Pack(value.X, value.Y, value.Z, value.W);
        }

        /// <summary>
        /// Gets or sets the raw 32 bit value used to back this vector.
        /// </summary>
        public uint RawValue => rawValue;

        /// <summary>
        /// Gets the R component.
        /// </summary>
        public float R => (rawValue & 0x3FF) / 1023.0f;

        /// <summary>
        /// Gets the G component.
        /// </summary>
        public float G => ((rawValue >> 10) & 0x3FF) / 1023.0f;

        /// <summary>
        /// Gets the B component.
        /// </summary>
        public float B => ((rawValue >> 20) & 0x3FF) / 1023.0f;

        /// <summary>
        /// Gets the A component.
        /// </summary>
        public float A => (rawValue >> 30) / 3.0f;

        /// <summary>
        /// Performs an explicit conversion from <see cref = "T:FlaxEngine.Float4" /> to <see cref = "T:FlaxEngine.FloatR10G10B10A2" />.
        /// </summary>
        /// <param name="value">The value to be converted.</param>
        /// <returns>The converted value.</returns>
        public static explicit operator FloatR10G10B10A2(Float4 value)
        {
            return new FloatR10G10B10A2(value);
        }

        /// <summary>
        /// Performs an implicit conversion from <see cref = "T:FlaxEngine.FloatR10G10B10A2" /> to <see cref = "T:FlaxEngine.Float4" />.
        /// </summary>
        /// <param name="value">The value to be converted.</param>
        /// <returns>The converted value.</returns>
        public static implicit operator Float4(FloatR10G10B10A2 value)
        {
            return value.ToFloat4();
        }

        /// <summary>
        /// Tests for equality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left" /> has the same value as <paramref name="right" />; otherwise, <c>false</c>.</returns>
        public static bool operator ==(FloatR10G10B10A2 left, FloatR10G10B10A2 right)
        {
            return left.rawValue == right.rawValue;
        }

        /// <summary>
        /// Tests for inequality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left" /> has a different value than <paramref name="right" />; otherwise, <c>false</c>.</returns>
        public static bool operator !=(FloatR10G10B10A2 left, FloatR10G10B10A2 right)
        {
            return left.rawValue != right.rawValue;
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public override string ToString()
        {
            return ToFloat4().ToString();
        }

        /// <summary>
        /// Returns the hash code for this instance.
        /// </summary>
        /// <returns>A 32-bit signed integer hash code.</returns>
        public override int GetHashCode()
        {
            return rawValue.GetHashCode();
        }

        /// <summary>
        /// Determines whether the specified object instances are considered equal.
        /// </summary>
        /// <param name="value1" />
        /// <param name="value2" />
        /// <returns><c>true</c> if <paramref name = "value1" /> is the same instance as <paramref name = "value2" /> or if both are <c>null</c> references or if <c>value1.Equals(value2)</c> returns <c>true</c>; otherwise, <c>false</c>.</returns>
        public static bool Equals(ref FloatR10G10B10A2 value1, ref FloatR10G10B10A2 value2)
        {
            return value1.rawValue == value2.rawValue;
        }

        /// <summary>
        /// Returns a value that indicates whether the current instance is equal to the specified object.
        /// </summary>
        /// <param name = "other">Object to make the comparison with.</param>
        /// <returns><c>true</c> if the current instance is equal to the specified object; <c>false</c> otherwise.</returns>
        public bool Equals(FloatR10G10B10A2 other)
        {
            return other.rawValue == rawValue;
        }

        /// <summary>
        /// Returns a value that indicates whether the current instance is equal to a specified object.
        /// </summary>
        /// <param name="obj">Object to make the comparison with.</param>
        /// <returns><c>true</c> if the current instance is equal to the specified object; <c>false</c> otherwise.</returns>
        public override bool Equals(object obj)
        {
            return obj is FloatR10G10B10A2 other && rawValue == other.rawValue;
        }

        private static uint Pack(float x, float y, float z, float w)
        {
            x = Mathf.Saturate(x);
            y = Mathf.Saturate(y);
            z = Mathf.Saturate(z);
            w = Mathf.Saturate(w);

            x = Mathf.Round(x * 1023.0f);
            y = Mathf.Round(y * 1023.0f);
            z = Mathf.Round(z * 1023.0f);
            w = Mathf.Round(w * 3.0f);

            return ((uint)w << 30) |
                   (((uint)z & 0x3FF) << 20) |
                   (((uint)y & 0x3FF) << 10) |
                   (((uint)x & 0x3FF));
        }

        /// <summary>
        /// Unpacks vector to Float3.
        /// </summary>
        /// <returns>Float3 value</returns>
        public Float3 ToFloat3()
        {
            Float3 vectorOut;

            uint tmp = rawValue & 0x3FF;
            vectorOut.X = tmp / 1023.0f;
            tmp = (rawValue >> 10) & 0x3FF;
            vectorOut.Y = tmp / 1023.0f;
            tmp = (rawValue >> 20) & 0x3FF;
            vectorOut.Z = tmp / 1023.0f;

            return vectorOut;
        }

        /// <summary>
        /// Unpacks vector to Float4.
        /// </summary>
        /// <returns>Float4 value</returns>
        public Float4 ToFloat4()
        {
            Float4 vectorOut;

            uint tmp = rawValue & 0x3FF;
            vectorOut.X = tmp / 1023.0f;
            tmp = (rawValue >> 10) & 0x3FF;
            vectorOut.Y = tmp / 1023.0f;
            tmp = (rawValue >> 20) & 0x3FF;
            vectorOut.Z = tmp / 1023.0f;
            vectorOut.W = (rawValue >> 30) / 3.0f;

            return vectorOut;
        }
    }
}
