// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/Platform.h"
#include "Vector3.h"
#include "CollisionsHelper.h"

/// <summary>
/// Represents an axis-aligned bounding box in three dimensional space.
/// </summary>
API_STRUCT() struct FLAXENGINE_API BoundingBox
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(BoundingBox);
public:
    /// <summary>
    /// A <see cref="BoundingBox"/> which represents an empty space.
    /// </summary>
    static const BoundingBox Empty;

    /// <summary>
    /// A <see cref="BoundingBox"/> located at zero point with zero size.
    /// </summary>
    static const BoundingBox Zero;

public:
    /// <summary>
    /// The minimum point of the box.
    /// </summary>
    API_FIELD() Vector3 Minimum;

    /// <summary>
    /// The maximum point of the box.
    /// </summary>
    API_FIELD() Vector3 Maximum;

public:
    /// <summary>
    /// Empty constructor.
    /// </summary>
    BoundingBox() = default;

    /// <summary>
    /// Initializes a new instance of the <see cref="BoundingBox"/> struct.
    /// </summary>
    /// <param name="point">The location of the empty bounding box.</param>
    BoundingBox(const Vector3& point)
        : Minimum(point)
        , Maximum(point)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="BoundingBox"/> struct.
    /// </summary>
    /// <param name="minimum">The minimum vertex of the bounding box.</param>
    /// <param name="maximum">The maximum vertex of the bounding box.</param>
    BoundingBox(const Vector3& minimum, const Vector3& maximum)
        : Minimum(minimum)
        , Maximum(maximum)
    {
    }

public:
    String ToString() const;

public:
    /// <summary>
    /// Gets the eight corners of the bounding box.
    /// </summary>
    /// <param name="corners">An array of points representing the eight corners of the bounding box.</param>
    void GetCorners(Float3 corners[8]) const;

    /// <summary>
    /// Gets the eight corners of the bounding box.
    /// </summary>
    /// <param name="corners">An array of points representing the eight corners of the bounding box.</param>
    void GetCorners(Double3 corners[8]) const;

    /// <summary>
    /// Calculates volume of the box.
    /// </summary>
    /// <returns>The box volume.</returns>
    Real GetVolume() const
    {
        Vector3 size;
        Vector3::Subtract(Maximum, Minimum, size);
        return size.X * size.Y * size.Z;
    }

    /// <summary>
    /// Calculates the size of the box.
    /// </summary>
    /// <returns>The box size.</returns>
    Vector3 GetSize() const
    {
        Vector3 size;
        Vector3::Subtract(Maximum, Minimum, size);
        return size;
    }

    /// <summary>
    /// Calculates the size of the box.
    /// </summary>
    /// <param name="result">The result box size.</param>
    void GetSize(Vector3& result) const
    {
        Vector3::Subtract(Maximum, Minimum, result);
    }

    /// <summary>
    /// Sets the size of the box.
    /// </summary>
    /// <param name="value">The box size to set.</param>
    void SetSize(const Vector3& value)
    {
        Vector3 center;
        GetCenter(center);
        const Vector3 sizeHalf = value * 0.5f;
        Minimum = center - sizeHalf;
        Maximum = center + sizeHalf;
    }

    /// <summary>
    /// Gets the center point location.
    /// </summary>
    /// <returns>The box center.</returns>
    Vector3 GetCenter() const
    {
        return Minimum + (Maximum - Minimum) * 0.5f;
    }

    /// <summary>
    /// Gets the center point location.
    /// </summary>
    /// <param name="result">The result box center.</param>
    void GetCenter(Vector3& result) const
    {
        result = Minimum + (Maximum - Minimum) * 0.5f;
    }

    /// <summary>
    /// Sets the center point location.
    /// </summary>
    /// <param name="value">The box center to set.</param>
    void SetCenter(const Vector3& value)
    {
        const Vector3 sizeHalf = GetSize() * 0.5f;
        Minimum = value - sizeHalf;
        Maximum = value + sizeHalf;
    }

public:
    static bool NearEqual(const BoundingBox& a, const BoundingBox& b)
    {
        return Vector3::NearEqual(a.Minimum, b.Minimum) && Vector3::NearEqual(a.Maximum, b.Maximum);
    }

    static bool NearEqual(const BoundingBox& a, const BoundingBox& b, Real epsilon)
    {
        return Vector3::NearEqual(a.Minimum, b.Minimum, epsilon) && Vector3::NearEqual(a.Maximum, b.Maximum, epsilon);
    }

public:
    /// <summary>
    /// Merges the box with a point.
    /// </summary>
    /// <param name="point">The point to add to the box bounds.</param>
    void Merge(const Vector3& point)
    {
        Vector3::Min(Minimum, point, Minimum);
        Vector3::Max(Maximum, point, Maximum);
    }

    /// <summary>
    /// Merges the box with the other box.
    /// </summary>
    /// <param name="box">The other box to add to the box bounds.</param>
    void Merge(const BoundingBox& box)
    {
        Vector3::Min(Minimum, box.Minimum, Minimum);
        Vector3::Max(Maximum, box.Maximum, Maximum);
    }

    /// <summary>
    /// Creates the bounding box that is offseted by the given vector. Adds the offset value to minimum and maximum points.
    /// </summary>
    /// <param name="offset">The offset.</param>
    /// <returns>The result.</returns>
    BoundingBox MakeOffsetted(const Vector3& offset) const;

public:
    FORCE_INLINE bool operator==(const BoundingBox& other) const
    {
        return Minimum == other.Minimum && Maximum == other.Maximum;
    }

    FORCE_INLINE bool operator!=(const BoundingBox& other) const
    {
        return Minimum != other.Minimum || Maximum != other.Maximum;
    }

    FORCE_INLINE BoundingBox operator*(const Matrix& matrix) const
    {
        BoundingBox result;
        Transform(*this, matrix, result);
        return result;
    }

public:
    /// <summary>
    /// Constructs a Bounding Box that fully contains the given pair of points.
    /// </summary>
    /// <param name="pointA">The first point that will be contained by the box.</param>
    /// <param name="pointB">The second point that will be contained by the box.</param>
    /// <param name="result">The constructed bounding box.</param>
    static void FromPoints(const Vector3& pointA, const Vector3& pointB, BoundingBox& result)
    {
        Vector3::Min(pointA, pointB, result.Minimum);
        Vector3::Max(pointA, pointB, result.Maximum);
    }

    /// <summary>
    /// Constructs a Bounding Box that fully contains the given points.
    /// </summary>
    /// <param name="points">The points that will be contained by the box.</param>
    /// <param name="pointsCount">The amount of points to use.</param>
    /// <param name="result">The constructed bounding box.</param>
    static void FromPoints(const Float3* points, int32 pointsCount, BoundingBox& result);

    /// <summary>
    /// Constructs a Bounding Box that fully contains the given points.
    /// </summary>
    /// <param name="points">The points that will be contained by the box.</param>
    /// <param name="pointsCount">The amount of points to use.</param>
    /// <param name="result">The constructed bounding box.</param>
    static void FromPoints(const Double3* points, int32 pointsCount, BoundingBox& result);

    /// <summary>
    /// Constructs a Bounding Box from a given sphere.
    /// </summary>
    /// <param name="sphere">The sphere that will designate the extents of the box.</param>
    /// <param name="result">The constructed bounding box.</param>
    static void FromSphere(const BoundingSphere& sphere, BoundingBox& result);

    /// <summary>
    /// Constructs a Bounding Box from a given sphere.
    /// </summary>
    /// <param name="sphere">The sphere that will designate the extents of the box.</param>
    /// <returns>The constructed bounding box.</returns>
    static BoundingBox FromSphere(const BoundingSphere& sphere);

    /// <summary>
    /// Constructs a Bounding Box that is as large as the total combined area of the two specified boxes.
    /// </summary>
    /// <param name="value1">The first box to merge.</param>
    /// <param name="value2">The second box to merge.</param>
    /// <param name="result">The constructed bounding box.</param>
    static void Merge(const BoundingBox& value1, const BoundingBox& value2, BoundingBox& result)
    {
        Vector3::Min(value1.Minimum, value2.Minimum, result.Minimum);
        Vector3::Max(value1.Maximum, value2.Maximum, result.Maximum);
    }

    /// <summary>
    /// Transforms the bounding box using the specified matrix.
    /// </summary>
    /// <param name="box">The box.</param>
    /// <param name="matrix">The matrix.</param>
    /// <returns>The result transformed box.</returns>
    static BoundingBox Transform(const BoundingBox& box, const Matrix& matrix);

    /// <summary>
    /// Creates the bounding box that is offseted by the given vector. Adds the offset value to minimum and maximum points.
    /// </summary>
    /// <param name="box">The box.</param>
    /// <param name="offset">The bounds offset.</param>
    /// <returns>The offsetted bounds.</returns>
    static BoundingBox MakeOffsetted(const BoundingBox& box, const Vector3& offset);

    /// <summary>
    /// Creates the bounding box that is scaled by the given factor. Applies scale to the size of the bounds.
    /// </summary>
    /// <param name="box">The box.</param>
    /// <param name="scale">The bounds scale.</param>
    /// <returns>The scaled bounds.</returns>
    static BoundingBox MakeScaled(const BoundingBox& box, Real scale);

    /// <summary>
    /// Transforms the bounding box using the specified matrix.
    /// </summary>
    /// <param name="box">The box.</param>
    /// <param name="matrix">The matrix.</param>
    /// <param name="result">The result transformed box.</param>
    static void Transform(const BoundingBox& box, const Matrix& matrix, BoundingBox& result);

    /// <summary>
    /// Transforms the bounding box using the specified transformation.
    /// </summary>
    /// <param name="box">The box.</param>
    /// <param name="transform">The transformation.</param>
    /// <param name="result">The result transformed box.</param>
    static void Transform(const BoundingBox& box, const ::Transform& transform, BoundingBox& result);

public:
    /// <summary>
    /// Determines if there is an intersection between box and a ray.
    /// </summary>
    /// <param name="ray">The ray to test.</param>
    /// <returns>Whether the two objects intersected.</returns>
    FORCE_INLINE bool Intersects(const Ray& ray) const
    {
        Real distance;
        return CollisionsHelper::RayIntersectsBox(ray, *this, distance);
    }

    /// <summary>
    /// Determines if there is an intersection between box and a ray.
    /// </summary>
    /// <param name="ray">The ray to test.</param>
    /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.</param>
    /// <returns> Whether the two objects intersected.</returns>
    FORCE_INLINE bool Intersects(const Ray& ray, Real& distance) const
    {
        return CollisionsHelper::RayIntersectsBox(ray, *this, distance);
    }

    /// <summary>
    /// Determines if there is an intersection between box and a ray.
    /// </summary>
    /// <param name="ray">The ray to test.</param>
    /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.</param>
    /// <param name="normal">When the method completes, contains the intersection surface normal vector, or Vector3::Up if there was no intersection.</param>
    /// <returns>Whether the two objects intersected.</returns>
    FORCE_INLINE bool Intersects(const Ray& ray, Real& distance, Vector3& normal) const
    {
        return CollisionsHelper::RayIntersectsBox(ray, *this, distance, normal);
    }

    /// <summary>
    /// Determines if there is an intersection between box and a ray.
    /// </summary>
    /// <param name="ray">The ray to test.</param>
    /// <param name="point">When the method completes, contains the point of intersection, or <see cref="Vector3.Zero"/> if there was no intersection.</param>
    /// <returns>Whether the two objects intersected.</returns>
    FORCE_INLINE bool Intersects(const Ray& ray, Vector3& point) const
    {
        return CollisionsHelper::RayIntersectsBox(ray, *this, point);
    }

    /// <summary>
    /// Determines if there is an intersection between box and a plane.
    /// </summary>
    /// <param name="plane">The plane to test.</param>
    /// <returns>Whether the two objects intersected.</returns>
    FORCE_INLINE PlaneIntersectionType Intersects(const Plane& plane) const
    {
        return CollisionsHelper::PlaneIntersectsBox(plane, *this);
    }

    /// <summary>
    /// Determines if there is an intersection between two boxes.
    /// </summary>
    /// <param name="box">The box to test.</param>
    /// <returns>Whether the two objects intersected.</returns>
    FORCE_INLINE bool Intersects(const BoundingBox& box) const
    {
        return CollisionsHelper::BoxIntersectsBox(*this, box);
    }

    /// <summary>
    /// Determines if there is an intersection between box and a sphere.
    /// </summary>
    /// <param name="sphere">The sphere to test.</param>
    /// <returns>Whether the two objects intersected.</returns>
    FORCE_INLINE bool Intersects(const BoundingSphere& sphere) const
    {
        return CollisionsHelper::BoxIntersectsSphere(*this, sphere);
    }

    /// <summary>
    /// Determines whether box contains a point.
    /// </summary>
    /// <param name="point">The point to test.</param>
    /// <returns>The type of containment the two objects have.</returns>
    FORCE_INLINE ContainmentType Contains(const Vector3& point) const
    {
        return CollisionsHelper::BoxContainsPoint(*this, point);
    }

    /// <summary>
    /// Determines whether box contains a Bounding Box.
    /// </summary>
    /// <param name="box">The box to test.</param>
    /// <returns>The type of containment the two objects have.</returns>
    FORCE_INLINE ContainmentType Contains(const BoundingBox& box) const
    {
        return CollisionsHelper::BoxContainsBox(*this, box);
    }

    /// <summary>
    /// Determines whether box contains a Bounding Sphere.
    /// </summary>
    /// <param name="sphere">The sphere to test.</param>
    /// <returns>The type of containment the two objects have.</returns>
    FORCE_INLINE ContainmentType Contains(const BoundingSphere& sphere) const
    {
        return CollisionsHelper::BoxContainsSphere(*this, sphere);
    }

    /// <summary>
    /// Determines the distance between box and a point.
    /// </summary>
    /// <param name="point">The point to test.</param>
    /// <returns>The distance between bounding box and a point.</returns>
    FORCE_INLINE Real Distance(const Vector3& point) const
    {
        return CollisionsHelper::DistanceBoxPoint(*this, point);
    }

    /// <summary>
    /// Determines the distance between two boxes.
    /// </summary>
    /// <param name="box">The bounding box to test.</param>
    /// <returns>The distance between bounding boxes.</returns>
    FORCE_INLINE Real Distance(const BoundingBox& box) const
    {
        return CollisionsHelper::DistanceBoxBox(*this, box);
    }
};

template<>
struct TIsPODType<BoundingBox>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(BoundingBox, "Minimum:{0} Maximum:{1}", v.Minimum, v.Maximum);
