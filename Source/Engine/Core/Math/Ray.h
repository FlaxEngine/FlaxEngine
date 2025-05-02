// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/Platform.h"
#include "Vector3.h"
#include "CollisionsHelper.h"

struct Viewport;

/// <summary>
/// Represents a three dimensional line based on a point in space and a direction.
/// </summary>
API_STRUCT() struct FLAXENGINE_API Ray
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(Ray);
public:
    /// <summary>
    /// The position in three dimensional space where the ray starts.
    /// </summary>
    API_FIELD() Vector3 Position;

    /// <summary>
    /// The normalized direction in which the ray points.
    /// </summary>
    API_FIELD() Vector3 Direction;

public:
    /// <summary>
    /// Identity ray (at zero origin pointing forwards).
    /// </summary>
    static Ray Identity;

public:
    /// <summary>
    /// Empty constructor.
    /// </summary>
    Ray() = default;

    /// <summary>
    /// Initializes a new instance of the <see cref="Ray"/> struct.
    /// </summary>
    /// <param name="position">The ray origin position in 3D space.</param>
    /// <param name="direction">The normalized ray direction in 3D space.</param>
    Ray(const Vector3& position, const Vector3& direction)
        : Position(position)
        , Direction(direction)
    {
        CHECK_DEBUG(Direction.IsNormalized());
    }

public:
    String ToString() const;

public:
    FORCE_INLINE bool operator==(const Ray& other) const
    {
        return Position == other.Position && Direction == other.Direction;
    }

    FORCE_INLINE bool operator!=(const Ray& other) const
    {
        return Position != other.Position || Direction != other.Direction;
    }

    static bool NearEqual(const Ray& a, const Ray& b)
    {
        return Vector3::NearEqual(a.Position, b.Position) && Vector3::NearEqual(a.Direction, b.Direction);
    }

    static bool NearEqual(const Ray& a, const Ray& b, Real epsilon)
    {
        return Vector3::NearEqual(a.Position, b.Position, epsilon) && Vector3::NearEqual(a.Direction, b.Direction, epsilon);
    }

public:
    /// <summary>
    /// Gets a point at distance long ray.
    /// </summary>
    /// <param name="distance">The distance from ray origin.</param>
    /// <returns>The calculated point.</returns>
    Vector3 GetPoint(Real distance) const;

    /// <summary>
    /// Determines if there is an intersection between ray and a point.
    /// </summary>
    /// <param name="point">The point to test.</param>
    /// <returns>Whether the two objects intersected.</returns>
    bool Intersects(const Vector3& point) const
    {
        return CollisionsHelper::RayIntersectsPoint(*this, point);
    }

    /// <summary>
    /// Determines if there is an intersection between two rays.
    /// </summary>
    /// <param name="ray">The ray to test.</param>
    /// <returns>Whether the two objects intersected.</returns>
    bool Intersects(const Ray& ray) const
    {
        Vector3 point;
        return CollisionsHelper::RayIntersectsRay(*this, ray, point);
    }

    /// <summary>
    /// Determines if there is an intersection between ray and a <see cref="Ray" />.
    /// </summary>
    /// <param name="ray">The ray to test.</param>
    /// <param name="point">When the method completes, contains the point of intersection, or <see cref="Vector3.Zero" /> if there was no intersection.
    /// </param>
    /// <returns>Whether the two objects intersected.</returns>
    bool Intersects(const Ray& ray, Vector3& point) const
    {
        return CollisionsHelper::RayIntersectsRay(*this, ray, point);
    }

    /// <summary>
    /// Determines if there is an intersection between ray and a <see cref="Plane" />.
    /// </summary>
    /// <param name="plane">The plane to test</param>
    /// <returns>Whether the two objects intersected.</returns>
    bool Intersects(const Plane& plane) const
    {
        Real distance;
        return CollisionsHelper::RayIntersectsPlane(*this, plane, distance);
    }

    /// <summary>
    /// Determines if there is an intersection between ray and a <see cref="Plane" />.
    /// </summary>
    /// <param name="plane">The plane to test.</param>
    /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.</param>
    /// <returns>Whether the two objects intersected.</returns>
    bool Intersects(const Plane& plane, Real& distance) const
    {
        return CollisionsHelper::RayIntersectsPlane(*this, plane, distance);
    }

    /// <summary>
    /// Determines if there is an intersection between ray and a <see cref="Plane" />.
    /// </summary>
    /// <param name="plane">The plane to test.</param>
    /// <param name="point">When the method completes, contains the point of intersection, or <see cref="Vector3.Zero" /> if there was no intersection.</param>
    /// <returns>Whether the two objects intersected.</returns>
    bool Intersects(const Plane& plane, Vector3& point) const
    {
        return CollisionsHelper::RayIntersectsPlane(*this, plane, point);
    }

    /// <summary>
    /// Determines if there is an intersection between ray and a triangle.
    /// </summary>
    /// <param name="vertex1">The first vertex of the triangle to test.</param>
    /// <param name="vertex2">The second vertex of the triangle to test.</param>
    /// <param name="vertex3">The third vertex of the triangle to test.</param>
    /// <returns>Whether the two objects intersected.</returns>
    bool Intersects(const Vector3& vertex1, const Vector3& vertex2, const Vector3& vertex3) const
    {
        Real distance;
        return CollisionsHelper::RayIntersectsTriangle(*this, vertex1, vertex2, vertex3, distance);
    }

    /// <summary>
    /// Determines if there is an intersection between ray and a triangle.
    /// </summary>
    /// <param name="vertex1">The first vertex of the triangle to test.</param>
    /// <param name="vertex2">The second vertex of the triangle to test.</param>
    /// <param name="vertex3">The third vertex of the triangle to test.</param>
    /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.</param>
    /// <returns>Whether the two objects intersected.</returns>
    bool Intersects(const Vector3& vertex1, const Vector3& vertex2, const Vector3& vertex3, Real& distance) const
    {
        return CollisionsHelper::RayIntersectsTriangle(*this, vertex1, vertex2, vertex3, distance);
    }

    /// <summary>
    /// Determines if there is an intersection between ray and a triangle.
    /// </summary>
    /// <param name="vertex1">The first vertex of the triangle to test.</param>
    /// <param name="vertex2">The second vertex of the triangle to test.</param>
    /// <param name="vertex3">The third vertex of the triangle to test.</param>
    /// <param name="point">When the method completes, contains the point of intersection, or <see cref="Vector3.Zero" /> if there was no intersection.</param>
    /// <returns>Whether the two objects intersected.</returns>
    bool Intersects(const Vector3& vertex1, const Vector3& vertex2, const Vector3& vertex3, Vector3& point) const
    {
        return CollisionsHelper::RayIntersectsTriangle(*this, vertex1, vertex2, vertex3, point);
    }

    /// <summary>
    /// Determines if there is an intersection between ray and a <see cref="BoundingBox" />.
    /// </summary>
    /// <param name="box">The box to test.</param>
    /// <returns>Whether the two objects intersected.</returns>
    bool Intersects(const BoundingBox& box) const
    {
        Real distance;
        return CollisionsHelper::RayIntersectsBox(*this, box, distance);
    }

    /// <summary>
    /// Determines if there is an intersection between ray and a <see cref="BoundingBox" />.
    /// </summary>
    /// <param name="box">The box to test.</param>
    /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.</param>
    /// <returns>Whether the two objects intersected.</returns>
    bool Intersects(const BoundingBox& box, Real& distance) const
    {
        return CollisionsHelper::RayIntersectsBox(*this, box, distance);
    }

    /// <summary>
    /// Determines if there is an intersection between ray and a <see cref="BoundingBox" />.
    /// </summary>
    /// <param name="box">The box to test.</param>
    /// <param name="point">When the method completes, contains the point of intersection, or <see cref="Vector3.Zero" /> if there was no intersection.</param>
    /// <returns>Whether the two objects intersected.</returns>
    bool Intersects(const BoundingBox& box, Vector3& point) const
    {
        return CollisionsHelper::RayIntersectsBox(*this, box, point);
    }

    /// <summary>
    /// Determines if there is an intersection between ray and a <see cref="BoundingSphere" />.
    /// </summary>
    /// <param name="sphere">The sphere to test.</param>
    /// <returns>Whether the two objects intersected.</returns>
    bool Intersects(const BoundingSphere& sphere) const
    {
        Real distance;
        return CollisionsHelper::RayIntersectsSphere(*this, sphere, distance);
    }

    /// <summary>
    /// Determines if there is an intersection between ray and a <see cref="BoundingSphere" />.
    /// </summary>
    /// <param name="sphere">The sphere to test.</param>
    /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.</param>
    /// <returns>Whether the two objects intersected.</returns>
    bool Intersects(const BoundingSphere& sphere, Real& distance) const
    {
        return CollisionsHelper::RayIntersectsSphere(*this, sphere, distance);
    }

    /// <summary>
    /// Determines if there is an intersection between ray and a <see cref="BoundingSphere" />.
    /// </summary>
    /// <param name="sphere">The sphere to test.</param>
    /// <param name="point">When the method completes, contains the point of intersection, or <see cref="Vector3.Zero" /> if there was no intersection.</param>
    /// <returns>Whether the two objects intersected.</returns>
    bool Intersects(const BoundingSphere& sphere, Vector3& point) const
    {
        return CollisionsHelper::RayIntersectsSphere(*this, sphere, point);
    }

public:
    /// <summary>
    /// Calculates a world space ray from 2d screen coordinates.
    /// </summary>
    /// <param name="x">The X coordinate on 2d screen.</param>
    /// <param name="y">The Y coordinate on 2d screen.</param>
    /// <param name="viewport">The screen viewport.</param>
    /// <param name="vp">The View*Projection matrix.</param>
    /// <returns>The resulting ray.</returns>
    static Ray GetPickRay(float x, float y, const Viewport& viewport, const Matrix& vp);
};

template<>
struct TIsPODType<Ray>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(Ray, "Position:{0} Direction:{1}", v.Position, v.Direction);
