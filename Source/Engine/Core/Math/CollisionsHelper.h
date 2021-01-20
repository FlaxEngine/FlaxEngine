// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Compiler.h"

struct Vector2;
struct Vector3;
struct Vector4;
struct Ray;
struct Plane;
struct Rectangle;
struct BoundingBox;
struct BoundingSphere;
struct BoundingFrustum;

/// <summary>
/// Describes how one bounding volume contains another.
/// </summary>
enum class ContainmentType
{
    /// <summary>
    /// The two bounding volumes don't intersect at all.
    /// </summary>
    Disjoint,

    /// <summary>
    /// One bounding volume completely contains another.
    /// </summary>
    Contains,

    /// <summary>
    /// The two bounding volumes overlap.
    /// </summary>
    Intersects
};

/// <summary>
/// Describes the result of an intersection with a plane in three dimensions.
/// </summary>
enum class PlaneIntersectionType
{
    /// <summary>
    /// The object is behind the plane.
    /// </summary>
    Back,

    /// <summary>
    /// The object is in front of the plane.
    /// </summary>
    Front,

    /// <summary>
    /// The object is intersecting the plane.
    /// </summary>
    Intersecting
};

/// <summary>
/// Contains static methods to help in determining intersections, containment, etc.
/// </summary>
class FLAXENGINE_API CollisionsHelper
{
public:

    /// <summary>
    /// Determines the closest point between a point and a line.
    /// </summary>
    /// <param name="point">The point to test.</param>
    /// <param name="p0">The line first point.</param>
    /// <param name="p1">The line second point.</param>
    /// <param name="result">When the method completes, contains the closest point between the two objects.</param>
    static void ClosestPointPointLine(const Vector2& point, const Vector2& p0, const Vector2& p1, Vector2& result);

    /// <summary>
    /// Determines the closest point between a point and a line.
    /// </summary>
    /// <param name="point">The point to test.</param>
    /// <param name="p0">The line first point.</param>
    /// <param name="p1">The line second point.</param>
    /// <param name="result">When the method completes, contains the closest point between the two objects.</param>
    static Vector3 ClosestPointPointLine(const Vector3& point, const Vector3& p0, const Vector3& p1);

    /// <summary>
    /// Determines the closest point between a point and a triangle.
    /// </summary>
    /// <param name="point">The point to test.</param>
    /// <param name="vertex1">The first vertex to test.</param>
    /// <param name="vertex2">The second vertex to test.</param>
    /// <param name="vertex3">The third vertex to test.</param>
    /// <param name="result">When the method completes, contains the closest point between the two objects.</param>
    static void ClosestPointPointTriangle(const Vector3& point, const Vector3& vertex1, const Vector3& vertex2, const Vector3& vertex3, Vector3& result);

    /// <summary>
    /// Determines the closest point between a <see cref="Plane" /> and a point.
    /// </summary>
    /// <param name="plane">The plane to test.</param>
    /// <param name="point">The point to test.</param>
    /// <param name="result">When the method completes, contains the closest point between the two objects.</param>
    static void ClosestPointPlanePoint(const Plane& plane, const Vector3& point, Vector3& result);

    /// <summary>
    /// Determines the closest point between a <see cref="BoundingBox" /> and a point.
    /// </summary>
    /// <param name="box">The box to test.</param>
    /// <param name="point">The point to test.</param>
    /// <param name="result">When the method completes, contains the closest point between the two objects.</param>
    static void ClosestPointBoxPoint(const BoundingBox& box, const Vector3& point, Vector3& result);

    /// <summary>
    /// Determines the closest point between a <see cref="Rectangle" /> and a point.
    /// </summary>
    /// <param name="rect">The rectangle to test.</param>
    /// <param name="point">The point to test.</param>
    /// <param name="result">When the method completes, contains the closest point between the two objects.</param>
    static void ClosestPointRectanglePoint(const Rectangle& rect, const Vector2& point, Vector2& result);

    /// <summary>
    /// Determines the closest point between a <see cref="BoundingSphere" /> and a point.
    /// </summary>
    /// <param name="sphere">The sphere to test.</param>
    /// <param name="point">The point to test.</param>
    /// <param name="result">When the method completes, contains the closest point between the two objects; or, if the point is directly in the center of the sphere, contains <see cref="Vector3.Zero" />.</param>
    static void ClosestPointSpherePoint(const BoundingSphere& sphere, const Vector3& point, Vector3& result);

    /// <summary>
    /// Determines the closest point between a <see cref="BoundingSphere" /> and a <see cref="BoundingSphere" />.
    /// </summary>
    /// <param name="sphere1">The first sphere to test.</param>
    /// <param name="sphere2">The second sphere to test.</param>
    /// <param name="result">When the method completes, contains the closest point between the two objects; or, if the point is directly in the center of the sphere, contains <see cref="Vector3.Zero" />.</param>
    /// <remarks>
    /// If the two spheres are overlapping, but not directly on top of each other, the closest point
    /// is the 'closest' point of intersection. This can also be considered is the deepest point of
    /// intersection.
    /// </remarks>
    static void ClosestPointSphereSphere(const BoundingSphere& sphere1, const BoundingSphere& sphere2, Vector3& result);

    /// <summary>
    /// Determines the distance between a <see cref="Plane" /> and a point.
    /// </summary>
    /// <param name="plane">The plane to test.</param>
    /// <param name="point">The point to test.</param>
    /// <returns>The distance between the two objects.</returns>
    static float DistancePlanePoint(const Plane& plane, const Vector3& point);

    /// <summary>
    /// Determines the distance between a <see cref="BoundingBox" /> and a point.
    /// </summary>
    /// <param name="box">The box to test.</param>
    /// <param name="point">The point to test.</param>
    /// <returns>The distance between the two objects.</returns>
    static float DistanceBoxPoint(const BoundingBox& box, const Vector3& point);

    /// <summary>
    /// Determines the distance between a <see cref="BoundingBox" /> and a <see cref="BoundingBox" />.
    /// </summary>
    /// <param name="box1">The first box to test.</param>
    /// <param name="box2">The second box to test.</param>
    /// <returns>The distance between the two objects.</returns>
    static float DistanceBoxBox(const BoundingBox& box1, const BoundingBox& box2);

    /// <summary>
    /// Determines the distance between a <see cref="BoundingSphere" /> and a point.
    /// </summary>
    /// <param name="sphere">The sphere to test.</param>
    /// <param name="point">The point to test.</param>
    /// <returns>The distance between the two objects.</returns>
    static float DistanceSpherePoint(const BoundingSphere& sphere, const Vector3& point);

    /// <summary>
    /// Determines the distance between a <see cref="BoundingSphere" /> and a <see cref="BoundingSphere" />.
    /// </summary>
    /// <param name="sphere1">The first sphere to test.</param>
    /// <param name="sphere2">The second sphere to test.</param>
    /// <returns>The distance between the two objects.</returns>
    static float DistanceSphereSphere(const BoundingSphere& sphere1, const BoundingSphere& sphere2);

    /// <summary>
    /// Determines whether there is an intersection between a <see cref="Ray" /> and a point.
    /// </summary>
    /// <param name="ray">The ray to test.</param>
    /// <param name="point">The point to test.</param>
    /// <returns>Whether the two objects intersect.</returns>
    static bool RayIntersectsPoint(const Ray& ray, const Vector3& point);

    /// <summary>
    /// Determines whether there is an intersection between a <see cref="Ray" /> and a <see cref="Ray" />.
    /// </summary>
    /// <param name="ray1">The first ray to test.</param>
    /// <param name="ray2">The second ray to test.</param>
    /// <param name="point">When the method completes, contains the point of intersection, or <see cref="Vector3.Zero" /> if there was no intersection.</param>
    /// <returns>Whether the two objects intersect.</returns>
    /// <remarks>
    /// This method performs a ray vs ray intersection test based on the following formula
    /// from Goldman.
    /// <code>s = det([o_2 - o_1, d_2, d_1 x d_2]) / ||d_1 x d_2||^2</code>
    /// <code>t = det([o_2 - o_1, d_1, d_1 x d_2]) / ||d_1 x d_2||^2</code>
    /// Where o_1 is the position of the first ray, o_2 is the position of the second ray,
    /// d_1 is the normalized direction of the first ray, d_2 is the normalized direction
    /// of the second ray, det denotes the determinant of a matrix, x denotes the cross
    /// product, [ ] denotes a matrix, and || || denotes the length or magnitude of a vector.
    /// </remarks>
    static bool RayIntersectsRay(const Ray& ray1, const Ray& ray2, Vector3& point);

    /// <summary>
    /// Determines whether there is an intersection between a <see cref="Ray" /> and a <see cref="Plane" />.
    /// </summary>
    /// <param name="ray">The ray to test.</param>
    /// <param name="plane">The plane to test.</param>
    /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.</param>
    /// <returns>Whether the two objects intersect.</returns>
    static bool RayIntersectsPlane(const Ray& ray, const Plane& plane, float& distance);

    /// <summary>
    /// Determines whether there is an intersection between a <see cref="Ray" /> and a <see cref="Plane" />.
    /// </summary>
    /// <param name="ray">The ray to test.</param>
    /// <param name="plane">The plane to test</param>
    /// <param name="point">When the method completes, contains the point of intersection, or <see cref="Vector3.Zero" /> if there was no intersection.</param>
    /// <returns>Whether the two objects intersected.</returns>
    static bool RayIntersectsPlane(const Ray& ray, const Plane& plane, Vector3& point);

    /// <summary>
    /// Determines whether there is an intersection between a <see cref="Ray" /> and a triangle.
    /// </summary>
    /// <param name="ray">The ray to test.</param>
    /// <param name="vertex1">The first vertex of the triangle to test.</param>
    /// <param name="vertex2">The second vertex of the triangle to test.</param>
    /// <param name="vertex3">The third vertex of the triangle to test.</param>
    /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.</param>
    /// <returns>Whether the two objects intersected.</returns>
    /// <remarks>
    /// This method tests if the ray intersects either the front or back of the triangle.
    /// If the ray is parallel to the triangle's plane, no intersection is assumed to have
    /// happened. If the intersection of the ray and the triangle is behind the origin of
    /// the ray, no intersection is assumed to have happened. In both cases of assumptions,
    /// this method returns false.
    /// </remarks>
    static bool RayIntersectsTriangle(const Ray& ray, const Vector3& vertex1, const Vector3& vertex2, const Vector3& vertex3, float& distance);

    /// <summary>
    /// Determines whether there is an intersection between a <see cref="Ray" /> and a triangle.
    /// </summary>
    /// <param name="ray">The ray to test.</param>
    /// <param name="vertex1">The first vertex of the triangle to test.</param>
    /// <param name="vertex2">The second vertex of the triangle to test.</param>
    /// <param name="vertex3">The third vertex of the triangle to test.</param>
    /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.</param>
    /// <param name="normal">When the method completes, contains the intersection surface normal vector, or Vector3::Up if there was no intersection.</param>
    /// <returns>Whether the two objects intersected.</returns>
    /// <remarks>
    /// This method tests if the ray intersects either the front or back of the triangle.
    /// If the ray is parallel to the triangle's plane, no intersection is assumed to have
    /// happened. If the intersection of the ray and the triangle is behind the origin of
    /// the ray, no intersection is assumed to have happened. In both cases of assumptions,
    /// this method returns false.
    /// </remarks>
    static bool RayIntersectsTriangle(const Ray& ray, const Vector3& vertex1, const Vector3& vertex2, const Vector3& vertex3, float& distance, Vector3& normal);

    /// <summary>
    /// Determines whether there is an intersection between a <see cref="Ray" /> and a triangle.
    /// </summary>
    /// <param name="ray">The ray to test.</param>
    /// <param name="vertex1">The first vertex of the triangle to test.</param>
    /// <param name="vertex2">The second vertex of the triangle to test.</param>
    /// <param name="vertex3">The third vertex of the triangle to test.</param>
    /// <param name="point">When the method completes, contains the point of intersection, or <see cref="Vector3.Zero" /> if there was no intersection.</param>
    /// <returns>Whether the two objects intersected.</returns>
    static bool RayIntersectsTriangle(const Ray& ray, const Vector3& vertex1, const Vector3& vertex2, const Vector3& vertex3, Vector3& point);

    /// <summary>
    /// Determines whether there is an intersection between a <see cref="Ray" /> and a <see cref="BoundingBox" />.
    /// </summary>
    /// <param name="ray">The ray to test.</param>
    /// <param name="box">The box to test.</param>
    /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.</param>
    /// <returns>Whether the two objects intersected.</returns>
    static bool RayIntersectsBox(const Ray& ray, const BoundingBox& box, float& distance);

    /// <summary>
    /// Determines whether there is an intersection between a <see cref="Ray" /> and a <see cref="BoundingBox" />.
    /// </summary>
    /// <param name="ray">The ray to test.</param>
    /// <param name="box">The box to test.</param>
    /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.</param>
    /// <param name="normal">When the method completes, contains the intersection surface normal vector, or Vector3::Up if there was no intersection.</param>
    /// <returns>Whether the two objects intersected.</returns>
    static bool RayIntersectsBox(const Ray& ray, const BoundingBox& box, float& distance, Vector3& normal);

    /// <summary>
    /// Determines whether there is an intersection between a <see cref="Ray" /> and a <see cref="Plane" />.
    /// </summary>
    /// <param name="ray">The ray to test.</param>
    /// <param name="box">The box to test.</param>
    /// <param name="point">When the method completes, contains the point of intersection, or <see cref="Vector3.Zero" /> if there was no intersection.</param>
    /// <returns>Whether the two objects intersected.</returns>
    static bool RayIntersectsBox(const Ray& ray, const BoundingBox& box, Vector3& point);

    /// <summary>
    /// Determines whether there is an intersection between a <see cref="Ray" /> and a <see cref="BoundingSphere" />.
    /// </summary>
    /// <param name="ray">The ray to test.</param>
    /// <param name="sphere">The sphere to test.</param>
    /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.</param>
    /// <returns>Whether the two objects intersected.</returns>
    static bool RayIntersectsSphere(const Ray& ray, const BoundingSphere& sphere, float& distance);

    /// <summary>
    /// Determines whether there is an intersection between a <see cref="Ray" /> and a <see cref="BoundingSphere" />.
    /// </summary>
    /// <param name="ray">The ray to test.</param>
    /// <param name="sphere">The sphere to test.</param>
    /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.</param>
    /// <param name="normal">When the method completes, contains the intersection surface normal vector, or Vector3::Up if there was no intersection.</param>
    /// <returns>Whether the two objects intersected.</returns>
    static bool RayIntersectsSphere(const Ray& ray, const BoundingSphere& sphere, float& distance, Vector3& normal);

    /// <summary>
    /// Determines whether there is an intersection between a <see cref="Ray" /> and a <see cref="BoundingSphere" />.
    /// </summary>
    /// <param name="ray">The ray to test.</param>
    /// <param name="sphere">The sphere to test.</param>
    /// <param name="point">When the method completes, contains the point of intersection, or <see cref="Vector3.Zero" /> if there was no intersection.</param>
    /// <returns>Whether the two objects intersected.</returns>
    static bool RayIntersectsSphere(const Ray& ray, const BoundingSphere& sphere, Vector3& point);

    /// <summary>
    /// Determines whether there is an intersection between a <see cref="Plane" /> and a point.
    /// </summary>
    /// <param name="plane">The plane to test.</param>
    /// <param name="point">The point to test.</param>
    /// <returns>Whether the two objects intersected.</returns>
    static PlaneIntersectionType PlaneIntersectsPoint(const Plane& plane, const Vector3& point);

    /// <summary>
    /// Determines whether there is an intersection between a <see cref="Plane" /> and a <see cref="Plane" />.
    /// </summary>
    /// <param name="plane1">The first plane to test.</param>
    /// <param name="plane2">The second plane to test.</param>
    /// <returns>Whether the two objects intersected.</returns>
    static bool PlaneIntersectsPlane(const Plane& plane1, const Plane& plane2);

    /// <summary>
    /// Determines whether there is an intersection between a <see cref="Plane" /> and a <see cref="Plane" />.
    /// </summary>
    /// <param name="plane1">The first plane to test.</param>
    /// <param name="plane2">The second plane to test.</param>
    /// <param name="line">When the method completes, contains the line of intersection as a <see cref="Ray" />, or a zero ray if there was no intersection.
    /// </param>
    /// <returns>Whether the two objects intersected.</returns>
    /// <remarks>
    /// Although a ray is set to have an origin, the ray returned by this method is really
    /// a line in three dimensions which has no real origin. The ray is considered valid when
    /// both the positive direction is used and when the negative direction is used.
    /// </remarks>
    static bool PlaneIntersectsPlane(const Plane& plane1, const Plane& plane2, Ray& line);

    /// <summary>
    /// Determines whether there is an intersection between a <see cref="Plane" /> and a triangle.
    /// </summary>
    /// <param name="plane">The plane to test.</param>
    /// <param name="vertex1">The first vertex of the triangle to test.</param>
    /// <param name="vertex2">The second vertex of the triangle to test.</param>
    /// <param name="vertex3">The third vertex of the triangle to test.</param>
    /// <returns>Whether the two objects intersected.</returns>
    static PlaneIntersectionType PlaneIntersectsTriangle(const Plane& plane, const Vector3& vertex1, const Vector3& vertex2, const Vector3& vertex3);

    /// <summary>
    /// Determines whether there is an intersection between a <see cref="Plane" /> and a <see cref="BoundingBox" />.
    /// </summary>
    /// <param name="plane">The plane to test.</param>
    /// <param name="box">The box to test.</param>
    /// <returns>Whether the two objects intersected.</returns>
    static PlaneIntersectionType PlaneIntersectsBox(const Plane& plane, const BoundingBox& box);

    /// <summary>
    /// Determines whether there is an intersection between a <see cref="Plane" /> and a <see cref="BoundingSphere" />.
    /// </summary>
    /// <param name="plane">The plane to test.</param>
    /// <param name="sphere">The sphere to test.</param>
    /// <returns>Whether the two objects intersected.</returns>
    static PlaneIntersectionType PlaneIntersectsSphere(const Plane& plane, const BoundingSphere& sphere);

    /// <summary>
    /// Determines whether there is an intersection between a <see cref="BoundingBox" /> and a <see cref="BoundingBox" />.
    /// </summary>
    /// <param name="box1">The first box to test.</param>
    /// <param name="box2">The second box to test.</param>
    /// <returns>Whether the two objects intersected.</returns>
    static bool BoxIntersectsBox(const BoundingBox& box1, const BoundingBox& box2);

    /// <summary>
    /// Determines whether there is an intersection between a <see cref="BoundingBox" /> and a <see cref="BoundingSphere" />.
    /// </summary>
    /// <param name="box">The box to test.</param>
    /// <param name="sphere">The sphere to test.</param>
    /// <returns>Whether the two objects intersected.</returns>
    static bool BoxIntersectsSphere(const BoundingBox& box, const BoundingSphere& sphere);

    /// <summary>
    /// Determines whether there is an intersection between a <see cref="BoundingSphere" /> and a triangle.
    /// </summary>
    /// <param name="sphere">The sphere to test.</param>
    /// <param name="vertex1">The first vertex of the triangle to test.</param>
    /// <param name="vertex2">The second vertex of the triangle to test.</param>
    /// <param name="vertex3">The third vertex of the triangle to test.</param>
    /// <returns>Whether the two objects intersected.</returns>
    static bool SphereIntersectsTriangle(const BoundingSphere& sphere, const Vector3& vertex1, const Vector3& vertex2, const Vector3& vertex3);

    /// <summary>
    /// Determines whether there is an intersection between a <see cref="BoundingSphere" /> and a
    /// <see cref="BoundingSphere" />.
    /// </summary>
    /// <param name="sphere1">First sphere to test.</param>
    /// <param name="sphere2">Second sphere to test.</param>
    /// <returns>Whether the two objects intersected.</returns>
    static bool SphereIntersectsSphere(const BoundingSphere& sphere1, const BoundingSphere& sphere2);

    /// <summary>
    /// Determines whether a <see cref="BoundingBox" /> contains a point.
    /// </summary>
    /// <param name="box">The box to test.</param>
    /// <param name="point">The point to test.</param>
    /// <returns>The type of containment the two objects have.</returns>
    static ContainmentType BoxContainsPoint(const BoundingBox& box, const Vector3& point);

    /// <summary>
    /// Determines whether a <see cref="BoundingBox" /> contains a <see cref="BoundingBox" />.
    /// </summary>
    /// <param name="box1">The first box to test.</param>
    /// <param name="box2">The second box to test.</param>
    /// <returns>The type of containment the two objects have.</returns>
    static ContainmentType BoxContainsBox(const BoundingBox& box1, const BoundingBox& box2);

    /// <summary>
    /// Determines whether a <see cref="BoundingBox" /> contains a <see cref="BoundingSphere" />.
    /// </summary>
    /// <param name="box">The box to test.</param>
    /// <param name="sphere">The sphere to test.</param>
    /// <returns>The type of containment the two objects have.</returns>
    static ContainmentType BoxContainsSphere(const BoundingBox& box, const BoundingSphere& sphere);

    /// <summary>
    /// Determines whether a <see cref="BoundingSphere" /> contains a point.
    /// </summary>
    /// <param name="sphere">The sphere to test.</param>
    /// <param name="point">The point to test.</param>
    /// <returns>The type of containment the two objects have.</returns>
    static ContainmentType SphereContainsPoint(const BoundingSphere& sphere, const Vector3& point);

    /// <summary>
    /// Determines whether a <see cref="BoundingSphere" /> contains a triangle.
    /// </summary>
    /// <param name="sphere">The sphere to test.</param>
    /// <param name="vertex1">The first vertex of the triangle to test.</param>
    /// <param name="vertex2">The second vertex of the triangle to test.</param>
    /// <param name="vertex3">The third vertex of the triangle to test.</param>
    /// <returns>The type of containment the two objects have.</returns>
    static ContainmentType SphereContainsTriangle(const BoundingSphere& sphere, const Vector3& vertex1, const Vector3& vertex2, const Vector3& vertex3);

    /// <summary>
    /// Determines whether a <see cref="BoundingSphere" /> contains a <see cref="BoundingBox" />.
    /// </summary>
    /// <param name="sphere">The sphere to test.</param>
    /// <param name="box">The box to test.</param>
    /// <returns>The type of containment the two objects have.</returns>
    static ContainmentType SphereContainsBox(const BoundingSphere& sphere, const BoundingBox& box);

    /// <summary>
    /// Determines whether a <see cref="BoundingSphere" /> contains a <see cref="BoundingSphere" />.
    /// </summary>
    /// <param name="sphere1">The first sphere to test.</param>
    /// <param name="sphere2">The second sphere to test.</param>
    /// <returns>The type of containment the two objects have.</returns>
    static ContainmentType SphereContainsSphere(const BoundingSphere& sphere1, const BoundingSphere& sphere2);

    static bool FrustumIntersectsBox(const BoundingFrustum& frustum, const BoundingBox& box);

    static ContainmentType FrustumContainsBox(const BoundingFrustum& frustum, const BoundingBox& box);

    static void GetBoxToPlanePVertexNVertex(const BoundingBox& box, const Vector3& planeNormal, Vector3& p, Vector3& n);

    /// <summary>
    /// Determines whether a line intersects with the other line.
    /// </summary>
    /// <param name="l1p1">The first line point 0.</param>
    /// <param name="l1p2">The first line point 1.</param>
    /// <param name="l2p1">The second line point 0.</param>
    /// <param name="l2p2">The second line point 1.</param>
    /// <returns>True if line intersects with the other line</returns>
    static bool LineIntersectsLine(const Vector2& l1p1, const Vector2& l1p2, const Vector2& l2p1, const Vector2& l2p2);

    /// <summary>
    /// Determines whether a line intersects with the rectangle.
    /// </summary>
    /// <param name="p1">The line point 0.</param>
    /// <param name="p2">The line point 1.</param>
    /// <param name="rect">The rectangle.</param>
    /// <returns>True if line intersects with the rectangle</returns>
    static bool LineIntersectsRect(const Vector2& p1, const Vector2& p2, const Rectangle& rect);

    /// <summary>
    /// Determines whether the given 2D point is inside the specified triangle.
    /// </summary>
    /// <param name="point">The point to check.</param>
    /// <param name="a">The first vertex of the triangle.</param>
    /// <param name="b">The second vertex of the triangle.</param>
    /// <param name="c">The third vertex of the triangle.</param>
    /// <returns><c>true</c> if point is inside the triangle; otherwise, <c>false</c>.</returns>
    static bool IsPointInTriangle(const Vector2& point, const Vector2& a, const Vector2& b, const Vector2& c);
};
