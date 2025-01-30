// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
    partial struct Matrix3x3 : IEquatable<Matrix3x3>, IFormattable
    {
        /// <summary>
        /// The size of the <see cref="Matrix3x3"/> type, in bytes.
        /// </summary>
        public static readonly int SizeInBytes = Marshal.SizeOf(typeof(Matrix3x3));

        /// <summary>
        /// A <see cref="Matrix3x3"/> with all of its components set to zero.
        /// </summary>
        public static readonly Matrix3x3 Zero;

        /// <summary>
        /// The identity <see cref="Matrix3x3"/>.
        /// </summary>
        public static readonly Matrix3x3 Identity = new Matrix3x3()
        {
            M11 = 1.0f,
            M22 = 1.0f,
            M33 = 1.0f
        };

        /// <summary>
        /// Value at row 1 column 1 of the Matrix3x3.
        /// </summary>
        public float M11;

        /// <summary>
        /// Value at row 1 column 2 of the Matrix3x3.
        /// </summary>
        public float M12;

        /// <summary>
        /// Value at row 1 column 3 of the Matrix3x3.
        /// </summary>
        public float M13;

        /// <summary>
        /// Value at row 2 column 1 of the Matrix3x3.
        /// </summary>
        public float M21;

        /// <summary>
        /// Value at row 2 column 2 of the Matrix3x3.
        /// </summary>
        public float M22;

        /// <summary>
        /// Value at row 2 column 3 of the Matrix3x3.
        /// </summary>
        public float M23;

        /// <summary>
        /// Value at row 3 column 1 of the Matrix3x3.
        /// </summary>
        public float M31;

        /// <summary>
        /// Value at row 3 column 2 of the Matrix3x3.
        /// </summary>
        public float M32;

        /// <summary>
        /// Value at row 3 column 3 of the Matrix3x3.
        /// </summary>
        public float M33;

        /// <summary>
        /// Initializes a new instance of the <see cref="Matrix3x3"/> struct.
        /// </summary>
        /// <param name="value">The value that will be assigned to all components.</param>
        public Matrix3x3(float value)
        {
            M11 = M12 = M13 = M21 = M22 = M23 = M31 = M32 = M33 = value;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Matrix3x3"/> struct.
        /// </summary>
        /// <param name="m11">The value to assign at row 1 column 1 of the Matrix3x3.</param>
        /// <param name="m12">The value to assign at row 1 column 2 of the Matrix3x3.</param>
        /// <param name="m13">The value to assign at row 1 column 3 of the Matrix3x3.</param>
        /// <param name="m21">The value to assign at row 2 column 1 of the Matrix3x3.</param>
        /// <param name="m22">The value to assign at row 2 column 2 of the Matrix3x3.</param>
        /// <param name="m23">The value to assign at row 2 column 3 of the Matrix3x3.</param>
        /// <param name="m31">The value to assign at row 3 column 1 of the Matrix3x3.</param>
        /// <param name="m32">The value to assign at row 3 column 2 of the Matrix3x3.</param>
        /// <param name="m33">The value to assign at row 3 column 3 of the Matrix3x3.</param>
        public Matrix3x3(float m11, float m12, float m13,
                         float m21, float m22, float m23,
                         float m31, float m32, float m33)
        {
            M11 = m11;
            M12 = m12;
            M13 = m13;
            M21 = m21;
            M22 = m22;
            M23 = m23;
            M31 = m31;
            M32 = m32;
            M33 = m33;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Matrix3x3"/> struct.
        /// </summary>
        /// <param name="values">The values to assign to the components of the Matrix3x3. This must be an array with nine elements.</param>
        /// <exception cref="ArgumentNullException">Thrown when <paramref name="values"/> is <c>null</c>.</exception>
        /// <exception cref="ArgumentOutOfRangeException">Thrown when <paramref name="values"/> contains more or less than nine elements.</exception>
        public Matrix3x3(float[] values)
        {
            if (values == null)
                throw new ArgumentNullException(nameof(values));
            if (values.Length != 9)
                throw new ArgumentOutOfRangeException(nameof(values), "There must be sixteen and only nine input values for Matrix3x3.");

            M11 = values[0];
            M12 = values[1];
            M13 = values[2];

            M21 = values[3];
            M22 = values[4];
            M23 = values[5];

            M31 = values[6];
            M32 = values[7];
            M33 = values[8];
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Matrix3x3" /> struct.
        /// </summary>
        /// <param name="m">The rotation/scale matrix.</param>
        public Matrix3x3(Matrix m)
        {
            M11 = m.M11;
            M12 = m.M12;
            M13 = m.M13;
            M21 = m.M21;
            M22 = m.M22;
            M23 = m.M23;
            M31 = m.M31;
            M32 = m.M32;
            M33 = m.M33;
        }

        /// <summary>
        /// Gets or sets the first row in the Matrix3x3; that is M11, M12, M13
        /// </summary>
        public Float3 Row1
        {
            get => new Float3(M11, M12, M13);
            set
            {
                M11 = value.X;
                M12 = value.Y;
                M13 = value.Z;
            }
        }

        /// <summary>
        /// Gets or sets the second row in the Matrix3x3; that is M21, M22, M23
        /// </summary>
        public Float3 Row2
        {
            get => new Float3(M21, M22, M23);
            set
            {
                M21 = value.X;
                M22 = value.Y;
                M23 = value.Z;
            }
        }

        /// <summary>
        /// Gets or sets the third row in the Matrix3x3; that is M31, M32, M33
        /// </summary>
        public Float3 Row3
        {
            get => new Float3(M31, M32, M33);
            set
            {
                M31 = value.X;
                M32 = value.Y;
                M33 = value.Z;
            }
        }

        /// <summary>
        /// Gets or sets the first column in the Matrix3x3; that is M11, M21, M31
        /// </summary>
        public Float3 Column1
        {
            get => new Float3(M11, M21, M31);
            set
            {
                M11 = value.X;
                M21 = value.Y;
                M31 = value.Z;
            }
        }

        /// <summary>
        /// Gets or sets the second column in the Matrix3x3; that is M12, M22, M32
        /// </summary>
        public Float3 Column2
        {
            get => new Float3(M12, M22, M32);
            set
            {
                M12 = value.X;
                M22 = value.Y;
                M32 = value.Z;
            }
        }

        /// <summary>
        /// Gets or sets the third column in the Matrix3x3; that is M13, M23, M33
        /// </summary>
        public Float3 Column3
        {
            get => new Float3(M13, M23, M33);
            set
            {
                M13 = value.X;
                M23 = value.Y;
                M33 = value.Z;
            }
        }

        /// <summary>
        /// Gets or sets the scale of the Matrix3x3; that is M11, M22, and M33.
        /// </summary>
        public Float3 ScaleVector
        {
            get => new Float3(M11, M22, M33);
            set
            {
                M11 = value.X;
                M22 = value.Y;
                M33 = value.Z;
            }
        }

        /// <summary>
        /// Gets a value indicating whether this instance is an identity Matrix3x3.
        /// </summary>
        public bool IsIdentity => Equals(Identity);

        /// <summary>
        /// Gets or sets the component at the specified index.
        /// </summary>
        /// <value>The value of the Matrix3x3 component, depending on the index.</value>
        /// <param name="index">The zero-based index of the component to access.</param>
        /// <returns>The value of the component at the specified index.</returns>
        /// <exception cref="System.ArgumentOutOfRangeException">Thrown when the <paramref name="index"/> is out of the range [0, 15].</exception>
        public float this[int index]
        {
            get
            {
                switch (index)
                {
                case 0: return M11;
                case 1: return M12;
                case 2: return M13;
                case 3: return M21;
                case 4: return M22;
                case 5: return M23;
                case 6: return M31;
                case 7: return M32;
                case 8: return M33;
                }

                throw new ArgumentOutOfRangeException(nameof(index), "Indices for Matrix3x3 run from 0 to 8, inclusive.");
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
                    M13 = value;
                    break;
                case 3:
                    M21 = value;
                    break;
                case 4:
                    M22 = value;
                    break;
                case 5:
                    M23 = value;
                    break;
                case 6:
                    M31 = value;
                    break;
                case 7:
                    M32 = value;
                    break;
                case 8:
                    M33 = value;
                    break;
                default: throw new ArgumentOutOfRangeException(nameof(index), "Indices for Matrix3x3 run from 0 to 8, inclusive.");
                }
            }
        }

        /// <summary>
        /// Gets or sets the component at the specified index.
        /// </summary>
        /// <value>The value of the Matrix3x3 component, depending on the index.</value>
        /// <param name="row">The row of the Matrix3x3 to access.</param>
        /// <param name="column">The column of the Matrix3x3 to access.</param>
        /// <returns>The value of the component at the specified index.</returns>
        /// <exception cref="System.ArgumentOutOfRangeException">Thrown when the <paramref name="row"/> or <paramref name="column"/>is out of the range [0, 3].</exception>
        public float this[int row, int column]
        {
            get
            {
                if (row < 0 || row > 2)
                    throw new ArgumentOutOfRangeException(nameof(row), "Rows and columns for matrices run from 0 to 2, inclusive.");
                if (column < 0 || column > 2)
                    throw new ArgumentOutOfRangeException(nameof(column), "Rows and columns for matrices run from 0 to 2, inclusive.");

                return this[(row * 3) + column];
            }
            set
            {
                if (row < 0 || row > 2)
                    throw new ArgumentOutOfRangeException(nameof(row), "Rows and columns for matrices run from 0 to 2, inclusive.");
                if (column < 0 || column > 2)
                    throw new ArgumentOutOfRangeException(nameof(column), "Rows and columns for matrices run from 0 to 2, inclusive.");

                this[(row * 3) + column] = value;
            }
        }

        /// <summary>
        /// Calculates the determinant of the Matrix3x3.
        /// </summary>
        /// <returns>The determinant of the Matrix3x3.</returns>
        public float Determinant()
        {
            return M11 * M22 * M33 + M12 * M23 * M31 + M13 * M21 * M32 - M13 * M22 * M31 - M12 * M21 * M33 - M11 * M23 * M32;
        }

        /// <summary>
        /// Inverts the Matrix3x3.
        /// </summary>
        public void Invert()
        {
            Invert(ref this, out this);
        }

        /// <summary>
        /// Transposes the Matrix3x3.
        /// </summary>
        public void Transpose()
        {
            Transpose(ref this, out this);
        }

        /// <summary>
        /// Orthogonalizes the specified Matrix3x3.
        /// </summary>
        /// <remarks>
        /// <para>Orthogonalization is the process of making all rows orthogonal to each other. This
        /// means that any given row in the Matrix3x3 will be orthogonal to any other given row in the
        /// Matrix3x3.</para>
        /// <para>Because this method uses the modified Gram-Schmidt process, the resulting Matrix3x3
        /// tends to be numerically unstable. The numeric stability decreases according to the rows
        /// so that the first row is the most stable and the last row is the least stable.</para>
        /// <para>This operation is performed on the rows of the Matrix3x3 rather than the columns.
        /// If you wish for this operation to be performed on the columns, first transpose the
        /// input and than transpose the output.</para>
        /// </remarks>
        public void Orthogonalize()
        {
            Orthogonalize(ref this, out this);
        }

        /// <summary>
        /// Orthonormalizes the specified Matrix3x3.
        /// </summary>
        /// <remarks>
        /// <para>Orthonormalization is the process of making all rows and columns orthogonal to each
        /// other and making all rows and columns of unit length. This means that any given row will
        /// be orthogonal to any other given row and any given column will be orthogonal to any other
        /// given column. Any given row will not be orthogonal to any given column. Every row and every
        /// column will be of unit length.</para>
        /// <para>Because this method uses the modified Gram-Schmidt process, the resulting Matrix3x3
        /// tends to be numerically unstable. The numeric stability decreases according to the rows
        /// so that the first row is the most stable and the last row is the least stable.</para>
        /// <para>This operation is performed on the rows of the Matrix3x3 rather than the columns.
        /// If you wish for this operation to be performed on the columns, first transpose the
        /// input and than transpose the output.</para>
        /// </remarks>
        public void Orthonormalize()
        {
            Orthonormalize(ref this, out this);
        }

        /// <summary>
        /// Decomposes a Matrix3x3 into an orthonormalized Matrix3x3 q and a right triangular Matrix3x3 r.
        /// </summary>
        /// <param name="q">When the method completes, contains the orthonormalized Matrix3x3 of the decomposition.</param>
        /// <param name="r">When the method completes, contains the right triangular Matrix3x3 of the decomposition.</param>
        // ReSharper disable once InconsistentNaming
        public void DecomposeQR(out Matrix3x3 q, out Matrix3x3 r)
        {
            Matrix3x3 temp = this;
            temp.Transpose();
            Orthonormalize(ref temp, out q);
            q.Transpose();

            r = new Matrix3x3
            {
                M11 = Float3.Dot(q.Column1, Column1),
                M12 = Float3.Dot(q.Column1, Column2),
                M13 = Float3.Dot(q.Column1, Column3),
                M22 = Float3.Dot(q.Column2, Column2),
                M23 = Float3.Dot(q.Column2, Column3),
                M33 = Float3.Dot(q.Column3, Column3)
            };
        }

        /// <summary>
        /// Decomposes a Matrix3x3 into a lower triangular Matrix3x3 l and an orthonormalized Matrix3x3 q.
        /// </summary>
        /// <param name="l">When the method completes, contains the lower triangular Matrix3x3 of the decomposition.</param>
        /// <param name="q">When the method completes, contains the orthonormalized Matrix3x3 of the decomposition.</param>
        // ReSharper disable once InconsistentNaming
        public void DecomposeLQ(out Matrix3x3 l, out Matrix3x3 q)
        {
            Orthonormalize(ref this, out q);

            l = new Matrix3x3
            {
                M11 = Float3.Dot(q.Row1, Row1),
                M21 = Float3.Dot(q.Row1, Row2),
                M22 = Float3.Dot(q.Row2, Row2),
                M31 = Float3.Dot(q.Row1, Row3),
                M32 = Float3.Dot(q.Row2, Row3),
                M33 = Float3.Dot(q.Row3, Row3)
            };
        }

        /// <summary>
        /// Decomposes a Matrix3x3 into a scale, rotation, and translation.
        /// </summary>
        /// <param name="scale">When the method completes, contains the scaling component of the decomposed Matrix3x3.</param>
        /// <param name="rotation">When the method completes, contains the rotation component of the decomposed Matrix3x3.</param>
        /// <remarks>
        /// This method is designed to decompose an SRT transformation Matrix3x3 only.
        /// </remarks>
        public bool Decompose(out Float3 scale, out Quaternion rotation)
        {
            //Source: Unknown
            //References: http://www.gamedev.net/community/forums/topic.asp?topic_id=441695

            //Scaling is the length of the rows.
            scale.X = (float)Math.Sqrt((M11 * M11) + (M12 * M12) + (M13 * M13));
            scale.Y = (float)Math.Sqrt((M21 * M21) + (M22 * M22) + (M23 * M23));
            scale.Z = (float)Math.Sqrt((M31 * M31) + (M32 * M32) + (M33 * M33));

            //If any of the scaling factors are zero, than the rotation Matrix3x3 can not exist.
            if (Mathf.IsZero(scale.X) ||
                Mathf.IsZero(scale.Y) ||
                Mathf.IsZero(scale.Z))
            {
                rotation = Quaternion.Identity;
                return false;
            }

            //The rotation is the left over Matrix3x3 after dividing out the scaling.
            Matrix3x3 rotationMatrix = new Matrix3x3
            {
                M11 = M11 / scale.X,
                M12 = M12 / scale.X,
                M13 = M13 / scale.X,
                M21 = M21 / scale.Y,
                M22 = M22 / scale.Y,
                M23 = M23 / scale.Y,
                M31 = M31 / scale.Z,
                M32 = M32 / scale.Z,
                M33 = M33 / scale.Z
            };

            Quaternion.RotationMatrix(ref rotationMatrix, out rotation);
            return true;
        }

        /// <summary>
        /// Decomposes a uniform scale matrix into a scale, rotation, and translation.
        /// A uniform scale matrix has the same scale in every axis.
        /// </summary>
        /// <param name="scale">When the method completes, contains the scaling component of the decomposed matrix.</param>
        /// <param name="rotation">When the method completes, contains the rotation component of the decomposed matrix.</param>
        /// <remarks>
        /// This method is designed to decompose only an SRT transformation matrix that has the same scale in every axis.
        /// </remarks>
        public bool DecomposeUniformScale(out float scale, out Quaternion rotation)
        {
            // Scaling is the length of the rows. ( just take one row since this is a uniform matrix)
            scale = (float)Math.Sqrt((M11 * M11) + (M12 * M12) + (M13 * M13));
            var invScale = 1f / scale;

            // If any of the scaling factors are zero, then the rotation matrix can not exist
            if (Math.Abs(scale) < Mathf.Epsilon)
            {
                rotation = Quaternion.Identity;
                return false;
            }

            // The rotation is the leftover matrix after dividing out the scaling
            var rotationMatrix = new Matrix3x3
            {
                M11 = M11 * invScale,
                M12 = M12 * invScale,
                M13 = M13 * invScale,
                M21 = M21 * invScale,
                M22 = M22 * invScale,
                M23 = M23 * invScale,
                M31 = M31 * invScale,
                M32 = M32 * invScale,
                M33 = M33 * invScale
            };
            Quaternion.RotationMatrix(ref rotationMatrix, out rotation);
            return true;
        }

        /// <summary>
        /// Exchanges two rows in the Matrix3x3.
        /// </summary>
        /// <param name="firstRow">The first row to exchange. This is an index of the row starting at zero.</param>
        /// <param name="secondRow">The second row to exchange. This is an index of the row starting at zero.</param>
        public void ExchangeRows(int firstRow, int secondRow)
        {
            if (firstRow < 0)
                throw new ArgumentOutOfRangeException(nameof(firstRow), "The parameter firstRow must be greater than or equal to zero.");
            if (firstRow > 2)
                throw new ArgumentOutOfRangeException(nameof(firstRow), "The parameter firstRow must be less than or equal to two.");
            if (secondRow < 0)
                throw new ArgumentOutOfRangeException(nameof(secondRow), "The parameter secondRow must be greater than or equal to zero.");
            if (secondRow > 2)
                throw new ArgumentOutOfRangeException(nameof(secondRow), "The parameter secondRow must be less than or equal to two.");

            if (firstRow == secondRow)
                return;

            float temp0 = this[secondRow, 0];
            float temp1 = this[secondRow, 1];
            float temp2 = this[secondRow, 2];

            this[secondRow, 0] = this[firstRow, 0];
            this[secondRow, 1] = this[firstRow, 1];
            this[secondRow, 2] = this[firstRow, 2];

            this[firstRow, 0] = temp0;
            this[firstRow, 1] = temp1;
            this[firstRow, 2] = temp2;
        }

        /// <summary>
        /// Exchanges two columns in the Matrix3x3.
        /// </summary>
        /// <param name="firstColumn">The first column to exchange. This is an index of the column starting at zero.</param>
        /// <param name="secondColumn">The second column to exchange. This is an index of the column starting at zero.</param>
        public void ExchangeColumns(int firstColumn, int secondColumn)
        {
            if (firstColumn < 0)
                throw new ArgumentOutOfRangeException(nameof(firstColumn), "The parameter firstColumn must be greater than or equal to zero.");
            if (firstColumn > 2)
                throw new ArgumentOutOfRangeException(nameof(firstColumn), "The parameter firstColumn must be less than or equal to two.");
            if (secondColumn < 0)
                throw new ArgumentOutOfRangeException(nameof(secondColumn), "The parameter secondColumn must be greater than or equal to zero.");
            if (secondColumn > 2)
                throw new ArgumentOutOfRangeException(nameof(secondColumn), "The parameter secondColumn must be less than or equal to two.");

            if (firstColumn == secondColumn)
                return;

            float temp0 = this[0, secondColumn];
            float temp1 = this[1, secondColumn];
            float temp2 = this[2, secondColumn];

            this[0, secondColumn] = this[0, firstColumn];
            this[1, secondColumn] = this[1, firstColumn];
            this[2, secondColumn] = this[2, firstColumn];

            this[0, firstColumn] = temp0;
            this[1, firstColumn] = temp1;
            this[2, firstColumn] = temp2;
        }

        /// <summary>
        /// Creates an array containing the elements of the Matrix3x3.
        /// </summary>
        /// <returns>A 9-element array containing the components of the Matrix3x3.</returns>
        public float[] ToArray()
        {
            return new[]
            {
                M11,
                M12,
                M13,
                M21,
                M22,
                M23,
                M31,
                M32,
                M33
            };
        }

        /// <summary>
        /// Determines the sum of two matrices.
        /// </summary>
        /// <param name="left">The first Matrix3x3 to add.</param>
        /// <param name="right">The second Matrix3x3 to add.</param>
        /// <param name="result">When the method completes, contains the sum of the two matrices.</param>
        public static void Add(ref Matrix3x3 left, ref Matrix3x3 right, out Matrix3x3 result)
        {
            result.M11 = left.M11 + right.M11;
            result.M12 = left.M12 + right.M12;
            result.M13 = left.M13 + right.M13;
            result.M21 = left.M21 + right.M21;
            result.M22 = left.M22 + right.M22;
            result.M23 = left.M23 + right.M23;
            result.M31 = left.M31 + right.M31;
            result.M32 = left.M32 + right.M32;
            result.M33 = left.M33 + right.M33;
        }

        /// <summary>
        /// Determines the sum of two matrices.
        /// </summary>
        /// <param name="left">The first Matrix3x3 to add.</param>
        /// <param name="right">The second Matrix3x3 to add.</param>
        /// <returns>The sum of the two matrices.</returns>
        public static Matrix3x3 Add(Matrix3x3 left, Matrix3x3 right)
        {
            Add(ref left, ref right, out Matrix3x3 result);
            return result;
        }

        /// <summary>
        /// Determines the difference between two matrices.
        /// </summary>
        /// <param name="left">The first Matrix3x3 to subtract.</param>
        /// <param name="right">The second Matrix3x3 to subtract.</param>
        /// <param name="result">When the method completes, contains the difference between the two matrices.</param>
        public static void Subtract(ref Matrix3x3 left, ref Matrix3x3 right, out Matrix3x3 result)
        {
            result.M11 = left.M11 - right.M11;
            result.M12 = left.M12 - right.M12;
            result.M13 = left.M13 - right.M13;
            result.M21 = left.M21 - right.M21;
            result.M22 = left.M22 - right.M22;
            result.M23 = left.M23 - right.M23;
            result.M31 = left.M31 - right.M31;
            result.M32 = left.M32 - right.M32;
            result.M33 = left.M33 - right.M33;
        }

        /// <summary>
        /// Determines the difference between two matrices.
        /// </summary>
        /// <param name="left">The first Matrix3x3 to subtract.</param>
        /// <param name="right">The second Matrix3x3 to subtract.</param>
        /// <returns>The difference between the two matrices.</returns>
        public static Matrix3x3 Subtract(Matrix3x3 left, Matrix3x3 right)
        {
            Subtract(ref left, ref right, out Matrix3x3 result);
            return result;
        }

        /// <summary>
        /// Scales a Matrix3x3 by the given value.
        /// </summary>
        /// <param name="left">The Matrix3x3 to scale.</param>
        /// <param name="right">The amount by which to scale.</param>
        /// <param name="result">When the method completes, contains the scaled Matrix3x3.</param>
        public static void Multiply(ref Matrix3x3 left, float right, out Matrix3x3 result)
        {
            result.M11 = left.M11 * right;
            result.M12 = left.M12 * right;
            result.M13 = left.M13 * right;
            result.M21 = left.M21 * right;
            result.M22 = left.M22 * right;
            result.M23 = left.M23 * right;
            result.M31 = left.M31 * right;
            result.M32 = left.M32 * right;
            result.M33 = left.M33 * right;
        }

        /// <summary>
        /// Scales a Matrix3x3 by the given value.
        /// </summary>
        /// <param name="left">The Matrix3x3 to scale.</param>
        /// <param name="right">The amount by which to scale.</param>
        /// <returns>The scaled Matrix3x3.</returns>
        public static Matrix3x3 Multiply(Matrix3x3 left, float right)
        {
            Multiply(ref left, right, out Matrix3x3 result);
            return result;
        }

        /// <summary>
        /// Determines the product of two matrices.
        /// </summary>
        /// <param name="left">The first Matrix3x3 to multiply.</param>
        /// <param name="right">The second Matrix3x3 to multiply.</param>
        /// <param name="result">The product of the two matrices.</param>
        public static void Multiply(ref Matrix3x3 left, ref Matrix3x3 right, out Matrix3x3 result)
        {
            Matrix3x3 temp = new Matrix3x3
            {
                M11 = (left.M11 * right.M11) + (left.M12 * right.M21) + (left.M13 * right.M31),
                M12 = (left.M11 * right.M12) + (left.M12 * right.M22) + (left.M13 * right.M32),
                M13 = (left.M11 * right.M13) + (left.M12 * right.M23) + (left.M13 * right.M33),
                M21 = (left.M21 * right.M11) + (left.M22 * right.M21) + (left.M23 * right.M31),
                M22 = (left.M21 * right.M12) + (left.M22 * right.M22) + (left.M23 * right.M32),
                M23 = (left.M21 * right.M13) + (left.M22 * right.M23) + (left.M23 * right.M33),
                M31 = (left.M31 * right.M11) + (left.M32 * right.M21) + (left.M33 * right.M31),
                M32 = (left.M31 * right.M12) + (left.M32 * right.M22) + (left.M33 * right.M32),
                M33 = (left.M31 * right.M13) + (left.M32 * right.M23) + (left.M33 * right.M33)
            };
            result = temp;
        }

        /// <summary>
        /// Determines the product of two matrices.
        /// </summary>
        /// <param name="left">The first Matrix3x3 to multiply.</param>
        /// <param name="right">The second Matrix3x3 to multiply.</param>
        /// <returns>The product of the two matrices.</returns>
        public static Matrix3x3 Multiply(Matrix3x3 left, Matrix3x3 right)
        {
            Multiply(ref left, ref right, out Matrix3x3 result);
            return result;
        }

        /// <summary>
        /// Scales a Matrix3x3 by the given value.
        /// </summary>
        /// <param name="left">The Matrix3x3 to scale.</param>
        /// <param name="right">The amount by which to scale.</param>
        /// <param name="result">When the method completes, contains the scaled Matrix3x3.</param>
        public static void Divide(ref Matrix3x3 left, float right, out Matrix3x3 result)
        {
            float inv = 1.0f / right;

            result.M11 = left.M11 * inv;
            result.M12 = left.M12 * inv;
            result.M13 = left.M13 * inv;
            result.M21 = left.M21 * inv;
            result.M22 = left.M22 * inv;
            result.M23 = left.M23 * inv;
            result.M31 = left.M31 * inv;
            result.M32 = left.M32 * inv;
            result.M33 = left.M33 * inv;
        }

        /// <summary>
        /// Scales a Matrix3x3 by the given value.
        /// </summary>
        /// <param name="left">The Matrix3x3 to scale.</param>
        /// <param name="right">The amount by which to scale.</param>
        /// <returns>The scaled Matrix3x3.</returns>
        public static Matrix3x3 Divide(Matrix3x3 left, float right)
        {
            Divide(ref left, right, out Matrix3x3 result);
            return result;
        }

        /// <summary>
        /// Determines the quotient of two matrices.
        /// </summary>
        /// <param name="left">The first Matrix3x3 to divide.</param>
        /// <param name="right">The second Matrix3x3 to divide.</param>
        /// <param name="result">When the method completes, contains the quotient of the two matrices.</param>
        public static void Divide(ref Matrix3x3 left, ref Matrix3x3 right, out Matrix3x3 result)
        {
            result.M11 = left.M11 / right.M11;
            result.M12 = left.M12 / right.M12;
            result.M13 = left.M13 / right.M13;
            result.M21 = left.M21 / right.M21;
            result.M22 = left.M22 / right.M22;
            result.M23 = left.M23 / right.M23;
            result.M31 = left.M31 / right.M31;
            result.M32 = left.M32 / right.M32;
            result.M33 = left.M33 / right.M33;
        }

        /// <summary>
        /// Determines the quotient of two matrices.
        /// </summary>
        /// <param name="left">The first Matrix3x3 to divide.</param>
        /// <param name="right">The second Matrix3x3 to divide.</param>
        /// <returns>The quotient of the two matrices.</returns>
        public static Matrix3x3 Divide(Matrix3x3 left, Matrix3x3 right)
        {
            Divide(ref left, ref right, out Matrix3x3 result);
            return result;
        }

        /// <summary>
        /// Performs the exponential operation on a Matrix3x3.
        /// </summary>
        /// <param name="value">The Matrix3x3 to perform the operation on.</param>
        /// <param name="exponent">The exponent to raise the Matrix3x3 to.</param>
        /// <param name="result">When the method completes, contains the exponential Matrix3x3.</param>
        /// <exception cref="System.ArgumentOutOfRangeException">Thrown when the <paramref name="exponent"/> is negative.</exception>
        public static void Exponent(ref Matrix3x3 value, int exponent, out Matrix3x3 result)
        {
            //Source: http://rosettacode.org
            //Reference: http://rosettacode.org/wiki/Matrix3x3-exponentiation_operator

            if (exponent < 0)
                throw new ArgumentOutOfRangeException(nameof(exponent), "The exponent can not be negative.");

            if (exponent == 0)
            {
                result = Identity;
                return;
            }

            if (exponent == 1)
            {
                result = value;
                return;
            }

            Matrix3x3 identity = Identity;
            Matrix3x3 temp = value;

            while (true)
            {
                if ((exponent & 1) != 0)
                    identity *= temp;

                exponent /= 2;

                if (exponent > 0)
                    temp *= temp;
                else
                    break;
            }

            result = identity;
        }

        /// <summary>
        /// Performs the exponential operation on a Matrix3x3.
        /// </summary>
        /// <param name="value">The Matrix3x3 to perform the operation on.</param>
        /// <param name="exponent">The exponent to raise the Matrix3x3 to.</param>
        /// <returns>The exponential Matrix3x3.</returns>
        /// <exception cref="System.ArgumentOutOfRangeException">Thrown when the <paramref name="exponent"/> is negative.</exception>
        public static Matrix3x3 Exponent(Matrix3x3 value, int exponent)
        {
            Exponent(ref value, exponent, out Matrix3x3 result);
            return result;
        }

        /// <summary>
        /// Negates a Matrix3x3.
        /// </summary>
        /// <param name="value">The Matrix3x3 to be negated.</param>
        /// <param name="result">When the method completes, contains the negated Matrix3x3.</param>
        public static void Negate(ref Matrix3x3 value, out Matrix3x3 result)
        {
            result.M11 = -value.M11;
            result.M12 = -value.M12;
            result.M13 = -value.M13;
            result.M21 = -value.M21;
            result.M22 = -value.M22;
            result.M23 = -value.M23;
            result.M31 = -value.M31;
            result.M32 = -value.M32;
            result.M33 = -value.M33;
        }

        /// <summary>
        /// Negates a Matrix3x3.
        /// </summary>
        /// <param name="value">The Matrix3x3 to be negated.</param>
        /// <returns>The negated Matrix3x3.</returns>
        public static Matrix3x3 Negate(Matrix3x3 value)
        {
            Negate(ref value, out Matrix3x3 result);
            return result;
        }

        /// <summary>
        /// Performs a linear interpolation between two matrices.
        /// </summary>
        /// <param name="start">Start Matrix3x3.</param>
        /// <param name="end">End Matrix3x3.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end"/>.</param>
        /// <param name="result">When the method completes, contains the linear interpolation of the two matrices.</param>
        /// <remarks>
        /// Passing <paramref name="amount"/> a value of 0 will cause <paramref name="start"/> to be returned; a value of 1 will cause <paramref name="end"/> to be returned. 
        /// </remarks>
        public static void Lerp(ref Matrix3x3 start, ref Matrix3x3 end, float amount, out Matrix3x3 result)
        {
            result.M11 = Mathf.Lerp(start.M11, end.M11, amount);
            result.M12 = Mathf.Lerp(start.M12, end.M12, amount);
            result.M13 = Mathf.Lerp(start.M13, end.M13, amount);
            result.M21 = Mathf.Lerp(start.M21, end.M21, amount);
            result.M22 = Mathf.Lerp(start.M22, end.M22, amount);
            result.M23 = Mathf.Lerp(start.M23, end.M23, amount);
            result.M31 = Mathf.Lerp(start.M31, end.M31, amount);
            result.M32 = Mathf.Lerp(start.M32, end.M32, amount);
            result.M33 = Mathf.Lerp(start.M33, end.M33, amount);
        }

        /// <summary>
        /// Performs a linear interpolation between two matrices.
        /// </summary>
        /// <param name="start">Start Matrix3x3.</param>
        /// <param name="end">End Matrix3x3.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end"/>.</param>
        /// <returns>The linear interpolation of the two matrices.</returns>
        /// <remarks>
        /// Passing <paramref name="amount"/> a value of 0 will cause <paramref name="start"/> to be returned; a value of 1 will cause <paramref name="end"/> to be returned. 
        /// </remarks>
        public static Matrix3x3 Lerp(Matrix3x3 start, Matrix3x3 end, float amount)
        {
            Lerp(ref start, ref end, amount, out Matrix3x3 result);
            return result;
        }

        /// <summary>
        /// Performs a cubic interpolation between two matrices.
        /// </summary>
        /// <param name="start">Start Matrix3x3.</param>
        /// <param name="end">End Matrix3x3.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end"/>.</param>
        /// <param name="result">When the method completes, contains the cubic interpolation of the two matrices.</param>
        public static void SmoothStep(ref Matrix3x3 start, ref Matrix3x3 end, float amount, out Matrix3x3 result)
        {
            amount = Mathf.SmoothStep(amount);
            Lerp(ref start, ref end, amount, out result);
        }

        /// <summary>
        /// Performs a cubic interpolation between two matrices.
        /// </summary>
        /// <param name="start">Start Matrix3x3.</param>
        /// <param name="end">End Matrix3x3.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end"/>.</param>
        /// <returns>The cubic interpolation of the two matrices.</returns>
        public static Matrix3x3 SmoothStep(Matrix3x3 start, Matrix3x3 end, float amount)
        {
            SmoothStep(ref start, ref end, amount, out Matrix3x3 result);
            return result;
        }

        /// <summary>
        /// Calculates the transpose of the specified Matrix3x3.
        /// </summary>
        /// <param name="value">The Matrix3x3 whose transpose is to be calculated.</param>
        /// <param name="result">When the method completes, contains the transpose of the specified Matrix3x3.</param>
        public static void Transpose(ref Matrix3x3 value, out Matrix3x3 result)
        {
            Matrix3x3 temp = new Matrix3x3
            {
                M11 = value.M11,
                M12 = value.M21,
                M13 = value.M31,
                M21 = value.M12,
                M22 = value.M22,
                M23 = value.M32,
                M31 = value.M13,
                M32 = value.M23,
                M33 = value.M33
            };

            result = temp;
        }

        /// <summary>
        /// Calculates the transpose of the specified Matrix3x3.
        /// </summary>
        /// <param name="value">The Matrix3x3 whose transpose is to be calculated.</param>
        /// <param name="result">When the method completes, contains the transpose of the specified Matrix3x3.</param>
        public static void TransposeByRef(ref Matrix3x3 value, ref Matrix3x3 result)
        {
            result.M11 = value.M11;
            result.M12 = value.M21;
            result.M13 = value.M31;
            result.M21 = value.M12;
            result.M22 = value.M22;
            result.M23 = value.M32;
            result.M31 = value.M13;
            result.M32 = value.M23;
            result.M33 = value.M33;
        }

        /// <summary>
        /// Calculates the transpose of the specified Matrix3x3.
        /// </summary>
        /// <param name="value">The Matrix3x3 whose transpose is to be calculated.</param>
        /// <returns>The transpose of the specified Matrix3x3.</returns>
        public static Matrix3x3 Transpose(Matrix3x3 value)
        {
            Transpose(ref value, out Matrix3x3 result);
            return result;
        }

        /// <summary>
        /// Calculates the inverse of the specified Matrix3x3.
        /// </summary>
        /// <param name="value">The Matrix3x3 whose inverse is to be calculated.</param>
        /// <param name="result">When the method completes, contains the inverse of the specified Matrix3x3.</param>
        public static void Invert(ref Matrix3x3 value, out Matrix3x3 result)
        {
            float d11 = value.M22 * value.M33 + value.M23 * -value.M32;
            float d12 = value.M21 * value.M33 + value.M23 * -value.M31;
            float d13 = value.M21 * value.M32 + value.M22 * -value.M31;

            float det = value.M11 * d11 - value.M12 * d12 + value.M13 * d13;
            if (Math.Abs(det) == 0.0f)
            {
                result = Zero;
                return;
            }

            det = 1f / det;

            float d21 = value.M12 * value.M33 + value.M13 * -value.M32;
            float d22 = value.M11 * value.M33 + value.M13 * -value.M31;
            float d23 = value.M11 * value.M32 + value.M12 * -value.M31;

            float d31 = (value.M12 * value.M23) - (value.M13 * value.M22);
            float d32 = (value.M11 * value.M23) - (value.M13 * value.M21);
            float d33 = (value.M11 * value.M22) - (value.M12 * value.M21);

            result.M11 = +d11 * det;
            result.M12 = -d21 * det;
            result.M13 = +d31 * det;
            result.M21 = -d12 * det;
            result.M22 = +d22 * det;
            result.M23 = -d32 * det;
            result.M31 = +d13 * det;
            result.M32 = -d23 * det;
            result.M33 = +d33 * det;
        }

        /// <summary>
        /// Calculates the inverse of the specified Matrix3x3.
        /// </summary>
        /// <param name="value">The Matrix3x3 whose inverse is to be calculated.</param>
        /// <returns>The inverse of the specified Matrix3x3.</returns>
        public static Matrix3x3 Invert(Matrix3x3 value)
        {
            value.Invert();
            return value;
        }

        /// <summary>
        /// Orthogonalizes the specified Matrix3x3.
        /// </summary>
        /// <param name="value">The Matrix3x3 to orthogonalize.</param>
        /// <param name="result">When the method completes, contains the orthogonalized Matrix3x3.</param>
        /// <remarks>
        /// <para>Orthogonalization is the process of making all rows orthogonal to each other. This
        /// means that any given row in the Matrix3x3 will be orthogonal to any other given row in the
        /// Matrix3x3.</para>
        /// <para>Because this method uses the modified Gram-Schmidt process, the resulting Matrix3x3
        /// tends to be numerically unstable. The numeric stability decreases according to the rows
        /// so that the first row is the most stable and the last row is the least stable.</para>
        /// <para>This operation is performed on the rows of the Matrix3x3 rather than the columns.
        /// If you wish for this operation to be performed on the columns, first transpose the
        /// input and than transpose the output.</para>
        /// </remarks>
        public static void Orthogonalize(ref Matrix3x3 value, out Matrix3x3 result)
        {
            //Uses the modified Gram-Schmidt process.
            //q1 = m1
            //q2 = m2 - ((q1 ⋅ m2) / (q1 ⋅ q1)) * q1
            //q3 = m3 - ((q1 ⋅ m3) / (q1 ⋅ q1)) * q1 - ((q2 ⋅ m3) / (q2 ⋅ q2)) * q2

            //By separating the above algorithm into multiple lines, we actually increase accuracy.
            result = value;

            result.Row2 -= (Float3.Dot(result.Row1, result.Row2) / Float3.Dot(result.Row1, result.Row1)) * result.Row1;

            result.Row3 -= (Float3.Dot(result.Row1, result.Row3) / Float3.Dot(result.Row1, result.Row1)) * result.Row1;
            result.Row3 -= (Float3.Dot(result.Row2, result.Row3) / Float3.Dot(result.Row2, result.Row2)) * result.Row2;
        }

        /// <summary>
        /// Orthogonalizes the specified Matrix3x3.
        /// </summary>
        /// <param name="value">The Matrix3x3 to orthogonalize.</param>
        /// <returns>The orthogonalized Matrix3x3.</returns>
        /// <remarks>
        /// <para>Orthogonalization is the process of making all rows orthogonal to each other. This
        /// means that any given row in the Matrix3x3 will be orthogonal to any other given row in the
        /// Matrix3x3.</para>
        /// <para>Because this method uses the modified Gram-Schmidt process, the resulting Matrix3x3
        /// tends to be numerically unstable. The numeric stability decreases according to the rows
        /// so that the first row is the most stable and the last row is the least stable.</para>
        /// <para>This operation is performed on the rows of the Matrix3x3 rather than the columns.
        /// If you wish for this operation to be performed on the columns, first transpose the
        /// input and than transpose the output.</para>
        /// </remarks>
        public static Matrix3x3 Orthogonalize(Matrix3x3 value)
        {
            Orthogonalize(ref value, out Matrix3x3 result);
            return result;
        }

        /// <summary>
        /// Orthonormalizes the specified Matrix3x3.
        /// </summary>
        /// <param name="value">The Matrix3x3 to orthonormalize.</param>
        /// <param name="result">When the method completes, contains the orthonormalized Matrix3x3.</param>
        /// <remarks>
        /// <para>Orthonormalization is the process of making all rows and columns orthogonal to each
        /// other and making all rows and columns of unit length. This means that any given row will
        /// be orthogonal to any other given row and any given column will be orthogonal to any other
        /// given column. Any given row will not be orthogonal to any given column. Every row and every
        /// column will be of unit length.</para>
        /// <para>Because this method uses the modified Gram-Schmidt process, the resulting Matrix3x3
        /// tends to be numerically unstable. The numeric stability decreases according to the rows
        /// so that the first row is the most stable and the last row is the least stable.</para>
        /// <para>This operation is performed on the rows of the Matrix3x3 rather than the columns.
        /// If you wish for this operation to be performed on the columns, first transpose the
        /// input and than transpose the output.</para>
        /// </remarks>
        public static void Orthonormalize(ref Matrix3x3 value, out Matrix3x3 result)
        {
            //Uses the modified Gram-Schmidt process.
            //Because we are making unit vectors, we can optimize the math for orthonormalization
            //and simplify the projection operation to remove the division.
            //q1 = m1 / |m1|
            //q2 = (m2 - (q1 ⋅ m2) * q1) / |m2 - (q1 ⋅ m2) * q1|
            //q3 = (m3 - (q1 ⋅ m3) * q1 - (q2 ⋅ m3) * q2) / |m3 - (q1 ⋅ m3) * q1 - (q2 ⋅ m3) * q2|

            //By separating the above algorithm into multiple lines, we actually increase accuracy.
            result = value;

            result.Row1 = Float3.Normalize(result.Row1);

            result.Row2 -= Float3.Dot(result.Row1, result.Row2) * result.Row1;
            result.Row2 = Float3.Normalize(result.Row2);

            result.Row3 -= Float3.Dot(result.Row1, result.Row3) * result.Row1;
            result.Row3 -= Float3.Dot(result.Row2, result.Row3) * result.Row2;
            result.Row3 = Float3.Normalize(result.Row3);
        }

        /// <summary>
        /// Orthonormalizes the specified Matrix3x3.
        /// </summary>
        /// <param name="value">The Matrix3x3 to orthonormalize.</param>
        /// <returns>The orthonormalized Matrix3x3.</returns>
        /// <remarks>
        /// <para>Orthonormalization is the process of making all rows and columns orthogonal to each
        /// other and making all rows and columns of unit length. This means that any given row will
        /// be orthogonal to any other given row and any given column will be orthogonal to any other
        /// given column. Any given row will not be orthogonal to any given column. Every row and every
        /// column will be of unit length.</para>
        /// <para>Because this method uses the modified Gram-Schmidt process, the resulting Matrix3x3
        /// tends to be numerically unstable. The numeric stability decreases according to the rows
        /// so that the first row is the most stable and the last row is the least stable.</para>
        /// <para>This operation is performed on the rows of the Matrix3x3 rather than the columns.
        /// If you wish for this operation to be performed on the columns, first transpose the
        /// input and than transpose the output.</para>
        /// </remarks>
        public static Matrix3x3 Orthonormalize(Matrix3x3 value)
        {
            Orthonormalize(ref value, out Matrix3x3 result);
            return result;
        }

        /// <summary>
        /// Brings the Matrix3x3 into upper triangular form using elementary row operations.
        /// </summary>
        /// <param name="value">The Matrix3x3 to put into upper triangular form.</param>
        /// <param name="result">When the method completes, contains the upper triangular Matrix3x3.</param>
        /// <remarks>
        /// If the Matrix3x3 is not invertible (i.e. its determinant is zero) than the result of this
        /// method may produce Single.Nan and Single.Inf values. When the Matrix3x3 represents a system
        /// of linear equations, than this often means that either no solution exists or an infinite
        /// number of solutions exist.
        /// </remarks>
        public static void UpperTriangularForm(ref Matrix3x3 value, out Matrix3x3 result)
        {
            //Adapted from the row echelon code.
            result = value;
            int lead = 0;
            int rowcount = 3;
            int columncount = 3;

            for (int r = 0; r < rowcount; ++r)
            {
                if (columncount <= lead)
                    return;

                int i = r;

                while (Mathf.IsZero(result[i, lead]))
                {
                    i++;

                    if (i == rowcount)
                    {
                        i = r;
                        lead++;

                        if (lead == columncount)
                            return;
                    }
                }

                if (i != r)
                {
                    result.ExchangeRows(i, r);
                }

                float multiplier = 1f / result[r, lead];

                for (; i < rowcount; ++i)
                {
                    if (i != r)
                    {
                        result[i, 0] -= result[r, 0] * multiplier * result[i, lead];
                        result[i, 1] -= result[r, 1] * multiplier * result[i, lead];
                        result[i, 2] -= result[r, 2] * multiplier * result[i, lead];
                    }
                }

                lead++;
            }
        }

        /// <summary>
        /// Brings the Matrix3x3 into upper triangular form using elementary row operations.
        /// </summary>
        /// <param name="value">The Matrix3x3 to put into upper triangular form.</param>
        /// <returns>The upper triangular Matrix3x3.</returns>
        /// <remarks>
        /// If the Matrix3x3 is not invertible (i.e. its determinant is zero) than the result of this
        /// method may produce Single.Nan and Single.Inf values. When the Matrix3x3 represents a system
        /// of linear equations, than this often means that either no solution exists or an infinite
        /// number of solutions exist.
        /// </remarks>
        public static Matrix3x3 UpperTriangularForm(Matrix3x3 value)
        {
            UpperTriangularForm(ref value, out Matrix3x3 result);
            return result;
        }

        /// <summary>
        /// Brings the Matrix3x3 into lower triangular form using elementary row operations.
        /// </summary>
        /// <param name="value">The Matrix3x3 to put into lower triangular form.</param>
        /// <param name="result">When the method completes, contains the lower triangular Matrix3x3.</param>
        /// <remarks>
        /// If the Matrix3x3 is not invertible (i.e. its determinant is zero) than the result of this
        /// method may produce Single.Nan and Single.Inf values. When the Matrix3x3 represents a system
        /// of linear equations, than this often means that either no solution exists or an infinite
        /// number of solutions exist.
        /// </remarks>
        public static void LowerTriangularForm(ref Matrix3x3 value, out Matrix3x3 result)
        {
            //Adapted from the row echelon code.
            Matrix3x3 temp = value;
            Transpose(ref temp, out result);

            int lead = 0;
            int rowcount = 3;
            int columncount = 3;

            for (int r = 0; r < rowcount; ++r)
            {
                if (columncount <= lead)
                    return;

                int i = r;

                while (Mathf.IsZero(result[i, lead]))
                {
                    i++;

                    if (i == rowcount)
                    {
                        i = r;
                        lead++;

                        if (lead == columncount)
                            return;
                    }
                }

                if (i != r)
                {
                    result.ExchangeRows(i, r);
                }

                float multiplier = 1f / result[r, lead];

                for (; i < rowcount; ++i)
                {
                    if (i != r)
                    {
                        result[i, 0] -= result[r, 0] * multiplier * result[i, lead];
                        result[i, 1] -= result[r, 1] * multiplier * result[i, lead];
                        result[i, 2] -= result[r, 2] * multiplier * result[i, lead];
                    }
                }

                lead++;
            }

            Transpose(ref result, out result);
        }

        /// <summary>
        /// Brings the Matrix3x3 into lower triangular form using elementary row operations.
        /// </summary>
        /// <param name="value">The Matrix3x3 to put into lower triangular form.</param>
        /// <returns>The lower triangular Matrix3x3.</returns>
        /// <remarks>
        /// If the Matrix3x3 is not invertible (i.e. its determinant is zero) than the result of this
        /// method may produce Single.Nan and Single.Inf values. When the Matrix3x3 represents a system
        /// of linear equations, than this often means that either no solution exists or an infinite
        /// number of solutions exist.
        /// </remarks>
        public static Matrix3x3 LowerTriangularForm(Matrix3x3 value)
        {
            LowerTriangularForm(ref value, out Matrix3x3 result);
            return result;
        }

        /// <summary>
        /// Brings the Matrix3x3 into row echelon form using elementary row operations;
        /// </summary>
        /// <param name="value">The Matrix3x3 to put into row echelon form.</param>
        /// <param name="result">When the method completes, contains the row echelon form of the Matrix3x3.</param>
        public static void RowEchelonForm(ref Matrix3x3 value, out Matrix3x3 result)
        {
            //Source: Wikipedia pseudo code
            //Reference: http://en.wikipedia.org/wiki/Row_echelon_form#Pseudocode

            result = value;
            int lead = 0;
            int rowcount = 3;
            int columncount = 3;

            for (int r = 0; r < rowcount; ++r)
            {
                if (columncount <= lead)
                    return;

                int i = r;

                while (Mathf.IsZero(result[i, lead]))
                {
                    i++;

                    if (i == rowcount)
                    {
                        i = r;
                        lead++;

                        if (lead == columncount)
                            return;
                    }
                }

                if (i != r)
                {
                    result.ExchangeRows(i, r);
                }

                float multiplier = 1f / result[r, lead];
                result[r, 0] *= multiplier;
                result[r, 1] *= multiplier;
                result[r, 2] *= multiplier;

                for (; i < rowcount; ++i)
                {
                    if (i != r)
                    {
                        result[i, 0] -= result[r, 0] * result[i, lead];
                        result[i, 1] -= result[r, 1] * result[i, lead];
                        result[i, 2] -= result[r, 2] * result[i, lead];
                    }
                }

                lead++;
            }
        }

        /// <summary>
        /// Brings the Matrix3x3 into row echelon form using elementary row operations;
        /// </summary>
        /// <param name="value">The Matrix3x3 to put into row echelon form.</param>
        /// <returns>When the method completes, contains the row echelon form of the Matrix3x3.</returns>
        public static Matrix3x3 RowEchelonForm(Matrix3x3 value)
        {
            RowEchelonForm(ref value, out Matrix3x3 result);
            return result;
        }

        /// <summary>
        /// Creates a left-handed spherical billboard that rotates around a specified object position.
        /// </summary>
        /// <param name="objectPosition">The position of the object around which the billboard will rotate.</param>
        /// <param name="cameraPosition">The position of the camera.</param>
        /// <param name="cameraUpFloat">The up vector of the camera.</param>
        /// <param name="cameraForwardFloat">The forward vector of the camera.</param>
        /// <param name="result">When the method completes, contains the created billboard Matrix3x3.</param>
        public static void Billboard(ref Float3 objectPosition, ref Float3 cameraPosition, ref Float3 cameraUpFloat, ref Float3 cameraForwardFloat, out Matrix3x3 result)
        {
            Float3 difference = cameraPosition - objectPosition;

            float lengthSq = difference.LengthSquared;
            if (Mathf.IsZero(lengthSq))
                difference = -cameraForwardFloat;
            else
                difference *= (float)(1.0 / Math.Sqrt(lengthSq));

            Float3.Cross(ref cameraUpFloat, ref difference, out Float3 crossed);
            crossed.Normalize();
            Float3.Cross(ref difference, ref crossed, out Float3 final);

            result.M11 = crossed.X;
            result.M12 = crossed.Y;
            result.M13 = crossed.Z;
            result.M21 = final.X;
            result.M22 = final.Y;
            result.M23 = final.Z;
            result.M31 = difference.X;
            result.M32 = difference.Y;
            result.M33 = difference.Z;
        }

        /// <summary>
        /// Creates a left-handed spherical billboard that rotates around a specified object position.
        /// </summary>
        /// <param name="objectPosition">The position of the object around which the billboard will rotate.</param>
        /// <param name="cameraPosition">The position of the camera.</param>
        /// <param name="cameraUpFloat">The up vector of the camera.</param>
        /// <param name="cameraForwardFloat">The forward vector of the camera.</param>
        /// <returns>The created billboard Matrix3x3.</returns>
        public static Matrix3x3 Billboard(Float3 objectPosition, Float3 cameraPosition, Float3 cameraUpFloat, Float3 cameraForwardFloat)
        {
            Billboard(ref objectPosition, ref cameraPosition, ref cameraUpFloat, ref cameraForwardFloat, out Matrix3x3 result);
            return result;
        }

        /// <summary>
        /// Creates a left-handed, look-at Matrix3x3.
        /// </summary>
        /// <param name="eye">The position of the viewer's eye.</param>
        /// <param name="target">The camera look-at target.</param>
        /// <param name="up">The camera's up vector.</param>
        /// <param name="result">When the method completes, contains the created look-at Matrix3x3.</param>
        public static void LookAt(ref Float3 eye, ref Float3 target, ref Float3 up, out Matrix3x3 result)
        {
            Float3.Subtract(ref target, ref eye, out Float3 zaxis);
            zaxis.Normalize();

            Float3.Cross(ref up, ref zaxis, out Float3 xaxis);
            xaxis.Normalize();

            Float3.Cross(ref zaxis, ref xaxis, out Float3 yaxis);

            result = Identity;
            result.M11 = xaxis.X;
            result.M21 = xaxis.Y;
            result.M31 = xaxis.Z;
            result.M12 = yaxis.X;
            result.M22 = yaxis.Y;
            result.M32 = yaxis.Z;
            result.M13 = zaxis.X;
            result.M23 = zaxis.Y;
            result.M33 = zaxis.Z;
        }

        /// <summary>
        /// Creates a left-handed, look-at Matrix3x3.
        /// </summary>
        /// <param name="eye">The position of the viewer's eye.</param>
        /// <param name="target">The camera look-at target.</param>
        /// <param name="up">The camera's up vector.</param>
        /// <returns>The created look-at Matrix3x3.</returns>
        public static Matrix3x3 LookAt(Float3 eye, Float3 target, Float3 up)
        {
            LookAt(ref eye, ref target, ref up, out Matrix3x3 result);
            return result;
        }

        /// <summary>
        /// Creates a Matrix3x3 that scales along the x-axis, y-axis, and y-axis.
        /// </summary>
        /// <param name="scale">Scaling factor for all three axes.</param>
        /// <param name="result">When the method completes, contains the created scaling Matrix3x3.</param>
        public static void Scaling(ref Float3 scale, out Matrix3x3 result)
        {
            Scaling(scale.X, scale.Y, scale.Z, out result);
        }

        /// <summary>
        /// Creates a Matrix3x3 that scales along the x-axis, y-axis, and z-axis.
        /// </summary>
        /// <param name="scale">Scaling factor for all three axes.</param>
        /// <returns>The created scaling Matrix3x3.</returns>
        public static Matrix3x3 Scaling(Float3 scale)
        {
            Scaling(ref scale, out Matrix3x3 result);
            return result;
        }

        /// <summary>
        /// Creates a Matrix3x3 that scales along the x-axis, y-axis, and z-axis.
        /// </summary>
        /// <param name="x">Scaling factor that is applied along the x-axis.</param>
        /// <param name="y">Scaling factor that is applied along the y-axis.</param>
        /// <param name="z">Scaling factor that is applied along the z-axis.</param>
        /// <param name="result">When the method completes, contains the created scaling Matrix3x3.</param>
        public static void Scaling(float x, float y, float z, out Matrix3x3 result)
        {
            result = Identity;
            result.M11 = x;
            result.M22 = y;
            result.M33 = z;
        }

        /// <summary>
        /// Creates a Matrix3x3 that scales along the x-axis, y-axis, and y-axis.
        /// </summary>
        /// <param name="x">Scaling factor that is applied along the x-axis.</param>
        /// <param name="y">Scaling factor that is applied along the y-axis.</param>
        /// <param name="z">Scaling factor that is applied along the z-axis.</param>
        /// <returns>The created scaling Matrix3x3.</returns>
        public static Matrix3x3 Scaling(float x, float y, float z)
        {
            Scaling(x, y, z, out Matrix3x3 result);
            return result;
        }

        /// <summary>
        /// Creates a Matrix3x3 that uniformly scales along all three axes.
        /// </summary>
        /// <param name="scale">The uniform scale that is applied along all axes.</param>
        /// <param name="result">When the method completes, contains the created scaling Matrix3x3.</param>
        public static void Scaling(float scale, out Matrix3x3 result)
        {
            result = Identity;
            result.M11 = result.M22 = result.M33 = scale;
        }

        /// <summary>
        /// Creates a Matrix3x3 that uniformly scales along all three axes.
        /// </summary>
        /// <param name="scale">The uniform scale that is applied along all axes.</param>
        /// <returns>The created scaling Matrix3x3.</returns>
        public static Matrix3x3 Scaling(float scale)
        {
            Scaling(scale, out Matrix3x3 result);
            return result;
        }

        /// <summary>
        /// Creates the 2D shear matrix. Represented by:
        /// [1 Y 0]
        /// [X 1 0]
        /// [0 0 1]
        /// </summary>
        /// <param name="shearAngles">The shear angles (in degrees).</param>
        /// <param name="result">The result.</param>
        public static void Shear(ref Float2 shearAngles, out Matrix3x3 result)
        {
            float shearX = shearAngles.X == 0 ? 0 : (1.0f / Mathf.Tan(Mathf.DegreesToRadians * (90 - Mathf.Clamp(shearAngles.X, -89.0f, 89.0f))));
            float shearY = shearAngles.Y == 0 ? 0 : (1.0f / Mathf.Tan(Mathf.DegreesToRadians * (90 - Mathf.Clamp(shearAngles.Y, -89.0f, 89.0f))));

            result = Identity;
            result.M21 = shearX;
            result.M12 = shearY;
        }

        /// <summary>
        /// Creates a Matrix3x3 that rotates around the x-axis.
        /// </summary>
        /// <param name="angle">Angle of rotation in radians. Angles are measured clockwise when looking along the rotation axis toward the origin.</param>
        /// <param name="result">When the method completes, contains the created rotation Matrix3x3.</param>
        public static void RotationX(float angle, out Matrix3x3 result)
        {
            float cos = (float)Math.Cos(angle);
            float sin = (float)Math.Sin(angle);

            result = Identity;
            result.M22 = cos;
            result.M23 = sin;
            result.M32 = -sin;
            result.M33 = cos;
        }

        /// <summary>
        /// Creates a Matrix3x3 that rotates around the x-axis.
        /// </summary>
        /// <param name="angle">Angle of rotation in radians. Angles are measured clockwise when looking along the rotation axis toward the origin.</param>
        /// <returns>The created rotation Matrix3x3.</returns>
        public static Matrix3x3 RotationX(float angle)
        {
            RotationX(angle, out Matrix3x3 result);
            return result;
        }

        /// <summary>
        /// Creates a Matrix3x3 that rotates around the y-axis.
        /// </summary>
        /// <param name="angle">Angle of rotation in radians. Angles are measured clockwise when looking along the rotation axis toward the origin.</param>
        /// <param name="result">When the method completes, contains the created rotation Matrix3x3.</param>
        public static void RotationY(float angle, out Matrix3x3 result)
        {
            float cos = Mathf.Cos(angle);
            float sin = Mathf.Sin(angle);

            result = Identity;
            result.M11 = cos;
            result.M13 = -sin;
            result.M31 = sin;
            result.M33 = cos;
        }

        /// <summary>
        /// Creates a Matrix3x3 that rotates around the y-axis.
        /// </summary>
        /// <param name="angle">Angle of rotation in radians. Angles are measured clockwise when looking along the rotation axis toward the origin.</param>
        /// <returns>The created rotation Matrix3x3.</returns>
        public static Matrix3x3 RotationY(float angle)
        {
            RotationY(angle, out Matrix3x3 result);
            return result;
        }

        /// <summary>
        /// Creates a Matrix3x3 that rotates around the z-axis.
        /// </summary>
        /// <param name="angle">Angle of rotation in radians. Angles are measured clockwise when looking along the rotation axis toward the origin.</param>
        /// <param name="result">When the method completes, contains the created rotation Matrix3x3.</param>
        public static void RotationZ(float angle, out Matrix3x3 result)
        {
            float cos = Mathf.Cos(angle);
            float sin = Mathf.Sin(angle);

            result = Identity;
            result.M11 = cos;
            result.M12 = sin;
            result.M21 = -sin;
            result.M22 = cos;
        }

        /// <summary>
        /// Creates a Matrix3x3 that rotates around the z-axis.
        /// </summary>
        /// <param name="angle">Angle of rotation in radians. Angles are measured clockwise when looking along the rotation axis toward the origin.</param>
        /// <returns>The created rotation Matrix3x3.</returns>
        public static Matrix3x3 RotationZ(float angle)
        {
            RotationZ(angle, out Matrix3x3 result);
            return result;
        }

        /// <summary>
        /// Creates a Matrix3x3 that rotates around an arbitrary axis.
        /// </summary>
        /// <param name="axis">The axis around which to rotate. This parameter is assumed to be normalized.</param>
        /// <param name="angle">Angle of rotation in radians. Angles are measured clockwise when looking along the rotation axis toward the origin.</param>
        /// <param name="result">When the method completes, contains the created rotation Matrix3x3.</param>
        public static void RotationAxis(ref Float3 axis, float angle, out Matrix3x3 result)
        {
            float x = axis.X;
            float y = axis.Y;
            float z = axis.Z;
            float cos = (float)Math.Cos(angle);
            float sin = (float)Math.Sin(angle);
            float xx = x * x;
            float yy = y * y;
            float zz = z * z;
            float xy = x * y;
            float xz = x * z;
            float yz = y * z;

            result = Identity;
            result.M11 = xx + (cos * (1.0f - xx));
            result.M12 = (xy - (cos * xy)) + (sin * z);
            result.M13 = (xz - (cos * xz)) - (sin * y);
            result.M21 = (xy - (cos * xy)) - (sin * z);
            result.M22 = yy + (cos * (1.0f - yy));
            result.M23 = (yz - (cos * yz)) + (sin * x);
            result.M31 = (xz - (cos * xz)) + (sin * y);
            result.M32 = (yz - (cos * yz)) - (sin * x);
            result.M33 = zz + (cos * (1.0f - zz));
        }

        /// <summary>
        /// Creates a Matrix3x3 that rotates around an arbitrary axis.
        /// </summary>
        /// <param name="axis">The axis around which to rotate. This parameter is assumed to be normalized.</param>
        /// <param name="angle">Angle of rotation in radians. Angles are measured clockwise when looking along the rotation axis toward the origin.</param>
        /// <returns>The created rotation Matrix3x3.</returns>
        public static Matrix3x3 RotationAxis(Float3 axis, float angle)
        {
            RotationAxis(ref axis, angle, out Matrix3x3 result);
            return result;
        }

        /// <summary>
        /// Creates a rotation Matrix3x3 from a quaternion.
        /// </summary>
        /// <param name="rotation">The quaternion to use to build the Matrix3x3.</param>
        /// <param name="result">The created rotation Matrix3x3.</param>
        public static void RotationQuaternion(ref Quaternion rotation, out Matrix3x3 result)
        {
            float xx = rotation.X * rotation.X;
            float yy = rotation.Y * rotation.Y;
            float zz = rotation.Z * rotation.Z;
            float xy = rotation.X * rotation.Y;
            float zw = rotation.Z * rotation.W;
            float zx = rotation.Z * rotation.X;
            float yw = rotation.Y * rotation.W;
            float yz = rotation.Y * rotation.Z;
            float xw = rotation.X * rotation.W;

            result = Identity;
            result.M11 = 1.0f - (2.0f * (yy + zz));
            result.M12 = 2.0f * (xy + zw);
            result.M13 = 2.0f * (zx - yw);
            result.M21 = 2.0f * (xy - zw);
            result.M22 = 1.0f - (2.0f * (zz + xx));
            result.M23 = 2.0f * (yz + xw);
            result.M31 = 2.0f * (zx + yw);
            result.M32 = 2.0f * (yz - xw);
            result.M33 = 1.0f - (2.0f * (yy + xx));
        }

        /// <summary>
        /// Creates a rotation Matrix3x3 from a quaternion.
        /// </summary>
        /// <param name="rotation">The quaternion to use to build the Matrix3x3.</param>
        /// <returns>The created rotation Matrix3x3.</returns>
        public static Matrix3x3 RotationQuaternion(Quaternion rotation)
        {
            RotationQuaternion(ref rotation, out Matrix3x3 result);
            return result;
        }

        /// <summary>
        /// Creates a rotation Matrix3x3 with a specified yaw, pitch, and roll.
        /// </summary>
        /// <param name="yaw">Yaw around the y-axis, in radians.</param>
        /// <param name="pitch">Pitch around the x-axis, in radians.</param>
        /// <param name="roll">Roll around the z-axis, in radians.</param>
        /// <param name="result">When the method completes, contains the created rotation Matrix3x3.</param>
        public static void RotationYawPitchRoll(float yaw, float pitch, float roll, out Matrix3x3 result)
        {
            Quaternion.RotationYawPitchRoll(yaw, pitch, roll, out Quaternion quaternion);
            RotationQuaternion(ref quaternion, out result);
        }

        /// <summary>
        /// Creates a rotation Matrix3x3 with a specified yaw, pitch, and roll.
        /// </summary>
        /// <param name="yaw">Yaw around the y-axis, in radians.</param>
        /// <param name="pitch">Pitch around the x-axis, in radians.</param>
        /// <param name="roll">Roll around the z-axis, in radians.</param>
        /// <returns>The created rotation Matrix3x3.</returns>
        public static Matrix3x3 RotationYawPitchRoll(float yaw, float pitch, float roll)
        {
            RotationYawPitchRoll(yaw, pitch, roll, out Matrix3x3 result);
            return result;
        }

        /// <summary>
        /// Creates 2D translation matrix.
        /// </summary>
        /// <param name="translation">The translation vector.</param>
        /// <param name="result">The result.</param>
        public static void Translation2D(ref Float2 translation, out Matrix3x3 result)
        {
            result = new Matrix3x3(    1, 0, 0, 0, 1, 0, translation.X, translation.Y, 1);
        }

        /// <summary>
        /// Creates 2D translation matrix.
        /// </summary>
        /// <param name="translation">The translation vector.</param>
        /// <returns>The result.</returns>
        public static Matrix3x3 Translation2D(Float2 translation)
        {
            return new Matrix3x3(1, 0, 0, 0, 1, 0, translation.X, translation.Y, 1);
        }

        /// <summary>
        /// Creates 2D translation matrix.
        /// </summary>
        /// <param name="x">The translation vector X.</param>
        /// <param name="y">The translation vector Y.</param>
        /// <returns>The result.</returns>
        public static Matrix3x3 Translation2D(float x, float y)
        {
            return new Matrix3x3(1, 0, 0, 0, 1, 0, x, y, 1);
        }

        /// <summary>
        /// Transforms given vector by the matrix (in 2D).
        /// </summary>
        /// <param name="vector">The vector.</param>
        /// <param name="transform">The transform.</param>
        /// <param name="result">The result.</param>
        public static void Transform2D(ref Float2 vector, ref Matrix3x3 transform, out Float2 result)
        {
            result = new Float2((vector.X * transform.M11) + (vector.Y * transform.M21) + transform.M31, (vector.X * transform.M12) + (vector.Y * transform.M22) + transform.M32);
        }

        /// <summary>
        /// Transforms given vector by the matrix (in 2D).
        /// </summary>
        /// <param name="vector">The vector.</param>
        /// <param name="transform">The transform.</param>
        /// <returns>The result.</returns>
        public static Float2 Transform2D(Float2 vector, Matrix3x3 transform)
        {
            Transform2D(ref vector, ref transform, out Float2 result);
            return result;
        }

        /// <summary>
        /// Adds two matrices.
        /// </summary>
        /// <param name="left">The first Matrix3x3 to add.</param>
        /// <param name="right">The second Matrix3x3 to add.</param>
        /// <returns>The sum of the two matrices.</returns>
        public static Matrix3x3 operator +(Matrix3x3 left, Matrix3x3 right)
        {
            Add(ref left, ref right, out Matrix3x3 result);
            return result;
        }

        /// <summary>
        /// Assert a Matrix3x3 (return it unchanged).
        /// </summary>
        /// <param name="value">The Matrix3x3 to assert (unchanged).</param>
        /// <returns>The asserted (unchanged) Matrix3x3.</returns>
        public static Matrix3x3 operator +(Matrix3x3 value)
        {
            return value;
        }

        /// <summary>
        /// Subtracts two matrices.
        /// </summary>
        /// <param name="left">The first Matrix3x3 to subtract.</param>
        /// <param name="right">The second Matrix3x3 to subtract.</param>
        /// <returns>The difference between the two matrices.</returns>
        public static Matrix3x3 operator -(Matrix3x3 left, Matrix3x3 right)
        {
            Subtract(ref left, ref right, out Matrix3x3 result);
            return result;
        }

        /// <summary>
        /// Negates a Matrix3x3.
        /// </summary>
        /// <param name="value">The Matrix3x3 to negate.</param>
        /// <returns>The negated Matrix3x3.</returns>
        public static Matrix3x3 operator -(Matrix3x3 value)
        {
            Negate(ref value, out Matrix3x3 result);
            return result;
        }

        /// <summary>
        /// Scales a Matrix3x3 by a given value.
        /// </summary>
        /// <param name="right">The Matrix3x3 to scale.</param>
        /// <param name="left">The amount by which to scale.</param>
        /// <returns>The scaled Matrix3x3.</returns>
        public static Matrix3x3 operator *(float left, Matrix3x3 right)
        {
            Multiply(ref right, left, out Matrix3x3 result);
            return result;
        }

        /// <summary>
        /// Scales a Matrix3x3 by a given value.
        /// </summary>
        /// <param name="left">The Matrix3x3 to scale.</param>
        /// <param name="right">The amount by which to scale.</param>
        /// <returns>The scaled Matrix3x3.</returns>
        public static Matrix3x3 operator *(Matrix3x3 left, float right)
        {
            Multiply(ref left, right, out Matrix3x3 result);
            return result;
        }

        /// <summary>
        /// Multiplies two matrices.
        /// </summary>
        /// <param name="left">The first Matrix3x3 to multiply.</param>
        /// <param name="right">The second Matrix3x3 to multiply.</param>
        /// <returns>The product of the two matrices.</returns>
        public static Matrix3x3 operator *(Matrix3x3 left, Matrix3x3 right)
        {
            Multiply(ref left, ref right, out Matrix3x3 result);
            return result;
        }

        /// <summary>
        /// Scales a Matrix3x3 by a given value.
        /// </summary>
        /// <param name="left">The Matrix3x3 to scale.</param>
        /// <param name="right">The amount by which to scale.</param>
        /// <returns>The scaled Matrix3x3.</returns>
        public static Matrix3x3 operator /(Matrix3x3 left, float right)
        {
            Divide(ref left, right, out Matrix3x3 result);
            return result;
        }

        /// <summary>
        /// Divides two matrices.
        /// </summary>
        /// <param name="left">The first Matrix3x3 to divide.</param>
        /// <param name="right">The second Matrix3x3 to divide.</param>
        /// <returns>The quotient of the two matrices.</returns>
        public static Matrix3x3 operator /(Matrix3x3 left, Matrix3x3 right)
        {
            Divide(ref left, ref right, out Matrix3x3 result);
            return result;
        }

        /// <summary>
        /// Tests for equality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left"/> has the same value as <paramref name="right"/>; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator ==(Matrix3x3 left, Matrix3x3 right)
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
        public static bool operator !=(Matrix3x3 left, Matrix3x3 right)
        {
            return !left.Equals(ref right);
        }

        /// <summary>
        /// Convert the 3x3 Matrix to a 4x4 Matrix.
        /// </summary>
        /// <returns>A 4x4 Matrix with zero translation and M44=1</returns>
        public static explicit operator Matrix(Matrix3x3 value)
        {
            return new Matrix(value.M11, value.M12, value.M13, 0, value.M21, value.M22, value.M23, 0, value.M31, value.M32, value.M33, 0, 0, 0, 0, 1);
        }

        /// <summary>
        /// Convert the 4x4 Matrix to a 3x3 Matrix.
        /// </summary>
        /// <returns>A 3x3 Matrix</returns>
        public static explicit operator Matrix3x3(Matrix value)
        {
            return new Matrix3x3(value.M11, value.M12, value.M13, value.M21, value.M22, value.M23, value.M31, value.M32, value.M33);
        }

        /// <summary>
        /// Returns a <see cref="System.String"/> that represents this instance.
        /// </summary>
        /// <returns>A <see cref="System.String"/> that represents this instance.</returns>
        public override string ToString()
        {
            return string.Format(CultureInfo.CurrentCulture, "[M11:{0} M12:{1} M13:{2}] [M21:{3} M22:{4} M23:{5}] [M31:{6} M32:{7} M33:{8}]", M11, M12, M13, M21, M22, M23, M31, M32, M33);
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
            return string.Format(format, CultureInfo.CurrentCulture, "[M11:{0} M12:{1} M13:{2}] [M21:{3} M22:{4} M23:{5}] [M31:{6} M32:{7} M33:{8}]",
                                 M11.ToString(format, CultureInfo.CurrentCulture), M12.ToString(format, CultureInfo.CurrentCulture), M13.ToString(format, CultureInfo.CurrentCulture),
                                 M21.ToString(format, CultureInfo.CurrentCulture), M22.ToString(format, CultureInfo.CurrentCulture), M23.ToString(format, CultureInfo.CurrentCulture),
                                 M31.ToString(format, CultureInfo.CurrentCulture), M32.ToString(format, CultureInfo.CurrentCulture), M33.ToString(format, CultureInfo.CurrentCulture));
        }

        /// <summary>
        /// Returns a <see cref="System.String"/> that represents this instance.
        /// </summary>
        /// <param name="formatProvider">The format provider.</param>
        /// <returns>A <see cref="System.String"/> that represents this instance.</returns>
        public string ToString(IFormatProvider formatProvider)
        {
            return string.Format(formatProvider, "[M11:{0} M12:{1} M13:{2}] [M21:{3} M22:{4} M23:{5}] [M31:{6} M32:{7} M33:{8}]",
                                 M11.ToString(formatProvider), M12.ToString(formatProvider), M13.ToString(formatProvider),
                                 M21.ToString(formatProvider), M22.ToString(formatProvider), M23.ToString(formatProvider),
                                 M31.ToString(formatProvider), M32.ToString(formatProvider), M33.ToString(formatProvider));
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
            return string.Format(format, formatProvider, "[M11:{0} M12:{1} M13:{2}] [M21:{3} M22:{4} M23:{5}] [M31:{6} M32:{7} M33:{8}]",
                                 M11.ToString(format, formatProvider), M12.ToString(format, formatProvider), M13.ToString(format, formatProvider),
                                 M21.ToString(format, formatProvider), M22.ToString(format, formatProvider), M23.ToString(format, formatProvider),
                                 M31.ToString(format, formatProvider), M32.ToString(format, formatProvider), M33.ToString(format, formatProvider));
        }

        /// <summary>
        /// Returns a hash code for this instance.
        /// </summary>
        /// <returns>A hash code for this instance, suitable for use in hashing algorithms and data structures like a hash table.</returns>
        public override int GetHashCode()
        {
            unchecked
            {
                var hashCode = M11.GetHashCode();
                hashCode = (hashCode * 397) ^ M12.GetHashCode();
                hashCode = (hashCode * 397) ^ M13.GetHashCode();
                hashCode = (hashCode * 397) ^ M21.GetHashCode();
                hashCode = (hashCode * 397) ^ M22.GetHashCode();
                hashCode = (hashCode * 397) ^ M23.GetHashCode();
                hashCode = (hashCode * 397) ^ M31.GetHashCode();
                hashCode = (hashCode * 397) ^ M32.GetHashCode();
                hashCode = (hashCode * 397) ^ M33.GetHashCode();
                return hashCode;
            }
        }

        /// <summary>
        /// Determines whether the specified <see cref="Matrix3x3"/> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="Matrix3x3"/> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="Matrix3x3"/> is equal to this instance; otherwise, <c>false</c>.</returns>
        public bool Equals(ref Matrix3x3 other)
        {
            return (Mathf.NearEqual(other.M11, M11) &&
                    Mathf.NearEqual(other.M12, M12) &&
                    Mathf.NearEqual(other.M13, M13) &&
                    Mathf.NearEqual(other.M21, M21) &&
                    Mathf.NearEqual(other.M22, M22) &&
                    Mathf.NearEqual(other.M23, M23) &&
                    Mathf.NearEqual(other.M31, M31) &&
                    Mathf.NearEqual(other.M32, M32) &&
                    Mathf.NearEqual(other.M33, M33));
        }

        /// <summary>
        /// Determines whether the specified <see cref="Matrix3x3"/> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="Matrix3x3"/> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="Matrix3x3"/> is equal to this instance; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(Matrix3x3 other)
        {
            return Equals(ref other);
        }

        /// <summary>
        /// Determines whether the specified <see cref="Matrix3x3"/> are equal.
        /// </summary>
        public static bool Equals(ref Matrix3x3 a, ref Matrix3x3 b)
        {
            return
            Mathf.NearEqual(a.M11, b.M11) &&
            Mathf.NearEqual(a.M12, b.M12) &&
            Mathf.NearEqual(a.M13, b.M13) &&
            Mathf.NearEqual(a.M21, b.M21) &&
            Mathf.NearEqual(a.M22, b.M22) &&
            Mathf.NearEqual(a.M23, b.M23) &&
            Mathf.NearEqual(a.M31, b.M31) &&
            Mathf.NearEqual(a.M32, b.M32) &&
            Mathf.NearEqual(a.M33, b.M33)
            ;
        }

        /// <summary>
        /// Determines whether the specified <see cref="System.Object"/> is equal to this instance.
        /// </summary>
        /// <param name="value">The <see cref="System.Object"/> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="System.Object"/> is equal to this instance; otherwise, <c>false</c>.</returns>
        public override bool Equals(object value)
        {
            return value is Matrix3x3 other && Equals(ref other);
        }
    }
}
