// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Runtime.InteropServices;

namespace FlaxEngine
{
    /// <summary>
    /// Packed vector, layout: R:11 bytes, G:11 bytes, B:10 bytes.
    /// The 3D vector is packed into 32 bits as follows: a 5-bit biased exponent
    /// and 6-bit mantissa for x component, a 5-bit biased exponent and
    /// 6-bit mantissa for y component, a 5-bit biased exponent and a 5-bit
    /// mantissa for z. The z component is stored in the most significant bits
    /// and the x component in the least significant bits. No sign bits so
    /// all partial-precision numbers are positive.
    /// (Z10Y11X11): [32] ZZZZZzzz zzzYYYYY yyyyyyXX XXXxxxxx [0]
    /// </summary>
    [Serializable]
    [StructLayout(LayoutKind.Sequential, Pack = 4)]
    public struct FloatR11G11B10
    {
        // Reference: [https://github.com/Microsoft/DirectXMath/blob/master/Inc/DirectXPackedVector.h]

        private uint rawValue;

        /// <summary>
        /// Initializes a new instance of the <see cref = "T:FlaxEngine.FloatR11G11B10" /> structure.
        /// </summary>
        /// <param name="x">The floating point value that should be stored in R component (11 bits format).</param>
        /// <param name="y">The floating point value that should be stored in G component (11 bits format).</param>
        /// <param name="z">The floating point value that should be stored in B component (10 bits format).</param>
        public FloatR11G11B10(float x, float y, float z)
        {
            rawValue = Pack(x, y, z);
        }

        /// <summary>
        /// Initializes a new instance of the <see cref = "T:FlaxEngine.FloatR11G11B10" /> structure.
        /// </summary>
        /// <param name="value">The floating point value that should be stored in compressed format.</param>
        public FloatR11G11B10(Float3 value)
        {
            rawValue = Pack(value.X, value.Y, value.Z);
        }

        /// <summary>
        /// Gets or sets the raw 32 bit value used to back this vector.
        /// </summary>
        public uint RawValue => rawValue;

        /// <summary>
        /// Performs an explicit conversion from <see cref = "T:FlaxEngine.Float3" /> to <see cref = "T:FlaxEngine.FloatR11G11B10" />.
        /// </summary>
        /// <param name="value">The value to be converted.</param>
        /// <returns>The converted value.</returns>
        public static explicit operator FloatR11G11B10(Float3 value)
        {
            return new FloatR11G11B10(value);
        }

        /// <summary>
        /// Performs an implicit conversion from <see cref = "T:FlaxEngine.FloatR11G11B10" /> to <see cref = "T:FlaxEngine.Float3" />.
        /// </summary>
        /// <param name="value">The value to be converted.</param>
        /// <returns>The converted value.</returns>
        public static implicit operator Float3(FloatR11G11B10 value)
        {
            return value.ToFloat3();
        }

        /// <summary>
        /// Tests for equality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left" /> has the same value as <paramref name="right" />; otherwise, <c>false</c>.</returns>
        public static bool operator ==(FloatR11G11B10 left, FloatR11G11B10 right)
        {
            return left.rawValue == right.rawValue;
        }

        /// <summary>
        /// Tests for inequality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left" /> has a different value than <paramref name="right" />; otherwise, <c>false</c>.</returns>
        public static bool operator !=(FloatR11G11B10 left, FloatR11G11B10 right)
        {
            return left.rawValue != right.rawValue;
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public override string ToString()
        {
            return ((Float3)this).ToString();
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
        /// <returns><c>true</c> if <paramref name="value1" /> is the same instance as <paramref name="value2" /> or if both are <c>null</c> references or if <c>value1.Equals(value2)</c> returns <c>true</c>; otherwise, <c>false</c>.</returns>
        public static bool Equals(ref FloatR11G11B10 value1, ref FloatR11G11B10 value2)
        {
            return value1.rawValue == value2.rawValue;
        }

        /// <summary>
        /// Returns a value that indicates whether the current instance is equal to the specified object.
        /// </summary>
        /// <param name="other">Object to make the comparison with.</param>
        /// <returns><c>true</c> if the current instance is equal to the specified object; <c>false</c> otherwise.</returns>
        public bool Equals(FloatR11G11B10 other)
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
            return obj is FloatR11G11B10 other && rawValue == other.rawValue;
        }

        private static unsafe uint Pack(float x, float y, float z)
        {
            uint* input = stackalloc uint[4];
            input[0] = *(uint*)&x;
            input[1] = *(uint*)&y;
            input[2] = *(uint*)&z;
            input[3] = 0;

            uint* output = stackalloc uint[3];

            // X & Y Channels (5-bit exponent, 6-bit mantissa)
            for (uint j = 0; j < 2; j++)
            {
                bool sign = (input[j] & 0x80000000) != 0;
                uint I = input[j] & 0x7FFFFFFF;

                if ((I & 0x7F800000) == 0x7F800000)
                {
                    // INF or NAN
                    output[j] = 0x7c0;
                    if ((I & 0x7FFFFF) != 0)
                    {
                        output[j] = 0x7c0 | (((I >> 17) | (I >> 11) | (I >> 6) | (I)) & 0x3f);
                    }
                    else if (sign)
                    {
                        // -INF is clamped to 0 since 3PK is positive only
                        output[j] = 0;
                    }
                }
                else if (sign)
                {
                    // 3PK is positive only, so clamp to zero
                    output[j] = 0;
                }
                else if (I > 0x477E0000U)
                {
                    // The number is too large to be represented as a float11, set to max
                    output[j] = 0x7BF;
                }
                else
                {
                    if (I < 0x38800000U)
                    {
                        // The number is too small to be represented as a normalized float11
                        // Convert it to a denormalized value.
                        uint shift = 113U - (I >> 23);
                        I = (0x800000U | (I & 0x7FFFFFU)) >> (int)shift;
                    }
                    else
                    {
                        // Rebias the exponent to represent the value as a normalized float11
                        I += 0xC8000000U;
                    }

                    output[j] = ((I + 0xFFFFU + ((I >> 17) & 1U)) >> 17) & 0x7ffU;
                }
            }

            // Z Channel (5-bit exponent, 5-bit mantissa)
            {
                bool sign = (input[2] & 0x80000000) != 0;
                uint I = input[2] & 0x7FFFFFFF;

                if ((I & 0x7F800000) == 0x7F800000)
                {
                    // INF or NAN
                    output[2] = 0x3e0;
                    if ((I & 0x7FFFFF) != 0)
                    {
                        output[2] = 0x3e0 | (((I >> 18) | (I >> 13) | (I >> 3) | (I)) & 0x1f);
                    }
                    else if (sign)
                    {
                        // -INF is clamped to 0 since 3PK is positive only
                        output[2] = 0;
                    }
                }
                else if (sign)
                {
                    // 3PK is positive only, so clamp to zero
                    output[2] = 0;
                }
                else if (I > 0x477C0000U)
                {
                    // The number is too large to be represented as a float10, set to max
                    output[2] = 0x3df;
                }
                else
                {
                    if (I < 0x38800000U)
                    {
                        // The number is too small to be represented as a normalized float10
                        // Convert it to a denormalized value.
                        uint shift = 113U - (I >> 23);
                        I = (0x800000U | (I & 0x7FFFFFU)) >> (int)shift;
                    }
                    else
                    {
                        // Rebias the exponent to represent the value as a normalized float10
                        I += 0xC8000000U;
                    }

                    output[2] = ((I + 0x1FFFFU + ((I >> 18) & 1U)) >> 18) & 0x3ffU;
                }
            }

            // Pack result
            return (output[0] & 0x7ff) | ((output[1] & 0x7ff) << 11) | ((output[2] & 0x3ff) << 22);
        }

        private struct Packed
        {
            public uint v;

            public uint xm; // x-mantissa
            public uint xe; // x-exponent
            public uint ym; // y-mantissa
            public uint ye; // y-exponent
            public uint zm; // z-mantissa
            public uint ze; // z-exponent

            public Packed(uint value)
            {
                v = value;

                // Get those bits!
                // In C++ we would use 'union', in C# we have to commit suicide with this:
                xm = v & 0b111111;
                xe = (v >> 6) & 0b011111;
                ym = (v >> 11) & 0b111111;
                ye = (v >> 17) & 0b011111;
                zm = (v >> 22) & 0b011111;
                ze = (v >> 27) & 0b011111;
            }
        }

        /// <summary>
        /// Unpacks vector to Float3.
        /// </summary>
        /// <returns>Float3 value</returns>
        public unsafe Float3 ToFloat3()
        {
            int zeroExponent = -112;

            Packed packed = new Packed(rawValue);
            uint* result = stackalloc uint[4];
            uint exponent;

            // X Channel (6-bit mantissa)
            var mantissa = packed.xm;

            // INF or NAN
            if (packed.xe == 0x1f)
            {
                result[0] = 0x7f800000 | (packed.xm << 17);
            }
            else
            {
                // The value is normalized
                if (packed.xe != 0)
                {
                    exponent = packed.xe;
                }
                else if (mantissa != 0)
                {
                    // The value is denormalized
                    // Normalize the value in the resulting float
                    exponent = 1;

                    do
                    {
                        exponent--;
                        mantissa <<= 1;
                    } while ((mantissa & 0x40) == 0);

                    mantissa &= 0x3F;
                }
                else
                {
                    // The value is zero
                    exponent = *(uint*)&zeroExponent;
                }

                result[0] = ((exponent + 112) << 23) | (mantissa << 17);
            }

            // Y Channel (6-bit mantissa)
            mantissa = packed.ym;

            if (packed.ye == 0x1f)
            {
                // INF or NAN
                result[1] = 0x7f800000 | (packed.ym << 17);
            }
            else
            {
                if (packed.ye != 0)
                {
                    // The value is normalized
                    exponent = packed.ye;
                }
                else if (mantissa != 0)
                {
                    // The value is denormalized
                    // Normalize the value in the resulting float
                    exponent = 1;

                    do
                    {
                        exponent--;
                        mantissa <<= 1;
                    } while ((mantissa & 0x40) == 0);

                    mantissa &= 0x3F;
                }
                else
                {
                    // The value is zero
                    exponent = *(uint*)&zeroExponent;
                }

                result[1] = ((exponent + 112) << 23) | (mantissa << 17);
            }

            // Z Channel (5-bit mantissa)
            mantissa = packed.zm;

            if (packed.ze == 0x1f)
            {
                // INF or NAN
                result[2] = 0x7f800000 | (packed.zm << 17);
            }
            else
            {
                if (packed.ze != 0)
                {
                    // The value is normalized
                    exponent = packed.ze;
                }
                else if (mantissa != 0) // The value is denormalized
                {
                    // Normalize the value in the resulting float
                    exponent = 1;

                    do
                    {
                        exponent--;
                        mantissa <<= 1;
                    } while ((mantissa & 0x20) == 0);

                    mantissa &= 0x1F;
                }
                else
                {
                    // The value is zero
                    exponent = *(uint*)&zeroExponent;
                }

                result[2] = ((exponent + 112) << 23) | (mantissa << 18);
            }

            float* resultAsFloat = (float*)result;
            return new Float3(resultAsFloat[0], resultAsFloat[1], resultAsFloat[2]);
        }
    }
}
