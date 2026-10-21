using FlaxEngine.Utilities;
using System;
using System.Collections.Generic;
using System.Text;

namespace FlaxEngine
{
    public partial struct LongFloat
    {
        /// <summary>Checks whether this value is negative.</summary>
        /// <returns>True if this value is negative; otherwise, false.</returns>
        public bool IsNegative() => LongFloatMath.IsNegative(this);

        /// <summary>Checks whether this value has a nonzero fractional part.</summary>
        /// <returns>True if this value has a nonzero fractional part; otherwise, false.</returns>
        public bool IsFractional() => LongFloatMath.IsFractional(this);

        /// <summary>Gets the sign of this value.</summary>
        /// <returns>-1 for a negative value, 0 for zero, or 1 for a positive value.</returns>
        public int Sign() => LongFloatMath.Sign(this);

        /// <summary>Checks whether this value is NaN.</summary>
        /// <returns>True if this value is NaN; otherwise, false.</returns>
        public bool IsNAN() => LongFloatMath.IsNAN(this);

        /// <summary>Gets the NaN value.</summary>
        /// <returns>The NaN value.</returns>
        public static LongFloat GetNAN() => LongFloatMath.GetNAN();

        /// <summary>Gets the zero value.</summary>
        /// <returns>A zero value.</returns>
        public static LongFloat GetZero() => LongFloatMath.GetZero();

        /// <summary>Returns the negation of this value.</summary>
        /// <returns>The negated value.</returns>
        public LongFloat Negate()
        {
            var value = this;
            LongFloatMath.Negate(value);
            return value;
        }

        /// <summary>Calculates the square root of this value.</summary>
        /// <returns>The square root.</returns>
        public LongFloat SquareRoot() => LongFloatMath.SquareRoot(this);

        /// <summary>Adds another value as an unsigned 128-bit integer, without fixed-point scaling.</summary>
        /// <param name="rhs">The raw value to add.</param>
        /// <returns>The raw sum.</returns>
        public LongFloat AddUInt128(LongFloat rhs) => LongFloatMath.AddUInt128(this, rhs);

        /// <summary>Subtracts another value as an unsigned 128-bit integer, without fixed-point scaling.</summary>
        /// <param name="rhs">The raw value to subtract.</param>
        /// <returns>The raw difference.</returns>
        public LongFloat SubtractUInt128(LongFloat rhs) => LongFloatMath.SubtractUInt128(this, rhs);

        /// <summary>Divides this raw unsigned 128-bit value by another, without fixed-point scaling.</summary>
        /// <param name="rhs">The raw divisor.</param>
        /// <returns>The raw quotient.</returns>
        public LongFloat DivideUInt128(LongFloat rhs) => LongFloatMath.DivideUInt128(this, rhs);

        /// <summary>Multiplies this value by another as unsigned 128-bit integers, without fixed-point scaling.</summary>
        /// <param name="rhs">The other raw value.</param>
        /// <returns>The raw product.</returns>
        public LongFloat MultiplyUInt128(LongFloat rhs) => LongFloatMath.MultiplyUInt128(this, rhs);

        /// <summary>Calculates the square root of this raw unsigned 128-bit value.</summary>
        /// <returns>The raw square root.</returns>
        public LongFloat SquareRootUInt128() => LongFloatMath.SquareRootUInt128(this);

        /// <summary>Performs a bitwise AND on this value and another value.</summary>
        /// <param name="rhs">The second value.</param>
        /// <returns>The bitwise AND result.</returns>
        public LongFloat BitwiseAND(LongFloat rhs) => LongFloatMath.BitwiseAND(this, rhs);

        /// <summary>Performs a bitwise OR on this value and another value.</summary>
        /// <param name="rhs">The second value.</param>
        /// <returns>The bitwise OR result.</returns>
        public LongFloat BitwiseOR(LongFloat rhs) => LongFloatMath.BitwiseOR(this, rhs);

        /// <summary>Performs a bitwise XOR on this value and another value.</summary>
        /// <param name="rhs">The second value.</param>
        /// <returns>The bitwise XOR result.</returns>
        public LongFloat BitwiseXOR(LongFloat rhs) => LongFloatMath.BitwiseXOR(this, rhs);

        /// <summary>Inverts every bit in this value.</summary>
        /// <returns>The bitwise complement.</returns>
        public LongFloat BitwiseNOT() => LongFloatMath.BitwiseNOT(this);

        /// <summary>Shifts the raw bits of this value to the left.</summary>
        /// <param name="shift">The number of bit positions to shift.</param>
        /// <returns>The shifted value.</returns>
        public LongFloat ShiftLeft(uint shift) => LongFloatMath.ShiftLeft(this, shift);

        /// <summary>Shifts the raw bits of this value to the right.</summary>
        /// <param name="shift">The number of bit positions to shift.</param>
        /// <returns>The shifted value.</returns>
        public LongFloat ShiftRight(uint shift) => LongFloatMath.ShiftRight(this, shift);

        /// <summary>Converts the raw bits of this value to a binary string.</summary>
        /// <param name="codeMode">True to prefix the result with the binary literal prefix.</param>
        /// <returns>The binary representation of the raw bits.</returns>
        public string BitsToString(bool codeMode = false) => LongFloatMath.BitsToString(this, codeMode);

        /// <summary>Converts this value to its decimal string representation.</summary>
        /// <returns>The decimal representation of this value.</returns>
        public override string ToString() => LongFloatMath.ToString(this);
    }
}
