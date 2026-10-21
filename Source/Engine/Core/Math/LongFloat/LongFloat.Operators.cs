using FlaxEngine.Utilities;
using System;

namespace FlaxEngine
{
    public partial struct LongFloat
    {
        /// <summary>
        /// Implements the operator +.
        /// </summary>
        /// <param name="lhs">The LHS.</param>
        /// <param name="rhs">The RHS.</param>
        /// <returns>
        /// The result of the operator.
        /// </returns>
        public static LongFloat operator +(LongFloat lhs, LongFloat rhs) =>
            LongFloatMath.Add(lhs, rhs);

        /// <summary>
        /// Implements the operator -.
        /// </summary>
        /// <param name="lhs">The LHS.</param>
        /// <param name="rhs">The RHS.</param>
        /// <returns>
        /// The result of the operator.
        /// </returns>
        public static LongFloat operator -(LongFloat lhs, LongFloat rhs) =>
            LongFloatMath.Subtract(lhs, rhs);

        /// <summary>
        /// Implements the operator *.
        /// </summary>
        /// <param name="lhs">The LHS.</param>
        /// <param name="rhs">The RHS.</param>
        /// <returns>
        /// The result of the operator.
        /// </returns>
        public static LongFloat operator *(LongFloat lhs, LongFloat rhs) =>
            LongFloatMath.Multiply(lhs, rhs);

        /// <summary>
        /// Implements the operator /.
        /// </summary>
        /// <param name="lhs">The LHS.</param>
        /// <param name="rhs">The RHS.</param>
        /// <returns>
        /// The result of the operator.
        /// </returns>
        public static LongFloat operator /(LongFloat lhs, LongFloat rhs) =>
            LongFloatMath.Divide(lhs, rhs);

        /// <summary>
        /// Implements the operator -.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>
        /// The result of the operator.
        /// </returns>
        public static LongFloat operator -(LongFloat value)
        {
            LongFloatMath.Negate(value);
            return value;
        }

        /// <summary>
        /// Implements the operator &amp;.
        /// </summary>
        /// <param name="lhs">The LHS.</param>
        /// <param name="rhs">The RHS.</param>
        /// <returns>
        /// The result of the operator.
        /// </returns>
        public static LongFloat operator &(LongFloat lhs, LongFloat rhs) =>
            LongFloatMath.BitwiseAND(lhs, rhs);

        /// <summary>
        /// Implements the operator |.
        /// </summary>
        /// <param name="lhs">The LHS.</param>
        /// <param name="rhs">The RHS.</param>
        /// <returns>
        /// The result of the operator.
        /// </returns>
        public static LongFloat operator |(LongFloat lhs, LongFloat rhs) =>
            LongFloatMath.BitwiseOR(lhs, rhs);

        /// <summary>
        /// Implements the operator ^.
        /// </summary>
        /// <param name="lhs">The LHS.</param>
        /// <param name="rhs">The RHS.</param>
        /// <returns>
        /// The result of the operator.
        /// </returns>
        public static LongFloat operator ^(LongFloat lhs, LongFloat rhs) =>
            LongFloatMath.BitwiseXOR(lhs, rhs);

        /// <summary>
        /// Implements the operator ~.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>
        /// The result of the operator.
        /// </returns>
        public static LongFloat operator ~(LongFloat value) =>
            LongFloatMath.BitwiseNOT(value);

        /// <summary>
        /// Implements the operator &lt;&lt;.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <param name="shift">The shift.</param>
        /// <returns>
        /// The result of the operator.
        /// </returns>
        /// <exception cref="ArgumentOutOfRangeException">shift</exception>
        public static LongFloat operator <<(LongFloat value, int shift)
        {
            if (shift < 0)
                throw new ArgumentOutOfRangeException(nameof(shift));

            return LongFloatMath.ShiftLeft(value, (uint)shift);
        }
        /// <summary>
        /// Implements the operator &gt;&gt;.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <param name="shift">The shift.</param>
        /// <returns>
        /// The result of the operator.
        /// </returns>
        /// <exception cref="ArgumentOutOfRangeException">shift</exception>
        public static LongFloat operator >>(LongFloat value, int shift)
        {
            if (shift < 0)
                throw new ArgumentOutOfRangeException(nameof(shift));

            return LongFloatMath.ShiftRight(value, (uint)shift);
        }
        /// <summary>
        /// Implements the operator ==.
        /// </summary>
        /// <param name="lhs">The LHS.</param>
        /// <param name="rhs">The RHS.</param>
        /// <returns>
        /// The result of the operator.
        /// </returns>
        public static bool operator ==(LongFloat lhs, LongFloat rhs) =>
            lhs.highbits == rhs.highbits && lhs.lowbits == rhs.lowbits;

        /// <summary>
        /// Implements the operator !=.
        /// </summary>
        /// <param name="lhs">The LHS.</param>
        /// <param name="rhs">The RHS.</param>
        /// <returns>
        /// The result of the operator.
        /// </returns>
        public static bool operator !=(LongFloat lhs, LongFloat rhs) =>
            !(lhs == rhs);

        /// <summary>
        /// Implements the operator &lt;.
        /// </summary>
        /// <param name="lhs">The LHS.</param>
        /// <param name="rhs">The RHS.</param>
        /// <returns>
        /// The result of the operator.
        /// </returns>
        public static bool operator <(LongFloat lhs, LongFloat rhs)
        {
            bool negativeA = LongFloatMath.IsNegative(lhs);
            bool negativeB = LongFloatMath.IsNegative(rhs);

            if (negativeA != negativeB)
                return negativeA;

            if (lhs.highbits != rhs.highbits)
                return negativeA
                    ? lhs.highbits > rhs.highbits
                    : lhs.highbits < rhs.highbits;

            return lhs.lowbits < rhs.lowbits;
        }

        /// <summary>
        /// Implements the operator &gt;.
        /// </summary>
        /// <param name="lhs">The LHS.</param>
        /// <param name="rhs">The RHS.</param>
        /// <returns>
        /// The result of the operator.
        /// </returns>
        public static bool operator >(LongFloat lhs, LongFloat rhs) =>
            rhs < lhs;

        /// <summary>
        /// Implements the operator &lt;=.
        /// </summary>
        /// <param name="lhs">The LHS.</param>
        /// <param name="rhs">The RHS.</param>
        /// <returns>
        /// The result of the operator.
        /// </returns>
        public static bool operator <=(LongFloat lhs, LongFloat rhs) =>
            !(rhs < lhs);

        /// <summary>
        /// Implements the operator &gt;=.
        /// </summary>
        /// <param name="lhs">The LHS.</param>
        /// <param name="rhs">The RHS.</param>
        /// <returns>
        /// The result of the operator.
        /// </returns>
        public static bool operator >=(LongFloat lhs, LongFloat rhs) =>
            !(lhs < rhs);

        /// <inheritdoc/>
        public override bool Equals(object obj) =>
            obj is LongFloat other && this == other;

        /// <inheritdoc/>
        public override int GetHashCode()
        {
            unchecked
            {
                return (int)(lowbits ^ (lowbits >> 32) ^ highbits ^ (highbits >> 32));
            }
        }
    }
}
