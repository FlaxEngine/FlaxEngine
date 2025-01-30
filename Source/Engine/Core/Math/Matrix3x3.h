// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Vector2.h"
#include "Vector3.h"
#include "Engine/Platform/Platform.h"

/// <summary>
/// Represents a 3x3 mathematical matrix.
/// </summary>
API_STRUCT() struct FLAXENGINE_API Matrix3x3
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(Matrix3x3);
public:
    union
    {
        struct
        {
            /// <summary>Value at row 1 column 1 of the matrix.</summary>
            float M11;

            /// <summary>Value at row 1 column 2 of the matrix.</summary>
            float M12;

            /// <summary>Value at row 1 column 3 of the matrix.</summary>
            float M13;

            /// <summary>Value at row 2 column 1 of the matrix.</summary>
            float M21;

            /// <summary>Value at row 2 column 2 of the matrix.</summary>
            float M22;

            /// <summary>Value at row 2 column 3 of the matrix.</summary>
            float M23;

            /// <summary>Value at row 3 column 1 of the matrix.</summary>
            float M31;

            /// <summary>Value at row 3 column 2 of the matrix.</summary>
            float M32;

            /// <summary>Value at row 3 column 3 of the matrix.</summary>
            float M33;
        };

        float Values[3][3];
        float Raw[9];
    };

public:
    /// <summary>
    /// A matrix with all of its components set to zero.
    /// </summary>
    static const Matrix3x3 Zero;

    /// <summary>
    /// The identity matrix.
    /// </summary>
    static const Matrix3x3 Identity;

public:
    /// <summary>
    /// Empty constructor.
    /// </summary>
    Matrix3x3() = default;

    /// <summary>
    /// Initializes a new instance of the <see cref="Matrix3x3"/> struct.
    /// </summary>
    /// <param name="m11">The value to assign at row 1 column 1 of the matrix.</param>
    /// <param name="m12">The value to assign at row 1 column 2 of the matrix.</param>
    /// <param name="m13">The value to assign at row 1 column 3 of the matrix.</param>
    /// <param name="m21">The value to assign at row 2 column 1 of the matrix.</param>
    /// <param name="m22">The value to assign at row 2 column 2 of the matrix.</param>
    /// <param name="m23">The value to assign at row 2 column 3 of the matrix.</param>
    /// <param name="m31">The value to assign at row 3 column 1 of the matrix.</param>
    /// <param name="m32">The value to assign at row 3 column 2 of the matrix.</param>
    /// <param name="m33">The value to assign at row 3 column 3 of the matrix.</param>
    Matrix3x3(float m11, float m12, float m13, float m21, float m22, float m23, float m31, float m32, float m33)
        : M11(m11)
        , M12(m12)
        , M13(m13)
        , M21(m21)
        , M22(m22)
        , M23(m23)
        , M31(m31)
        , M32(m32)
        , M33(m33)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Matrix3x3"/> struct.
    /// </summary>
    /// <param name="values">The values to assign to the components of the matrix. This must be an array with nine elements.</param>
    Matrix3x3(float values[9])
    {
        Platform::MemoryCopy(Raw, values, sizeof(float) * 9);
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Matrix3x3"/> struct.
    /// </summary>
    /// <param name="values">The values to assign to the components of the matrix. This must be an array with 3 by 3 elements.</param>
    Matrix3x3(float values[3][3])
    {
        Platform::MemoryCopy(Raw, values, sizeof(float) * 9);
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Matrix3x3"/> struct.
    /// </summary>
    /// <param name="matrix">The 4 by 4 matrix to initialize from with rotation and scale (translation is skipped).</param>
    explicit Matrix3x3(const Matrix& matrix);

public:
    String ToString() const;

public:
    // Gets the up Float3 of the matrix; that is M21, M22, and M23.
    Float3 GetUp() const
    {
        return Float3(M21, M22, M23);
    }

    // Sets Float3 of the matrix; that is M21, M22, and M23.
    void SetUp(const Float3& value)
    {
        M21 = value.X;
        M22 = value.Y;
        M23 = value.Z;
    }

    // Gets the down Float3 of the matrix; that is -M21, -M22, and -M23.
    Float3 GetDown() const
    {
        return -Float3(M21, M22, M23);
    }

    // Sets the down Float3 of the matrix; that is -M21, -M22, and -M23.
    void SetDown(const Float3& value)
    {
        M21 = -value.X;
        M22 = -value.Y;
        M23 = -value.Z;
    }

    // Gets the right Float3 of the matrix; that is M11, M12, and M13.
    Float3 GetRight() const
    {
        return Float3(M11, M12, M13);
    }

    // Sets the right Float3 of the matrix; that is M11, M12, and M13.
    void SetRight(const Float3& value)
    {
        M11 = value.X;
        M12 = value.Y;
        M13 = value.Z;
    }

    // Gets the left Float3 of the matrix; that is -M11, -M12, and -M13.
    Float3 GetLeft() const
    {
        return -Float3(M11, M12, M13);
    }

    // Sets the left Float3 of the matrix; that is -M11, -M12, and -M13.
    void SetLeft(const Float3& value)
    {
        M11 = -value.X;
        M12 = -value.Y;
        M13 = -value.Z;
    }

    // Gets the forward Float3 of the matrix; that is M31, M32, and M33.
    Float3 GetForward() const
    {
        return -Float3(M31, M32, M33);
    }

    // Sets the forward Float3 of the matrix; that is M31, M32, and M33.
    void SetForward(const Float3& value)
    {
        M31 = value.X;
        M32 = value.Y;
        M33 = value.Z;
    }

    // Gets the backward Float3 of the matrix; that is -M31, -M32, and -M33.
    Float3 GetBackward() const
    {
        return Float3(-M31, -M32, -M33);
    }

    // Sets the backward Float3 of the matrix; that is -M31, -M32, and -M33.
    void SetBackward(const Float3& value)
    {
        M31 = -value.X;
        M32 = -value.Y;
        M33 = -value.Z;
    }

    // Gets the first row in the matrix; that is M11, M12 and M13.
    Float3 GetRow1() const
    {
        return Float3(M11, M12, M13);
    }

    // Sets the first row in the matrix; that is M11, M12 and M13.
    void SetRow1(const Float3& value)
    {
        M11 = value.X;
        M12 = value.Y;
        M13 = value.Z;
    }

    // Gets the second row in the matrix; that is M21, M22 and M23.
    Float3 GetRow2() const
    {
        return Float3(M21, M22, M23);
    }

    // Sets the second row in the matrix; that is M21, M22 and M23.
    void SetRow2(const Float3& value)
    {
        M21 = value.X;
        M22 = value.Y;
        M23 = value.Z;
    }

    // Gets the third row in the matrix; that is M31, M32 and M33.
    Float3 GetRow3() const
    {
        return Float3(M31, M32, M33);
    }

    // Sets the third row in the matrix; that is M31, M32 and M33.
    void SetRow3(const Float3& value)
    {
        M31 = value.X;
        M32 = value.Y;
        M33 = value.Z;
    }

    // Gets the first column in the matrix; that is M11, M21 and M31.
    Float3 GetColumn1() const
    {
        return Float3(M11, M21, M31);
    }

    // Sets the first column in the matrix; that is M11, M21 and M31.
    void SetColumn1(const Float3& value)
    {
        M11 = value.X;
        M21 = value.Y;
        M31 = value.Z;
    }

    // Gets the second column in the matrix; that is M12, M22 and M32.
    Float3 GetColumn2() const
    {
        return Float3(M12, M22, M32);
    }

    // Sets the second column in the matrix; that is M12, M22 and M32.
    void SetColumn2(const Float3& value)
    {
        M12 = value.X;
        M22 = value.Y;
        M32 = value.Z;
    }

    // Gets the third column in the matrix; that is M13, M23 and M33.
    Float3 GetColumn3() const
    {
        return Float3(M13, M23, M33);
    }

    // Sets the third column in the matrix; that is M13, M23 and M33.
    void SetColumn3(const Float3& value)
    {
        M13 = value.X;
        M23 = value.Y;
        M33 = value.Z;
    }

    // Gets the scale of the matrix; that is M11, M22, and M33.
    Float3 GetScaleVector() const
    {
        return Float3(M11, M22, M33);
    }

    // Sets the scale of the matrix; that is M11, M22, and M33.
    void SetScaleVector(const Float3& value)
    {
        M11 = value.X;
        M22 = value.Y;
        M33 = value.Z;
    }

    /// <summary>
    /// Gets a value indicating whether this instance is an identity Matrix3x3.
    /// </summary>
    bool IsIdentity() const
    {
        return *this == Identity;
    }

    /// <summary>
    /// Calculates the determinant of the Matrix3x3.
    /// </summary>
    float GetDeterminant() const;

public:
    /// <summary>
    /// Inverts the Matrix3x3.
    /// </summary>
    void Invert()
    {
        Invert(*this, *this);
    }

    /// <summary>
    /// Transposes the Matrix3x3.
    /// </summary>
    void Transpose()
    {
        Transpose(*this, *this);
    }

    /// <summary>
    /// Removes any scaling from the matrix by performing the normalization (each row magnitude is 1).
    /// </summary>
    void NormalizeScale();

public:
    /// <summary>
    /// Calculates the inverse of the specified Matrix3x3.
    /// </summary>
    /// <param name="value">The Matrix3x3 whose inverse is to be calculated.</param>
    /// <param name="result">When the method completes, contains the inverse of the specified Matrix3x3.</param>
    static void Invert(const Matrix3x3& value, Matrix3x3& result);

    /// <summary>
    /// Calculates the inverse of the specified Matrix3x3.
    /// </summary>
    /// <param name="value">The Matrix3x3 whose inverse is to be calculated.</param>
    /// <returns>The inverse of the specified Matrix3x3.</returns>
    static Matrix3x3 Invert(const Matrix3x3& value)
    {
        Matrix3x3 result;
        Invert(value, result);
        return result;
    }

    /// <summary>
    /// Calculates the transpose of the specified Matrix3x3.
    /// </summary>
    /// <param name="value">The Matrix3x3 whose transpose is to be calculated.</param>
    /// <param name="result">When the method completes, contains the transpose of the specified Matrix3x3.</param>
    static void Transpose(const Matrix3x3& value, Matrix3x3& result);

    /// <summary>
    /// Calculates the transpose of the specified Matrix3x3.
    /// </summary>
    /// <param name="value">The Matrix3x3 whose transpose is to be calculated.</param>
    /// <returns>The transpose of the specified Matrix3x3.</returns>
    static Matrix3x3 Transpose(const Matrix3x3& value)
    {
        Matrix3x3 result;
        Transpose(value, result);
        return result;
    }

public:
    /// <summary>
    /// Determines the sum of two matrices.
    /// </summary>
    /// <param name="left">The first Matrix3x3 to add.</param>
    /// <param name="right">The second Matrix3x3 to add.</param>
    /// <param name="result">When the method completes, contains the sum of the two matrices.</param>
    static void Add(const Matrix3x3& left, const Matrix3x3& right, Matrix3x3& result);

    /// <summary>
    /// Determines the sum of two matrices.
    /// </summary>
    /// <param name="left">The first Matrix3x3 to add.</param>
    /// <param name="right">The second Matrix3x3 to add.</param>
    /// <returns>The sum of the two matrices.</returns>
    static Matrix3x3 Add(const Matrix3x3& left, const Matrix3x3& right)
    {
        Matrix3x3 result;
        Add(left, right, result);
        return result;
    }

    /// <summary>
    /// Determines the difference between two matrices.
    /// </summary>
    /// <param name="left">The first Matrix3x3 to subtract.</param>
    /// <param name="right">The second Matrix3x3 to subtract.</param>
    /// <param name="result">When the method completes, contains the difference between the two matrices.</param>
    static void Subtract(const Matrix3x3& left, const Matrix3x3& right, Matrix3x3& result);

    /// <summary>
    /// Determines the difference between two matrices.
    /// </summary>
    /// <param name="left">The first Matrix3x3 to subtract.</param>
    /// <param name="right">The second Matrix3x3 to subtract.</param>
    /// <returns>The difference between the two matrices.</returns>
    static Matrix3x3 Subtract(const Matrix3x3& left, const Matrix3x3& right)
    {
        Matrix3x3 result;
        Subtract(left, right, result);
        return result;
    }

    /// <summary>
    /// Scales a Matrix3x3 by the given value.
    /// </summary>
    /// <param name="left">The Matrix3x3 to scale.</param>
    /// <param name="right">The amount by which to scale.</param>
    /// <param name="result">When the method completes, contains the scaled Matrix3x3.</param>
    static void Multiply(const Matrix3x3& left, float right, Matrix3x3& result);

    /// <summary>
    /// Scales a Matrix3x3 by the given value.
    /// </summary>
    /// <param name="left">The Matrix3x3 to scale.</param>
    /// <param name="right">The amount by which to scale.</param>
    /// <returns>The scaled Matrix3x3.</returns>
    static Matrix3x3 Multiply(const Matrix3x3& left, float right)
    {
        Matrix3x3 result;
        Multiply(left, right, result);
        return result;
    }

    /// <summary>
    /// Determines the product of two matrices.
    /// </summary>
    /// <param name="left">The first Matrix3x3 to multiply.</param>
    /// <param name="right">The second Matrix3x3 to multiply.</param>
    /// <param name="result">The product of the two matrices.</param>
    static void Multiply(const Matrix3x3& left, const Matrix3x3& right, Matrix3x3& result);

    /// <summary>
    /// Determines the product of two matrices.
    /// </summary>
    /// <param name="left">The first Matrix3x3 to multiply.</param>
    /// <param name="right">The second Matrix3x3 to multiply.</param>
    /// <returns>The product of the two matrices.</returns>
    static Matrix3x3 Multiply(const Matrix3x3& left, const Matrix3x3& right)
    {
        Matrix3x3 result;
        Multiply(left, right, result);
        return result;
    }

    /// <summary>
    /// Scales a Matrix3x3 by the given value.
    /// </summary>
    /// <param name="left">The Matrix3x3 to scale.</param>
    /// <param name="right">The amount by which to scale.</param>
    /// <param name="result">When the method completes, contains the scaled Matrix3x3.</param>
    static void Divide(const Matrix3x3& left, float right, Matrix3x3& result);

    /// <summary>
    /// Scales a Matrix3x3 by the given value.
    /// </summary>
    /// <param name="left">The Matrix3x3 to scale.</param>
    /// <param name="right">The amount by which to scale.</param>
    /// <returns>The scaled Matrix3x3.</returns>
    static Matrix3x3 Divide(const Matrix3x3& left, float right)
    {
        Matrix3x3 result;
        Divide(left, right, result);
        return result;
    }

    /// <summary>
    /// Determines the quotient of two matrices.
    /// </summary>
    /// <param name="left">The first Matrix3x3 to divide.</param>
    /// <param name="right">The second Matrix3x3 to divide.</param>
    /// <param name="result">When the method completes, contains the quotient of the two matrices.</param>
    static void Divide(const Matrix3x3& left, const Matrix3x3& right, Matrix3x3& result);

    /// <summary>
    /// Determines the quotient of two matrices.
    /// </summary>
    /// <param name="left">The first Matrix3x3 to divide.</param>
    /// <param name="right">The second Matrix3x3 to divide.</param>
    /// <returns>The quotient of the two matrices.</returns>
    static Matrix3x3 Divide(const Matrix3x3& left, const Matrix3x3& right)
    {
        Matrix3x3 result;
        Divide(left, right, result);
        return result;
    }

public:
    /// <summary>
    /// Creates 2D translation matrix.
    /// </summary>
    /// <param name="translation">The translation vector.</param>
    /// <param name="result">The result.</param>
    static void Translation2D(const Float2& translation, Matrix3x3& result)
    {
        result = Matrix3x3(
            1, 0, 0,
            0, 1, 0,
            translation.X, translation.Y, 1
        );
    }

    /// <summary>
    /// Creates 2D translation matrix.
    /// </summary>
    /// <param name="translation">The translation vector.</param>
    /// <returns>The result.</returns>
    static Matrix3x3 Translation2D(const Float2& translation)
    {
        Matrix3x3 result;
        Translation2D(translation, result);
        return result;
    }

    /// <summary>
    /// Transforms given point by the matrix (in 2D).
    /// Useful to transform location of point.
    /// </summary>
    /// <param name="point">The point.</param>
    /// <param name="transform">The transform.</param>
    /// <param name="result">The result.</param>
    static void Transform2DPoint(const Float2& point, const Matrix3x3& transform, Float2& result)
    {
        result = Float2(
            point.X * transform.M11 + point.Y * transform.M21 + transform.M31,
            point.X * transform.M12 + point.Y * transform.M22 + transform.M32);
    }

    /// <summary>
    /// Transforms given vector by the matrix (in 2D).
    /// Useful to transform size or distance.
    /// </summary>
    /// <param name="vector">The vector.</param>
    /// <param name="transform">The transform.</param>
    /// <param name="result">The result.</param>
    static void Transform2DVector(const Float2& vector, const Matrix3x3& transform, Float2& result)
    {
        result = Float2(
            vector.X * transform.M11 + vector.Y * transform.M21,
            vector.X * transform.M12 + vector.Y * transform.M22);
    }

public:
    /// <summary>
    /// Creates a rotation matrix from a quaternion
    /// </summary>
    /// <param name="rotation">The quaternion to use to build the matrix.</param>
    /// <returns>The created rotation matrix</returns>
    static Matrix3x3 RotationQuaternion(const Quaternion& rotation)
    {
        Matrix3x3 result;
        RotationQuaternion(rotation, result);
        return result;
    }

    /// <summary>
    /// Creates a rotation matrix from a quaternion
    /// </summary>
    /// <param name="rotation">The quaternion to use to build the matrix.</param>
    /// <param name="result">The created rotation matrix.</param>
    static void RotationQuaternion(const Quaternion& rotation, Matrix3x3& result);

    /// <summary>
    /// Decomposes a matrix into a scale and rotation.
    /// </summary>
    /// <param name="scale">When the method completes, contains the scaling component of the decomposed matrix.</param>
    /// <param name="rotation">When the method completes, contains the rotation component of the decomposed matrix.</param>
    /// <remarks>This method is designed to decompose an scale-rotation transformation matrix only.</remarks>
    void Decompose(Float3& scale, Matrix3x3& rotation) const;

    /// <summary>
    /// Decomposes a matrix into a scale and rotation.
    /// </summary>
    /// <param name="scale">When the method completes, contains the scaling component of the decomposed matrix.</param>
    /// <param name="rotation">When the method completes, contains the rotation component of the decomposed matrix.</param>
    /// <remarks>This method is designed to decompose an scale-rotation transformation matrix only.</remarks>
    void Decompose(Float3& scale, Quaternion& rotation) const;

public:
    /// <summary>
    /// Tests for equality between two objects.
    /// </summary>
    /// <param name="other">The other value to compare.</param>
    /// <returns><c>true</c> if object has the same value as <paramref name="other"/>; otherwise, <c>false</c>.</returns>
    bool operator==(const Matrix3x3& other) const;

    /// <summary>
    /// Tests for equality between two objects.
    /// </summary>
    /// <param name="other">The other value to compare.</param>
    /// <returns><c>true</c> if object has a different value as <paramref name="other"/>; otherwise, <c>false</c>.</returns>
    FORCE_INLINE bool operator!=(const Matrix3x3& other) const
    {
        return !operator==(other);
    }
};

template<>
struct TIsPODType<Matrix3x3>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(Matrix3x3, "[M11:{0} M12:{1} M13:{2}] [M21:{3} M22:{5} M23:{5}] [M31:{6} M32:{7} M33:{8}]", v.M11, v.M12, v.M13, v.M21, v.M22, v.M23, v.M31, v.M32, v.M33);
