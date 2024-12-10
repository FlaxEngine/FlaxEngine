// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "Matrix3x3.h"
#include "Matrix.h"
#include "Quaternion.h"
#include "../Types/String.h"

const Matrix3x3 Matrix3x3::Zero(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
const Matrix3x3 Matrix3x3::Identity(
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 1.0f);

Matrix3x3::Matrix3x3(const Matrix& matrix)
{
    Platform::MemoryCopy(&M11, &matrix.M11, sizeof(Float3));
    Platform::MemoryCopy(&M21, &matrix.M21, sizeof(Float3));
    Platform::MemoryCopy(&M31, &matrix.M31, sizeof(Float3));
}

String Matrix3x3::ToString() const
{
    return String::Format(TEXT("{}"), *this);
}

float Matrix3x3::GetDeterminant() const
{
    return M11 * M22 * M33 + M12 * M23 * M31 + M13 * M21 * M32 - M13 * M22 * M31 - M12 * M21 * M33 - M11 * M23 * M32;
}

void Matrix3x3::NormalizeScale()
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

void Matrix3x3::Invert(const Matrix3x3& value, Matrix3x3& result)
{
    const float d11 = value.M22 * value.M33 + value.M23 * -value.M32;
    const float d12 = value.M21 * value.M33 + value.M23 * -value.M31;
    const float d13 = value.M21 * value.M32 + value.M22 * -value.M31;

    float det = value.M11 * d11 - value.M12 * d12 + value.M13 * d13;
    if (Math::Abs(det) < ZeroTolerance)
    {
        result = Zero;
        return;
    }

    det = 1.0f / det;

    const float d21 = value.M12 * value.M33 + value.M13 * -value.M32;
    const float d22 = value.M11 * value.M33 + value.M13 * -value.M31;
    const float d23 = value.M11 * value.M32 + value.M12 * -value.M31;

    const float d31 = value.M12 * value.M23 - value.M13 * value.M22;
    const float d32 = value.M11 * value.M23 - value.M13 * value.M21;
    const float d33 = value.M11 * value.M22 - value.M12 * value.M21;

    result = Matrix3x3(
        +d11 * det,
        -d21 * det,
        +d31 * det,
        -d12 * det,
        +d22 * det,
        -d32 * det,
        +d13 * det,
        -d23 * det,
        +d33 * det
    );
}

void Matrix3x3::Transpose(const Matrix3x3& value, Matrix3x3& result)
{
    result = Matrix3x3(
        value.M11,
        value.M21,
        value.M31,
        value.M12,
        value.M22,
        value.M32,
        value.M13,
        value.M23,
        value.M33
    );
}

void Matrix3x3::Add(const Matrix3x3& left, const Matrix3x3& right, Matrix3x3& result)
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

void Matrix3x3::Subtract(const Matrix3x3& left, const Matrix3x3& right, Matrix3x3& result)
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

void Matrix3x3::Multiply(const Matrix3x3& left, float right, Matrix3x3& result)
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

void Matrix3x3::Multiply(const Matrix3x3& left, const Matrix3x3& right, Matrix3x3& result)
{
    result = Matrix3x3(
        left.M11 * right.M11 + left.M12 * right.M21 + left.M13 * right.M31,
        left.M11 * right.M12 + left.M12 * right.M22 + left.M13 * right.M32,
        left.M11 * right.M13 + left.M12 * right.M23 + left.M13 * right.M33,
        left.M21 * right.M11 + left.M22 * right.M21 + left.M23 * right.M31,
        left.M21 * right.M12 + left.M22 * right.M22 + left.M23 * right.M32,
        left.M21 * right.M13 + left.M22 * right.M23 + left.M23 * right.M33,
        left.M31 * right.M11 + left.M32 * right.M21 + left.M33 * right.M31,
        left.M31 * right.M12 + left.M32 * right.M22 + left.M33 * right.M32,
        left.M31 * right.M13 + left.M32 * right.M23 + left.M33 * right.M33
    );
}

void Matrix3x3::Divide(const Matrix3x3& left, float right, Matrix3x3& result)
{
    ASSERT(!Math::IsZero(right));
    const float inv = 1.0f / right;

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

void Matrix3x3::Divide(const Matrix3x3& left, const Matrix3x3& right, Matrix3x3& result)
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

void Matrix3x3::RotationQuaternion(const Quaternion& rotation, Matrix3x3& result)
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

    result.M21 = 2.0f * (xy - zw);
    result.M22 = 1.0f - 2.0f * (zz + xx);
    result.M23 = 2.0f * (yz + xw);

    result.M31 = 2.0f * (zx + yw);
    result.M32 = 2.0f * (yz - xw);
    result.M33 = 1.0f - 2.0f * (yy + xx);
}

void Matrix3x3::Decompose(Float3& scale, Matrix3x3& rotation) const
{
    // Scaling is the length of the rows
    scale = Float3(
        Math::Sqrt(M11 * M11 + M12 * M12 + M13 * M13),
        Math::Sqrt(M21 * M21 + M22 * M22 + M23 * M23),
        Math::Sqrt(M31 * M31 + M32 * M32 + M33 * M33));

    // If any of the scaling factors are zero, than the rotation matrix can not exist
    rotation = Identity;
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
    scale.Z = Float3::Dot(at, GetBackward()) > 0.0f ? scale.Z : -scale.Z;
}

void Matrix3x3::Decompose(Float3& scale, Quaternion& rotation) const
{
    Matrix3x3 rotationMatrix;
    Decompose(scale, rotationMatrix);
    Quaternion::RotationMatrix(rotationMatrix, rotation);
}

bool Matrix3x3::operator==(const Matrix3x3& other) const
{
    return
            Math::NearEqual(M11, other.M11) &&
            Math::NearEqual(M12, other.M12) &&
            Math::NearEqual(M13, other.M13) &&
            Math::NearEqual(M21, other.M21) &&
            Math::NearEqual(M22, other.M22) &&
            Math::NearEqual(M23, other.M23) &&
            Math::NearEqual(M31, other.M31) &&
            Math::NearEqual(M32, other.M32) &&
            Math::NearEqual(M33, other.M33);
}
