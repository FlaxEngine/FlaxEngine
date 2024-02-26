// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Vector3.h"
#include "CollisionsHelper.h"

/// <summary>
/// Represents a three dimensional triangle.
/// </summary>
struct FLAXENGINE_API Triangle
{
public:
    /// <summary>
    /// The first vertex.
    /// </summary>
    Vector3 V0;

    /// <summary>
    /// The second vertex.
    /// </summary>
    Vector3 V1;

    /// <summary>
    /// The third vertex.
    /// </summary>
    Vector3 V2;

public:
    /// <summary>
    /// Empty constructor.
    /// </summary>
    Triangle() = default;

    /// <summary>
    /// Initializes a new instance of the <see cref="Triangle"/> struct.
    /// </summary>
    /// <param name="v0">The first vertex.</param>
    /// <param name="v1">The second vertex .</param>
    /// <param name="v2">The third vertex.</param>
    Triangle(const Vector3& v0, const Vector3& v1, const Vector3& v2)
        : V0(v0)
        , V1(v1)
        , V2(v2)
    {
    }

public:
    Vector3 GetNormal() const
    {
        return Vector3::Normalize((V1 - V0) ^ (V2 - V0));
    }

public:
    // Determines if there is an intersection between the current object and a Ray
    // @param ray The ray to test
    // @returns Whether the two objects intersected
    bool Intersects(const Ray& ray) const
    {
        Real distance;
        return CollisionsHelper::RayIntersectsTriangle(ray, V0, V1, V2, distance);
    }

    // Determines if there is an intersection between the current object and a Ray
    // @param ray The ray to test
    // @param distance When the method completes, contains the distance of the intersection, or 0 if there was no intersection
    // @returns Whether the two objects intersected
    bool Intersects(const Ray& ray, Real& distance) const
    {
        return CollisionsHelper::RayIntersectsTriangle(ray, V0, V1, V2, distance);
    }

    // Determines if there is an intersection between the current object and a Ray
    // @param ray The ray to test
    // @param distance When the method completes, contains the distance of the intersection, or 0 if there was no intersection
    // @param normal When the method completes, contains the intersection surface normal vector, or Vector3::Up if there was no intersection
    // @returns Whether the two objects intersected
    bool Intersects(const Ray& ray, Real& distance, Vector3& normal) const
    {
        return CollisionsHelper::RayIntersectsTriangle(ray, V0, V1, V2, distance, normal);
    }

    // Determines if there is an intersection between the current object and a Ray
    // @param ray The ray to test
    // @param point When the method completes, contains the point of intersection, or <see cref="Vector3.Zero"/> if there was no intersection
    // @returns Whether the two objects intersected
    bool Intersects(const Ray& ray, Vector3& point) const
    {
        return CollisionsHelper::RayIntersectsTriangle(ray, V0, V1, V2, point);
    }
};

template<>
struct TIsPODType<Triangle>
{
    enum { Value = true };
};
