// Copyright (c) Wojciech Figat. All rights reserved.
// Copyright (c) 2010-2014 SharpDX - Alexandre Mutel

#pragma once

#include "Transform.h"

enum class ContainmentType;

/// <summary>
/// Oriented Bounding Box (OBB) is a rectangular block, much like an AABB (Bounding Box) but with an arbitrary orientation in 3D space.
/// </summary>
API_STRUCT() struct FLAXENGINE_API OrientedBoundingBox
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(OrientedBoundingBox);

    /// <summary>
    /// Half lengths of the box along each axis.
    /// </summary>
    API_FIELD() Vector3 Extents;

    /// <summary>
    /// The transformation which aligns and scales the box, and its translation vector represents the center of the box.
    /// </summary>
    API_FIELD() Transform Transformation;

public:
    /// <summary>
    /// Empty constructor.
    /// </summary>
    OrientedBoundingBox()
    {
    }

    OrientedBoundingBox(const Vector3& extents, const Transform& transformation)
    {
        Extents = extents;
        Transformation = transformation;
    }

    OrientedBoundingBox(const BoundingBox& bb);
    OrientedBoundingBox(const Vector3& extents, const Matrix& transformation);
    OrientedBoundingBox(const Vector3& extents, const Matrix3x3& rotationScale, const Vector3& translation);
    OrientedBoundingBox(const Vector3& minimum, const Vector3& maximum);
    OrientedBoundingBox(Vector3 points[], int32 pointCount);

public:
    String ToString() const;

public:
    // Gets the eight corners of the bounding box.
    void GetCorners(Float3 corners[8]) const;
    void GetCorners(Double3 corners[8]) const;

    // The size of the OBB if no scaling is applied to the transformation matrix.
    Vector3 GetSizeUnscaled() const
    {
        return Extents * 2.0f;
    }

    // Returns the size of the OBB taking into consideration the scaling applied to the transformation matrix.
    Vector3 GetSize() const;

    // Returns the square size of the OBB taking into consideration the scaling applied to the transformation matrix.
    Vector3 GetSizeSquared() const;

    // Gets the center of the OBB.
    FORCE_INLINE Vector3 GetCenter() const
    {
        return Transformation.Translation;
    }

    /// <summary>
    /// Gets the AABB which contains all OBB corners.
    /// </summary>
    /// <returns>The result</returns>
    BoundingBox GetBoundingBox() const;

    /// <summary>
    /// Gets the AABB which contains all OBB corners.
    /// </summary>
    /// <param name="result">The result.</param>
    void GetBoundingBox(BoundingBox& result) const;

public:
    // Transforms this box using a transformation matrix.
    void Transform(const Matrix& matrix);
    void Transform(const ::Transform& transform);

    // Scales the OBB by scaling its Extents without affecting the Transformation matrix.
    // By keeping Transformation matrix scaling-free, the collision detection methods will be more accurate.
    void Scale(const Vector3& scaling)
    {
        Extents *= scaling;
    }

    // Scales the OBB by scaling its Extents without affecting the Transformation matrix.
    // By keeping Transformation matrix scaling-free, the collision detection methods will be more accurate.
    void Scale(Real scaling)
    {
        Extents *= scaling;
    }

    // Translates the OBB to a new position using a translation vector.
    void Translate(const Vector3& translation)
    {
        Transformation.Translation += translation;
    }

public:
    FORCE_INLINE bool operator==(const OrientedBoundingBox& other) const
    {
        return Extents == other.Extents && Transformation == other.Transformation;
    }

    FORCE_INLINE bool operator!=(const OrientedBoundingBox& other) const
    {
        return Extents != other.Extents || Transformation != other.Transformation;
    }

    FORCE_INLINE OrientedBoundingBox operator*(const Matrix& matrix) const
    {
        OrientedBoundingBox result = *this;
        result.Transform(matrix);
        return result;
    }

    FORCE_INLINE OrientedBoundingBox operator*(const ::Transform& matrix) const
    {
        OrientedBoundingBox result = *this;
        result.Transform(matrix);
        return result;
    }

public:
    /// <summary>
    /// Creates the centered box (axis aligned).
    /// </summary>
    /// <param name="center">The center.</param>
    /// <param name="size">The size.</param>
    /// <param name="result">The result.</param>
    static void CreateCentered(const Vector3& center, const Vector3& size, OrientedBoundingBox& result)
    {
        result.Extents = size * 0.5f;
        result.Transformation = ::Transform(center);
    }

    /// <summary>
    /// Creates the centered box (axis-aligned).
    /// </summary>
    /// <param name="center">The center.</param>
    /// <param name="size">The size.</param>
    /// <returns>The result.</returns>
    static OrientedBoundingBox CreateCentered(const Vector3& center, const Vector3& size)
    {
        OrientedBoundingBox result;
        CreateCentered(center, size, result);
        return result;
    }

public:
    // Determines whether a OBB contains a point.
    ContainmentType Contains(const Vector3& point, Real* distance = nullptr) const;

    /// <summary>
    /// Determines whether a OBB contains a sphere.
    /// </summary>
    /// <param name="sphere">The sphere to test.</param>
    /// <param name="ignoreScale">Optimize the check operation by assuming that OBB has no scaling applied.</param>
    /// <returns>The type of containment the two objects have.</returns>
    ContainmentType Contains(const BoundingSphere& sphere, bool ignoreScale = false) const;

    // Determines whether there is an intersection between oriented box and a ray. Returns point of the intersection.
    bool Intersects(const Ray& ray, Vector3& point) const;

    // Determines if there is an intersection between oriented box and a ray. Returns distance to the intersection.
    bool Intersects(const Ray& ray, Real& distance) const;

    // Determines if there is an intersection between oriented box and a ray. Returns distance to the intersection and surface normal.
    bool Intersects(const Ray& ray, Real& distance, Vector3& normal) const;

    // Determines whether there is an intersection between a Ray and a OBB.
    bool Intersects(const Ray& ray) const
    {
        Vector3 point;
        return Intersects(ray, point);
    }
};

template<>
struct TIsPODType<OrientedBoundingBox>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(OrientedBoundingBox, "Center: {0}, Size: {1}", v.GetCenter(), v.GetSize());
