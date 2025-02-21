// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "Matrix.h"
#include "Double4x4.h"
#include "Matrix3x3.h"
#include "Matrix3x4.h"
#include "Vector2.h"
#include "Quaternion.h"
#include "Transform.h"
#include "../Types/String.h"

static_assert(sizeof(Matrix) == 4 * 4 * 4, "Invalid Matrix type size.");

const Matrix Matrix::Zero(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
const Matrix Matrix::Identity(
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f);

Matrix::Matrix(const Matrix3x3& matrix)
{
    Platform::MemoryCopy(&M11, &matrix.M11, sizeof(Float3));
    Platform::MemoryCopy(&M21, &matrix.M21, sizeof(Float3));
    Platform::MemoryCopy(&M31, &matrix.M31, sizeof(Float3));
    M14 = 0.0f;
    M24 = 0.0f;
    M34 = 0.0f;
    M44 = 1.0f;
}

Matrix::Matrix(const Double4x4& matrix)
{
    for (int32 i = 0; i < 16; i++)
        Raw[i] = (float)matrix.Raw[i];
}

String Matrix::ToString() const
{
    return String::Format(TEXT("{}"), *this);
}

float Matrix::GetDeterminant() const
{
    const float temp1 = M33 * M44 - M34 * M43;
    const float temp2 = M32 * M44 - M34 * M42;
    const float temp3 = M32 * M43 - M33 * M42;
    const float temp4 = M31 * M44 - M34 * M41;
    const float temp5 = M31 * M43 - M33 * M41;
    const float temp6 = M31 * M42 - M32 * M41;
    return M11 * (M22 * temp1 - M23 * temp2 + M24 * temp3) - M12 * (M21 * temp1 -M23 * temp4 + M24 * temp5) + M13 * (M21 * temp2 - M22 * temp4 + M24 * temp6) - M14 * (M21 * temp3 - M22 * temp5 + M23 * temp6);
}

float Matrix::RotDeterminant() const
{
    return
            Values[0][0] * (Values[1][1] * Values[2][2] - Values[1][2] * Values[2][1]) -
            Values[1][0] * (Values[0][1] * Values[2][2] - Values[0][2] * Values[2][1]) +
            Values[2][0] * (Values[0][1] * Values[1][2] - Values[0][2] * Values[1][1]);
}

void Matrix::NormalizeScale()
{
    const float scaleX = 1.0f / Float3(M11, M21, M31).Length();
    const float scaleY = 1.0f / Float3(M12, M22, M32).Length();
    const float scaleZ = 1.0f / Float3(M13, M23, M33).Length();

    M11 *= scaleX;
    M21 *= scaleX;
    M31 *= scaleX;

    M12 *= scaleY;
    M22 *= scaleY;
    M32 *= scaleY;

    M13 *= scaleZ;
    M23 *= scaleZ;
    M33 *= scaleZ;
}

void Matrix::Decompose(float& yaw, float& pitch, float& roll) const
{
    pitch = Math::Asin(-M32);
    if (Math::Cos(pitch) > 1e-12f)
    {
        roll = Math::Atan2(M12, M22);
        yaw = Math::Atan2(M31, M33);
    }
    else
    {
        roll = Math::Atan2(-M21, M11);
        yaw = 0.0f;
    }
}

void Matrix::Decompose(Float3& scale, Float3& translation) const
{
    // Get the translation
    translation = Float3(M41, M42, M43);

    // Scaling is the length of the rows
    scale = Float3(
        Math::Sqrt(M11 * M11 + M12 * M12 + M13 * M13),
        Math::Sqrt(M21 * M21 + M22 * M22 + M23 * M23),
        Math::Sqrt(M31 * M31 + M32 * M32 + M33 * M33));
}

void Matrix::Decompose(Transform& transform) const
{
    Matrix3x3 rotationMatrix;
    Float3 translation;
    Decompose(transform.Scale, rotationMatrix, translation);
    transform.Translation = translation;
    Quaternion::RotationMatrix(rotationMatrix, transform.Orientation);
}

void Matrix::Decompose(Float3& scale, Quaternion& rotation, Float3& translation) const
{
    Matrix3x3 rotationMatrix;
    Decompose(scale, rotationMatrix, translation);
    Quaternion::RotationMatrix(rotationMatrix, rotation);
}

void Matrix::Decompose(Float3& scale, Matrix3x3& rotation, Float3& translation) const
{
    // Get the translation
    translation = Float3(M41, M42, M43);

    // Scaling is the length of the rows
    scale = Float3(
        Math::Sqrt(M11 * M11 + M12 * M12 + M13 * M13),
        Math::Sqrt(M21 * M21 + M22 * M22 + M23 * M23),
        Math::Sqrt(M31 * M31 + M32 * M32 + M33 * M33));

    // If any of the scaling factors are zero, than the rotation matrix can not exist
    rotation = Matrix3x3::Identity;
    if (scale.IsAnyZero())
        return;

    // Calculate an perfect orthonormal matrix (no reflections)
    const auto at = Float3(M31 / scale.Z, M32 / scale.Z, M33 / scale.Z);
    const auto up = Float3::Cross(at, Float3(M11 / scale.X, M12 / scale.X, M13 / scale.X));
    const auto right = Float3::Cross(up, at);
    rotation.SetRight(right);
    rotation.SetUp(up);
    rotation.SetForward(at);

    // In case of reflexions
    scale.X = Float3::Dot(right, GetRight()) > 0.0f ? scale.X : -scale.X;
    scale.Y = Float3::Dot(up, GetUp()) > 0.0f ? scale.Y : -scale.Y;
    scale.Z = Float3::Dot(at, GetForward()) > 0.0f ? scale.Z : -scale.Z;
}

void Matrix::Decompose(Float3& scale, Matrix& rotation, Float3& translation) const
{
    // [Deprecated on 20.02.2024, expires on 20.02.2026]
    Matrix3x3 r;
    Decompose(scale, r, translation);
    rotation = Matrix(r);
}

bool Matrix::operator==(const Matrix& other) const
{
    for (int32 i = 0; i < 16; i++)
    {
        if (Math::NotNearEqual(other.Raw[i], Raw[i]))
            return false;
    }
    return true;
}

void Matrix::Add(const Matrix& left, const Matrix& right, Matrix& result)
{
    result.M11 = left.M11 + right.M11;
    result.M12 = left.M12 + right.M12;
    result.M13 = left.M13 + right.M13;
    result.M14 = left.M14 + right.M14;
    result.M21 = left.M21 + right.M21;
    result.M22 = left.M22 + right.M22;
    result.M23 = left.M23 + right.M23;
    result.M24 = left.M24 + right.M24;
    result.M31 = left.M31 + right.M31;
    result.M32 = left.M32 + right.M32;
    result.M33 = left.M33 + right.M33;
    result.M34 = left.M34 + right.M34;
    result.M41 = left.M41 + right.M41;
    result.M42 = left.M42 + right.M42;
    result.M43 = left.M43 + right.M43;
    result.M44 = left.M44 + right.M44;
}

void Matrix::Subtract(const Matrix& left, const Matrix& right, Matrix& result)
{
    result.M11 = left.M11 - right.M11;
    result.M12 = left.M12 - right.M12;
    result.M13 = left.M13 - right.M13;
    result.M14 = left.M14 - right.M14;
    result.M21 = left.M21 - right.M21;
    result.M22 = left.M22 - right.M22;
    result.M23 = left.M23 - right.M23;
    result.M24 = left.M24 - right.M24;
    result.M31 = left.M31 - right.M31;
    result.M32 = left.M32 - right.M32;
    result.M33 = left.M33 - right.M33;
    result.M34 = left.M34 - right.M34;
    result.M41 = left.M41 - right.M41;
    result.M42 = left.M42 - right.M42;
    result.M43 = left.M43 - right.M43;
    result.M44 = left.M44 - right.M44;
}

void Matrix::Multiply(const Matrix& left, float right, Matrix& result)
{
    result.M11 = left.M11 * right;
    result.M12 = left.M12 * right;
    result.M13 = left.M13 * right;
    result.M14 = left.M14 * right;
    result.M21 = left.M21 * right;
    result.M22 = left.M22 * right;
    result.M23 = left.M23 * right;
    result.M24 = left.M24 * right;
    result.M31 = left.M31 * right;
    result.M32 = left.M32 * right;
    result.M33 = left.M33 * right;
    result.M34 = left.M34 * right;
    result.M41 = left.M41 * right;
    result.M42 = left.M42 * right;
    result.M43 = left.M43 * right;
    result.M44 = left.M44 * right;
}

void Matrix::Multiply(const Matrix& left, const Matrix& right, Matrix& result)
{
    result.M11 = left.M11 * right.M11 + left.M12 * right.M21 + left.M13 * right.M31 + left.M14 * right.M41;
    result.M12 = left.M11 * right.M12 + left.M12 * right.M22 + left.M13 * right.M32 + left.M14 * right.M42;
    result.M13 = left.M11 * right.M13 + left.M12 * right.M23 + left.M13 * right.M33 + left.M14 * right.M43;
    result.M14 = left.M11 * right.M14 + left.M12 * right.M24 + left.M13 * right.M34 + left.M14 * right.M44;
    result.M21 = left.M21 * right.M11 + left.M22 * right.M21 + left.M23 * right.M31 + left.M24 * right.M41;
    result.M22 = left.M21 * right.M12 + left.M22 * right.M22 + left.M23 * right.M32 + left.M24 * right.M42;
    result.M23 = left.M21 * right.M13 + left.M22 * right.M23 + left.M23 * right.M33 + left.M24 * right.M43;
    result.M24 = left.M21 * right.M14 + left.M22 * right.M24 + left.M23 * right.M34 + left.M24 * right.M44;
    result.M31 = left.M31 * right.M11 + left.M32 * right.M21 + left.M33 * right.M31 + left.M34 * right.M41;
    result.M32 = left.M31 * right.M12 + left.M32 * right.M22 + left.M33 * right.M32 + left.M34 * right.M42;
    result.M33 = left.M31 * right.M13 + left.M32 * right.M23 + left.M33 * right.M33 + left.M34 * right.M43;
    result.M34 = left.M31 * right.M14 + left.M32 * right.M24 + left.M33 * right.M34 + left.M34 * right.M44;
    result.M41 = left.M41 * right.M11 + left.M42 * right.M21 + left.M43 * right.M31 + left.M44 * right.M41;
    result.M42 = left.M41 * right.M12 + left.M42 * right.M22 + left.M43 * right.M32 + left.M44 * right.M42;
    result.M43 = left.M41 * right.M13 + left.M42 * right.M23 + left.M43 * right.M33 + left.M44 * right.M43;
    result.M44 = left.M41 * right.M14 + left.M42 * right.M24 + left.M43 * right.M34 + left.M44 * right.M44;
}

void Matrix::Divide(const Matrix& left, float right, Matrix& result)
{
    ASSERT(!Math::IsZero(right));
    const float inv = 1.0f / right;

    result.M11 = left.M11 * inv;
    result.M12 = left.M12 * inv;
    result.M13 = left.M13 * inv;
    result.M14 = left.M14 * inv;
    result.M21 = left.M21 * inv;
    result.M22 = left.M22 * inv;
    result.M23 = left.M23 * inv;
    result.M24 = left.M24 * inv;
    result.M31 = left.M31 * inv;
    result.M32 = left.M32 * inv;
    result.M33 = left.M33 * inv;
    result.M34 = left.M34 * inv;
    result.M41 = left.M41 * inv;
    result.M42 = left.M42 * inv;
    result.M43 = left.M43 * inv;
    result.M44 = left.M44 * inv;
}

void Matrix::Divide(const Matrix& left, const Matrix& right, Matrix& result)
{
    result.M11 = left.M11 / right.M11;
    result.M12 = left.M12 / right.M12;
    result.M13 = left.M13 / right.M13;
    result.M14 = left.M14 / right.M14;
    result.M21 = left.M21 / right.M21;
    result.M22 = left.M22 / right.M22;
    result.M23 = left.M23 / right.M23;
    result.M24 = left.M24 / right.M24;
    result.M31 = left.M31 / right.M31;
    result.M32 = left.M32 / right.M32;
    result.M33 = left.M33 / right.M33;
    result.M34 = left.M34 / right.M34;
    result.M41 = left.M41 / right.M41;
    result.M42 = left.M42 / right.M42;
    result.M43 = left.M43 / right.M43;
    result.M44 = left.M44 / right.M44;
}

void Matrix::Negate(const Matrix& value, Matrix& result)
{
    result.M11 = -value.M11;
    result.M12 = -value.M12;
    result.M13 = -value.M13;
    result.M14 = -value.M14;
    result.M21 = -value.M21;
    result.M22 = -value.M22;
    result.M23 = -value.M23;
    result.M24 = -value.M24;
    result.M31 = -value.M31;
    result.M32 = -value.M32;
    result.M33 = -value.M33;
    result.M34 = -value.M34;
    result.M41 = -value.M41;
    result.M42 = -value.M42;
    result.M43 = -value.M43;
    result.M44 = -value.M44;
}

void Matrix::Lerp(const Matrix& start, const Matrix& end, float amount, Matrix& result)
{
    result.M11 = Math::Lerp(start.M11, end.M11, amount);
    result.M12 = Math::Lerp(start.M12, end.M12, amount);
    result.M13 = Math::Lerp(start.M13, end.M13, amount);
    result.M14 = Math::Lerp(start.M14, end.M14, amount);
    result.M21 = Math::Lerp(start.M21, end.M21, amount);
    result.M22 = Math::Lerp(start.M22, end.M22, amount);
    result.M23 = Math::Lerp(start.M23, end.M23, amount);
    result.M24 = Math::Lerp(start.M24, end.M24, amount);
    result.M31 = Math::Lerp(start.M31, end.M31, amount);
    result.M32 = Math::Lerp(start.M32, end.M32, amount);
    result.M33 = Math::Lerp(start.M33, end.M33, amount);
    result.M34 = Math::Lerp(start.M34, end.M34, amount);
    result.M41 = Math::Lerp(start.M41, end.M41, amount);
    result.M42 = Math::Lerp(start.M42, end.M42, amount);
    result.M43 = Math::Lerp(start.M43, end.M43, amount);
    result.M44 = Math::Lerp(start.M44, end.M44, amount);
}

Matrix Matrix::Transpose(const Matrix& value)
{
    Matrix result;
    result.M11 = value.M11;
    result.M12 = value.M21;
    result.M13 = value.M31;
    result.M14 = value.M41;
    result.M21 = value.M12;
    result.M22 = value.M22;
    result.M23 = value.M32;
    result.M24 = value.M42;
    result.M31 = value.M13;
    result.M32 = value.M23;
    result.M33 = value.M33;
    result.M34 = value.M43;
    result.M41 = value.M14;
    result.M42 = value.M24;
    result.M43 = value.M34;
    result.M44 = value.M44;
    return result;
}

void Matrix::Transpose(const Matrix& value, Matrix& result)
{
    Matrix temp;
    temp.M11 = value.M11;
    temp.M12 = value.M21;
    temp.M13 = value.M31;
    temp.M14 = value.M41;
    temp.M21 = value.M12;
    temp.M22 = value.M22;
    temp.M23 = value.M32;
    temp.M24 = value.M42;
    temp.M31 = value.M13;
    temp.M32 = value.M23;
    temp.M33 = value.M33;
    temp.M34 = value.M43;
    temp.M41 = value.M14;
    temp.M42 = value.M24;
    temp.M43 = value.M34;
    temp.M44 = value.M44;
    result = temp;
}

void Matrix::Invert(const Matrix& value, Matrix& result)
{
    const float b0 = value.M31 * value.M42 - value.M32 * value.M41;
    const float b1 = value.M31 * value.M43 - value.M33 * value.M41;
    const float b2 = value.M34 * value.M41 - value.M31 * value.M44;
    const float b3 = value.M32 * value.M43 - value.M33 * value.M42;
    const float b4 = value.M34 * value.M42 - value.M32 * value.M44;
    const float b5 = value.M33 * value.M44 - value.M34 * value.M43;

    const float d11 = value.M22 * b5 + value.M23 * b4 + value.M24 * b3;
    const float d12 = value.M21 * b5 + value.M23 * b2 + value.M24 * b1;
    const float d13 = value.M21 * -b4 + value.M22 * b2 + value.M24 * b0;
    const float d14 = value.M21 * b3 + value.M22 * -b1 + value.M23 * b0;

    float det = value.M11 * d11 - value.M12 * d12 + value.M13 * d13 - value.M14 * d14;
    if (Math::Abs(det) <= 1e-12f)
    {
        result = Zero;
        return;
    }

    det = 1.0f / det;

    const float a0 = value.M11 * value.M22 - value.M12 * value.M21;
    const float a1 = value.M11 * value.M23 - value.M13 * value.M21;
    const float a2 = value.M14 * value.M21 - value.M11 * value.M24;
    const float a3 = value.M12 * value.M23 - value.M13 * value.M22;
    const float a4 = value.M14 * value.M22 - value.M12 * value.M24;
    const float a5 = value.M13 * value.M24 - value.M14 * value.M23;

    const float d21 = value.M12 * b5 + value.M13 * b4 + value.M14 * b3;
    const float d22 = value.M11 * b5 + value.M13 * b2 + value.M14 * b1;
    const float d23 = value.M11 * -b4 + value.M12 * b2 + value.M14 * b0;
    const float d24 = value.M11 * b3 + value.M12 * -b1 + value.M13 * b0;

    const float d31 = value.M42 * a5 + value.M43 * a4 + value.M44 * a3;
    const float d32 = value.M41 * a5 + value.M43 * a2 + value.M44 * a1;
    const float d33 = value.M41 * -a4 + value.M42 * a2 + value.M44 * a0;
    const float d34 = value.M41 * a3 + value.M42 * -a1 + value.M43 * a0;

    const float d41 = value.M32 * a5 + value.M33 * a4 + value.M34 * a3;
    const float d42 = value.M31 * a5 + value.M33 * a2 + value.M34 * a1;
    const float d43 = value.M31 * -a4 + value.M32 * a2 + value.M34 * a0;
    const float d44 = value.M31 * a3 + value.M32 * -a1 + value.M33 * a0;

    result.M11 = +d11 * det;
    result.M12 = -d21 * det;
    result.M13 = +d31 * det;
    result.M14 = -d41 * det;
    result.M21 = -d12 * det;
    result.M22 = +d22 * det;
    result.M23 = -d32 * det;
    result.M24 = +d42 * det;
    result.M31 = +d13 * det;
    result.M32 = -d23 * det;
    result.M33 = +d33 * det;
    result.M34 = -d43 * det;
    result.M41 = -d14 * det;
    result.M42 = +d24 * det;
    result.M43 = -d34 * det;
    result.M44 = +d44 * det;
}

void Matrix::Billboard(const Float3& objectPosition, const Float3& cameraPosition, const Float3& cameraUpFloat, const Float3& cameraForwardFloat, Matrix& result)
{
    Float3 crossed;
    Float3 final;
    Float3 difference = cameraPosition - objectPosition;

    const float lengthSq = difference.LengthSquared();
    if (Math::IsZero(lengthSq))
        difference = -cameraForwardFloat;
    else
        difference *= 1.0f / Math::Sqrt(lengthSq);

    Float3::Cross(cameraUpFloat, difference, crossed);
    crossed.Normalize();
    Float3::Cross(difference, crossed, final);

    result.M11 = crossed.X;
    result.M12 = crossed.Y;
    result.M13 = crossed.Z;
    result.M14 = 0.0f;

    result.M21 = final.X;
    result.M22 = final.Y;
    result.M23 = final.Z;
    result.M24 = 0.0f;

    result.M31 = difference.X;
    result.M32 = difference.Y;
    result.M33 = difference.Z;
    result.M34 = 0.0f;

    result.M41 = objectPosition.X;
    result.M42 = objectPosition.Y;
    result.M43 = objectPosition.Z;
    result.M44 = 1.0f;
}

void Matrix::LookAt(const Float3& eye, const Float3& target, const Float3& up, Matrix& result)
{
    Float3 xaxis, yaxis, zaxis;
    Float3::Subtract(target, eye, zaxis);
    zaxis.Normalize();
    Float3::Cross(up, zaxis, xaxis);
    xaxis.Normalize();
    Float3::Cross(zaxis, xaxis, yaxis);

    result.M11 = xaxis.X;
    result.M21 = xaxis.Y;
    result.M31 = xaxis.Z;

    result.M12 = yaxis.X;
    result.M22 = yaxis.Y;
    result.M32 = yaxis.Z;

    result.M13 = zaxis.X;
    result.M23 = zaxis.Y;
    result.M33 = zaxis.Z;

    result.M14 = 0.0f;
    result.M24 = 0.0f;
    result.M34 = 0.0f;

    result.M41 = -Float3::Dot(xaxis, eye);
    result.M42 = -Float3::Dot(yaxis, eye);
    result.M43 = -Float3::Dot(zaxis, eye);
    result.M44 = 1.0f;
}

void Matrix::OrthoOffCenter(float left, float right, float bottom, float top, float zNear, float zFar, Matrix& result)
{
    const float zRange = 1.0f / (zFar - zNear);

    result = Identity;
    result.M11 = 2.0f / (right - left);
    result.M22 = 2.0f / (top - bottom);
    result.M33 = zRange;
    result.M41 = (left + right) / (left - right);
    result.M42 = (top + bottom) / (bottom - top);
    result.M43 = -zNear * zRange;
}

void Matrix::PerspectiveFov(float fov, float aspect, float zNear, float zFar, Matrix& result)
{
    const float yScale = 1.0f / Math::Tan(fov * 0.5f);
    const float xScale = yScale / aspect;

    const float halfWidth = zNear / xScale;
    const float halfHeight = zNear / yScale;

    PerspectiveOffCenter(-halfWidth, halfWidth, -halfHeight, halfHeight, zNear, zFar, result);
}

void Matrix::PerspectiveOffCenter(float left, float right, float bottom, float top, float zNear, float zFar, Matrix& result)
{
    const float zRange = zFar / (zFar - zNear);

    result = Zero;
    result.M11 = 2.0f * zNear / (right - left);
    result.M22 = 2.0f * zNear / (top - bottom);
    result.M31 = (left + right) / (left - right);
    result.M32 = (top + bottom) / (bottom - top);
    result.M33 = zRange;
    result.M34 = 1.0f;
    result.M43 = -zNear * zRange;
}

void Matrix::RotationX(float angle, Matrix& result)
{
    const float cosA = Math::Cos(angle);
    const float sinA = Math::Sin(angle);
    result = Identity;
    result.M22 = cosA;
    result.M23 = sinA;
    result.M32 = -sinA;
    result.M33 = cosA;
}

void Matrix::RotationY(float angle, Matrix& result)
{
    const float cosA = Math::Cos(angle);
    const float sinA = Math::Sin(angle);
    result = Identity;
    result.M11 = cosA;
    result.M13 = -sinA;
    result.M31 = sinA;
    result.M33 = cosA;
}

void Matrix::RotationZ(float angle, Matrix& result)
{
    const float cosA = Math::Cos(angle);
    const float sinA = Math::Sin(angle);
    result = Identity;
    result.M11 = cosA;
    result.M12 = sinA;
    result.M21 = -sinA;
    result.M22 = cosA;
}

void Matrix::RotationAxis(const Float3& axis, float angle, Matrix& result)
{
    const float x = axis.X;
    const float y = axis.Y;
    const float z = axis.Z;
    const float cosA = Math::Cos(angle);
    const float sinA = Math::Sin(angle);
    const float xx = x * x;
    const float yy = y * y;
    const float zz = z * z;
    const float xy = x * y;
    const float xz = x * z;
    const float yz = y * z;

    result = Identity;
    result.M11 = xx + cosA * (1.0f - xx);
    result.M12 = xy - cosA * xy + sinA * z;
    result.M13 = xz - cosA * xz - sinA * y;
    result.M21 = xy - cosA * xy - sinA * z;
    result.M22 = yy + cosA * (1.0f - yy);
    result.M23 = yz - cosA * yz + sinA * x;
    result.M31 = xz - cosA * xz + sinA * y;
    result.M32 = yz - cosA * yz - sinA * x;
    result.M33 = zz + cosA * (1.0f - zz);
}

void Matrix::RotationQuaternion(const Quaternion& rotation, Matrix& result)
{
    const float xx = rotation.X * rotation.X;
    const float yy = rotation.Y * rotation.Y;
    const float zz = rotation.Z * rotation.Z;
    const float xy = rotation.X * rotation.Y;
    const float zw = rotation.Z * rotation.W;
    const float zx = rotation.Z * rotation.X;
    const float yw = rotation.Y * rotation.W;
    const float yz = rotation.Y * rotation.Z;
    const float xw = rotation.X * rotation.W;

    result.M11 = 1.0f - 2.0f * (yy + zz);
    result.M12 = 2.0f * (xy + zw);
    result.M13 = 2.0f * (zx - yw);
    result.M14 = 0;

    result.M21 = 2.0f * (xy - zw);
    result.M22 = 1.0f - 2.0f * (zz + xx);
    result.M23 = 2.0f * (yz + xw);
    result.M24 = 0;

    result.M31 = 2.0f * (zx + yw);
    result.M32 = 2.0f * (yz - xw);
    result.M33 = 1.0f - 2.0f * (yy + xx);
    result.M34 = 0;

    result.M41 = 0;
    result.M42 = 0;
    result.M43 = 0;
    result.M44 = 1;
}

void Matrix::RotationYawPitchRoll(float yaw, float pitch, float roll, Matrix& result)
{
    Quaternion quaternion;
    Quaternion::RotationYawPitchRoll(yaw, pitch, roll, quaternion);
    RotationQuaternion(quaternion, result);
}

Matrix Matrix::Translation(const Float3& value)
{
    Matrix result = Identity;
    result.M41 = value.X;
    result.M42 = value.Y;
    result.M43 = value.Z;
    return result;
}

void Matrix::Translation(const Float3& value, Matrix& result)
{
    result = Identity;
    result.M41 = value.X;
    result.M42 = value.Y;
    result.M43 = value.Z;
}

void Matrix::Translation(float x, float y, float z, Matrix& result)
{
    result = Identity;
    result.M41 = x;
    result.M42 = y;
    result.M43 = z;
}

void Matrix::Skew(float angle, const Float3& rotationVec, const Float3& transVec, Matrix& matrix)
{
    // http://elckerlyc.ewi.utwente.nl/browser/Elckerlyc/Hmi/HmiMath/src/hmi/math/Mat3f.java
    const float MINIMAL_SKEW_ANGLE = 0.000001f;

    Float3 e0 = rotationVec;
    Float3 e1;
    Float3::Normalize(transVec, e1);

    const float rv1 = Float3::Dot(rotationVec, e1);
    e0 += rv1 * e1;
    const float rv0 = Float3::Dot(rotationVec, e0);
    const float cosa = Math::Cos(angle);
    const float sina = Math::Sin(angle);
    const float rr0 = rv0 * cosa - rv1 * sina;
    const float rr1 = rv0 * sina + rv1 * cosa;

    ASSERT(rr0 >= MINIMAL_SKEW_ANGLE);

    const float d = rr1 / rr0 - rv1 / rv0;

    matrix = Identity;
    matrix.M11 = d * e1.X * e0.X + 1.0f;
    matrix.M12 = d * e1.X * e0.Y;
    matrix.M13 = d * e1.X * e0.Z;
    matrix.M21 = d * e1.Y * e0.X;
    matrix.M22 = d * e1.Y * e0.Y + 1.0f;
    matrix.M23 = d * e1.Y * e0.Z;
    matrix.M31 = d * e1.Z * e0.X;
    matrix.M32 = d * e1.Z * e0.Y;
    matrix.M33 = d * e1.Z * e0.Z + 1.0f;
}

void Matrix::Transformation(const Float3& scaling, const Quaternion& rotation, const Float3& translation, Matrix& result)
{
    // Equivalent to:
    //result =
    //    Matrix.Scaling(scaling)
    //    *Matrix.RotationX(rotation.X)
    //    *Matrix.RotationY(rotation.Y)
    //    *Matrix.RotationZ(rotation.Z)
    //    *Matrix.Position(translation);

    // Rotation
    const float xx = rotation.X * rotation.X;
    const float yy = rotation.Y * rotation.Y;
    const float zz = rotation.Z * rotation.Z;
    const float xy = rotation.X * rotation.Y;
    const float zw = rotation.Z * rotation.W;
    const float zx = rotation.Z * rotation.X;
    const float yw = rotation.Y * rotation.W;
    const float yz = rotation.Y * rotation.Z;
    const float xw = rotation.X * rotation.W;
    result.M11 = 1.0f - 2.0f * (yy + zz);
    result.M12 = 2.0f * (xy + zw);
    result.M13 = 2.0f * (zx - yw);
    result.M21 = 2.0f * (xy - zw);
    result.M22 = 1.0f - 2.0f * (zz + xx);
    result.M23 = 2.0f * (yz + xw);
    result.M31 = 2.0f * (zx + yw);
    result.M32 = 2.0f * (yz - xw);
    result.M33 = 1.0f - 2.0f * (yy + xx);

    // Position
    result.M41 = translation.X;
    result.M42 = translation.Y;
    result.M43 = translation.Z;

    // Scale
    result.M11 *= scaling.X;
    result.M12 *= scaling.X;
    result.M13 *= scaling.X;
    result.M21 *= scaling.Y;
    result.M22 *= scaling.Y;
    result.M23 *= scaling.Y;
    result.M31 *= scaling.Z;
    result.M32 *= scaling.Z;
    result.M33 *= scaling.Z;

    result.M14 = 0.0f;
    result.M24 = 0.0f;
    result.M34 = 0.0f;
    result.M44 = 1.0f;
}

void Matrix::AffineTransformation(float scaling, const Quaternion& rotation, const Float3& translation, Matrix& result)
{
    result = Scaling(scaling) * RotationQuaternion(rotation) * Translation(translation);
}

void Matrix::AffineTransformation(float scaling, const Float3& rotationCenter, const Quaternion& rotation, const Float3& translation, Matrix& result)
{
    result = Scaling(scaling) * Translation(-rotationCenter) * RotationQuaternion(rotation) * Translation(rotationCenter) * Translation(translation);
}

void Matrix::AffineTransformation2D(float scaling, float rotation, const Float2& translation, Matrix& result)
{
    result = Scaling(scaling, scaling, 1.0f) * RotationZ(rotation) * Translation((Float3)translation);
}

void Matrix::AffineTransformation2D(float scaling, const Float2& rotationCenter, float rotation, const Float2& translation, Matrix& result)
{
    result = Scaling(scaling, scaling, 1.0f) * Translation((Float3)-rotationCenter) * RotationZ(rotation) * Translation((Float3)rotationCenter) * Translation((Float3)translation);
}

void Matrix::Transformation(const Float3& scalingCenter, const Quaternion& scalingRotation, const Float3& scaling, const Float3& rotationCenter, const Quaternion& rotation, const Float3& translation, Matrix& result)
{
    Matrix sr;
    RotationQuaternion(scalingRotation, sr);
    result = Translation(-scalingCenter) * Transpose(sr) * Scaling(scaling) * sr * Translation(scalingCenter) * Translation(-rotationCenter) * RotationQuaternion(rotation) * Translation(rotationCenter) * Translation(translation);
}

void Matrix::Transformation2D(const Float2& scalingCenter, float scalingRotation, const Float2& scaling, const Float2& rotationCenter, float rotation, const Float2& translation, Matrix& result)
{
    result = Translation((Float3)-scalingCenter) * RotationZ(-scalingRotation) * Scaling((Float3)scaling) * RotationZ(scalingRotation) * Translation((Float3)scalingCenter) * Translation((Float3)-rotationCenter) * RotationZ(rotation) * Translation((Float3)rotationCenter) * Translation((Float3)translation);
    result.M33 = 1.0f;
    result.M44 = 1.0f;
}

Matrix Matrix::CreateWorld(const Float3& position, const Float3& forward, const Float3& up)
{
    Matrix result;
    CreateWorld(position, forward, up, result);
    return result;
}

void Matrix::CreateWorld(const Float3& position, const Float3& forward, const Float3& up, Matrix& result)
{
    Float3 vector3, vector31, vector32;

    Float3::Normalize(forward, vector3);
    vector3 = vector3.GetNegative();
    Float3::Normalize(Float3::Cross(up, vector3), vector31);
    Float3::Cross(vector3, vector31, vector32);

    result.M11 = vector31.X;
    result.M12 = vector31.Y;
    result.M13 = vector31.Z;
    result.M14 = 0.0f;

    result.M21 = vector32.X;
    result.M22 = vector32.Y;
    result.M23 = vector32.Z;
    result.M24 = 0.0f;

    result.M31 = vector3.X;
    result.M32 = vector3.Y;
    result.M33 = vector3.Z;
    result.M34 = 0.0f;

    result.M41 = position.X;
    result.M42 = position.Y;
    result.M43 = position.Z;
    result.M44 = 1.0f;
}

Matrix Matrix::CreateFromAxisAngle(const Float3& axis, float angle)
{
    Matrix result;
    CreateFromAxisAngle(axis, angle, result);
    return result;
}

void Matrix::CreateFromAxisAngle(const Float3& axis, float angle, Matrix& result)
{
    const float x = axis.X;
    const float y = axis.Y;
    const float z = axis.Z;
    const float single = Math::Sin(angle);
    const float single1 = Math::Cos(angle);
    const float single2 = x * x;
    const float single3 = y * y;
    const float single4 = z * z;
    const float single5 = x * y;
    const float single6 = x * z;
    const float single7 = y * z;

    result.M11 = single2 + single1 * (1.0f - single2);
    result.M12 = single5 - single1 * single5 + single * z;
    result.M13 = single6 - single1 * single6 - single * y;
    result.M14 = 0.0f;

    result.M21 = single5 - single1 * single5 - single * z;
    result.M22 = single3 + single1 * (1.0f - single3);
    result.M23 = single7 - single1 * single7 + single * x;
    result.M24 = 0.0f;

    result.M31 = single6 - single1 * single6 + single * y;
    result.M32 = single7 - single1 * single7 - single * x;
    result.M33 = single4 + single1 * (1.0f - single4);
    result.M34 = 0.0f;

    result.M41 = 0.0f;
    result.M42 = 0.0f;
    result.M43 = 0.0f;
    result.M44 = 1.0f;
}

Vector3 Matrix::TransformVector(const Matrix& m, const Vector3& v)
{
    return Vector3(
        m.Values[0][0] * v.Raw[0] + m.Values[1][0] * v.Raw[1] + m.Values[2][0] * v.Raw[2],
        m.Values[0][1] * v.Raw[0] + m.Values[1][1] * v.Raw[1] + m.Values[2][1] * v.Raw[2],
        m.Values[0][2] * v.Raw[0] + m.Values[1][2] * v.Raw[1] + m.Values[2][2] * v.Raw[2]
    );
}

Float4 Matrix::TransformPosition(const Matrix& m, const Float3& v)
{
    return Float4(
        m.Values[0][0] * v.Raw[0] + m.Values[1][0] * v.Raw[1] + m.Values[2][0] * v.Raw[2] + m.Values[3][0],
        m.Values[0][1] * v.Raw[0] + m.Values[1][1] * v.Raw[1] + m.Values[2][1] * v.Raw[2] + m.Values[3][1],
        m.Values[0][2] * v.Raw[0] + m.Values[1][2] * v.Raw[1] + m.Values[2][2] * v.Raw[2] + m.Values[3][2],
        m.Values[0][3] * v.Raw[0] + m.Values[1][3] * v.Raw[1] + m.Values[2][3] * v.Raw[2] + m.Values[3][3]
    );
}

Float4 Matrix::TransformPosition(const Matrix& m, const Float4& v)
{
    return Float4(
        m.Values[0][0] * v.Raw[0] + m.Values[1][0] * v.Raw[1] + m.Values[2][0] * v.Raw[2] + m.Values[3][0] * v.Raw[3],
        m.Values[0][1] * v.Raw[0] + m.Values[1][1] * v.Raw[1] + m.Values[2][1] * v.Raw[2] + m.Values[3][1] * v.Raw[3],
        m.Values[0][2] * v.Raw[0] + m.Values[1][2] * v.Raw[1] + m.Values[2][2] * v.Raw[2] + m.Values[3][2] * v.Raw[3],
        m.Values[0][3] * v.Raw[0] + m.Values[1][3] * v.Raw[1] + m.Values[2][3] * v.Raw[2] + m.Values[3][3] * v.Raw[3]
    );
}

void Matrix3x4::SetMatrix(const Matrix& m)
{
    const float* src = m.Raw;
    float* dst = Raw;
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = src[3];
    dst[4] = src[4];
    dst[5] = src[5];
    dst[6] = src[6];
    dst[7] = src[7];
    dst[8] = src[8];
    dst[9] = src[9];
    dst[10] = src[10];
    dst[11] = src[11];
}

void Matrix3x4::SetMatrixTranspose(const Matrix& m)
{
    const float* src = m.Raw;
    float* dst = Raw;
    dst[0] = src[0];
    dst[1] = src[4];
    dst[2] = src[8];
    dst[3] = src[12];
    dst[4] = src[1];
    dst[5] = src[5];
    dst[6] = src[9];
    dst[7] = src[13];
    dst[8] = src[2];
    dst[9] = src[6];
    dst[10] = src[10];
    dst[11] = src[14];
}

Double4x4::Double4x4(const Matrix& matrix)
{
    for (int32 i = 0; i < 16; i++)
        Raw[i] = matrix.Raw[i];
}

void Double4x4::Invert(const Double4x4& value, Double4x4& result)
{
    const double b0 = value.M31 * value.M42 - value.M32 * value.M41;
    const double b1 = value.M31 * value.M43 - value.M33 * value.M41;
    const double b2 = value.M34 * value.M41 - value.M31 * value.M44;
    const double b3 = value.M32 * value.M43 - value.M33 * value.M42;
    const double b4 = value.M34 * value.M42 - value.M32 * value.M44;
    const double b5 = value.M33 * value.M44 - value.M34 * value.M43;

    const double d11 = value.M22 * b5 + value.M23 * b4 + value.M24 * b3;
    const double d12 = value.M21 * b5 + value.M23 * b2 + value.M24 * b1;
    const double d13 = value.M21 * -b4 + value.M22 * b2 + value.M24 * b0;
    const double d14 = value.M21 * b3 + value.M22 * -b1 + value.M23 * b0;

    double det = value.M11 * d11 - value.M12 * d12 + value.M13 * d13 - value.M14 * d14;
    if (Math::Abs(det) <= 1e-12)
    {
        Platform::MemoryClear(&result, sizeof(Double4x4));
        return;
    }

    det = 1.0 / det;

    const double a0 = value.M11 * value.M22 - value.M12 * value.M21;
    const double a1 = value.M11 * value.M23 - value.M13 * value.M21;
    const double a2 = value.M14 * value.M21 - value.M11 * value.M24;
    const double a3 = value.M12 * value.M23 - value.M13 * value.M22;
    const double a4 = value.M14 * value.M22 - value.M12 * value.M24;
    const double a5 = value.M13 * value.M24 - value.M14 * value.M23;

    const double d21 = value.M12 * b5 + value.M13 * b4 + value.M14 * b3;
    const double d22 = value.M11 * b5 + value.M13 * b2 + value.M14 * b1;
    const double d23 = value.M11 * -b4 + value.M12 * b2 + value.M14 * b0;
    const double d24 = value.M11 * b3 + value.M12 * -b1 + value.M13 * b0;

    const double d31 = value.M42 * a5 + value.M43 * a4 + value.M44 * a3;
    const double d32 = value.M41 * a5 + value.M43 * a2 + value.M44 * a1;
    const double d33 = value.M41 * -a4 + value.M42 * a2 + value.M44 * a0;
    const double d34 = value.M41 * a3 + value.M42 * -a1 + value.M43 * a0;

    const double d41 = value.M32 * a5 + value.M33 * a4 + value.M34 * a3;
    const double d42 = value.M31 * a5 + value.M33 * a2 + value.M34 * a1;
    const double d43 = value.M31 * -a4 + value.M32 * a2 + value.M34 * a0;
    const double d44 = value.M31 * a3 + value.M32 * -a1 + value.M33 * a0;

    result.M11 = +d11 * det;
    result.M12 = -d21 * det;
    result.M13 = +d31 * det;
    result.M14 = -d41 * det;
    result.M21 = -d12 * det;
    result.M22 = +d22 * det;
    result.M23 = -d32 * det;
    result.M24 = +d42 * det;
    result.M31 = +d13 * det;
    result.M32 = -d23 * det;
    result.M33 = +d33 * det;
    result.M34 = -d43 * det;
    result.M41 = -d14 * det;
    result.M42 = +d24 * det;
    result.M43 = -d34 * det;
    result.M44 = +d44 * det;
}

void Double4x4::Multiply(const Double4x4& left, const Double4x4& right, Double4x4& result)
{
    result.M11 = left.M11 * right.M11 + left.M12 * right.M21 + left.M13 * right.M31 + left.M14 * right.M41;
    result.M12 = left.M11 * right.M12 + left.M12 * right.M22 + left.M13 * right.M32 + left.M14 * right.M42;
    result.M13 = left.M11 * right.M13 + left.M12 * right.M23 + left.M13 * right.M33 + left.M14 * right.M43;
    result.M14 = left.M11 * right.M14 + left.M12 * right.M24 + left.M13 * right.M34 + left.M14 * right.M44;
    result.M21 = left.M21 * right.M11 + left.M22 * right.M21 + left.M23 * right.M31 + left.M24 * right.M41;
    result.M22 = left.M21 * right.M12 + left.M22 * right.M22 + left.M23 * right.M32 + left.M24 * right.M42;
    result.M23 = left.M21 * right.M13 + left.M22 * right.M23 + left.M23 * right.M33 + left.M24 * right.M43;
    result.M24 = left.M21 * right.M14 + left.M22 * right.M24 + left.M23 * right.M34 + left.M24 * right.M44;
    result.M31 = left.M31 * right.M11 + left.M32 * right.M21 + left.M33 * right.M31 + left.M34 * right.M41;
    result.M32 = left.M31 * right.M12 + left.M32 * right.M22 + left.M33 * right.M32 + left.M34 * right.M42;
    result.M33 = left.M31 * right.M13 + left.M32 * right.M23 + left.M33 * right.M33 + left.M34 * right.M43;
    result.M34 = left.M31 * right.M14 + left.M32 * right.M24 + left.M33 * right.M34 + left.M34 * right.M44;
    result.M41 = left.M41 * right.M11 + left.M42 * right.M21 + left.M43 * right.M31 + left.M44 * right.M41;
    result.M42 = left.M41 * right.M12 + left.M42 * right.M22 + left.M43 * right.M32 + left.M44 * right.M42;
    result.M43 = left.M41 * right.M13 + left.M42 * right.M23 + left.M43 * right.M33 + left.M44 * right.M43;
    result.M44 = left.M41 * right.M14 + left.M42 * right.M24 + left.M43 * right.M34 + left.M44 * right.M44;
}

void Double4x4::Transformation(const Float3& scaling, const Quaternion& rotation, const Vector3& translation, Double4x4& result)
{
    // Equivalent to:
    //result =
    //    Matrix.Scaling(scaling)
    //    *Matrix.RotationX(rotation.X)
    //    *Matrix.RotationY(rotation.Y)
    //    *Matrix.RotationZ(rotation.Z)
    //    *Matrix.Position(translation);

    // Rotation
    const float xx = rotation.X * rotation.X;
    const float yy = rotation.Y * rotation.Y;
    const float zz = rotation.Z * rotation.Z;
    const float xy = rotation.X * rotation.Y;
    const float zw = rotation.Z * rotation.W;
    const float zx = rotation.Z * rotation.X;
    const float yw = rotation.Y * rotation.W;
    const float yz = rotation.Y * rotation.Z;
    const float xw = rotation.X * rotation.W;
    result.M11 = 1.0f - 2.0f * (yy + zz);
    result.M12 = 2.0f * (xy + zw);
    result.M13 = 2.0f * (zx - yw);
    result.M21 = 2.0f * (xy - zw);
    result.M22 = 1.0f - 2.0f * (zz + xx);
    result.M23 = 2.0f * (yz + xw);
    result.M31 = 2.0f * (zx + yw);
    result.M32 = 2.0f * (yz - xw);
    result.M33 = 1.0f - 2.0f * (yy + xx);

    // Position
    result.M41 = translation.X;
    result.M42 = translation.Y;
    result.M43 = translation.Z;

    // Scale
    result.M11 *= scaling.X;
    result.M12 *= scaling.X;
    result.M13 *= scaling.X;
    result.M21 *= scaling.Y;
    result.M22 *= scaling.Y;
    result.M23 *= scaling.Y;
    result.M31 *= scaling.Z;
    result.M32 *= scaling.Z;
    result.M33 *= scaling.Z;

    result.M14 = 0.0;
    result.M24 = 0.0;
    result.M34 = 0.0;
    result.M44 = 1.0;
}
