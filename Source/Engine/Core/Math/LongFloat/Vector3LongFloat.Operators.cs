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
    }
}
