// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/Platform.h"
#include "Vector3.h"
#include "Quaternion.h"

struct Matrix;

/// <summary>
/// Describes transformation in a 3D space.
/// </summary>
API_STRUCT() struct FLAXENGINE_API Transform
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(Transform);

    /// <summary>
    /// The translation vector of the transform.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10), EditorDisplay(null, \"Position\"), ValueCategory(Utils.ValueCategory.Distance)")
    Vector3 Translation;

    /// <summary>
    /// The rotation of the transform.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(20), EditorDisplay(null, \"Rotation\"), ValueCategory(Utils.ValueCategory.Angle)")
    Quaternion Orientation;

    /// <summary>
    /// The scale vector of the transform.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(30), Limit(float.MinValue, float.MaxValue, 0.01f)")
    Float3 Scale;

public:
    /// <summary>
    /// An identity transform.
    /// </summary>
    static Transform Identity;

public:
    /// <summary>
    /// Empty constructor.
    /// </summary>
    Transform() = default;

    /// <summary>
    /// Initializes a new instance of the <see cref="Transform"/> struct.
    /// </summary>
    /// <param name="position">3D position</param>
    Transform(const Vector3& position)
        : Translation(position)
        , Orientation(0.0f, 0.0f, 0.0f, 1.0f)
        , Scale(1.0f)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Transform"/> struct.
    /// </summary>
    /// <param name="position">3D position</param>
    /// <param name="rotation">Transform rotation</param>
    Transform(const Vector3& position, const Quaternion& rotation)
        : Translation(position)
        , Orientation(rotation)
        , Scale(1.0f)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Transform"/> struct.
    /// </summary>
    /// <param name="position">3D position</param>
    /// <param name="rotation">Transform rotation</param>
    /// <param name="scale">Transform scale</param>
    Transform(const Vector3& position, const Quaternion& rotation, const Float3& scale)
        : Translation(position)
        , Orientation(rotation)
        , Scale(scale)
    {
    }

    Transform(const Vector3& position, const Matrix3x3& rotationScale);

public:
    String ToString() const;

public:
    /// <summary>
    /// Checks if transform is an identity transformation.
    /// </summary>
    bool IsIdentity() const;

    /// <summary>
    /// Returns true if transform has one or more components equal to +/- infinity or NaN.
    /// </summary>
    bool IsNanOrInfinity() const;

    /// <summary>
    /// Calculates the determinant of this transformation.
    /// </summary>
    FORCE_INLINE float GetDeterminant() const
    {
        return Scale.X * Scale.Y * Scale.Z;
    }

public:
    /// <summary>
    /// Gets rotation matrix (from Orientation).
    /// </summary>
    /// <returns>Rotation matrix</returns>
    Matrix GetRotation() const;

    /// <summary>
    /// Gets rotation matrix (from Orientation).
    /// </summary>
    /// <param name="result">Matrix to set</param>
    void GetRotation(Matrix& result) const;

    /// <summary>
    /// Sets rotation matrix (from Orientation).
    /// </summary>
    /// <param name="value">Rotation matrix</param>
    void SetRotation(const Matrix& value);

    /// <summary>
    /// Gets world matrix that describes transformation as a 4 by 4 matrix.
    /// </summary>
    /// <returns>World matrix</returns>
    Matrix GetWorld() const;

    /// <summary>
    /// Gets world matrix that describes transformation as a 4 by 4 matrix.
    /// </summary>
    /// <param name="result">World matrix</param>
    void GetWorld(Matrix& result) const;

    /// <summary>
    /// Gets world matrix that describes transformation as a 4 by 4 matrix.
    /// </summary>
    /// <param name="result">World matrix</param>
    void GetWorld(Double4x4& result) const;

public:
    /// <summary>
    /// Adds translation to this transform.
    /// </summary>
    /// <param name="translation">The translation.</param>
    /// <returns>The result.</returns>
    Transform Add(const Vector3& translation) const;

    /// <summary>
    /// Adds transformation to this transform.
    /// </summary>
    /// <param name="other">The other transformation.</param>
    /// <returns>The sum of two transformations.</returns>
    Transform Add(const Transform& other) const;

    /// <summary>
    /// Subtracts transformation from this transform.
    /// </summary>
    /// <param name="other">The other transformation.</param>
    /// <returns>The different of two transformations.</returns>
    Transform Subtract(const Transform& other) const;

    /// <summary>
    /// Performs transformation of the given transform in local space to the world space of this transform.
    /// </summary>
    /// <param name="other">The local space transformation.</param>
    /// <returns>The world space transformation.</returns>
    Transform LocalToWorld(const Transform& other) const
    {
        Transform result;
        LocalToWorld(other, result);
        return result;
    }

    /// <summary>
    /// Performs transformation of the given transform in local space to the world space of this transform.
    /// </summary>
    /// <param name="other">The local space transformation.</param>
    /// <param name="result">The world space transformation.</param>
    void LocalToWorld(const Transform& other, Transform& result) const;

    /// <summary>
    /// Performs transformation of the given point in local space to the world space of this transform.
    /// </summary>
    /// <param name="point">The local space point.</param>
    /// <returns>The world space point.</returns>
    Vector3 LocalToWorld(const Vector3& point) const
    {
        Vector3 result;
        LocalToWorld(point, result);
        return result;
    }

    /// <summary>
    /// Performs transformation of the given vector in local space to the world space of this transform.
    /// </summary>
    /// <param name="vector">The local space vector.</param>
    /// <param name="result">The world space vector.</param>
    void LocalToWorldVector(const Vector3& vector, Vector3& result) const;

    /// <summary>
    /// Performs transformation of the given vector in local space to the world space of this transform.
    /// </summary>
    /// <param name="vector">The local space vector.</param>
    /// <returns>The world space vector.</returns>
    Vector3 LocalToWorldVector(const Vector3& vector) const
    {
        Vector3 result;
        LocalToWorldVector(vector, result);
        return result;
    }

    /// <summary>
    /// Performs transformation of the given point in local space to the world space of this transform.
    /// </summary>
    /// <param name="point">The local space point.</param>
    /// <param name="result">The world space point.</param>
    void LocalToWorld(const Vector3& point, Vector3& result) const;

    /// <summary>
    /// Performs transformation of the given transform in local space to the world space of this transform.
    /// </summary>
    /// <param name="other">The world space transformation.</param>
    /// <returns>The local space transformation.</returns>
    Transform WorldToLocal(const Transform& other) const
    {
        Transform result;
        WorldToLocal(other, result);
        return result;
    }

    /// <summary>
    /// Performs transformation of the given transform in world space to the local space of this transform.
    /// </summary>
    /// <param name="other">The world space transformation.</param>
    /// <param name="result">The local space transformation.</param>
    void WorldToLocal(const Transform& other, Transform& result) const;

    /// <summary>
    /// Performs transformation of the given point in world space to the local space of this transform.
    /// </summary>
    /// <param name="point">The world space point.</param>
    /// <param name="result">The local space point.</param>
    void WorldToLocal(const Vector3& point, Vector3& result) const;

    /// <summary>
    /// Performs transformation of the given point in world space to the local space of this transform.
    /// </summary>
    /// <param name="point">The world space point.</param>
    /// <returns>The local space point.</returns>
    Vector3 WorldToLocal(const Vector3& point) const
    {
        Vector3 result;
        WorldToLocal(point, result);
        return result;
    }

    /// <summary>
    /// Performs transformation of the given vector in world space to the local space of this transform.
    /// </summary>
    /// <param name="vector">The world space vector.</param>
    /// <param name="result">The local space vector.</param>
    void WorldToLocalVector(const Vector3& vector, Vector3& result) const;

    /// <summary>
    /// Performs transformation of the given vector in world space to the local space of this transform.
    /// </summary>
    /// <param name="vector">The world space vector.</param>
    /// <returns>The local space vector.</returns>
    Vector3 WorldToLocalVector(const Vector3& vector) const
    {
        Vector3 result;
        WorldToLocalVector(vector, result);
        return result;
    }

    /// <summary>
    /// Performs transformation of the given rotation in world space to the local space of this transform.
    /// </summary>
    /// <param name="rotation">The world space rotation.</param>
    /// <param name="result">The local space rotation.</param>
    void WorldToLocal(const Quaternion& rotation, Quaternion& result) const;

    /// <summary>
    /// Performs transformation of the given rotation in world space to the local space of this transform.
    /// </summary>
    /// <param name="rotation">The world space rotation.</param>
    /// <returns>The local space rotation.</returns>
    Quaternion WorldToLocal(const Quaternion& rotation) const
    {
        Quaternion result;
        WorldToLocal(rotation, result);
        return result;
    }

    /// <summary>
    /// Combines the functions: <br/>
    /// <see cref="SnapToGrid"/>,<br/>
    /// <see cref="GetRotationFromNormal"/>.
    /// </summary>
    /// <param name="point">The position to snap.</param>
    /// <param name="gridSize">The size of the grid.</param>
    /// <param name="normalOffset">The local grid offset to apply after snapping.</param>
    /// <param name="normal">The normal vector.</param>
    /// <param name="relativeTo">The relative transform.</param>
    /// <param name="scale">The scale to apply to the transform.</param>
    /// <returns>The rotated and snapped transform.</returns>
    static Transform AlignRotationToNormalAndSnapToGrid(const Vector3& point, const Vector3& normal, const Vector3& normalOffset, const Transform& relativeTo, const Vector3& gridSize, const Float3& scale = Float3::One);

public:
    FORCE_INLINE Transform operator*(const Transform& other) const
    {
        return LocalToWorld(other);
    }

    FORCE_INLINE Transform operator+(const Transform& other) const
    {
        return Add(other);
    }

    FORCE_INLINE Transform operator-(const Transform& other) const
    {
        return Subtract(other);
    }

    FORCE_INLINE Transform operator+(const Vector3& other) const
    {
        return Add(other);
    }

    FORCE_INLINE bool operator==(const Transform& other) const
    {
        return Translation == other.Translation && Orientation == other.Orientation && Scale == other.Scale;
    }

    FORCE_INLINE bool operator!=(const Transform& other) const
    {
        return Translation != other.Translation || Orientation != other.Orientation || Scale != other.Scale;
    }

    static bool NearEqual(const Transform& a, const Transform& b)
    {
        return Vector3::NearEqual(a.Translation, b.Translation) && Quaternion::NearEqual(a.Orientation, b.Orientation) && Float3::NearEqual(a.Scale, b.Scale);
    }

    static bool NearEqual(const Transform& a, const Transform& b, float epsilon)
    {
        return Vector3::NearEqual(a.Translation, b.Translation, epsilon) && Quaternion::NearEqual(a.Orientation, b.Orientation, epsilon) && Float3::NearEqual(a.Scale, b.Scale, epsilon);
    }

public:
    Float3 GetRight() const;
    Float3 GetLeft() const;
    Float3 GetUp() const;
    Float3 GetDown() const;
    Float3 GetForward() const;
    Float3 GetBackward() const;

public:
    static Transform Lerp(const Transform& t1, const Transform& t2, float amount);
    static void Lerp(const Transform& t1, const Transform& t2, float amount, Transform& result);
};

namespace Math
{
    FORCE_INLINE static bool NearEqual(const Transform& a, const Transform& b)
    {
        return Transform::NearEqual(a, b);
    }
}

template<>
struct TIsPODType<Transform>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(Transform, "Translation:{0} Orientation:{1} Scale:{2}", v.Translation, v.Orientation, v.Scale);
