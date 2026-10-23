using FlaxEngine;
using FlaxEngine.Utilities;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Text;

namespace FlaxEngine
{
    public partial struct Vector3LongFloat : IEquatable<Vector3LongFloat>
    {
        /// <summary>
        /// Vector3LongFloat zero vector
        /// </summary>
        public readonly static Vector3LongFloat Zero = new(0ul);

        /// <summary>
        /// Implements the operator +.
        /// </summary>
        /// <param name="lhs">The LHS.</param>
        /// <param name="rhs">The RHS.</param>
        /// <returns>
        /// The result of the operator.
        /// </returns>
        public static Vector3LongFloat operator +(Vector3LongFloat lhs, Vector3LongFloat rhs)
        {
            lhs.X = LongFloatMath.Add(lhs.X, rhs.X);
            lhs.Y = LongFloatMath.Add(lhs.Y, rhs.Y);
            lhs.Z = LongFloatMath.Add(lhs.Z, rhs.Z);
            return lhs;
        }
        /// <summary>
        /// Implements the operator -.
        /// </summary>
        /// <param name="lhs">The LHS.</param>
        /// <param name="rhs">The RHS.</param>
        /// <returns>
        /// The result of the operator.
        /// </returns>
        public static Vector3LongFloat operator -(Vector3LongFloat lhs, Vector3LongFloat rhs)
        {
            lhs.X = LongFloatMath.Subtract(lhs.X, rhs.X);
            lhs.Y = LongFloatMath.Subtract(lhs.Y, rhs.Y);
            lhs.Z = LongFloatMath.Subtract(lhs.Z, rhs.Z);
            return lhs;
        }

        /// <summary>
        /// Implements the operator *.
        /// </summary>
        /// <param name="lhs">The LHS.</param>
        /// <param name="rhs">The RHS.</param>
        /// <returns>
        /// The result of the operator.
        /// </returns>
        public static Vector3LongFloat operator *(Vector3LongFloat lhs, Vector3LongFloat rhs)
        {
            lhs.X = LongFloatMath.Multiply(lhs.X, rhs.X);
            lhs.Y = LongFloatMath.Multiply(lhs.Y, rhs.Y);
            lhs.Z = LongFloatMath.Multiply(lhs.Z, rhs.Z);
            return lhs;
        }

        /// <summary>
        /// Implements the operator *.
        /// </summary>
        /// <param name="lhs">The LHS.</param>
        /// <param name="rhs">The RHS.</param>
        /// <returns>
        /// The result of the operator.
        /// </returns>
        public static Vector3LongFloat operator *(Vector3LongFloat lhs, LongFloat rhs)
        {
            lhs.X = LongFloatMath.Multiply(lhs.X, rhs);
            lhs.Y = LongFloatMath.Multiply(lhs.Y, rhs);
            lhs.Z = LongFloatMath.Multiply(lhs.Z, rhs);
            return lhs;
        }

        /// <summary>
        /// Implements the operator /.
        /// </summary>
        /// <param name="lhs">The LHS.</param>
        /// <param name="rhs">The RHS.</param>
        /// <returns>
        /// The result of the operator.
        /// </returns>
        public static Vector3LongFloat operator /(Vector3LongFloat lhs, Vector3LongFloat rhs)
        {
            lhs.X = LongFloatMath.Divide(lhs.X, rhs.X);
            lhs.Y = LongFloatMath.Divide(lhs.Y, rhs.Y);
            lhs.Z = LongFloatMath.Divide(lhs.Z, rhs.Z);
            return lhs;
        }

        /// <summary>
        /// Implements the operator -.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>
        /// The result of the operator.
        /// </returns>
        public static Vector3LongFloat operator -(Vector3LongFloat value)
        {
            value.X = LongFloatMath.Negate(value.X);
            value.Y = LongFloatMath.Negate(value.Y);
            value.Z = LongFloatMath.Negate(value.Z);
            return value;
        }

        /// <summary>
        /// Implements the operator ==.
        /// </summary>
        /// <param name="lhs">The LHS.</param>
        /// <param name="rhs">The RHS.</param>
        /// <returns>
        /// The result of the operator.
        /// </returns>
        public static bool operator ==(Vector3LongFloat lhs, Vector3LongFloat rhs) =>
            lhs.X == rhs.X && lhs.X == rhs.X && lhs.Z == rhs.Z;

        /// <summary>
        /// Implements the operator !=.
        /// </summary>
        /// <param name="lhs">The LHS.</param>
        /// <param name="rhs">The RHS.</param>
        /// <returns>
        /// The result of the operator.
        /// </returns>
        public static bool operator !=(Vector3LongFloat lhs, Vector3LongFloat rhs) =>
            !(lhs == rhs);

        /// <summary>
        /// Implements the operator &lt;.
        /// </summary>
        /// <param name="lhs">The LHS.</param>
        /// <param name="rhs">The RHS.</param>
        /// <returns>
        /// The result of the operator.
        /// </returns>
        public static bool operator <(Vector3LongFloat lhs, Vector3LongFloat rhs)
        {
            return lhs.X < rhs.X && lhs.Y < rhs.Y && lhs.Z < rhs.Z;
        }

        /// <summary>
        /// Implements the operator &gt;.
        /// </summary>
        /// <param name="lhs">The LHS.</param>
        /// <param name="rhs">The RHS.</param>
        /// <returns>
        /// The result of the operator.
        /// </returns>
        public static bool operator >(Vector3LongFloat lhs, Vector3LongFloat rhs) =>
            rhs < lhs;

        /// <summary>
        /// Implements the operator &lt;=.
        /// </summary>
        /// <param name="lhs">The LHS.</param>
        /// <param name="rhs">The RHS.</param>
        /// <returns>
        /// The result of the operator.
        /// </returns>
        public static bool operator <=(Vector3LongFloat lhs, Vector3LongFloat rhs) =>
            !(rhs < lhs);

        /// <summary>
        /// Implements the operator &gt;=.
        /// </summary>
        /// <param name="lhs">The LHS.</param>
        /// <param name="rhs">The RHS.</param>
        /// <returns>
        /// The result of the operator.
        /// </returns>
        public static bool operator >=(Vector3LongFloat lhs, Vector3LongFloat rhs) =>
            !(lhs < rhs);

        /// <inheritdoc/>
        public override bool Equals(object obj) =>
            obj is Vector3LongFloat other && this == other;

        /// <inheritdoc/>
        public override int GetHashCode()
        {
            unchecked
            {
                int hash = X.GetHashCode();
                hash = (hash * 397) ^ Y.GetHashCode();
                hash = (hash * 397) ^ Z.GetHashCode();
                return hash;
            }
        }

        /// <summary>
        /// Indicates whether the current object is equal to another object of the same type.
        /// </summary>
        /// <param name="other">An object to compare with this object.</param>
        /// <returns>
        ///   <see langword="true" /> if the current object is equal to the <paramref name="other" /> parameter; otherwise, <see langword="false" />.
        /// </returns>
        public bool Equals(Vector3LongFloat other)
        {
            return this == other;
        }

        /// <summary>
        /// Expands the specified float3 to Vector3LongFloat.
        /// </summary>
        /// <param name="value">The float3.</param>
        /// <returns></returns>
        public static Vector3LongFloat Expand(Float3 value)
        {
            return new Vector3LongFloat(value.X, value.Y, value.Z);
        }

        /// <summary>
        /// Distances the specified position.
        /// </summary>
        /// <param name="position">The position.</param>
        /// <param name="targetPoint">The target point.</param>
        /// <returns></returns>
        public static LongFloat Distance(Vector3LongFloat position, Vector3LongFloat targetPoint)
        {
            LongFloat dx = position.X - targetPoint.X;
            LongFloat dy = position.Y - targetPoint.Y;
            LongFloat dz = position.Z - targetPoint.Z;

            return LongFloatMath.SquareRoot(dx * dx + dy * dy + dz * dz);
        }

        /// <summary>
        /// lerp.
        /// </summary>
        /// <param name="start">The start.</param>
        /// <param name="end">The end.</param>
        /// <param name="amount">The amount.</param>
        /// <returns></returns>
        public static Vector3LongFloat Lerp(Vector3LongFloat start, Vector3LongFloat end, LongFloat amount)
        {
            return start + (end - start) * amount;
        }
    }
}
