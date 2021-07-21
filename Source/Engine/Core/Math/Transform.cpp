// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Transform.h"
#include "Matrix.h"
#include "../Types/String.h"

Transform Transform::Identity(Vector3(0, 0, 0));

String Transform::ToString() const
{
    return String::Format(TEXT("{}"), *this);
}

Matrix Transform::GetRotation() const
{
    Matrix result;
    Matrix::RotationQuaternion(Orientation, result);
    return result;
}

void Transform::GetRotation(Matrix& result) const
{
    Matrix::RotationQuaternion(Orientation, result);
}

void Transform::SetRotation(const Matrix& value)
{
    Quaternion::RotationMatrix(value, Orientation);
}

Matrix Transform::GetWorld() const
{
    Matrix result;
    Matrix::Transformation(Scale, Orientation, Translation, result);
    return result;
}

void Transform::GetWorld(Matrix& result) const
{
    Matrix::Transformation(Scale, Orientation, Translation, result);
}

Transform Transform::Add(const Vector3& translation) const
{
    Transform result;
    result.Orientation = Orientation;
    result.Scale = Scale;
    Vector3::Add(Translation, translation, result.Translation);
    return result;
}

Transform Transform::Add(const Transform& other) const
{
    Transform result;
    Quaternion::Multiply(Orientation, other.Orientation, result.Orientation);
    result.Orientation.Normalize();
    Vector3::Multiply(Scale, other.Scale, result.Scale);
    Vector3::Add(Translation, other.Translation, result.Translation);
    return result;
}

Transform Transform::Subtract(const Transform& other) const
{
    Transform result;
    Vector3::Subtract(Translation, other.Translation, result.Translation);
    const Quaternion invRotation = other.Orientation.Conjugated();
    Quaternion::Multiply(Orientation, invRotation, result.Orientation);
    result.Orientation.Normalize();
    Vector3::Divide(Scale, other.Scale, result.Scale);
    return result;
}

Transform Transform::LocalToWorld(const Transform& other) const
{
    Transform result;
    LocalToWorld(other, result);
    return result;
}

void Transform::LocalToWorld(const Transform& other, Transform& result) const
{
    //Quaternion::Multiply(Orientation, other.Orientation, result.Orientation);
    const float a = Orientation.Y * other.Orientation.Z - Orientation.Z * other.Orientation.Y;
    const float b = Orientation.Z * other.Orientation.X - Orientation.X * other.Orientation.Z;
    const float c = Orientation.X * other.Orientation.Y - Orientation.Y * other.Orientation.X;
    const float d = Orientation.X * other.Orientation.X + Orientation.Y * other.Orientation.Y + Orientation.Z * other.Orientation.Z;
    result.Orientation.X = Orientation.X * other.Orientation.W + other.Orientation.X * Orientation.W + a;
    result.Orientation.Y = Orientation.Y * other.Orientation.W + other.Orientation.Y * Orientation.W + b;
    result.Orientation.Z = Orientation.Z * other.Orientation.W + other.Orientation.Z * Orientation.W + c;
    result.Orientation.W = Orientation.W * other.Orientation.W - d;

    //result.Orientation.Normalize();
    const float length = result.Orientation.Length();
    if (length > ZeroTolerance)
    {
        const float inv = 1.0f / length;
        result.Orientation.X *= inv;
        result.Orientation.Y *= inv;
        result.Orientation.Z *= inv;
        result.Orientation.W *= inv;
    }

    //Vector3::Multiply(Scale, other.Scale, result.Scale);
    result.Scale = Vector3(Scale.X * other.Scale.X, Scale.Y * other.Scale.Y, Scale.Z * other.Scale.Z);

    //Vector3 tmp; Vector3::Multiply(other.Translation, Scale, tmp);
    Vector3 tmp = Vector3(other.Translation.X * Scale.X, other.Translation.Y * Scale.Y, other.Translation.Z * Scale.Z);

    //Vector3::Transform(tmp, Orientation, tmp);
    const float x = Orientation.X + Orientation.X;
    const float y = Orientation.Y + Orientation.Y;
    const float z = Orientation.Z + Orientation.Z;
    const float wx = Orientation.W * x;
    const float wy = Orientation.W * y;
    const float wz = Orientation.W * z;
    const float xx = Orientation.X * x;
    const float xy = Orientation.X * y;
    const float xz = Orientation.X * z;
    const float yy = Orientation.Y * y;
    const float yz = Orientation.Y * z;
    const float zz = Orientation.Z * z;
    tmp = Vector3(
        tmp.X * (1.0f - yy - zz) + tmp.Y * (xy - wz) + tmp.Z * (xz + wy),
        tmp.X * (xy + wz) + tmp.Y * (1.0f - xx - zz) + tmp.Z * (yz - wx),
        tmp.X * (xz - wy) + tmp.Y * (yz + wx) + tmp.Z * (1.0f - xx - yy));

    //Vector3::Add(tmp, Translation, result.Translation);
    result.Translation = Vector3(tmp.X + Translation.X, tmp.Y + Translation.Y, tmp.Z + Translation.Z);
}

Vector3 Transform::LocalToWorld(const Vector3& point) const
{
    Vector3 result = point * Scale;
    Vector3::Transform(result, Orientation, result);
    return result + Translation;
}

Vector3 Transform::LocalToWorldVector(const Vector3& vector) const
{
    Vector3 result = vector * Scale;
    Vector3::Transform(result, Orientation, result);
    return result;
}

void Transform::LocalToWorld(const Vector3& point, Vector3& result) const
{
    Vector3 tmp = point * Scale;
    Vector3::Transform(tmp, Orientation, tmp);
    Vector3::Add(tmp, Translation, result);
}

void Transform::LocalToWorld(const Vector3* points, int32 pointsCount, Vector3* result) const
{
    for (int32 i = 0; i < pointsCount; i++)
    {
        result[i] = Vector3::Transform(points[i] * Scale, Orientation) + Translation;
    }
}

Transform Transform::WorldToLocal(const Transform& other) const
{
    Transform result;

    Vector3 invScale = Scale;
    if (invScale.X != 0.0f)
        invScale.X = 1.0f / invScale.X;
    if (invScale.Y != 0.0f)
        invScale.Y = 1.0f / invScale.Y;
    if (invScale.Z != 0.0f)
        invScale.Z = 1.0f / invScale.Z;

    const Quaternion invRotation = Orientation.Conjugated();

    Quaternion::Multiply(invRotation, other.Orientation, result.Orientation);
    result.Orientation.Normalize();
    Vector3::Multiply(other.Scale, invScale, result.Scale);
    const Vector3 tmp = other.Translation - Translation;
    Vector3::Transform(tmp, invRotation, result.Translation);
    Vector3::Multiply(result.Translation, invScale, result.Translation);

    return result;
}

void Transform::WorldToLocal(const Transform& other, Transform& result) const
{
    Vector3 invScale = Scale;
    if (invScale.X != 0.0f)
        invScale.X = 1.0f / invScale.X;
    if (invScale.Y != 0.0f)
        invScale.Y = 1.0f / invScale.Y;
    if (invScale.Z != 0.0f)
        invScale.Z = 1.0f / invScale.Z;

    const Quaternion invRotation = Orientation.Conjugated();

    Quaternion::Multiply(invRotation, other.Orientation, result.Orientation);
    result.Orientation.Normalize();
    Vector3::Multiply(other.Scale, invScale, result.Scale);
    const Vector3 tmp = other.Translation - Translation;
    Vector3::Transform(tmp, invRotation, result.Translation);
    Vector3::Multiply(result.Translation, invScale, result.Translation);
}

Vector3 Transform::WorldToLocal(const Vector3& point) const
{
    Vector3 invScale = Scale;
    if (invScale.X != 0.0f)
        invScale.X = 1.0f / invScale.X;
    if (invScale.Y != 0.0f)
        invScale.Y = 1.0f / invScale.Y;
    if (invScale.Z != 0.0f)
        invScale.Z = 1.0f / invScale.Z;

    const Quaternion invRotation = Orientation.Conjugated();

    Vector3 result = point - Translation;
    Vector3::Transform(result, invRotation, result);

    return result * invScale;
}

Vector3 Transform::WorldToLocalVector(const Vector3& vector) const
{
    Vector3 invScale = Scale;
    if (invScale.X != 0.0f)
        invScale.X = 1.0f / invScale.X;
    if (invScale.Y != 0.0f)
        invScale.Y = 1.0f / invScale.Y;
    if (invScale.Z != 0.0f)
        invScale.Z = 1.0f / invScale.Z;

    const Quaternion invRotation = Orientation.Conjugated();

    Vector3 result;
    Vector3::Transform(vector, invRotation, result);

    return result * invScale;
}

void Transform::WorldToLocal(const Vector3* points, int32 pointsCount, Vector3* result) const
{
    Vector3 invScale = Scale;
    if (invScale.X != 0.0f)
        invScale.X = 1.0f / invScale.X;
    if (invScale.Y != 0.0f)
        invScale.Y = 1.0f / invScale.Y;
    if (invScale.Z != 0.0f)
        invScale.Z = 1.0f / invScale.Z;

    const Quaternion invRotation = Orientation.Conjugated();

    for (int32 i = 0; i < pointsCount; i++)
    {
        result[i] = points[i] - Translation;
        Vector3::Transform(result[i], invRotation, result[i]);
        result[i] *= invScale;
    }
}

Transform Transform::Lerp(const Transform& t1, const Transform& t2, float amount)
{
    Transform result;
    Vector3::Lerp(t1.Translation, t2.Translation, amount, result.Translation);
    Quaternion::Slerp(t1.Orientation, t2.Orientation, amount, result.Orientation);
    Vector3::Lerp(t1.Scale, t2.Scale, amount, result.Scale);
    return result;
}

void Transform::Lerp(const Transform& t1, const Transform& t2, float amount, Transform& result)
{
    Vector3::Lerp(t1.Translation, t2.Translation, amount, result.Translation);
    Quaternion::Slerp(t1.Orientation, t2.Orientation, amount, result.Orientation);
    Vector3::Lerp(t1.Scale, t2.Scale, amount, result.Scale);
}
