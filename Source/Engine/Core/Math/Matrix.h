// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Vector3.h"
#include "Vector4.h"
#include "Engine/Platform/Platform.h"

struct Transform;

/// <summary>
/// Represents a 4x4 mathematical matrix.
/// </summary>
API_STRUCT() struct FLAXENGINE_API Matrix
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(Matrix);
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

            /// <summary>Value at row 1 column 4 of the matrix.</summary>
            float M14;

            /// <summary>Value at row 2 column 1 of the matrix.</summary>
            float M21;

            /// <summary>Value at row 2 column 2 of the matrix.</summary>
            float M22;

            /// <summary>Value at row 2 column 3 of the matrix.</summary>
            float M23;

            /// <summary>Value at row 2 column 4 of the matrix.</summary>
            float M24;

            /// <summary>Value at row 3 column 1 of the matrix.</summary>
            float M31;

            /// <summary>Value at row 3 column 2 of the matrix.</summary>
            float M32;

            /// <summary>Value at row 3 column 3 of the matrix.</summary>
            float M33;

            /// <summary>Value at row 3 column 4 of the matrix.</summary>
            float M34;

            /// <summary>Value at row 4 column 1 of the matrix.</summary>
            float M41;

            /// <summary>Value at row 4 column 2 of the matrix.</summary>
            float M42;

            /// <summary>Value at row 4 column 3 of the matrix.</summary>
            float M43;

            /// <summary>Value at row 4 column 4 of the matrix.</summary>
            float M44;
        };

        float Values[4][4];
        float Raw[16];
    };

public:
    /// <summary>A matrix with all of its components set to zero.</summary>
    static const Matrix Zero;

    /// <summary>The identity matrix.</summary>
    static const Matrix Identity;

public:
    /// <summary>
    /// Empty constructor.
    /// </summary>
    Matrix() = default;

    /// <summary>
    /// Initializes a new instance of the <see cref="Matrix"/> struct.
    /// </summary>
    /// <param name="m11">The value to assign at row 1 column 1 of the matrix.</param>
    /// <param name="m12">The value to assign at row 1 column 2 of the matrix.</param>
    /// <param name="m13">The value to assign at row 1 column 3 of the matrix.</param>
    /// <param name="m14">The value to assign at row 1 column 4 of the matrix.</param>
    /// <param name="m21">The value to assign at row 2 column 1 of the matrix.</param>
    /// <param name="m22">The value to assign at row 2 column 2 of the matrix.</param>
    /// <param name="m23">The value to assign at row 2 column 3 of the matrix.</param>
    /// <param name="m24">The value to assign at row 2 column 4 of the matrix.</param>
    /// <param name="m31">The value to assign at row 3 column 1 of the matrix.</param>
    /// <param name="m32">The value to assign at row 3 column 2 of the matrix.</param>
    /// <param name="m33">The value to assign at row 3 column 3 of the matrix.</param>
    /// <param name="m34">The value to assign at row 3 column 4 of the matrix.</param>
    /// <param name="m41">The value to assign at row 4 column 1 of the matrix.</param>
    /// <param name="m42">The value to assign at row 4 column 2 of the matrix.</param>
    /// <param name="m43">3The value to assign at row 4 column 3 of the matrix.</param>
    /// <param name="m44">The value to assign at row 4 column 4 of the matrix.</param>
    Matrix(float m11, float m12, float m13, float m14,
           float m21, float m22, float m23, float m24,
           float m31, float m32, float m33, float m34,
           float m41, float m42, float m43, float m44)
        : M11(m11)
        , M12(m12)
        , M13(m13)
        , M14(m14)
        , M21(m21)
        , M22(m22)
        , M23(m23)
        , M24(m24)
        , M31(m31)
        , M32(m32)
        , M33(m33)
        , M34(m34)
        , M41(m41)
        , M42(m42)
        , M43(m43)
        , M44(m44)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Matrix"/> struct.
    /// </summary>
    /// <param name="values">The values to assign to the components of the matrix. This must be an array with sixteen elements.</param>
    Matrix(float values[16])
    {
        Platform::MemoryCopy(Raw, values, sizeof(float) * 16);
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Matrix"/> struct.
    /// </summary>
    /// <param name="values">The values to assign to the components of the matrix. This must be an array with 4 by 4 elements.</param>
    Matrix(float values[4][4])
    {
        Platform::MemoryCopy(Raw, values, sizeof(float) * 16);
    }

    explicit Matrix(const Matrix3x3& matrix);
    explicit Matrix(const Double4x4& matrix);

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

    // Gets the forward Float3 of the matrix; that is -M31, -M32, and -M33.
    Float3 GetForward() const
    {
        return Float3(M31, M32, M33);
    }

    // Sets the forward Float3 of the matrix; that is -M31, -M32, and -M33.
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

    // Gets the first row in the matrix; that is M11, M12, M13, and M14.
    Float4 GetRow1() const
    {
        return Float4(M11, M12, M13, M14);
    }

    // Sets the first row in the matrix; that is M11, M12, M13, and M14.
    void SetRow1(const Float4& value)
    {
        M11 = value.X;
        M12 = value.Y;
        M13 = value.Z;
        M14 = value.W;
    }

    // Gets the second row in the matrix; that is M21, M22, M23, and M24.
    Float4 GetRow2() const
    {
        return Float4(M21, M22, M23, M24);
    }

    // Sets the second row in the matrix; that is M21, M22, M23, and M24.
    void SetRow2(const Float4& value)
    {
        M21 = value.X;
        M22 = value.Y;
        M23 = value.Z;
        M24 = value.W;
    }

    // Gets the third row in the matrix; that is M31, M32, M33, and M34.
    Float4 GetRow3() const
    {
        return Float4(M31, M32, M33, M34);
    }

    // Sets the third row in the matrix; that is M31, M32, M33, and M34.
    void SetRow3(const Float4& value)
    {
        M31 = value.X;
        M32 = value.Y;
        M33 = value.Z;
        M34 = value.W;
    }

    // Gets the fourth row in the matrix; that is M41, M42, M43, and M44.
    Float4 GetRow4() const
    {
        return Float4(M41, M42, M43, M44);
    }

    // Sets the fourth row in the matrix; that is M41, M42, M43, and M44.
    void SetRow4(const Float4& value)
    {
        M41 = value.X;
        M42 = value.Y;
        M43 = value.Z;
        M44 = value.W;
    }

    // Gets the first column in the matrix; that is M11, M21, M31, and M41.
    Float4 GetColumn1() const
    {
        return Float4(M11, M21, M31, M41);
    }

    // Sets the first column in the matrix; that is M11, M21, M31, and M41.
    void SetColumn1(const Float4& value)
    {
        M11 = value.X;
        M21 = value.Y;
        M31 = value.Z;
        M41 = value.W;
    }

    // Gets the second column in the matrix; that is M12, M22, M32, and M42.
    Float4 GetColumn2() const
    {
        return Float4(M12, M22, M32, M42);
    }

    // Sets the second column in the matrix; that is M12, M22, M32, and M42.
    void SetColumn2(const Float4& value)
    {
        M12 = value.X;
        M22 = value.Y;
        M32 = value.Z;
        M42 = value.W;
    }

    // Gets the third column in the matrix; that is M13, M23, M33, and M43.
    Float4 GetColumn3() const
    {
        return Float4(M13, M23, M33, M43);
    }

    // Sets the third column in the matrix; that is M13, M23, M33, and M43.
    void SetColumn3(const Float4& value)
    {
        M13 = value.X;
        M23 = value.Y;
        M33 = value.Z;
        M43 = value.W;
    }

    // Gets the fourth column in the matrix; that is M14, M24, M34, and M44.
    Float4 GetColumn4() const
    {
        return Float4(M14, M24, M34, M44);
    }

    // Sets the fourth column in the matrix; that is M14, M24, M34, and M44.
    void SetColumn4(const Float4& value)
    {
        M14 = value.X;
        M24 = value.Y;
        M34 = value.Z;
        M44 = value.W;
    }

    // Sets part of the first row in the matrix; that is M11, M12, M13.
    void SetX(const Float3& value)
    {
        M11 = value.X;
        M12 = value.Y;
        M13 = value.Z;
    }

    // Sets part of the second row in the matrix; that is M21, M22, M23.
    void SetY(const Float3& value)
    {
        M21 = value.X;
        M22 = value.Y;
        M23 = value.Z;
    }

    // Sets part of the third row in the matrix; that is M31, M32, M33.
    void SetZ(const Float3& value)
    {
        M31 = value.X;
        M32 = value.Y;
        M33 = value.Z;
    }

    // Gets the translation of the matrix; that is M41, M42, and M43.
    Float3 GetTranslation() const
    {
        return Float3(M41, M42, M43);
    }

    // Sets the translation of the matrix; that is M41, M42, and M43.
    void SetTranslation(const Float3& value)
    {
        M41 = value.X;
        M42 = value.Y;
        M43 = value.Z;
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

    // Gets a value indicating whether this instance is an identity matrix.
    bool IsIdentity() const
    {
        return *this == Identity;
    }

    // Calculates the determinant of the matrix.
    float GetDeterminant() const;

    /// <summary>
    /// Calculates determinant of the rotation 3x3 matrix.
    /// </summary>
    /// <returns>The determinant of the 3x3 matrix.</returns>
    float RotDeterminant() const;

public:
    // Inverts the matrix.
    void Invert()
    {
        Invert(*this, *this);
    }

    // Transposes the matrix.
    void Transpose()
    {
        Transpose(*this, *this);
    }

    /// <summary>
    /// Removes any scaling from the matrix by performing the normalization (each row magnitude is 1). Does not modify the 4th row with translation vector.
    /// </summary>
    void NormalizeScale();

public:
    /// <summary>
    /// Decomposes a rotation matrix with the specified yaw, pitch, roll.
    /// </summary>
    /// <param name="yaw">The result yaw (in radians).</param>
    /// <param name="pitch">The result pitch (in radians).</param>
    /// <param name="roll">The result roll (in radians).</param>
    void Decompose(float& yaw, float& pitch, float& roll) const;

    /// <summary>
    /// Decomposes a matrix into a scale, rotation, and translation.
    /// </summary>
    /// <param name="scale">When the method completes, contains the scaling component of the decomposed matrix.</param>
    /// <param name="translation">When the method completes, contains the translation component of the decomposed matrix.</param>
    /// <remarks>This method is designed to decompose an SRT transformation matrix only.</remarks>
    void Decompose(Float3& scale, Float3& translation) const;

    /// <summary>
    /// Decomposes a matrix into a scale, rotation, and translation.
    /// </summary>
    /// <param name="transform">When the method completes, contains the transformation of the decomposed matrix.</param>
    /// <remarks>This method is designed to decompose an SRT transformation matrix only.</remarks>
    void Decompose(Transform& transform) const;

    /// <summary>
    /// Decomposes a matrix into a scale, rotation, and translation.
    /// </summary>
    /// <param name="scale">When the method completes, contains the scaling component of the decomposed matrix.</param>
    /// <param name="rotation">When the method completes, contains the rotation component of the decomposed matrix.</param>
    /// <param name="translation">When the method completes, contains the translation component of the decomposed matrix.</param>
    /// <remarks>This method is designed to decompose an SRT transformation matrix only.</remarks>
    void Decompose(Float3& scale, Quaternion& rotation, Float3& translation) const;

    /// <summary>
    /// Decomposes a matrix into a scale, rotation, and translation.
    /// </summary>
    /// <param name="scale">When the method completes, contains the scaling component of the decomposed matrix.</param>
    /// <param name="rotation">When the method completes, contains the rotation component of the decomposed matrix.</param>
    /// <param name="translation">When the method completes, contains the translation component of the decomposed matrix.</param>
    /// <remarks>This method is designed to decompose an SRT transformation matrix only.</remarks>
    void Decompose(Float3& scale, Matrix3x3& rotation, Float3& translation) const;
    DEPRECATED("Use Decompose with 'Matrix3x3& rotation' parameter instead") void Decompose(Float3& scale, Matrix& rotation, Float3& translation) const;

public:
    Matrix operator*(const float scale) const
    {
        Matrix result;
        Multiply(*this, scale, result);
        return result;
    }

    Matrix operator*(const Matrix& other) const
    {
        Matrix result;
        Multiply(*this, other, result);
        return result;
    }

    Matrix& operator+=(const Matrix& other)
    {
        Add(*this, other, *this);
        return *this;
    }

    Matrix& operator-=(const Matrix& other)
    {
        Subtract(*this, other, *this);
        return *this;
    }

    Matrix& operator*=(const Matrix& other)
    {
        const Matrix tmp = *this;
        Multiply(tmp, other, *this);
        return *this;
    }

    Matrix& operator*=(const float scale)
    {
        Multiply(*this, scale, *this);
        return *this;
    }

    bool operator==(const Matrix& other) const;
    bool operator!=(const Matrix& other) const
    {
        return !operator==(other);
    }

public:
    // Calculates the sum of two matrices.
    static void Add(const Matrix& left, const Matrix& right, Matrix& result);

    // Calculates the difference between two matrices.
    static void Subtract(const Matrix& left, const Matrix& right, Matrix& result);

    // Scales a matrix by the given value.
    static void Multiply(const Matrix& left, float right, Matrix& result);

    // Calculates the product of two matrices.
    static Matrix Multiply(const Matrix& left, const Matrix& right)
    {
        Matrix result;
        Multiply(left, right, result);
        return result;
    }

    // Calculates the product of two matrices.
    static void Multiply(const Matrix& left, const Matrix& right, Matrix& result);

    // Scales a matrix by the given value.
    static void Divide(const Matrix& left, float right, Matrix& result);

    // Calculates the quotient of two matrices.
    static void Divide(const Matrix& left, const Matrix& right, Matrix& result);

    // Negates a matrix.
    static void Negate(const Matrix& value, Matrix& result);

    // Performs a linear interpolation between two matrices.
    static void Lerp(const Matrix& start, const Matrix& end, float amount, Matrix& result);

    // Performs a cubic interpolation between two matrices.
    static void SmoothStep(const Matrix& start, const Matrix& end, float amount, Matrix& result)
    {
        amount = Math::SmoothStep(amount);
        Lerp(start, end, amount, result);
    }

    // Calculates the transpose of the specified matrix.
    static Matrix Transpose(const Matrix& value);

    // Calculates the transpose of the specified matrix.
    static void Transpose(const Matrix& value, Matrix& result);

    /// <summary>
    /// Calculates the inverse of the specified matrix.
    /// </summary>
    /// <param name="value">The matrix whose inverse is to be calculated.</param>
    /// <returns>The inverse of the specified matrix.</returns>
    static Matrix Invert(const Matrix& value)
    {
        Matrix result;
        Invert(value, result);
        return result;
    }

    /// <summary>
    /// Calculates the inverse of the specified matrix.
    /// </summary>
    /// <param name="value">The matrix whose inverse is to be calculated.</param>
    /// <param name="result">When the method completes, contains the inverse of the specified matrix.</param>
    static void Invert(const Matrix& value, Matrix& result);

    // Creates a left-handed spherical billboard that rotates around a specified object position.
    static void Billboard(const Float3& objectPosition, const Float3& cameraPosition, const Float3& cameraUpFloat, const Float3& cameraForwardFloat, Matrix& result);

    // Creates a left-handed, look-at matrix.
    static void LookAt(const Float3& eye, const Float3& target, const Float3& up, Matrix& result);

    // Creates a left-handed, orthographic projection matrix.
    static void Ortho(float width, float height, float zNear, float zFar, Matrix& result)
    {
        const float halfWidth = width * 0.5f;
        const float halfHeight = height * 0.5f;
        OrthoOffCenter(-halfWidth, halfWidth, -halfHeight, halfHeight, zNear, zFar, result);
    }

    // Creates a left-handed, customized orthographic projection matrix.
    static void OrthoOffCenter(float left, float right, float bottom, float top, float zNear, float zFar, Matrix& result);

    // Creates a left-handed, perspective projection matrix.
    static void Perspective(float width, float height, float zNear, float zFar, Matrix& result)
    {
        const float halfWidth = width * 0.5f;
        const float halfHeight = height * 0.5f;
        PerspectiveOffCenter(-halfWidth, halfWidth, -halfHeight, halfHeight, zNear, zFar, result);
    }

    // Creates a left-handed, perspective projection matrix based on a field of view.
    static void PerspectiveFov(float fov, float aspect, float zNear, float zFar, Matrix& result);

    // Creates a left-handed, customized perspective projection matrix.
    static void PerspectiveOffCenter(float left, float right, float bottom, float top, float zNear, float zFar, Matrix& result);

    // Creates a matrix that scales along the x-axis, y-axis, and y-axis.
    static Matrix Scaling(const Float3& scale)
    {
        return Scaling(scale.X, scale.Y, scale.Z);
    }

    // Creates a matrix that scales along the x-axis, y-axis, and y-axis.
    static void Scaling(const Float3& scale, Matrix& result)
    {
        Scaling(scale.X, scale.Y, scale.Z, result);
    }

    // Creates a matrix that scales along the x-axis, y-axis, and y-axis.
    static Matrix Scaling(float x, float y, float z)
    {
        Matrix result = Identity;
        result.M11 = x;
        result.M22 = y;
        result.M33 = z;
        return result;
    }

    // Creates a matrix that scales along the x-axis, y-axis, and y-axis.
    static void Scaling(float x, float y, float z, Matrix& result)
    {
        result = Identity;
        result.M11 = x;
        result.M22 = y;
        result.M33 = z;
    }

    // Creates a matrix that uniformly scales along all three axis.
    static Matrix Scaling(float scale)
    {
        Matrix result = Identity;
        result.M11 = result.M22 = result.M33 = scale;
        return result;
    }

    // Creates a matrix that uniformly scales along all three axis.
    static void Scaling(float scale, Matrix& result)
    {
        result = Identity;
        result.M11 = result.M22 = result.M33 = scale;
    }

    // Creates a matrix that rotates around the x-axis. Angle of rotation in radians. Angles are measured clockwise when looking along the rotation axis toward the origin.
    static Matrix RotationX(float angle)
    {
        Matrix result;
        RotationX(angle, result);
        return result;
    }

    // Creates a matrix that rotates around the x-axis. Angle of rotation in radians. Angles are measured clockwise when looking along the rotation axis toward the origin.
    static void RotationX(float angle, Matrix& result);

    // Creates a matrix that rotates around the y-axis. Angle of rotation in radians. Angles are measured clockwise when looking along the rotation axis toward the origin.
    static Matrix RotationY(float angle)
    {
        Matrix result;
        RotationY(angle, result);
        return result;
    }

    // Creates a matrix that rotates around the y-axis. Angle of rotation in radians. Angles are measured clockwise when looking along the rotation axis toward the origin.
    static void RotationY(float angle, Matrix& result);

    // Creates a matrix that rotates around the z-axis. Angle of rotation in radians. Angles are measured clockwise when looking along the rotation axis toward the origin.
    static Matrix RotationZ(float angle)
    {
        Matrix result;
        RotationZ(angle, result);
        return result;
    }

    // Creates a matrix that rotates around the z-axis. Angle of rotation in radians. Angles are measured clockwise when looking along the rotation axis toward the origin.
    static void RotationZ(float angle, Matrix& result);

    // Creates a matrix that rotates around an arbitrary axis (normalized). Angle of rotation in radians. Angles are measured clockwise when looking along the rotation axis toward the origin.
    static Matrix RotationAxis(const Float3& axis, float angle)
    {
        Matrix result;
        RotationAxis(axis, angle, result);
        return result;
    }

    // Creates a matrix that rotates around an arbitrary axis (normalized). Angle of rotation in radians. Angles are measured clockwise when looking along the rotation axis toward the origin.
    static void RotationAxis(const Float3& axis, float angle, Matrix& result);

    /// <summary>
    /// Creates a rotation matrix from a quaternion
    /// </summary>
    /// <param name="rotation">The quaternion to use to build the matrix.</param>
    /// <returns>The created rotation matrix</returns>
    static Matrix RotationQuaternion(const Quaternion& rotation)
    {
        Matrix result;
        RotationQuaternion(rotation, result);
        return result;
    }

    /// <summary>
    /// Creates a rotation matrix from a quaternion.
    /// </summary>
    /// <param name="rotation">The quaternion to use to build the matrix.</param>
    /// <param name="result">The created rotation matrix.</param>
    static void RotationQuaternion(const Quaternion& rotation, Matrix& result);

    // Creates a rotation matrix with a specified yaw, pitch, and roll.
    static Matrix RotationYawPitchRoll(float yaw, float pitch, float roll)
    {
        Matrix result;
        RotationYawPitchRoll(yaw, pitch, roll, result);
        return result;
    }

    // Creates a rotation matrix with a specified yaw, pitch, and roll.
    static void RotationYawPitchRoll(float yaw, float pitch, float roll, Matrix& result);

    // Creates a translation matrix using the specified offsets.
    static Matrix Translation(const Float3& value);

    // Creates a translation matrix using the specified offsets.
    static void Translation(const Float3& value, Matrix& result);

    // Creates a translation matrix using the specified offsets.
    static void Translation(float x, float y, float z, Matrix& result);

    // Creates a skew/shear matrix by means of a translation vector, a rotation vector, and a rotation angle.
    static void Skew(float angle, const Float3& rotationVec, const Float3& transVec, Matrix& matrix);

    /// <summary>
    /// Creates a matrix that contains both the X, Y and Z rotation, as well as scaling and translation.
    /// </summary>
    /// <param name="translation">The translation.</param>
    /// <param name="rotation">Angle of rotation in radians. Angles are measured clockwise when looking along the rotation axis toward the origin.</param>
    /// <param name="scaling">The scaling.</param>
    /// <param name="result">When the method completes, contains the created transformation matrix.</param>
    static void Transformation(const Float3& scaling, const Quaternion& rotation, const Float3& translation, Matrix& result);

    // Creates a 3D affine transformation matrix.
    static void AffineTransformation(float scaling, const Quaternion& rotation, const Float3& translation, Matrix& result);

    // Creates a 3D affine transformation matrix.
    static void AffineTransformation(float scaling, const Float3& rotationCenter, const Quaternion& rotation, const Float3& translation, Matrix& result);

    // Creates a 2D affine transformation matrix.
    static void AffineTransformation2D(float scaling, float rotation, const Float2& translation, Matrix& result);

    // Creates a 2D affine transformation matrix.
    static void AffineTransformation2D(float scaling, const Float2& rotationCenter, float rotation, const Float2& translation, Matrix& result);

    // Creates a transformation matrix.
    static void Transformation(const Float3& scalingCenter, const Quaternion& scalingRotation, const Float3& scaling, const Float3& rotationCenter, const Quaternion& rotation, const Float3& translation, Matrix& result);

    // Creates a 2D transformation matrix.
    static void Transformation2D(const Float2& scalingCenter, float scalingRotation, const Float2& scaling, const Float2& rotationCenter, float rotation, const Float2& translation, Matrix& result);

    /// <summary>
    /// Creates a world matrix with the specified parameters.
    /// </summary>
    /// <param name="position">Position of the object. This value is used in translation operations.</param>
    /// <param name="forward">Forward direction of the object.</param>
    /// <param name="up">Upward direction of the object; usually [0, 1, 0].</param>
    /// <returns>Created world matrix of given transformation world.</returns>
    static Matrix CreateWorld(const Float3& position, const Float3& forward, const Float3& up);

    /// <summary>
    /// Creates a world matrix with the specified parameters.
    /// </summary>
    /// <param name="position">Position of the object. This value is used in translation operations.</param>
    /// <param name="forward">Forward direction of the object.</param>
    /// <param name="up">Upward direction of the object; usually [0, 1, 0].</param>
    /// <param name="result">Created world matrix of given transformation world.</param>
    static void CreateWorld(const Float3& position, const Float3& forward, const Float3& up, Matrix& result);

    /// <summary>
    /// Creates a new Matrix that rotates around an arbitrary vector.
    /// </summary>
    /// <param name="axis">The axis to rotate around.</param>
    /// <param name="angle">The angle to rotate around the vector.</param>
    /// <returns>Result matrix.</returns>
    static Matrix CreateFromAxisAngle(const Float3& axis, float angle);

    /// <summary>
    /// Creates a new Matrix that rotates around an arbitrary vector.
    /// </summary>
    /// <param name="axis"The axis to rotate around.></param>
    /// <param name="angle">The angle to rotate around the vector.</param>
    /// <param name="result">Result matrix.</param>
    static void CreateFromAxisAngle(const Float3& axis, float angle, Matrix& result);

public:
    static Vector3 TransformVector(const Matrix& m, const Vector3& v);
    static Float4 TransformPosition(const Matrix& m, const Float3& v);
    static Float4 TransformPosition(const Matrix& m, const Float4& v);
};

template<>
struct TIsPODType<Matrix>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(Matrix, "[M11:{0} M12:{1} M13:{2} M14:{3}] [M21:{4} M22:{5} M23:{6} M24:{7}] [M31:{8} M32:{9} M33:{10} M34:{11}] [M41:{12} M42:{13} M43:{14} M44:{15}]",
                          v.M11, v.M12, v.M13, v.M14, v.M21, v.M22, v.M23, v.M24, v.M31, v.M32, v.M33, v.M34, v.M41, v.M42, v.M43, v.M44);
