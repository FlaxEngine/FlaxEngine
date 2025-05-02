// Copyright (c) Wojciech Figat. All rights reserved.

#include "Quaternion.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix.h"
#include "Matrix3x3.h"
#include "Math.h"
#include "../Types/String.h"
#include "Engine/Core/Math/Transform.h"

Quaternion Quaternion::Zero(0, 0, 0, 0);
Quaternion Quaternion::One(1, 1, 1, 1);
Quaternion Quaternion::Identity(0, 0, 0, 1);

Quaternion::Quaternion(const Float4& value)
    : X(value.X)
    , Y(value.Y)
    , Z(value.Z)
    , W(value.W)
{
}

String Quaternion::ToString() const
{
    return String::Format(TEXT("{}"), *this);
}

float Quaternion::GetAngle() const
{
    const float length = X * X + Y * Y + Z * Z;
    if (Math::IsZero(length))
        return 0.0f;
    return 2.0f * acosf(Math::Clamp(W, -1.0f, 1.0f));
}

Float3 Quaternion::GetAxis() const
{
    const float length = X * X + Y * Y + Z * Z;
    if (Math::IsZero(length))
        return Float3::UnitX;
    const float inv = 1.0f / Math::Sqrt(length);
    return Float3(X * inv, Y * inv, Z * inv);
}

Float3 Quaternion::GetEuler() const
{
    Float3 result;
    const float sqw = W * W;
    const float sqx = X * X;
    const float sqy = Y * Y;
    const float sqz = Z * Z;
    const float unit = sqx + sqy + sqz + sqw; // if normalised is one, otherwise is correction factor
    const float test = X * W - Y * Z;

    if (test > 0.499995f * unit)
    {
        // singularity at north pole

        // yaw pitch roll
        result.Y = 2.0f * Math::Atan2(Y, X);
        result.X = PI_OVER_2;
        result.Z = 0;
    }
    else if (test < -0.499995f * unit)
    {
        // singularity at south pole

        // yaw pitch roll
        result.Y = -2.0f * Math::Atan2(Y, X);
        result.X = -PI_OVER_2;
        result.Z = 0;
    }
    else
    {
        // yaw pitch roll
        const Quaternion q = Quaternion(W, Z, X, Y);
        result.Y = Math::Atan2(2.0f * q.X * q.W + 2.0f * q.Y * q.Z, 1 - 2.0f * (q.Z * q.Z + q.W * q.W));
        result.X = Math::Asin(2.0f * (q.X * q.Z - q.W * q.Y));
        result.Z = Math::Atan2(2.0f * q.X * q.Y + 2.0f * q.Z * q.W, 1 - 2.0f * (q.Y * q.Y + q.Z * q.Z));
    }

    return Math::UnwindDegrees(result * RadiansToDegrees);
}

void Quaternion::Multiply(const Quaternion& other)
{
    const float a = Y * other.Z - Z * other.Y;
    const float b = Z * other.X - X * other.Z;
    const float c = X * other.Y - Y * other.X;
    const float d = X * other.X + Y * other.Y + Z * other.Z;
    X = X * other.W + other.X * W + a;
    Y = Y * other.W + other.Y * W + b;
    Z = Z * other.W + other.Z * W + c;
    W = W * other.W - d;
}

Float3 Quaternion::operator*(const Float3& vector) const
{
    return Float3::Transform(vector, *this);
}

void Quaternion::Add(const Quaternion& left, const Quaternion& right, Quaternion& result)
{
    result.X = left.X + right.X;
    result.Y = left.Y + right.Y;
    result.Z = left.Z + right.Z;
    result.W = left.W + right.W;
}

void Quaternion::Subtract(const Quaternion& left, const Quaternion& right, Quaternion& result)
{
    result.X = left.X - right.X;
    result.Y = left.Y - right.Y;
    result.Z = left.Z - right.Z;
    result.W = left.W - right.W;
}

void Quaternion::Multiply(const Quaternion& left, const Quaternion& right, Quaternion& result)
{
    const float a = left.Y * right.Z - left.Z * right.Y;
    const float b = left.Z * right.X - left.X * right.Z;
    const float c = left.X * right.Y - left.Y * right.X;
    const float d = left.X * right.X + left.Y * right.Y + left.Z * right.Z;
    result.X = left.X * right.W + right.X * left.W + a;
    result.Y = left.Y * right.W + right.Y * left.W + b;
    result.Z = left.Z * right.W + right.Z * left.W + c;
    result.W = left.W * right.W - d;
}

void Quaternion::Negate(const Quaternion& value, Quaternion& result)
{
    result.X = -value.X;
    result.Y = -value.Y;
    result.Z = -value.Z;
    result.W = -value.W;
}

void Quaternion::Lerp(const Quaternion& start, const Quaternion& end, float amount, Quaternion& result)
{
    const float inverse = 1.0f - amount;
    if (Dot(start, end) >= 0.0f)
    {
        result.X = inverse * start.X + amount * end.X;
        result.Y = inverse * start.Y + amount * end.Y;
        result.Z = inverse * start.Z + amount * end.Z;
        result.W = inverse * start.W + amount * end.W;
    }
    else
    {
        result.X = inverse * start.X - amount * end.X;
        result.Y = inverse * start.Y - amount * end.Y;
        result.Z = inverse * start.Z - amount * end.Z;
        result.W = inverse * start.W - amount * end.W;
    }
    result.Normalize();
}

void Quaternion::RotationAxis(const Float3& axis, float angle, Quaternion& result)
{
    Float3 normalized;
    Float3::Normalize(axis, normalized);

    const float half = angle * 0.5f;
    const float sinHalf = Math::Sin(half);
    const float cosHalf = Math::Cos(half);

    result.X = normalized.X * sinHalf;
    result.Y = normalized.Y * sinHalf;
    result.Z = normalized.Z * sinHalf;
    result.W = cosHalf;
}

void Quaternion::RotationCosAxis(const Float3& axis, float cos, Quaternion& result)
{
    Float3 normalized;
    Float3::Normalize(axis, normalized);

    const float cosHalf2 = (1.0f + cos) * 0.5f;
    const float sinHalf2 = 1.0f - cosHalf2;
    const float cosHalf = Math::Sqrt(cosHalf2);
    const float sinHalf = Math::Sqrt(sinHalf2);

    result.X = normalized.X * sinHalf;
    result.Y = normalized.Y * sinHalf;
    result.Z = normalized.Z * sinHalf;
    result.W = cosHalf;
}

void Quaternion::RotationMatrix(const Matrix& matrix, Quaternion& result)
{
    float sqrtV;
    float half;
    const float scale = matrix.M11 + matrix.M22 + matrix.M33;

    if (scale > 0.0f)
    {
        sqrtV = Math::Sqrt(scale + 1.0f);
        result.W = sqrtV * 0.5f;
        sqrtV = 0.5f / sqrtV;

        result.X = (matrix.M23 - matrix.M32) * sqrtV;
        result.Y = (matrix.M31 - matrix.M13) * sqrtV;
        result.Z = (matrix.M12 - matrix.M21) * sqrtV;
    }
    else if (matrix.M11 >= matrix.M22 && matrix.M11 >= matrix.M33)
    {
        sqrtV = Math::Sqrt(1.0f + matrix.M11 - matrix.M22 - matrix.M33);
        half = 0.5f / sqrtV;

        result = Quaternion(
            0.5f * sqrtV,
            (matrix.M12 + matrix.M21) * half,
            (matrix.M13 + matrix.M31) * half,
            (matrix.M23 - matrix.M32) * half);
    }
    else if (matrix.M22 > matrix.M33)
    {
        sqrtV = Math::Sqrt(1.0f + matrix.M22 - matrix.M11 - matrix.M33);
        half = 0.5f / sqrtV;

        result = Quaternion(
            (matrix.M21 + matrix.M12) * half,
            0.5f * sqrtV,
            (matrix.M32 + matrix.M23) * half,
            (matrix.M31 - matrix.M13) * half);
    }
    else
    {
        sqrtV = Math::Sqrt(1.0f + matrix.M33 - matrix.M11 - matrix.M22);
        half = 0.5f / sqrtV;

        result = Quaternion(
            (matrix.M31 + matrix.M13) * half,
            (matrix.M32 + matrix.M23) * half,
            0.5f * sqrtV,
            (matrix.M12 - matrix.M21) * half);
    }

    result.Normalize();
}

void Quaternion::RotationMatrix(const Matrix3x3& matrix, Quaternion& result)
{
    float sqrtV;
    float half;
    const float scale = matrix.M11 + matrix.M22 + matrix.M33;

    if (scale > 0.0f)
    {
        sqrtV = Math::Sqrt(scale + 1.0f);
        result.W = sqrtV * 0.5f;
        sqrtV = 0.5f / sqrtV;

        result.X = (matrix.M23 - matrix.M32) * sqrtV;
        result.Y = (matrix.M31 - matrix.M13) * sqrtV;
        result.Z = (matrix.M12 - matrix.M21) * sqrtV;
    }
    else if (matrix.M11 >= matrix.M22 && matrix.M11 >= matrix.M33)
    {
        sqrtV = Math::Sqrt(1.0f + matrix.M11 - matrix.M22 - matrix.M33);
        half = 0.5f / sqrtV;

        result = Quaternion(
            0.5f * sqrtV,
            (matrix.M12 + matrix.M21) * half,
            (matrix.M13 + matrix.M31) * half,
            (matrix.M23 - matrix.M32) * half);
    }
    else if (matrix.M22 > matrix.M33)
    {
        sqrtV = Math::Sqrt(1.0f + matrix.M22 - matrix.M11 - matrix.M33);
        half = 0.5f / sqrtV;

        result = Quaternion(
            (matrix.M21 + matrix.M12) * half,
            0.5f * sqrtV,
            (matrix.M32 + matrix.M23) * half,
            (matrix.M31 - matrix.M13) * half);
    }
    else
    {
        sqrtV = Math::Sqrt(1.0f + matrix.M33 - matrix.M11 - matrix.M22);
        half = 0.5f / sqrtV;

        result = Quaternion(
            (matrix.M31 + matrix.M13) * half,
            (matrix.M32 + matrix.M23) * half,
            0.5f * sqrtV,
            (matrix.M12 - matrix.M21) * half);
    }

    result.Normalize();
}

void Quaternion::LookAt(const Float3& eye, const Float3& target, const Float3& up, Quaternion& result)
{
    Matrix matrix;
    Matrix::LookAt(eye, target, up, matrix);
    RotationMatrix(matrix, result);
}

void Quaternion::RotationLookAt(const Float3& forward, const Float3& up, Quaternion& result)
{
    LookAt(Float3::Zero, forward, up, result);
}

void Quaternion::Billboard(const Float3& objectPosition, const Float3& cameraPosition, const Float3& cameraUpVector, const Float3& cameraForwardVector, Quaternion& result)
{
    Matrix matrix;
    Matrix::Billboard(objectPosition, cameraPosition, cameraUpVector, cameraForwardVector, matrix);
    RotationMatrix(matrix, result);
}

Quaternion Quaternion::FromDirection(const Float3& direction)
{
    CHECK_RETURN_DEBUG(direction.IsNormalized(), Quaternion::Identity);
    Quaternion orientation;
    if (Float3::Dot(direction, Float3::Up) >= 0.999f)
    {
        RotationAxis(Float3::Left, PI_OVER_2, orientation);
    }
    else if (Float3::Dot(direction, Float3::Down) >= 0.999f)
    {
        RotationAxis(Float3::Right, PI_OVER_2, orientation);
    }
    else
    {
        Float3 right, up;
        Float3::Cross(direction, Float3::Up, right);
        Float3::Cross(right, direction, up);
        LookRotation(direction, up, orientation);
    }
    return orientation;
}

void Quaternion::LookRotation(const Float3& forward, const Float3& up, Quaternion& result)
{
    Float3 forwardNorm = forward;
    forwardNorm.Normalize();
    Float3 rightNorm;
    Float3::Cross(up, forwardNorm, rightNorm);
    rightNorm.Normalize();
    Float3 upNorm;
    Float3::Cross(forwardNorm, rightNorm, upNorm);

#define m00 rightNorm.X
#define m01 rightNorm.Y
#define m02 rightNorm.Z
#define m10 upNorm.X
#define m11 upNorm.Y
#define m12 upNorm.Z
#define m20 forwardNorm.X
#define m21 forwardNorm.Y
#define m22 forwardNorm.Z

    const float sum = m00 + m11 + m22;
    if (sum > 0)
    {
        const float num = Math::Sqrt(sum + 1);
        const float invNumHalf = 0.5f / num;
        result.X = (m12 - m21) * invNumHalf;
        result.Y = (m20 - m02) * invNumHalf;
        result.Z = (m01 - m10) * invNumHalf;
        result.W = num * 0.5f;
    }
    else if (m00 >= m11 && m00 >= m22)
    {
        const float num = Math::Sqrt(1 + m00 - m11 - m22);
        const float invNumHalf = 0.5f / num;
        result.X = 0.5f * num;
        result.Y = (m01 + m10) * invNumHalf;
        result.Z = (m02 + m20) * invNumHalf;
        result.W = (m12 - m21) * invNumHalf;
    }
    else if (m11 > m22)
    {
        const float num = Math::Sqrt(1 + m11 - m00 - m22);
        const float invNumHalf = 0.5f / num;
        result.X = (m10 + m01) * invNumHalf;
        result.Y = 0.5f * num;
        result.Z = (m21 + m12) * invNumHalf;
        result.W = (m20 - m02) * invNumHalf;
    }
    else
    {
        const float num = Math::Sqrt(1 + m22 - m00 - m11);
        const float invNumHalf = 0.5f / num;
        result.X = (m20 + m02) * invNumHalf;
        result.Y = (m21 + m12) * invNumHalf;
        result.Z = 0.5f * num;
        result.W = (m01 - m10) * invNumHalf;
    }

#undef m00
#undef m01
#undef m02
#undef m10
#undef m11
#undef m12
#undef m20
#undef m21
#undef m22
}

void Quaternion::GetRotationFromTo(const Float3& from, const Float3& to, Quaternion& result, const Float3& fallbackAxis)
{
    // Based on Stan Melax's article in Game Programming Gems

    Float3 v0 = from;
    Float3 v1 = to;
    v0.Normalize();
    v1.Normalize();

    // If dot == 1, vectors are the same
    const float d = Float3::Dot(v0, v1);
    if (d >= 1.0f)
    {
        result = Identity;
        return;
    }

    if (d < 1e-6f - 1.0f)
    {
        if (fallbackAxis != Float3::Zero)
        {
            // Rotate 180 degrees about the fallback axis
            RotationAxis(fallbackAxis, PI, result);
        }
        else
        {
            // Generate an axis
            Float3 axis = Float3::Cross(Float3::UnitX, from);
            if (axis.LengthSquared() < ZeroTolerance) // Pick another if colinear
                axis = Float3::Cross(Float3::UnitY, from);
            axis.Normalize();
            RotationAxis(axis, PI, result);
        }
    }
    else
    {
        const float s = Math::Sqrt((1 + d) * 2);
        const float invS = 1 / s;

        Float3 c;
        Float3::Cross(v0, v1, c);

        result.X = c.X * invS;
        result.Y = c.Y * invS;
        result.Z = c.Z * invS;
        result.W = s * 0.5f;
        result.Normalize();
    }
}

void Quaternion::FindBetween(const Float3& from, const Float3& to, Quaternion& result)
{
    // http://lolengine.net/blog/2014/02/24/quaternion-from-two-vectors-final
    const float normFromNormTo = Math::Sqrt(from.LengthSquared() * to.LengthSquared());
    if (normFromNormTo < ZeroTolerance)
    {
        result = Identity;
        return;
    }
    const float w = normFromNormTo + Float3::Dot(from, to);
    if (w < 1.e-6f * normFromNormTo)
    {
        result = Math::Abs(from.X) > Math::Abs(from.Z)
                     ? Quaternion(-from.Y, from.X, 0.0f, 0.0f)
                     : Quaternion(0.0f, -from.Z, from.Y, 0.0f);
    }
    else
    {
        const Float3 cross = Float3::Cross(from, to);
        result = Quaternion(cross.X, cross.Y, cross.Z, w);
    }
    result.Normalize();
}

void Quaternion::Slerp(const Quaternion& start, const Quaternion& end, float amount, Quaternion& result)
{
    float opposite;
    float inverse;
    const float dot = Dot(start, end);

    if (Math::Abs(dot) > 1.0f - ZeroTolerance)
    {
        inverse = 1.0f - amount;
        opposite = amount * Math::Sign(dot);
    }
    else
    {
        const float acos1 = Math::Acos(Math::Abs(dot));
        const float invSin = 1.0f / Math::Sin(acos1);
        inverse = Math::Sin((1.0f - amount) * acos1) * invSin;
        opposite = Math::Sin(amount * acos1) * invSin * Math::Sign(dot);
    }

    result.X = inverse * start.X + opposite * end.X;
    result.Y = inverse * start.Y + opposite * end.Y;
    result.Z = inverse * start.Z + opposite * end.Z;
    result.W = inverse * start.W + opposite * end.W;
}

Quaternion Quaternion::Euler(float x, float y, float z)
{
    const float halfRoll = z * (DegreesToRadians * 0.5f);
    const float halfPitch = x * (DegreesToRadians * 0.5f);
    const float halfYaw = y * (DegreesToRadians * 0.5f);

    const float sinRollOver2 = Math::Sin(halfRoll);
    const float cosRollOver2 = Math::Cos(halfRoll);
    const float sinPitchOver2 = Math::Sin(halfPitch);
    const float cosPitchOver2 = Math::Cos(halfPitch);
    const float sinYawOver2 = Math::Sin(halfYaw);
    const float cosYawOver2 = Math::Cos(halfYaw);

    return Quaternion(
        cosYawOver2 * sinPitchOver2 * cosRollOver2 + sinYawOver2 * cosPitchOver2 * sinRollOver2,
        sinYawOver2 * cosPitchOver2 * cosRollOver2 - cosYawOver2 * sinPitchOver2 * sinRollOver2,
        cosYawOver2 * cosPitchOver2 * sinRollOver2 - sinYawOver2 * sinPitchOver2 * cosRollOver2,
        cosYawOver2 * cosPitchOver2 * cosRollOver2 + sinYawOver2 * sinPitchOver2 * sinRollOver2
    );
}

Quaternion Quaternion::Euler(const Float3& euler)
{
    const float halfRoll = euler.Z * (DegreesToRadians * 0.5f);
    const float halfPitch = euler.X * (DegreesToRadians * 0.5f);
    const float halfYaw = euler.Y * (DegreesToRadians * 0.5f);

    const float sinRollOver2 = Math::Sin(halfRoll);
    const float cosRollOver2 = Math::Cos(halfRoll);
    const float sinPitchOver2 = Math::Sin(halfPitch);
    const float cosPitchOver2 = Math::Cos(halfPitch);
    const float sinYawOver2 = Math::Sin(halfYaw);
    const float cosYawOver2 = Math::Cos(halfYaw);

    return Quaternion(
        cosYawOver2 * sinPitchOver2 * cosRollOver2 + sinYawOver2 * cosPitchOver2 * sinRollOver2,
        sinYawOver2 * cosPitchOver2 * cosRollOver2 - cosYawOver2 * sinPitchOver2 * sinRollOver2,
        cosYawOver2 * cosPitchOver2 * sinRollOver2 - sinYawOver2 * sinPitchOver2 * cosRollOver2,
        cosYawOver2 * cosPitchOver2 * cosRollOver2 + sinYawOver2 * sinPitchOver2 * sinRollOver2
    );
}

void Quaternion::RotationYawPitchRoll(float yaw, float pitch, float roll, Quaternion& result)
{
    const float halfRoll = roll * 0.5f;
    const float halfPitch = pitch * 0.5f;
    const float halfYaw = yaw * 0.5f;

    const float sinRollOver2 = Math::Sin(halfRoll);
    const float cosRollOver2 = Math::Cos(halfRoll);
    const float sinPitchOver2 = Math::Sin(halfPitch);
    const float cosPitchOver2 = Math::Cos(halfPitch);
    const float sinYawOver2 = Math::Sin(halfYaw);
    const float cosYawOver2 = Math::Cos(halfYaw);

    result.W = cosYawOver2 * cosPitchOver2 * cosRollOver2 + sinYawOver2 * sinPitchOver2 * sinRollOver2;
    result.X = cosYawOver2 * sinPitchOver2 * cosRollOver2 + sinYawOver2 * cosPitchOver2 * sinRollOver2;
    result.Y = sinYawOver2 * cosPitchOver2 * cosRollOver2 - cosYawOver2 * sinPitchOver2 * sinRollOver2;
    result.Z = cosYawOver2 * cosPitchOver2 * sinRollOver2 - sinYawOver2 * sinPitchOver2 * cosRollOver2;
}

Quaternion Quaternion::GetRotationFromNormal(const Vector3& normal, const Transform& reference)
{
    Float3 up = reference.GetUp();
    const Real dot = Vector3::Dot(normal, up);
    if (Math::NearEqual(Math::Abs(dot), 1))
        up = reference.GetRight();
    return Quaternion::LookRotation(normal, up);
}
