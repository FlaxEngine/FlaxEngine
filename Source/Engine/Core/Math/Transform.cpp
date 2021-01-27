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
    Quaternion::Multiply(Orientation, other.Orientation, result.Orientation);
    result.Orientation.Normalize();
    Vector3::Multiply(Scale, other.Scale, result.Scale);
    Vector3 tmp = other.Translation * Scale;
    Vector3::Transform(tmp, Orientation, tmp);
    Vector3::Add(tmp, Translation, result.Translation);
    return result;
}

void Transform::LocalToWorld(const Transform& other, Transform& result) const
{
    Quaternion::Multiply(Orientation, other.Orientation, result.Orientation);
    result.Orientation.Normalize();
    Vector3::Multiply(Scale, other.Scale, result.Scale);
    Vector3 tmp = other.Translation * Scale;
    Vector3::Transform(tmp, Orientation, tmp);
    Vector3::Add(tmp, Translation, result.Translation);
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
