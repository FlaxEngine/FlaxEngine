// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/Platform.h"
#include "Vector3.h"
#include "Math.h"
#include "CollisionsHelper.h"

/// <summary>
/// Represents a bounding sphere in three dimensional space.
/// </summary>
API_STRUCT() struct FLAXENGINE_API BoundingSphere
{
DECLARE_SCRIPTING_TYPE_MINIMAL(BoundingSphere);
public:

    /// <summary>
    /// An empty bounding sphere (Center = 0 and Radius = 0).
    /// </summary>
    static const BoundingSphere Empty;

public:

    /// <summary>
    /// The center of the sphere in three dimensional space.
    /// </summary>
    API_FIELD() Vector3 Center;

    /// <summary>
    /// The radius of the sphere.
    /// </summary>
    API_FIELD() float Radius;

public:

    /// <summary>
    /// Empty constructor.
    /// </summary>
    BoundingSphere()
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="BoundingSphere"/> struct.
    /// </summary>
    /// <param name="center">The center of the sphere in three dimensional space.</param>
    /// <param name="radius">The radius of the sphere.</param>
    BoundingSphere(const Vector3& center, float radius)
        : Center(center)
        , Radius(radius)
    {
    }

public:

    String ToString() const;

public:

    FORCE_INLINE bool operator==(const BoundingSphere& other) const
    {
        return Center == other.Center && Radius == other.Radius;
    }

    FORCE_INLINE bool operator!=(const BoundingSphere& other) const
    {
        return Center != other.Center || Radius != other.Radius;
    }

public:

    static bool NearEqual(const BoundingSphere& a, const BoundingSphere& b)
    {
        return Vector3::NearEqual(a.Center, b.Center) && Math::NearEqual(a.Radius, b.Radius);
    }

    static bool NearEqual(const BoundingSphere& a, const BoundingSphere& b, float epsilon)
    {
        return Vector3::NearEqual(a.Center, b.Center, epsilon) && Math::NearEqual(a.Radius, b.Radius, epsilon);
    }

public:

    /// <summary>
    /// Determines if there is an intersection between the current object and a Ray.
    /// </summary>
    /// <param name="ray">The ray to test.</param>
    /// <returns>Whether the two objects intersected.</returns>
    bool Intersects(const Ray& ray) const;

    /// <summary>
    /// Determines if there is an intersection between the current object and a Ray.
    /// </summary>
    /// <param name="ray">The ray to test.</param>
    /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.</param>
    /// <returns>Whether the two objects intersected.</returns>
    bool Intersects(const Ray& ray, float& distance) const;

    /// <summary>
    /// Determines if there is an intersection between the current object and a Ray.
    /// </summary>
    /// <param name="ray">The ray to test.</param>
    /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.</param>
    /// <param name="normal">When the method completes, contains the intersection surface normal vector, or Vector3::Up if there was no intersection.</param>
    /// <returns>Whether the two objects intersected.</returns>
    bool Intersects(const Ray& ray, float& distance, Vector3& normal) const;

    /// <summary>
    /// Determines if there is an intersection between the current object and a Ray.
    /// </summary>
    /// <param name="ray">The ray to test.</param>
    /// <param name="point">When the method completes, contains the point of intersection, or Vector3::Zero if there was no intersection.</param>
    /// <returns>Whether the two objects intersected.</returns>
    bool Intersects(const Ray& ray, Vector3& point) const;

    /// <summary>
    /// Determines if there is an intersection between the current object and a Plane.
    /// </summary>
    /// <param name="plane">The plane to test.</param>
    /// <returns>Whether the two objects intersected.</returns>
    PlaneIntersectionType Intersects(const Plane& plane) const;

    /// <summary>
    /// Determines if there is an intersection between the current object and a triangle.
    /// </summary>
    /// <param name="vertex1">The first vertex of the triangle to test.</param>
    /// <param name="vertex2">The second vertex of the triangle to test.</param>
    /// <param name="vertex3">The third vertex of the triangle to test.</param>
    /// <returns>Whether the two objects intersected.</returns>
    bool Intersects(const Vector3& vertex1, const Vector3& vertex2, const Vector3& vertex3) const;

    /// <summary>
    /// Determines if there is an intersection between the current object and a Bounding Box.
    /// </summary>
    /// <param name="box">The box to test.</param>
    /// <returns>Whether the two objects intersected.</returns>
    bool Intersects(const BoundingBox& box) const;

    /// <summary>
    /// Determines if there is an intersection between the current object and a Bounding Sphere.
    /// </summary>
    /// <param name="sphere">The sphere to test.</param>
    /// <returns>Whether the two objects intersected.</returns>
    bool Intersects(const BoundingSphere& sphere) const;

    /// <summary>
    /// Determines whether the current objects contains a point.
    /// </summary>
    /// <param name="point">The point to test.</param>
    /// <returns> The type of containment the two objects have.</returns>
    ContainmentType Contains(const Vector3& point) const;

    /// <summary>
    /// Determines whether the current objects contains a triangle.
    /// </summary>
    /// <param name="vertex1">The first vertex of the triangle to test.</param>
    /// <param name="vertex2">The second vertex of the triangle to test.</param>
    /// <param name="vertex3">The third vertex of the triangle to test.</param>
    /// <returns>The type of containment the two objects have.</returns>
    ContainmentType Contains(const Vector3& vertex1, const Vector3& vertex2, const Vector3& vertex3) const;

    /// <summary>
    /// Determines whether the current objects contains a Bounding Box
    /// </summary>
    /// <param name="box">The box to test.</param>
    /// <returns>The type of containment the two objects have.</returns>
    ContainmentType Contains(const BoundingBox& box) const;

    /// <summary>
    /// Determines whether the current objects contains a Bounding Sphere.
    /// </summary>
    /// <param name="sphere">The sphere to test.</param>
    /// <returns>The type of containment the two objects have.</returns>
    ContainmentType Contains(const BoundingSphere& sphere) const;

public:

    /// <summary>
    /// Gets the box which contains whole sphere.
    /// </summary>
    /// <returns>The box.</returns>
    BoundingBox GetBoundingBox() const;

    /// <summary>
    /// Gets the box which contains whole sphere.
    /// </summary>
    /// <param name="result">The result box.</param>
    void GetBoundingBox(BoundingBox& result) const;

    /// <summary>
    /// Constructs a BoundingSphere that fully contains the given points
    /// </summary>
    /// <param name="points">The points that will be contained by the sphere.</param>
    /// <param name="pointsCount">The amount of points to use.</param>
    /// <param name="result">When the method completes, contains the newly constructed bounding sphere.</param>
    static void FromPoints(const Vector3* points, int32 pointsCount, BoundingSphere& result);

    /// <summary>
    /// Constructs a Bounding Sphere from a given box.
    /// </summary>
    /// <param name="box">The box that will designate the extents of the sphere.</param>
    /// <param name="result">When the method completes, the newly constructed bounding sphere.</param>
    static void FromBox(const BoundingBox& box, BoundingSphere& result);

    /// <summary>
    /// Constructs a BoundingSphere that is the as large as the total combined area of the two specified spheres
    /// </summary>
    /// <param name="value1">The first sphere to merge.</param>
    /// <param name="value2">The second sphere to merge.</param>
    /// <param name="result">When the method completes, contains the newly constructed bounding sphere.</param>
    static void Merge(const BoundingSphere& value1, const BoundingSphere& value2, BoundingSphere& result);

    /// <summary>
    /// Constructs a BoundingSphere that is the as large as the total combined area of the specified sphere and the point.
    /// </summary>
    /// <param name="value1">The sphere to merge.</param>
    /// <param name="value2">The point to merge.</param>
    /// <param name="result">When the method completes, contains the newly constructed bounding sphere.</param>
    static void Merge(const BoundingSphere& value1, const Vector3& value2, BoundingSphere& result);

    /// <summary>
    /// Transforms the bounding sphere using the specified matrix.
    /// </summary>
    /// <param name="sphere">The sphere.</param>
    /// <param name="matrix">The matrix.</param>
    /// <param name="result">The result transformed sphere.</param>
    static void Transform(const BoundingSphere& sphere, const Matrix& matrix, BoundingSphere& result);
};

template<>
struct TIsPODType<BoundingSphere>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(BoundingSphere, "Center:{0} Radius:{1}", v.Center, v.Radius);
