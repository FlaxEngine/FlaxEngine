// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Globalization;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace FlaxEngine
{
    /// <summary>
    /// Represents a 2x2 Matrix (contains only scale and rotation in 2D).
    /// </summary>
    [Serializable]
    [StructLayout(LayoutKind.Sequential, Pack = 4)]
    // ReSharper disable once InconsistentNaming
    public struct Matrix2x2 : IEquatable<Matrix2x2>, IFormattable
    {
        private const string FormatString = "[M11:{0} M12:{1}] [M21:{2} M22:{3}]";

        /// <summary>
        /// The size of the <see cref="Matrix2x2"/> type, in bytes.
        /// </summary>
        public static readonly int SizeInBytes = Marshal.SizeOf(typeof(Matrix2x2));

        /// <summary>
        /// A <see cref="Matrix2x2"/> with all of its components set to zero.
        /// </summary>
        public static readonly Matrix2x2 Zero;

        /// <summary>
        /// The identity <see cref="Matrix2x2"/>.
        /// </summary>
        public static readonly Matrix2x2 Identity = new Matrix2x2
        {
            M11 = 1.0f,
            M22 = 1.0f
        };

        /// <summary>
        /// Value at row 1 column 1 of the Matrix2x2.
        /// </summary>
        public float M11;

        /// <summary>
        /// Value at row 1 column 2 of the Matrix2x2.
        /// </summary>
        public float M12;

        /// <summary>
        /// Value at row 2 column 1 of the Matrix2x2.
        /// </summary>
        public float M21;

        /// <summary>
        /// Value at row 2 column 2 of the Matrix2x2.
        /// </summary>
        public float M22;

        /// <summary>
        /// Initializes a new instance of the <see cref="Matrix2x2"/> struct.
        /// </summary>
        /// <param name="value">The value that will be assigned to all components.</param>
        public Matrix2x2(float value)
        {
            M11 = M12 = M21 = M22 = value;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Matrix2x2"/> struct.
        /// </summary>
        /// <param name="m11">The value to assign at row 1 column 1 of the Matrix2x2.</param>
        /// <param name="m12">The value to assign at row 1 column 2 of the Matrix2x2.</param>
        /// <param name="m21">The value to assign at row 2 column 1 of the Matrix2x2.</param>
        /// <param name="m22">The value to assign at row 2 column 2 of the Matrix2x2.</param>
        public Matrix2x2(float m11, float m12, float m21, float m22)
        {
            M11 = m11;
            M12 = m12;
            M21 = m21;
            M22 = m22;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Matrix2x2"/> struct.
        /// </summary>
        /// <param name="values">The values to assign to the components of the Matrix2x2. This must be an array with four elements.</param>
        /// <exception cref="ArgumentNullException">Thrown when <paramref name="values"/> is <c>null</c>.</exception>
        /// <exception cref="ArgumentOutOfRangeException">Thrown when <paramref name="values"/> contains more or less than four elements.</exception>
        public Matrix2x2(float[] values)
        {
            if (values == null)
                throw new ArgumentNullException(nameof(values));
            if (values.Length != 4)
                throw new ArgumentOutOfRangeException(nameof(values), "There must be sixteen and only four input values for Matrix2x2.");
            M11 = values[0];
            M12 = values[1];
            M21 = values[2];
            M22 = values[3];
        }

        /// <summary>
        /// Gets or sets the first row in the Matrix2x2; that is M11, M12
        /// </summary>
        public Float2 Row1
        {
            get => new Float2(M11, M12);
            set
            {
                M11 = value.X;
                M12 = value.Y;
            }
        }

        /// <summary>
        /// Gets or sets the second row in the Matrix2x2; that is M21, M22
        /// </summary>
        public Float2 Row2
        {
            get => new Float2(M21, M22);
            set
            {
                M21 = value.X;
                M22 = value.Y;
            }
        }

        /// <summary>
        /// Gets or sets the first column in the Matrix2x2; that is M11, M21
        /// </summary>
        public Float2 Column1
        {
            get => new Float2(M11, M21);
            set
            {
                M11 = value.X;
                M21 = value.Y;
            }
        }

        /// <summary>
        /// Gets or sets the second column in the Matrix2x2; that is M12, M22
        /// </summary>
        public Float2 Column2
        {
            get => new Float2(M12, M22);
            set
            {
                M12 = value.X;
                M22 = value.Y;
            }
        }

        /// <summary>
        /// Gets or sets the scale of the Matrix2x2; that is M11, M22.
        /// </summary>
        public Float2 ScaleVector
        {
            get => new Float2(M11, M22);
            set
            {
                M11 = value.X;
                M22 = value.Y;
            }
        }

        /// <summary>
        /// Gets a value indicating whether this instance is an identity Matrix2x2.
        /// </summary>
        public bool IsIdentity => Equals(Identity);

        /// <summary>
        /// Gets or sets the component at the specified index.
        /// </summary>
        /// <value>The value of the Matrix2x2 component, depending on the index.</value>
        /// <param name="index">The zero-based index of the component to access.</param>
        /// <returns>The value of the component at the specified index.</returns>
        /// <exception cref="System.ArgumentOutOfRangeException">Thrown when the <paramref name="index"/> is out of the range [0, 3].</exception>
        public float this[int index]
        {
            get
            {
                switch (index)
                {
                case 0: return M11;
                case 1: return M12;
                case 2: return M21;
                case 3: return M22;
                }
                throw new ArgumentOutOfRangeException(nameof(index), "Indices for Matrix2x2 run from 0 to 3, inclusive.");
            }
            set
            {
                switch (index)
                {
                case 0:
                    M11 = value;
                    break;
                case 1:
                    M12 = value;
                    break;
                case 2:
                    M21 = value;
                    break;
                case 3:
                    M22 = value;
                    break;
                default: throw new ArgumentOutOfRangeException(nameof(index), "Indices for Matrix2x2 run from 0 to 3, inclusive.");
                }
            }
        }

        /// <summary>
        /// Gets or sets the component at the specified index.
        /// </summary>
        /// <value>The value of the Matrix2x2 component, depending on the index.</value>
        /// <param name="row">The row of the Matrix2x2 to access.</param>
        /// <param name="column">The column of the Matrix2x2 to access.</param>
        /// <returns>The value of the component at the specified index.</returns>
        /// <exception cref="System.ArgumentOutOfRangeException">Thrown when the <paramref name="row"/> or <paramref name="column"/>is out of the range [0, 1].</exception>
        public float this[int row, int column]
        {
            get
            {
                if (row < 0 || row > 1)
                    throw new ArgumentOutOfRangeException(nameof(row), "Rows and columns for matrices run from 0 to 1, inclusive.");
                if (column < 0 || column > 1)
                    throw new ArgumentOutOfRangeException(nameof(column), "Rows and columns for matrices run from 0 to 1, inclusive.");
                return this[(row * 2) + column];
            }
            set
            {
                if (row < 0 || row > 1)
                    throw new ArgumentOutOfRangeException(nameof(row), "Rows and columns for matrices run from 0 to 1, inclusive.");
                if (column < 0 || column > 1)
                    throw new ArgumentOutOfRangeException(nameof(column), "Rows and columns for matrices run from 0 to 1, inclusive.");
                this[(row * 2) + column] = value;
            }
        }

        /// <summary>
        /// Calculates the determinant of the Matrix2x2.
        /// </summary>
        /// <returns>The determinant of the Matrix2x2.</returns>
        public float Determinant()
        {
            return M11 * M22 - M12 * M21;
        }

        /// <summary>
        /// Calculates inverse of the determinant of the Matrix2x2.
        /// </summary>
        /// <returns>The inverse determinant of the Matrix2x2.</returns>
        public float InverseDeterminant()
        {
            float det = M11 * M22 - M12 * M21;
            Assertions.Assert.IsFalse(Mathf.IsZero(det));
            return 1.0f / det;
        }

        /// <summary>
        /// Creates an array containing the elements of the Matrix2x2.
        /// </summary>
        /// <returns>A 4-element array containing the components of the Matrix2x2.</returns>
        public float[] ToArray()
        {
            return new[] { M11, M12, M21, M22 };
        }

        /// <summary>
        /// Creates the uniform scale matrix.
        /// </summary>
        /// <param name="scale">The scale.</param>
        /// <param name="result">The result.</param>
        public static void Scale(float scale, out Matrix2x2 result)
        {
            result = new Matrix2x2(scale, 0, 0, scale);
        }

        /// <summary>
        /// Creates the scale matrix.
        /// </summary>
        /// <param name="scaleX">The scale x.</param>
        /// <param name="scaleY">The scale y.</param>
        /// <param name="result">The result.</param>
        public static void Scale(float scaleX, float scaleY, out Matrix2x2 result)
        {
            result = new Matrix2x2(scaleX, 0, 0, scaleY);
        }

        /// <summary>
        /// Creates the scale matrix.
        /// </summary>
        /// <param name="scale">The scale vector.</param>
        /// <param name="result">The result.</param>
        public static void Scale(ref Float2 scale, out Matrix2x2 result)
        {
            result = new Matrix2x2(scale.X, 0, 0, scale.Y);
        }

        /// <summary>
        /// Creates the shear matrix. Represented by:
        /// [1 Y]
        /// [X 1]
        /// </summary>
        /// <param name="shearAngles">The shear angles.</param>
        /// <param name="result">The result.</param>
        public static void Shear(ref Float2 shearAngles, out Matrix2x2 result)
        {
            float shearX = shearAngles.X == 0 ? 0 : (1.0f / Mathf.Tan(Mathf.DegreesToRadians * (90 - Mathf.Clamp(shearAngles.X, -89.0f, 89.0f))));
            float shearY = shearAngles.Y == 0 ? 0 : (1.0f / Mathf.Tan(Mathf.DegreesToRadians * (90 - Mathf.Clamp(shearAngles.Y, -89.0f, 89.0f))));
            result = new Matrix2x2(1, shearY, shearX, 1);
        }

        /// <summary>
        /// Creates the rotation matrix.
        /// </summary>
        /// <param name="rotationRadians">The rotation angle (in radians).</param>
        /// <param name="result">The result.</param>
        public static void Rotation(float rotationRadians, out Matrix2x2 result)
        {
            float sin = Mathf.Sin(rotationRadians);
            float cos = Mathf.Cos(rotationRadians);
            result = new Matrix2x2(cos, sin, -sin, cos);
        }

        /// <summary>
        /// Transforms the specified vector by the given matrix.
        /// </summary>
        /// <param name="vector">The vector.</param>
        /// <param name="matrix">The matrix.</param>
        /// <param name="result">The result.</param>
        public static void Transform(ref Float2 vector, ref Matrix2x2 matrix, out Float2 result)
        {
            result = new Float2(vector.X * matrix.M11 + vector.Y * matrix.M21, vector.X * matrix.M12 + vector.Y * matrix.M22);
        }

        /// <summary>
        /// Determines the product of two matrices.
        /// </summary>
        /// <param name="left">The first Matrix2x2 to multiply.</param>
        /// <param name="right">The second Matrix2x2 to multiply.</param>
        /// <param name="result">The product of the two matrices.</param>
        public static void Multiply(ref Matrix2x2 left, ref Matrix2x2 right, out Matrix2x2 result)
        {
            result = new Matrix2x2((left.M11 * right.M11) + (left.M12 * right.M21), (left.M11 * right.M12) + (left.M12 * right.M22), (left.M21 * right.M11) + (left.M22 * right.M21), (left.M21 * right.M12) + (left.M22 * right.M22));
        }

        /// <summary>
        /// Calculates the inverse of the specified Matrix2x2.
        /// </summary>
        /// <param name="value">The Matrix2x2 whose inverse is to be calculated.</param>
        /// <param name="result">When the method completes, contains the inverse of the specified Matrix2x2.</param>
        public static void Invert(ref Matrix2x2 value, out Matrix2x2 result)
        {
            float invDet = value.InverseDeterminant();
            result = new Matrix2x2(value.M22 * invDet, -value.M12 * invDet, -value.M21 * invDet, value.M11 * invDet);
        }

        /// <summary>
        /// Tests for equality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left"/> has the same value as <paramref name="right"/>; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator ==(Matrix2x2 left, Matrix2x2 right)
        {
            return left.Equals(ref right);
        }

        /// <summary>
        /// Tests for inequality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left"/> has a different value than <paramref name="right"/>; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator !=(Matrix2x2 left, Matrix2x2 right)
        {
            return !left.Equals(ref right);
        }

        /// <summary>
        /// Convert the 2x2 Matrix to a 4x4 Matrix.
        /// </summary>
        /// <returns>A 4x4 Matrix with zero translation and M44=1</returns>
        public static explicit operator Matrix(Matrix2x2 value)
        {
            return new Matrix(value.M11, value.M12, 0, 0, value.M21, value.M22, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
        }

        /// <summary>
        /// Convert the 4x4 Matrix to a 3x3 Matrix.
        /// </summary>
        /// <returns>A 2x2 Matrix</returns>
        public static explicit operator Matrix2x2(Matrix value)
        {
            return new Matrix2x2(value.M11, value.M12, value.M21, value.M22);
        }

        /// <summary>
        /// Convert the 2x2 Matrix to a 4x4 Matrix.
        /// </summary>
        /// <returns>A 3x3 Matrix with zero translation and M44=1</returns>
        public static explicit operator Matrix3x3(Matrix2x2 value)
        {
            return new Matrix3x3(value.M11, value.M12, 0, value.M21, value.M22, 0, 0, 0, 1);
        }

        /// <summary>
        /// Convert the 3x3 Matrix to a 2x2 Matrix.
        /// </summary>
        /// <returns>A 2x2 Matrix</returns>
        public static explicit operator Matrix2x2(Matrix3x3 value)
        {
            return new Matrix2x2(value.M11, value.M12, value.M21, value.M22);
        }

        /// <summary>
        /// Returns a <see cref="System.String"/> that represents this instance.
        /// </summary>
        /// <returns>A <see cref="System.String"/> that represents this instance.</returns>
        public override string ToString()
        {
            return string.Format(CultureInfo.CurrentCulture, FormatString, M11, M12, M21, M22);
        }

        /// <summary>
        /// Returns a <see cref="System.String"/> that represents this instance.
        /// </summary>
        /// <param name="format">The format.</param>
        /// <returns>A <see cref="System.String"/> that represents this instance.</returns>
        public string ToString(string format)
        {
            if (format == null)
                return ToString();
            return string.Format(format, CultureInfo.CurrentCulture, FormatString, M11.ToString(format, CultureInfo.CurrentCulture), M12.ToString(format, CultureInfo.CurrentCulture), M21.ToString(format, CultureInfo.CurrentCulture), M22.ToString(format, CultureInfo.CurrentCulture));
        }

        /// <summary>
        /// Returns a <see cref="System.String"/> that represents this instance.
        /// </summary>
        /// <param name="formatProvider">The format provider.</param>
        /// <returns>A <see cref="System.String"/> that represents this instance.</returns>
        public string ToString(IFormatProvider formatProvider)
        {
            return string.Format(formatProvider, FormatString, M11.ToString(formatProvider), M12.ToString(formatProvider), M21.ToString(formatProvider), M22.ToString(formatProvider));
        }

        /// <summary>
        /// Returns a <see cref="System.String"/> that represents this instance.
        /// </summary>
        /// <param name="format">The format.</param>
        /// <param name="formatProvider">The format provider.</param>
        /// <returns>A <see cref="System.String"/> that represents this instance.</returns>
        public string ToString(string format, IFormatProvider formatProvider)
        {
            if (format == null)
                return ToString(formatProvider);
            return string.Format(format, formatProvider, FormatString, M11.ToString(format, formatProvider), M12.ToString(format, formatProvider), M21.ToString(format, formatProvider), M22.ToString(format, formatProvider));
        }

        /// <summary>
        /// Returns a hash code for this instance.
        /// </summary>
        /// <returns>A hash code for this instance, suitable for use in hashing algorithms and data structures like a hash table. </returns>
        public override int GetHashCode()
        {
            unchecked
            {
                var hashCode = M11.GetHashCode();
                hashCode = (hashCode * 397) ^ M12.GetHashCode();
                hashCode = (hashCode * 397) ^ M21.GetHashCode();
                hashCode = (hashCode * 397) ^ M22.GetHashCode();
                return hashCode;
            }
        }

        /// <summary>
        /// Determines whether the specified <see cref="Matrix2x2"/> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="Matrix2x2"/> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="Matrix2x2"/> is equal to this instance; otherwise, <c>false</c>.</returns>
        public bool Equals(ref Matrix2x2 other)
        {
            return M11 == other.M11 && M12 == other.M12 && M21 == other.M21 && M22 == other.M22;
        }

        /// <summary>
        /// Determines whether the specified <see cref="Matrix2x2"/> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="Matrix2x2"/> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="Matrix2x2"/> is equal to this instance; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(Matrix2x2 other)
        {
            return Equals(ref other);
        }

        /// <summary>
        /// Determines whether the specified <see cref="Matrix2x2"/> are equal.
        /// </summary>
        public static bool Equals(ref Matrix2x2 a, ref Matrix2x2 b)
        {
            return a.Equals(ref b);
        }

        /// <summary>
        /// Determines whether the specified <see cref="System.Object"/> is equal to this instance.
        /// </summary>
        /// <param name="value">The <see cref="System.Object"/> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="System.Object"/> is equal to this instance; otherwise, <c>false</c>.</returns>
        public override bool Equals(object value)
        {
            return value is Matrix2x2 other && Equals(ref other);
        }
    }
}
