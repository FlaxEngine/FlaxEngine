// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Vector3.h"
#include "CollisionsHelper.h"

/// <summary>
/// Represents a plane in three dimensional space.
/// </summary>
API_STRUCT() struct FLAXENGINE_API Plane
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(Plane);
public:
    static const Real DistanceEpsilon;
    static const Real NormalEpsilon;

public:
    /// <summary>
    /// The normal vector of the plane.
    /// </summary>
    API_FIELD() Vector3 Normal;

    /// <summary>
    /// The distance of the plane along its normal from the origin.
    /// </summary>
    API_FIELD() Real D;

public:
    /// <summary>
    /// Empty constructor.
    /// </summary>
    Plane() = default;

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="normal">The normal of the plane</param>
    /// <param name="d">The distance of the plane along its normal from the origin</param>
    Plane(const Vector3& normal, Real d)
        : Normal(normal)
        , D(d)
    {
    }

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="point">Any point that lies along the plane</param>
    /// <param name="normal">The normal vector to the plane</param>
    Plane(const Vector3& point, const Vector3& normal)
        : Normal(normal)
        , D(-Vector3::Dot(normal, point))
    {
    }

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="point1">First point of a triangle defining the plane</param>
    /// <param name="point2">Second point of a triangle defining the plane</param>
    /// <param name="point3">Third point of a triangle defining the plane</param>
    Plane(const Vector3& point1, const Vector3& point2, const Vector3& point3);

public:
    String ToString() const;

public:
    /// <summary>
    /// Changes the coefficients of the normal vector of the plane to make it of unit length.
    /// </summary>
    void Normalize();

    void Negate()
    {
        Normal = Normal.GetNegative();
        D *= -1;
    }

    Vector3 PointOnPlane() const
    {
        return Normal * D;
    }

    Plane Negated() const
    {
        return Plane(-Normal, -D);
    }

public:
    Plane operator*(Real scale) const
    {
        return Plane(Normal * scale, D * scale);
    }

    bool operator==(const Plane& other) const
    {
        return Normal == other.Normal && D == other.D;
    }

    bool operator!=(const Plane& other) const
    {
        return Normal != other.Normal || D != other.D;
    }

    static bool NearEqual(const Plane& a, const Plane& b)
    {
        return Vector3::NearEqual(a.Normal, b.Normal) && Math::NearEqual(a.D, b.D);
    }

public:
    void Translate(const Vector3& translation)
    {
        const Vector3 mul = Normal * translation;
        D += mul.SumValues();
    }

    static Plane Translated(const Plane& plane, const Vector3& translation)
    {
        Plane result = plane;
        result.Translate(translation);
        return result;
    }

    static Plane Translated(const Plane& plane, float translateX, float translateY, float translateZ)
    {
        Plane result = plane;
        result.Translate(Vector3(translateX, translateY, translateZ));
        return result;
    }

public:
    static Vector3 Intersection(const Plane& inPlane1, const Plane& inPlane2, const Plane& inPlane3);

    // Determines if there is an intersection between the current object and a point
    // @param point The point to test
    // @returns Whether the two objects intersected
    PlaneIntersectionType Intersects(const Vector3& point) const
    {
        return CollisionsHelper::PlaneIntersectsPoint(*this, point);
    }

    // summary>
    // Determines if there is an intersection between the current object and a Ray
    // @param ray The ray to test
    // @returns Whether the two objects intersected
    bool Intersects(const Ray& ray) const
    {
        Real distance;
        return CollisionsHelper::RayIntersectsPlane(ray, *this, distance);
    }

    // summary>
    // Determines if there is an intersection between the current object and a Ray.
    // /summary>
    // @param ray The ray to test
    // @param distance When the method completes, contains the distance of the intersection, or 0 if there was no intersection
    // @returns Whether the two objects intersected
    bool Intersects(const Ray& ray, Real& distance) const
    {
        return CollisionsHelper::RayIntersectsPlane(ray, *this, distance);
    }

    // summary>
    // Determines if there is an intersection between the current object and a Ray.
    // /summary>
    // @param ray The ray to test
    // @param point When the method completes, contains the point of intersection, or <see const="Vector3.Zero"/> if there was no intersection
    // @returns Whether the two objects intersected
    bool Intersects(const Ray& ray, Vector3& point) const
    {
        return CollisionsHelper::RayIntersectsPlane(ray, *this, point);
    }

    // Determines if there is an intersection between the current object and a Plane
    // @param plane The plane to test
    // @returns Whether the two objects intersected
    bool Intersects(const Plane& plane) const
    {
        return CollisionsHelper::PlaneIntersectsPlane(*this, plane);
    }

    // Determines if there is an intersection between the current object and a Plane
    // @param plane The plane to test
    // @param line When the method completes, contains the line of intersection as a Ray, or a zero ray if there was no intersection
    // @returns Whether the two objects intersected
    bool Intersects(const Plane& plane, Ray& line) const
    {
        return CollisionsHelper::PlaneIntersectsPlane(*this, plane, line);
    }

    // Determines if there is an intersection between the current object and a triangle
    // @param vertex1 The first vertex of the triangle to test
    // @param vertex2 The second vertex of the triangle to test
    // @param vertex3 The third vertex of the triangle to test
    // @returns Whether the two objects intersected
    PlaneIntersectionType Intersects(const Vector3& vertex1, const Vector3& vertex2, const Vector3& vertex3) const
    {
        return CollisionsHelper::PlaneIntersectsTriangle(*this, vertex1, vertex2, vertex3);
    }

    // Determines if there is an intersection between the current object and a Bounding Box
    // @param box The box to test
    // @returns Whether the two objects intersected
    PlaneIntersectionType Intersects(const BoundingBox& box) const
    {
        return CollisionsHelper::PlaneIntersectsBox(*this, box);
    }

    // Determines if there is an intersection between the current object and a Bounding Sphere
    // @param sphere The sphere to test
    // @returns Whether the two objects intersected
    PlaneIntersectionType Intersects(const BoundingSphere& sphere) const
    {
        return CollisionsHelper::PlaneIntersectsSphere(*this, sphere);
    }

public:
    // Scales the plane by the given scaling factor
    // @param value The plane to scale
    // @param scale The amount by which to scale the plane
    // @param result When the method completes, contains the scaled plane
    static void Multiply(const Plane& value, Real scale, Plane& result);

    // Scales the plane by the given scaling factor
    // @param value The plane to scale
    // @param scale The amount by which to scale the plane
    // @returns The scaled plane
    static Plane Multiply(const Plane& value, Real scale);

    // Calculates the dot product of the specified vector and plane
    // @param left The source plane
    // @param right The source vector
    // @param result When the method completes, contains the dot product of the specified plane and vector
    static void Dot(const Plane& left, const Vector4& right, Real& result);

    // Calculates the dot product of the specified vector and plane
    // @param left The source plane
    // @param right The source vector
    // @returns The dot product of the specified plane and vector
    static Real Dot(const Plane& left, const Vector4& right);

    // Calculates the dot product of a specified vector and the normal of the plane plus the distance value of the plane
    // @param left The source plane
    // @param right The source vector
    // @param result When the method completes, contains the dot product of a specified vector and the normal of the Plane plus the distance value of the plane
    static void DotCoordinate(const Plane& left, const Vector3& right, Real& result);

    // Calculates the dot product of a specified vector and the normal of the plane plus the distance value of the plane
    // @param left The source plane
    // @param right The source vector
    // @returns The dot product of a specified vector and the normal of the Plane plus the distance value of the plane
    static Real DotCoordinate(const Plane& left, const Vector3& right);

    // Calculates the dot product of the specified vector and the normal of the plane
    // @param left The source plane
    // @param right The source vector
    // @param result When the method completes, contains the dot product of the specified vector and the normal of the plane
    static void DotNormal(const Plane& left, const Vector3& right, Real& result);

    // Calculates the dot product of the specified vector and the normal of the plane
    // @param left The source plane
    // @param right The source vector
    // @returns The dot product of the specified vector and the normal of the plane
    static Real DotNormal(const Plane& left, const Vector3& right);

    // Changes the coefficients of the normal vector of the plane to make it of unit length
    // @param plane The source plane
    // @param result When the method completes, contains the normalized plane
    static void Normalize(const Plane& plane, Plane& result);

    // Changes the coefficients of the normal vector of the plane to make it of unit length
    // @param plane The source plane
    // @returns The normalized plane
    static Plane Normalize(const Plane& plane);

    // Transforms a normalized plane by a quaternion rotation
    // @param plane The normalized source plane
    // @param rotation The quaternion rotation
    // @param result When the method completes, contains the transformed plane
    static void Transform(const Plane& plane, const Quaternion& rotation, Plane& result);

    // Transforms a normalized plane by a quaternion rotation
    // @param plane The normalized source plane
    // @param rotation The quaternion rotation
    // @returns The transformed plane
    static Plane Transform(const Plane& plane, const Quaternion& rotation);

    // Transforms a normalized plane by a matrix
    // @param plane The normalized source plane
    // @param transformation The transformation matrix
    // @param result When the method completes, contains the transformed plane
    static void Transform(const Plane& plane, const Matrix& transformation, Plane& result);

    // Transforms a normalized plane by a matrix
    // @param plane The normalized source plane
    // @param transformation The transformation matrix
    // @returns When the method completes, contains the transformed plane
    static Plane Transform(const Plane& plane, const Matrix& transformation);
};

inline Plane operator*(Real a, const Plane& b)
{
    return b * a;
}

template<>
struct TIsPODType<Plane>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(Plane, "Normal:{0} D:{1}", v.Normal, v.D);
