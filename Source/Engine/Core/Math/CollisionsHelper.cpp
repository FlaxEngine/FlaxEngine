// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "CollisionsHelper.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Ray.h"
#include "Rectangle.h"
#include "BoundingBox.h"
#include "BoundingSphere.h"
#include "BoundingFrustum.h"

void CollisionsHelper::ClosestPointPointLine(const Vector2& point, const Vector2& p0, const Vector2& p1, Vector2& result)
{
    const Vector2 p = point - p0;
    Vector2 n = p1 - p0;

    const float length = n.Length();
    if (length < 1e-10)
    {
        // Both points are the same, just give any
        result = p0;
        return;
    }
    n /= length;

    const float dot = Vector2::Dot(n, p);
    if (dot <= 0.0)
    {
        // Before first point
        result = p0;
    }
    else if (dot >= length)
    {
        // After first point
        result = p1;
    }
    else
    {
        // Inside
        result = p0 + n * dot;
    }
}

Vector2 CollisionsHelper::ClosestPointPointLine(const Vector2& point, const Vector2& p0, const Vector2& p1)
{
    Vector2 result;
    ClosestPointPointLine(point, p0, p1, result);
    return result;
}


void CollisionsHelper::ClosestPointPointLine(const Vector3& point, const Vector3& p0, const Vector3& p1, Vector3& result)
{
    const Vector3 p = point - p0;
    Vector3 n = p1 - p0;
    const float length = n.Length();
    if (length < 1e-10)
    {
        result = p0;
        return;
    }
    n /= length;
    const float dot = Vector3::Dot(n, p);
    if (dot <= 0.0)
    {
        result = p0;
        return;
    }
    else if (dot >= length)
    {
        result = p1;
        return;
    }
    result = p0 + n * dot;
}

Vector3 CollisionsHelper::ClosestPointPointLine(const Vector3& point, const Vector3& p0, const Vector3& p1)
{
    Vector3 result;
    ClosestPointPointLine(point, p0, p1, result);
    return result;
}

void CollisionsHelper::ClosestPointPointTriangle(const Vector3& point, const Vector3& vertex1, const Vector3& vertex2, const Vector3& vertex3, Vector3& result)
{
    // Source: Real-Time Collision Detection by Christer Ericson
    // Consterence: Page 136

    // Check if P in vertex region outside A
    const Vector3 ab = vertex2 - vertex1;
    const Vector3 ac = vertex3 - vertex1;
    const Vector3 ap = point - vertex1;

    const float d1 = Vector3::Dot(ab, ap);
    const float d2 = Vector3::Dot(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f)
        result = vertex1; //Barycentric coordinates (1,0,0)

    // Check if P in vertex region outside B
    const Vector3 bp = point - vertex2;
    const float d3 = Vector3::Dot(ab, bp);
    const float d4 = Vector3::Dot(ac, bp);
    if (d3 >= 0.0f && d4 <= d3)
        result = vertex2; // Barycentric coordinates (0,1,0)

    // Check if P in edge region of AB, if so return projection of P onto AB
    const float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
    {
        const float v = d1 / (d1 - d3);
        result = vertex1 + v * ab; //Barycentric coordinates (1-v,v,0)
    }

    //Check if P in vertex region outside C
    const Vector3 cp = point - vertex3;
    const float d5 = Vector3::Dot(ab, cp);
    const float d6 = Vector3::Dot(ac, cp);
    if (d6 >= 0.0f && d5 <= d6)
        result = vertex3; //Barycentric coordinates (0,0,1)

    //Check if P in edge region of AC, if so return projection of P onto AC
    const float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
    {
        const float w = d2 / (d2 - d6);
        result = vertex1 + w * ac; //Barycentric coordinates (1-w,0,w)
    }

    //Check if P in edge region of BC, if so return projection of P onto BC
    const float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && d4 - d3 >= 0.0f && d5 - d6 >= 0.0f)
    {
        const float w = (d4 - d3) / (d4 - d3 + (d5 - d6));
        result = vertex2 + w * (vertex3 - vertex2); //Barycentric coordinates (0,1-w,w)
    }

    //P inside face region. Compute Q through its Barycentric coordinates (u,v,w)
    const float denom = 1.0f / (va + vb + vc);
    const float v2 = vb * denom;
    const float w2 = vc * denom;
    result = vertex1 + ab * v2 + ac * w2; //= u*vertex1 + v*vertex2 + w*vertex3, u = va * denom = 1.0f - v - w
}

Vector3 CollisionsHelper::ClosestPointPointTriangle(const Vector3& point, const Vector3& vertex1, const Vector3& vertex2, const Vector3& vertex3)
{
    Vector3 result;
    ClosestPointPointTriangle(point, vertex1, vertex2, vertex3, result);
    return result;
}

void CollisionsHelper::ClosestPointPlanePoint(const Plane& plane, const Vector3& point, Vector3& result)
{
    // Source: Real-Time Collision Detection by Christer Ericson
    // Consterence: Page 126

    const float dot = Vector3::Dot(plane.Normal, point);
    const float t = dot - plane.D;

    result = point - t * plane.Normal;
}

Vector3 CollisionsHelper::ClosestPointPlanePoint(const Plane& plane, const Vector3& point)
{
    Vector3 result;
    ClosestPointPlanePoint(plane, point, result);
    return result;
}

void CollisionsHelper::ClosestPointBoxPoint(const BoundingBox& box, const Vector3& point, Vector3& result)
{
    // Source: Real-Time Collision Detection by Christer Ericson
    // Consterence: Page 130

    Vector3 temp;
    Vector3::Max(point, box.Minimum, temp);
    Vector3::Min(temp, box.Maximum, result);
}

Vector3 CollisionsHelper::ClosestPointBoxPoint(const BoundingBox& box, const Vector3& point)
{
    Vector3 result;
    ClosestPointBoxPoint(box, point, result);
    return result;
}

void CollisionsHelper::ClosestPointRectanglePoint(const Rectangle& rect, const Vector2& point, Vector2& result)
{
    Vector2 temp, end;
    Vector2::Add(rect.Location, rect.Size, end);
    Vector2::Max(point, rect.Location, temp);
    Vector2::Min(temp, end, result);
}

Vector2 CollisionsHelper::ClosestPointRectanglePoint(const Rectangle& rect, const Vector2& point)
{
    Vector2 result;
    ClosestPointRectanglePoint(rect, point, result);
    return result;
}

void CollisionsHelper::ClosestPointSpherePoint(const BoundingSphere& sphere, const Vector3& point, Vector3& result)
{
    // Source: Jorgy343
    // Consterence: None

    //Get the unit direction from the sphere's center to the point.
    Vector3::Subtract(point, sphere.Center, result);
    result.Normalize();

    //Multiply the unit direction by the sphere's radius to get a vector
    //the length of the sphere.
    result *= sphere.Radius;

    //Add the sphere's center to the direction to get a point on the sphere.
    result += sphere.Center;
}

Vector3 CollisionsHelper::ClosestPointSpherePoint(const BoundingSphere& sphere, const Vector3& point)
{
    Vector3 result;
    ClosestPointSpherePoint(sphere, point, result);
    return result;
}

void CollisionsHelper::ClosestPointSphereSphere(const BoundingSphere& sphere1, const BoundingSphere& sphere2, Vector3& result)
{
    // Source: Jorgy343
    // Consterence: None

    //Get the unit direction from the first sphere's center to the second sphere's center.
    Vector3::Subtract(sphere2.Center, sphere1.Center, result);
    result.Normalize();

    //Multiply the unit direction by the first sphere's radius to get a vector
    //the length of the first sphere.
    result *= sphere1.Radius;

    //Add the first sphere's center to the direction to get a point on the first sphere.
    result += sphere1.Center;
}

Vector3 CollisionsHelper::ClosestPointSphereSphere(const BoundingSphere& sphere1, const BoundingSphere& sphere2)
{
    Vector3 result;
    ClosestPointSphereSphere(sphere1, sphere2, result);
    return result;
}

float CollisionsHelper::DistancePlanePoint(const Plane& plane, const Vector3& point)
{
    // Source: Real-Time Collision Detection by Christer Ericson
    // Consterence: Page 127

    const float dot = Vector3::Dot(plane.Normal, point);
    return dot - plane.D;
}

// Determines the distance between a Bounding Box and a point
// @param box The box to test
// @param point The point to test
// @returns The distance between the two objects
float CollisionsHelper::DistanceBoxPoint(const BoundingBox& box, const Vector3& point)
{
    // Source: Real-Time Collision Detection by Christer Ericson
    // Consterence: Page 131

    float distance = 0.0f;

    if (point.X < box.Minimum.X)
        distance += (box.Minimum.X - point.X) * (box.Minimum.X - point.X);
    if (point.X > box.Maximum.X)
        distance += (point.X - box.Maximum.X) * (point.X - box.Maximum.X);

    if (point.Y < box.Minimum.Y)
        distance += (box.Minimum.Y - point.Y) * (box.Minimum.Y - point.Y);
    if (point.Y > box.Maximum.Y)
        distance += (point.Y - box.Maximum.Y) * (point.Y - box.Maximum.Y);

    if (point.Z < box.Minimum.Z)
        distance += (box.Minimum.Z - point.Z) * (box.Minimum.Z - point.Z);
    if (point.Z > box.Maximum.Z)
        distance += (point.Z - box.Maximum.Z) * (point.Z - box.Maximum.Z);

    return Math::Sqrt(distance);
}

// Determines the distance between a Bounding Box and a Bounding Box
// @param box1 The first box to test
// @param box2 The second box to test
// @returns The distance between the two objects
float CollisionsHelper::DistanceBoxBox(const BoundingBox& box1, const BoundingBox& box2)
{
    // Source:
    // Consterence:

    float distance = 0.0f;

    // Distance for X
    if (box1.Minimum.X > box2.Maximum.X)
    {
        const float delta = box2.Maximum.X - box1.Minimum.X;
        distance += delta * delta;
    }
    else if (box2.Minimum.X > box1.Maximum.X)
    {
        const float delta = box1.Maximum.X - box2.Minimum.X;
        distance += delta * delta;
    }

    // Distance for Y
    if (box1.Minimum.Y > box2.Maximum.Y)
    {
        const float delta = box2.Maximum.Y - box1.Minimum.Y;
        distance += delta * delta;
    }
    else if (box2.Minimum.Y > box1.Maximum.Y)
    {
        const float delta = box1.Maximum.Y - box2.Minimum.Y;
        distance += delta * delta;
    }

    // Distance for Z
    if (box1.Minimum.Z > box2.Maximum.Z)
    {
        const float delta = box2.Maximum.Z - box1.Minimum.Z;
        distance += delta * delta;
    }
    else if (box2.Minimum.Z > box1.Maximum.Z)
    {
        const float delta = box1.Maximum.Z - box2.Minimum.Z;
        distance += delta * delta;
    }

    return Math::Sqrt(distance);
}

// Determines the distance between a Bounding Sphere and a point
// @param sphere The sphere to test
// @param point The point to test
// @returns The distance between the two objects
float CollisionsHelper::DistanceSpherePoint(const BoundingSphere& sphere, const Vector3& point)
{
    // Source: Jorgy343
    // Consterence: None

    float distance = Vector3::Distance(sphere.Center, point);
    distance -= sphere.Radius;

    return Math::Max(distance, 0.0f);
}

// Determines the distance between a Bounding Sphere and a Bounding Sphere
// @param sphere1 The first sphere to test
// @param sphere2 The second sphere to test
// @returns The distance between the two objects
float CollisionsHelper::DistanceSphereSphere(const BoundingSphere& sphere1, const BoundingSphere& sphere2)
{
    // Source: Jorgy343
    // Consterence: None

    float distance = Vector3::Distance(sphere1.Center, sphere2.Center);
    distance -= sphere1.Radius + sphere2.Radius;

    return Math::Max(distance, 0.0f);
}

// Determines whether there is an intersection between a Ray and a point
// @param ray The ray to test
// @param point The point to test
// @returns Whether the two objects intersect
bool CollisionsHelper::RayIntersectsPoint(const Ray& ray, const Vector3& point)
{
    // Source: RayIntersectsSphere
    // Consterence: None

    Vector3 m;
    Vector3::Subtract(ray.Position, point, m);

    //Same thing as RayIntersectsSphere except that the radius of the sphere (point)
    //is the epsilon for zero.
    const float b = Vector3::Dot(m, ray.Direction);
    const float c = Vector3::Dot(m, m) - ZeroTolerance;

    if (c > 0.0f && b > 0.0f)
        return false;

    const float discriminant = b * b - c;

    if (discriminant < 0.0f)
        return false;

    return true;
}

// Determines whether there is an intersection between a Ray and a Ray
// @param ray1 The first ray to test
// @param ray2 The second ray to test
// @param point When the method completes, contains the point of intersection, or Vector3::Zero if there was no intersection
// @returns Whether the two objects intersect
bool CollisionsHelper::RayIntersectsRay(const Ray& ray1, const Ray& ray2, Vector3& point)
{
    // Source: Real-Time Rendering, Third Edition
    // Consterence: Page 780

    Vector3 cross;

    Vector3::Cross(ray1.Direction, ray2.Direction, cross);
    float denominator = cross.Length();

    // Lines are parallel
    if (Math::IsZero(denominator))
    {
        // Lines are parallel and on top of each other
        if (Math::NearEqual(ray2.Position.X, ray1.Position.X) &&
            Math::NearEqual(ray2.Position.Y, ray1.Position.Y) &&
            Math::NearEqual(ray2.Position.Z, ray1.Position.Z))
        {
            point = Vector3::Zero;
            return true;
        }
    }

    denominator = denominator * denominator;

    // 3x3 matrix for the first ray
    const float m11 = ray2.Position.X - ray1.Position.X;
    const float m12 = ray2.Position.Y - ray1.Position.Y;
    const float m13 = ray2.Position.Z - ray1.Position.Z;
    float m21 = ray2.Direction.X;
    float m22 = ray2.Direction.Y;
    float m23 = ray2.Direction.Z;
    const float m31 = cross.X;
    const float m32 = cross.Y;
    const float m33 = cross.Z;

    // Determinant of first matrix
    const float dets =
            m11 * m22 * m33 +
            m12 * m23 * m31 +
            m13 * m21 * m32 -
            m11 * m23 * m32 -
            m12 * m21 * m33 -
            m13 * m22 * m31;

    // 3x3 matrix for the second ray
    m21 = ray1.Direction.X;
    m22 = ray1.Direction.Y;
    m23 = ray1.Direction.Z;

    // Determinant of the second matrix
    const float dett =
            m11 * m22 * m33 +
            m12 * m23 * m31 +
            m13 * m21 * m32 -
            m11 * m23 * m32 -
            m12 * m21 * m33 -
            m13 * m22 * m31;

    // t values of the point of intersection
    const float s = dets / denominator;
    const float t = dett / denominator;

    // The points of intersection.
    const Vector3 point1 = ray1.Position + s * ray1.Direction;
    const Vector3 point2 = ray2.Position + t * ray2.Direction;

    // If the points are not equal, no intersection has occurred
    if (!Math::NearEqual(point2.X, point1.X) ||
        !Math::NearEqual(point2.Y, point1.Y) ||
        !Math::NearEqual(point2.Z, point1.Z))
    {
        point = Vector3::Zero;
        return false;
    }

    point = point1;
    return true;
}

bool CollisionsHelper::RayIntersectsPlane(const Ray& ray, const Plane& plane, float& distance)
{
    // Source: Real-Time Collision Detection by Christer Ericson
    // Consterence: Page 175

    const float direction = Vector3::Dot(plane.Normal, ray.Direction);

    if (Math::IsZero(direction))
    {
        distance = 0.0f;
        return false;
    }

    const float position = Vector3::Dot(plane.Normal, ray.Position);
    distance = (-plane.D - position) / direction;

    if (distance < Plane::DistanceEpsilon)
    {
        distance = 0.0f;
        return false;
    }

    return true;
}

bool CollisionsHelper::RayIntersectsPlane(const Ray& ray, const Plane& plane, Vector3& point)
{
    // Source: Real-Time Collision Detection by Christer Ericson
    // Consterence: Page 175

    float distance;
    if (!RayIntersectsPlane(ray, plane, distance))
    {
        point = Vector3::Zero;
        return false;
    }

    point = ray.Position + ray.Direction * distance;
    return true;
}

bool CollisionsHelper::RayIntersectsTriangle(const Ray& ray, const Vector3& vertex1, const Vector3& vertex2, const Vector3& vertex3, float& distance)
{
    /*
    //bool Collision::RayIntersectsTriangle(const Ray& ray, const Vector3& vertex1, const Vector3& vertex2, const Vector3& vertex3, float& distance)
bool Collision::RayIntersectsTriangle(const Ray& ray, const Vector3& a, const Vector3& b, const Vector3& c, float& distance)
{
    // todo: optimize this
    Vector3 p = ray.Position;

    //int IntersectSegmentTriangle(Point p, Point q, Point a, Point b, Point c, float &u, float &v, float &w, float &t)

    Vector3 ab = b - a;
    Vector3 ac = c - a;
    Vector3 qp = ray.Direction;

    // Compute triangle normal. Can be precalculated or cached if
    // intersecting multiple segments against the same triangle
    Vector3 n = Vector3::Cross(ab, ac);

    // Compute denominator d. If d <= 0, segment is parallel to or points
    // away from triangle, so exit early
    float d = Vector3::Dot(qp, n);
    if (d <= 0.0f)
        return false;

    // Compute intersection t value of pq with plane of triangle. A ray
    // intersects iff 0 <= t. Segment intersects iff 0 <= t <= 1. Delay
    // dividing by d until intersection has been found to pierce triangle
    Vector3 ap = p - a;
    float t = Vector3::Dot(ap, n);
    if (t < 0.0f || t > d)
        return false;
    // For segment; exclude this code line for a ray test
    
    // Compute barycentric coordinate components and test if within bounds 
    Vector3 e = Vector3::Cross(qp, ap);
    Vector3 v = Vector3::Dot(ac, e);
    if (v < 0.0f || v > d)
        return false;
    Vector3 w = -Vector3::Dot(ab, e);
    if (w < 0.0f || v + w > d)
        return false;

    // Segment/ray intersects triangle. Perform delayed division and
    // compute the last barycentric coordinate component
    float ood = 1.0f / d;
    t *= ood;
    v *= ood;
    w *= ood;
    Vector3 u = Vector3::One - v - w;
    return true;
*/

    // Source: Fast Minimum Storage Ray / Triangle Intersection
    // Consterence: http://www.cs.virginia.edu/~gfx/Courses/2003/ImageSynthesis/papers/Acceleration/Fast%20MinimumStorage%20RayTriangle%20Intersection.pdf

    distance = 0.0f;

    // Compute vectors along two edges of the triangle
    Vector3 edge1, edge2;

    // Edge 1
    edge1.X = vertex2.X - vertex1.X;
    edge1.Y = vertex2.Y - vertex1.Y;
    edge1.Z = vertex2.Z - vertex1.Z;

    // Edge2
    edge2.X = vertex3.X - vertex1.X;
    edge2.Y = vertex3.Y - vertex1.Y;
    edge2.Z = vertex3.Z - vertex1.Z;

    // Cross product of ray direction and edge2 - first part of determinant
    Vector3 directionCrossEdge2;
    directionCrossEdge2.X = ray.Direction.Y * edge2.Z - ray.Direction.Z * edge2.Y;
    directionCrossEdge2.Y = ray.Direction.Z * edge2.X - ray.Direction.X * edge2.Z;
    directionCrossEdge2.Z = ray.Direction.X * edge2.Y - ray.Direction.Y * edge2.X;

    // Compute the determinant (dot product of edge1 and the first part of determinant)
    const float determinant = edge1.X * directionCrossEdge2.X + edge1.Y * directionCrossEdge2.Y + edge1.Z * directionCrossEdge2.Z;

    // If the ray is parallel to the triangle plane, there is no collision
    // This also means that we are not culling, the ray may hit both the
    // back and the front of the triangle.
    if (Math::IsZero(determinant))
    {
        return false;
    }

    const float inverseDeterminant = 1.0f / determinant;

    // Calculate the U parameter of the intersection point
    Vector3 distanceVector;
    distanceVector.X = ray.Position.X - vertex1.X;
    distanceVector.Y = ray.Position.Y - vertex1.Y;
    distanceVector.Z = ray.Position.Z - vertex1.Z;

    float triangleU = distanceVector.X * directionCrossEdge2.X + distanceVector.Y * directionCrossEdge2.Y + distanceVector.Z * directionCrossEdge2.Z;
    triangleU *= inverseDeterminant;

    // Make sure it is inside the triangle
    if (triangleU < 0.0f || triangleU > 1.0f)
    {
        return false;
    }

    // Calculate the V parameter of the intersection point
    Vector3 distanceCrossEdge1;
    distanceCrossEdge1.X = distanceVector.Y * edge1.Z - distanceVector.Z * edge1.Y;
    distanceCrossEdge1.Y = distanceVector.Z * edge1.X - distanceVector.X * edge1.Z;
    distanceCrossEdge1.Z = distanceVector.X * edge1.Y - distanceVector.Y * edge1.X;

    float triangleV = ray.Direction.X * distanceCrossEdge1.X + ray.Direction.Y * distanceCrossEdge1.Y + ray.Direction.Z * distanceCrossEdge1.Z;
    triangleV *= inverseDeterminant;

    // Make sure it is inside the triangle
    if (triangleV < 0.0f || triangleU + triangleV > 1.0f)
    {
        return false;
    }

    // Compute the distance along the ray to the triangle
    float rayDistance = edge2.X * distanceCrossEdge1.X + edge2.Y * distanceCrossEdge1.Y + edge2.Z * distanceCrossEdge1.Z;
    rayDistance *= inverseDeterminant;

    // Check if the triangle is behind the ray origin
    if (rayDistance < 0.0f)
    {
        return false;
    }

    distance = rayDistance;
    return true;
}

bool CollisionsHelper::RayIntersectsTriangle(const Ray& ray, const Vector3& vertex1, const Vector3& vertex2, const Vector3& vertex3, float& distance, Vector3& normal)
{
    // Source: Fast Minimum Storage Ray / Triangle Intersection
    // Consterence: http://www.cs.virginia.edu/~gfx/Courses/2003/ImageSynthesis/papers/Acceleration/Fast%20MinimumStorage%20RayTriangle%20Intersection.pdf

    distance = 0.0f;
    normal = Vector3::Up;

    // Compute vectors along two edges of the triangle
    Vector3 edge1, edge2;

    // Edge 1
    edge1.X = vertex2.X - vertex1.X;
    edge1.Y = vertex2.Y - vertex1.Y;
    edge1.Z = vertex2.Z - vertex1.Z;

    // Edge2
    edge2.X = vertex3.X - vertex1.X;
    edge2.Y = vertex3.Y - vertex1.Y;
    edge2.Z = vertex3.Z - vertex1.Z;

    // Cross product of ray direction and edge2 - first part of determinant
    Vector3 directionCrossEdge2;
    directionCrossEdge2.X = ray.Direction.Y * edge2.Z - ray.Direction.Z * edge2.Y;
    directionCrossEdge2.Y = ray.Direction.Z * edge2.X - ray.Direction.X * edge2.Z;
    directionCrossEdge2.Z = ray.Direction.X * edge2.Y - ray.Direction.Y * edge2.X;

    // Compute the determinant (dot product of edge1 and the first part of determinant)
    const float determinant = edge1.X * directionCrossEdge2.X + edge1.Y * directionCrossEdge2.Y + edge1.Z * directionCrossEdge2.Z;

    // If the ray is parallel to the triangle plane, there is no collision
    // This also means that we are not culling, the ray may hit both the
    // back and the front of the triangle.
    if (Math::IsZero(determinant))
    {
        return false;
    }

    const float inverseDeterminant = 1.0f / determinant;

    // Calculate the U parameter of the intersection point
    Vector3 distanceVector;
    distanceVector.X = ray.Position.X - vertex1.X;
    distanceVector.Y = ray.Position.Y - vertex1.Y;
    distanceVector.Z = ray.Position.Z - vertex1.Z;

    float triangleU = distanceVector.X * directionCrossEdge2.X + distanceVector.Y * directionCrossEdge2.Y + distanceVector.Z * directionCrossEdge2.Z;
    triangleU *= inverseDeterminant;

    // Make sure it is inside the triangle
    if (triangleU < 0.0f || triangleU > 1.0f)
    {
        return false;
    }

    // Calculate the V parameter of the intersection point
    Vector3 distanceCrossEdge1;
    distanceCrossEdge1.X = distanceVector.Y * edge1.Z - distanceVector.Z * edge1.Y;
    distanceCrossEdge1.Y = distanceVector.Z * edge1.X - distanceVector.X * edge1.Z;
    distanceCrossEdge1.Z = distanceVector.X * edge1.Y - distanceVector.Y * edge1.X;

    float triangleV = ray.Direction.X * distanceCrossEdge1.X + ray.Direction.Y * distanceCrossEdge1.Y + ray.Direction.Z * distanceCrossEdge1.Z;
    triangleV *= inverseDeterminant;

    // Make sure it is inside the triangle
    if (triangleV < 0.0f || triangleU + triangleV > 1.0f)
    {
        return false;
    }

    // Compute the distance along the ray to the triangle
    float rayDistance = edge2.X * distanceCrossEdge1.X + edge2.Y * distanceCrossEdge1.Y + edge2.Z * distanceCrossEdge1.Z;
    rayDistance *= inverseDeterminant;

    // Check if the triangle is behind the ray origin
    if (rayDistance < 0.0f)
    {
        return false;
    }

    // Calculate hit normal (handle both triangle sides)
    const Vector3 vd0 = vertex2 - vertex1;
    const Vector3 vd1 = vertex3 - vertex1;
    const Vector3 n1 = Vector3::Normalize(vd0 ^ vd1);
    const Vector3 n2 = Vector3::Normalize(vd1 ^ vd0);
    // TODO: optimize it
    const float BiasAdjust = 0.01f;
    const Vector3 center = (vertex1 + vertex2 + vertex3) * 0.333333f;
    if (Vector3::DistanceSquared(center + n1 * BiasAdjust, ray.Position) < Vector3::DistanceSquared(center + n2 * BiasAdjust, ray.Position))
        normal = n1;
    else
        normal = n2;
    //*normal = Vector3::Normalize(vd0 ^ vd1) * -1;

    distance = rayDistance;

    return true;
}

bool CollisionsHelper::RayIntersectsTriangle(const Ray& ray, const Vector3& vertex1, const Vector3& vertex2, const Vector3& vertex3, Vector3& point)
{
    float distance;
    if (!RayIntersectsTriangle(ray, vertex1, vertex2, vertex3, distance))
    {
        point = Vector3::Zero;
        return false;
    }

    point = ray.Position + ray.Direction * distance;
    return true;
}

bool CollisionsHelper::RayIntersectsBox(const Ray& ray, const BoundingBox& box, float& distance)
{
    // Source: Real-Time Collision Detection by Christer Ericson
    // Consterence: Page 179

    distance = 0.0f;
    float tmax = MAX_float;

    if (Math::IsZero(ray.Direction.X))
    {
        if (ray.Position.X < box.Minimum.X || ray.Position.X > box.Maximum.X)
        {
            distance = 0.0f;
            return false;
        }
    }
    else
    {
        const float inverse = 1.0f / ray.Direction.X;
        float t1 = (box.Minimum.X - ray.Position.X) * inverse;
        float t2 = (box.Maximum.X - ray.Position.X) * inverse;

        if (t1 > t2)
        {
            const float temp = t1;
            t1 = t2;
            t2 = temp;
        }

        distance = Math::Max(t1, distance);
        tmax = Math::Min(t2, tmax);

        if (distance > tmax)
        {
            distance = 0.0f;
            return false;
        }
    }

    if (Math::IsZero(ray.Direction.Y))
    {
        if (ray.Position.Y < box.Minimum.Y || ray.Position.Y > box.Maximum.Y)
        {
            distance = 0.0f;
            return false;
        }
    }
    else
    {
        const float inverse = 1.0f / ray.Direction.Y;
        float t1 = (box.Minimum.Y - ray.Position.Y) * inverse;
        float t2 = (box.Maximum.Y - ray.Position.Y) * inverse;

        if (t1 > t2)
        {
            const float temp = t1;
            t1 = t2;
            t2 = temp;
        }

        distance = Math::Max(t1, distance);
        tmax = Math::Min(t2, tmax);

        if (distance > tmax)
        {
            distance = 0.0f;
            return false;
        }
    }

    if (Math::IsZero(ray.Direction.Z))
    {
        if (ray.Position.Z < box.Minimum.Z || ray.Position.Z > box.Maximum.Z)
        {
            distance = 0.0f;
            return false;
        }
    }
    else
    {
        const float inverse = 1.0f / ray.Direction.Z;
        float t1 = (box.Minimum.Z - ray.Position.Z) * inverse;
        float t2 = (box.Maximum.Z - ray.Position.Z) * inverse;

        if (t1 > t2)
        {
            const float temp = t1;
            t1 = t2;
            t2 = temp;
        }

        distance = Math::Max(t1, distance);
        tmax = Math::Min(t2, tmax);

        if (distance > tmax)
        {
            distance = 0.0f;
            return false;
        }
    }

    return true;
}

bool CollisionsHelper::RayIntersectsBox(const Ray& ray, const BoundingBox& box, float& distance, Vector3& normal)
{
    if (!RayIntersectsBox(ray, box, distance))
    {
        normal = Vector3::Up;
        return false;
    }

    // TODO: optimize this

    const Vector3 point = ray.Position + ray.Direction * distance;
    Vector3 size;
    Vector3::Subtract(box.Maximum, box.Minimum, size);
    const Vector3 center = box.Minimum + size * 0.5f;
    const Vector3 localPoint = point - center;

    float dMin = MAX_float;

    float d = Math::Abs(size.X - Math::Abs(localPoint.X));
    if (d < dMin)
    {
        dMin = d;
        normal = Vector3(Math::Sign(localPoint.X), 0, 0);
    }

    d = Math::Abs(size.Y - Math::Abs(localPoint.Y));
    if (d < dMin)
    {
        dMin = d;
        normal = Vector3(0, Math::Sign(localPoint.Y), 0);
    }

    d = Math::Abs(size.Z - Math::Abs(localPoint.Z));
    if (d < dMin)
    {
        dMin = d;
        normal = Vector3(0, 0, Math::Sign(localPoint.Z));
    }

    return true;
}

bool CollisionsHelper::RayIntersectsBox(const Ray& ray, const BoundingBox& box, Vector3& point)
{
    float distance;
    if (!RayIntersectsBox(ray, box, distance))
    {
        point = Vector3::Zero;
        return false;
    }

    point = ray.Position + ray.Direction * distance;
    return true;
}

bool CollisionsHelper::RayIntersectsSphere(const Ray& ray, const BoundingSphere& sphere, float& distance)
{
    // Source: Real-Time Collision Detection by Christer Ericson
    // Consterence: Page 177

    Vector3 m;
    Vector3::Subtract(ray.Position, sphere.Center, m);

    const float b = Vector3::Dot(m, ray.Direction);
    const float c = Vector3::Dot(m, m) - sphere.Radius * sphere.Radius;

    if (c > 0.0f && b > 0.0f)
    {
        distance = 0.0f;
        return false;
    }

    const float discriminant = b * b - c;

    if (discriminant < 0.0f)
    {
        distance = 0.0f;
        return false;
    }

    distance = -b - Math::Sqrt(discriminant);

    if (distance < 0.0f)
        distance = 0.0f;

    return true;
}

bool CollisionsHelper::RayIntersectsSphere(const Ray& ray, const BoundingSphere& sphere, float& distance, Vector3& normal)
{
    if (!RayIntersectsSphere(ray, sphere, distance))
    {
        normal = Vector3::Up;
        return false;
    }

    const Vector3 point = ray.Position + ray.Direction * distance;
    normal = Vector3::Normalize(point - sphere.Center);
    return true;
}

bool CollisionsHelper::RayIntersectsSphere(const Ray& ray, const BoundingSphere& sphere, Vector3& point)
{
    float distance;
    if (!RayIntersectsSphere(ray, sphere, distance))
    {
        point = Vector3::Zero;
        return false;
    }

    point = ray.Position + ray.Direction * distance;
    return true;
}

PlaneIntersectionType CollisionsHelper::PlaneIntersectsPoint(const Plane& plane, const Vector3& point)
{
    float distance = Vector3::Dot(plane.Normal, point);
    distance += plane.D;

    if (distance > Plane::DistanceEpsilon)
        return PlaneIntersectionType::Front;

    if (distance < Plane::DistanceEpsilon)
        return PlaneIntersectionType::Back;

    return PlaneIntersectionType::Intersecting;
}

bool CollisionsHelper::PlaneIntersectsPlane(const Plane& plane1, const Plane& plane2)
{
    Vector3 direction;
    Vector3::Cross(plane1.Normal, plane2.Normal, direction);

    // If direction is the zero vector, the planes are parallel and possibly
    // coincident. It is not an intersection. The dot product will tell us.
    const float denominator = Vector3::Dot(direction, direction);

    return !Math::IsZero(denominator);
}

bool CollisionsHelper::PlaneIntersectsPlane(const Plane& plane1, const Plane& plane2, Ray& line)
{
    // Source: Real-Time Collision Detection by Christer Ericson
    // Consterence: Page 207

    Vector3 direction;
    Vector3::Cross(plane1.Normal, plane2.Normal, direction);

    // If direction is the zero vector, the planes are parallel and possibly coincident. It is not an intersection. The dot product will tell us.
    const float denominator = Vector3::Dot(direction, direction);

    // We assume the planes are normalized, theconstore the denominator
    // only serves as a parallel and coincident check. Otherwise we need
    // to divide the point by the denominator.
    if (Math::IsZero(denominator))
    {
        return false;
    }

    Vector3 point;
    const Vector3 temp = plane1.D * plane2.Normal - plane2.D * plane1.Normal;
    Vector3::Cross(temp, direction, point);

    line.Position = point;
    line.Direction = direction;
    line.Direction.Normalize();

    return true;
}

PlaneIntersectionType CollisionsHelper::PlaneIntersectsTriangle(const Plane& plane, const Vector3& vertex1, const Vector3& vertex2, const Vector3& vertex3)
{
    // Source: Real-Time Collision Detection by Christer Ericson
    // Consterence: Page 207

    const PlaneIntersectionType test1 = PlaneIntersectsPoint(plane, vertex1);
    const PlaneIntersectionType test2 = PlaneIntersectsPoint(plane, vertex2);
    const PlaneIntersectionType test3 = PlaneIntersectsPoint(plane, vertex3);

    if (test1 == PlaneIntersectionType::Front && test2 == PlaneIntersectionType::Front && test3 == PlaneIntersectionType::Front)
        return PlaneIntersectionType::Front;

    if (test1 == PlaneIntersectionType::Back && test2 == PlaneIntersectionType::Back && test3 == PlaneIntersectionType::Back)
        return PlaneIntersectionType::Back;

    return PlaneIntersectionType::Intersecting;
}

PlaneIntersectionType CollisionsHelper::PlaneIntersectsBox(const Plane& plane, const BoundingBox& box)
{
    // Source: Real-Time Collision Detection by Christer Ericson
    // Consterence: Page 161

    Vector3 min;
    Vector3 max;

    max.X = plane.Normal.X >= 0.0f ? box.Minimum.X : box.Maximum.X;
    max.Y = plane.Normal.Y >= 0.0f ? box.Minimum.Y : box.Maximum.Y;
    max.Z = plane.Normal.Z >= 0.0f ? box.Minimum.Z : box.Maximum.Z;
    min.X = plane.Normal.X >= 0.0f ? box.Maximum.X : box.Minimum.X;
    min.Y = plane.Normal.Y >= 0.0f ? box.Maximum.Y : box.Minimum.Y;
    min.Z = plane.Normal.Z >= 0.0f ? box.Maximum.Z : box.Minimum.Z;

    float distance = Vector3::Dot(plane.Normal, max);

    if (distance + plane.D > Plane::DistanceEpsilon)
        return PlaneIntersectionType::Front;

    distance = Vector3::Dot(plane.Normal, min);

    if (distance + plane.D < Plane::DistanceEpsilon)
        return PlaneIntersectionType::Back;

    return PlaneIntersectionType::Intersecting;
}

PlaneIntersectionType CollisionsHelper::PlaneIntersectsSphere(const Plane& plane, const BoundingSphere& sphere)
{
    // Source: Real-Time Collision Detection by Christer Ericson
    // Consterence: Page 160

    float distance = Vector3::Dot(plane.Normal, sphere.Center);
    distance += plane.D;

    if (distance > sphere.Radius)
        return PlaneIntersectionType::Front;

    if (distance < -sphere.Radius)
        return PlaneIntersectionType::Back;

    return PlaneIntersectionType::Intersecting;
}

bool CollisionsHelper::BoxIntersectsBox(const BoundingBox& box1, const BoundingBox& box2)
{
    if (box1.Minimum.X > box2.Maximum.X || box2.Minimum.X > box1.Maximum.X)
        return false;

    if (box1.Minimum.Y > box2.Maximum.Y || box2.Minimum.Y > box1.Maximum.Y)
        return false;

    if (box1.Minimum.Z > box2.Maximum.Z || box2.Minimum.Z > box1.Maximum.Z)
        return false;

    return true;
}

bool CollisionsHelper::BoxIntersectsSphere(const BoundingBox& box, const BoundingSphere& sphere)
{
    // Source: Real-Time Collision Detection by Christer Ericson
    // Consterence: Page 166

    Vector3 vector;
    Vector3::Clamp(sphere.Center, box.Minimum, box.Maximum, vector);
    const float distance = Vector3::DistanceSquared(sphere.Center, vector);

    return distance <= sphere.Radius * sphere.Radius;
}

bool CollisionsHelper::SphereIntersectsTriangle(const BoundingSphere& sphere, const Vector3& vertex1, const Vector3& vertex2, const Vector3& vertex3)
{
    // Source: Real-Time Collision Detection by Christer Ericson
    // Consterence: Page 167

    Vector3 point;
    ClosestPointPointTriangle(sphere.Center, vertex1, vertex2, vertex3, point);
    const Vector3 v = point - sphere.Center;

    const float dot = Vector3::Dot(v, v);

    return dot <= sphere.Radius * sphere.Radius;
}

bool CollisionsHelper::SphereIntersectsSphere(const BoundingSphere& sphere1, const BoundingSphere& sphere2)
{
    const float radiisum = sphere1.Radius + sphere2.Radius;
    return Vector3::DistanceSquared(sphere1.Center, sphere2.Center) <= radiisum * radiisum;
}

ContainmentType CollisionsHelper::BoxContainsPoint(const BoundingBox& box, const Vector3& point)
{
    if (box.Minimum.X <= point.X && box.Maximum.X >= point.X &&
        box.Minimum.Y <= point.Y && box.Maximum.Y >= point.Y &&
        box.Minimum.Z <= point.Z && box.Maximum.Z >= point.Z)
    {
        return ContainmentType::Contains;
    }

    return ContainmentType::Disjoint;
}

ContainmentType CollisionsHelper::BoxContainsBox(const BoundingBox& box1, const BoundingBox& box2)
{
    if (box1.Maximum.X < box2.Minimum.X || box1.Minimum.X > box2.Maximum.X)
        return ContainmentType::Disjoint;

    if (box1.Maximum.Y < box2.Minimum.Y || box1.Minimum.Y > box2.Maximum.Y)
        return ContainmentType::Disjoint;

    if (box1.Maximum.Z < box2.Minimum.Z || box1.Minimum.Z > box2.Maximum.Z)
        return ContainmentType::Disjoint;

    if (box1.Minimum.X <= box2.Minimum.X && (box2.Maximum.X <= box1.Maximum.X &&
            box1.Minimum.Y <= box2.Minimum.Y && box2.Maximum.Y <= box1.Maximum.Y) &&
        box1.Minimum.Z <= box2.Minimum.Z && box2.Maximum.Z <= box1.Maximum.Z)
    {
        return ContainmentType::Contains;
    }

    return ContainmentType::Intersects;
}

ContainmentType CollisionsHelper::BoxContainsSphere(const BoundingBox& box, const BoundingSphere& sphere)
{
    Vector3 vector;
    Vector3::Clamp(sphere.Center, box.Minimum, box.Maximum, vector);
    const float distance = Vector3::DistanceSquared(sphere.Center, vector);

    if (distance > sphere.Radius * sphere.Radius)
        return ContainmentType::Disjoint;

    if (box.Minimum.X + sphere.Radius <= sphere.Center.X && sphere.Center.X <= box.Maximum.X - sphere.Radius && (box.Maximum.X - box.Minimum.X > sphere.Radius &&
        box.Minimum.Y + sphere.Radius <= sphere.Center.Y) && (sphere.Center.Y <= box.Maximum.Y - sphere.Radius && box.Maximum.Y - box.Minimum.Y > sphere.Radius &&
        (box.Minimum.Z + sphere.Radius <= sphere.Center.Z && sphere.Center.Z <= box.Maximum.Z - sphere.Radius && box.Maximum.Z - box.Minimum.Z > sphere.Radius)))
    {
        return ContainmentType::Contains;
    }

    return ContainmentType::Intersects;
}

ContainmentType CollisionsHelper::SphereContainsPoint(const BoundingSphere& sphere, const Vector3& point)
{
    if (Vector3::DistanceSquared(point, sphere.Center) <= sphere.Radius * sphere.Radius)
        return ContainmentType::Contains;

    return ContainmentType::Disjoint;
}

ContainmentType CollisionsHelper::SphereContainsTriangle(const BoundingSphere& sphere, const Vector3& vertex1, const Vector3& vertex2, const Vector3& vertex3)
{
    // Source: Jorgy343
    // Consterence: None

    const ContainmentType test1 = SphereContainsPoint(sphere, vertex1);
    const ContainmentType test2 = SphereContainsPoint(sphere, vertex2);
    const ContainmentType test3 = SphereContainsPoint(sphere, vertex3);

    if (test1 == ContainmentType::Contains && test2 == ContainmentType::Contains && test3 == ContainmentType::Contains)
        return ContainmentType::Contains;

    if (SphereIntersectsTriangle(sphere, vertex1, vertex2, vertex3))
        return ContainmentType::Intersects;

    return ContainmentType::Disjoint;
}

ContainmentType CollisionsHelper::SphereContainsBox(const BoundingSphere& sphere, const BoundingBox& box)
{
    Vector3 vector;

    if (!BoxIntersectsSphere(box, sphere))
        return ContainmentType::Disjoint;

    const float radiussquared = sphere.Radius * sphere.Radius;
    vector.X = sphere.Center.X - box.Minimum.X;
    vector.Y = sphere.Center.Y - box.Maximum.Y;
    vector.Z = sphere.Center.Z - box.Maximum.Z;

    if (vector.LengthSquared() > radiussquared)
        return ContainmentType::Intersects;

    vector.X = sphere.Center.X - box.Maximum.X;
    vector.Y = sphere.Center.Y - box.Maximum.Y;
    vector.Z = sphere.Center.Z - box.Maximum.Z;

    if (vector.LengthSquared() > radiussquared)
        return ContainmentType::Intersects;

    vector.X = sphere.Center.X - box.Maximum.X;
    vector.Y = sphere.Center.Y - box.Minimum.Y;
    vector.Z = sphere.Center.Z - box.Maximum.Z;

    if (vector.LengthSquared() > radiussquared)
        return ContainmentType::Intersects;

    vector.X = sphere.Center.X - box.Minimum.X;
    vector.Y = sphere.Center.Y - box.Minimum.Y;
    vector.Z = sphere.Center.Z - box.Maximum.Z;

    if (vector.LengthSquared() > radiussquared)
        return ContainmentType::Intersects;

    vector.X = sphere.Center.X - box.Minimum.X;
    vector.Y = sphere.Center.Y - box.Maximum.Y;
    vector.Z = sphere.Center.Z - box.Minimum.Z;

    if (vector.LengthSquared() > radiussquared)
        return ContainmentType::Intersects;

    vector.X = sphere.Center.X - box.Maximum.X;
    vector.Y = sphere.Center.Y - box.Maximum.Y;
    vector.Z = sphere.Center.Z - box.Minimum.Z;

    if (vector.LengthSquared() > radiussquared)
        return ContainmentType::Intersects;

    vector.X = sphere.Center.X - box.Maximum.X;
    vector.Y = sphere.Center.Y - box.Minimum.Y;
    vector.Z = sphere.Center.Z - box.Minimum.Z;

    if (vector.LengthSquared() > radiussquared)
        return ContainmentType::Intersects;

    vector.X = sphere.Center.X - box.Minimum.X;
    vector.Y = sphere.Center.Y - box.Minimum.Y;
    vector.Z = sphere.Center.Z - box.Minimum.Z;

    if (vector.LengthSquared() > radiussquared)
        return ContainmentType::Intersects;

    return ContainmentType::Contains;
}

ContainmentType CollisionsHelper::SphereContainsSphere(const BoundingSphere& sphere1, const BoundingSphere& sphere2)
{
    const float distance = Vector3::Distance(sphere1.Center, sphere2.Center);

    if (sphere1.Radius + sphere2.Radius < distance)
        return ContainmentType::Disjoint;

    if (sphere1.Radius - sphere2.Radius < distance)
        return ContainmentType::Intersects;

    return ContainmentType::Contains;
}

bool CollisionsHelper::FrustumIntersectsBox(const BoundingFrustum& frustum, const BoundingBox& box)
{
    return FrustumContainsBox(frustum, box) != ContainmentType::Disjoint;
}

ContainmentType CollisionsHelper::FrustumContainsBox(const BoundingFrustum& frustum, const BoundingBox& box)
{
    Vector3 p, n;
    Plane plane;
    auto result = ContainmentType::Contains;

    for (int32 i = 0; i < 6; i++)
    {
        plane = frustum.GetPlane(i);
        GetBoxToPlanePVertexNVertex(box, plane.Normal, p, n);

        if (PlaneIntersectsPoint(plane, p) == PlaneIntersectionType::Back)
            return ContainmentType::Disjoint;

        if (PlaneIntersectsPoint(plane, n) == PlaneIntersectionType::Back)
            result = ContainmentType::Intersects;
    }
    return result;
}

void CollisionsHelper::GetBoxToPlanePVertexNVertex(const BoundingBox& box, const Vector3& planeNormal, Vector3& p, Vector3& n)
{
    p = box.Minimum;
    if (planeNormal.X >= 0)
        p.X = box.Maximum.X;
    if (planeNormal.Y >= 0)
        p.Y = box.Maximum.Y;
    if (planeNormal.Z >= 0)
        p.Z = box.Maximum.Z;

    n = box.Maximum;
    if (planeNormal.X >= 0)
        n.X = box.Minimum.X;
    if (planeNormal.Y >= 0)
        n.Y = box.Minimum.Y;
    if (planeNormal.Z >= 0)
        n.Z = box.Minimum.Z;
}

bool CollisionsHelper::LineIntersectsLine(const Vector2& l1p1, const Vector2& l1p2, const Vector2& l2p1, const Vector2& l2p2)
{
    float q = (l1p1.Y - l2p1.Y) * (l2p2.X - l2p1.X) - (l1p1.X - l2p1.X) * (l2p2.Y - l2p1.Y);
    const float d = (l1p2.X - l1p1.X) * (l2p2.Y - l2p1.Y) - (l1p2.Y - l1p1.Y) * (l2p2.X - l2p1.X);

    if (Math::IsZero(d))
        return false;

    const float r = q / d;
    q = (l1p1.Y - l2p1.Y) * (l1p2.X - l1p1.X) - (l1p1.X - l2p1.X) * (l1p2.Y - l1p1.Y);
    const float s = q / d;

    return !(r < 0 || r > 1 || s < 0 || s > 1);
}

bool CollisionsHelper::LineIntersectsRect(const Vector2& p1, const Vector2& p2, const Rectangle& rect)
{
    /*Vector2 c = rect.Location; // Box center-point
    Vector2 e= rect.Size * 0.5f; // Box halflength extents
    Vector2 m=(p2+p1)*0.5f; // Segment midpoint
    Vector2 d=p1-m; // Segment halflength vector
    m = m - c; // Translate box and segment to origin
             
    // Try world coordinate axes as separating axes
    float adx = Math::Abs(d.X);
    if (Math::Abs(m.X) > e.X + adx) return false;
    float ady = Math::Abs(d.Y);
    if (Math::Abs(m.Y) > e.Y + ady) return false;
    
    // Add in an epsilon term to counteract arithmetic errors when segment is
    // (near) parallel to a coordinate axis (see text for detail) 
    adx += ZeroTolerance; ady += ZeroTolerance;

    // Try cross products of segment direction vector with coordinate axes
    if (Math::Abs(m.Y * d.X - m.X * d.Y) > e.X * ady + e.Y * adx) return false;
    if (Math::Abs(m.X * d.Y - m.Y * d.X) > e.X * ady + e.Y * adx) return false;
    
    // No separating axis found; segment must be overlapping AABB 
    return true;*/

    // TODO: optimize it
    const Vector2 pA(rect.GetRight(), rect.GetY());
    const Vector2 pB(rect.GetRight(), rect.GetBottom());
    const Vector2 pC(rect.GetX(), rect.GetBottom());
    return LineIntersectsLine(p1, p2, rect.Location, pA) ||
            LineIntersectsLine(p1, p2, pA, pB) ||
            LineIntersectsLine(p1, p2, pB, pC) ||
            LineIntersectsLine(p1, p2, pC, rect.Location) ||
            (rect.Contains(p1) && rect.Contains(p2));

    /*float minX = Math::Min(p1.X, p2.X);
    float maxX = Math::Max(p1.X, p2.X);
    float minY = Math::Min(p1.Y, p2.Y);
    float maxY = Math::Max(p1.Y, p2.Y);*/

    /*float l = rect.GetLeft();
    float r = rect.GetRight();
    float t = rect.GetTop();
    float b = rect.GetBottom();

    // Calculate m and c for the equation for the line (y = mx+c)
    float m = (p2.Y - p1.Y) / (p2.X - p1.X);
    float c = p1.Y - (m * p1.X);

    // if the line is going up from right to left then the top intersect point is on the left
    float top_intersection, bottom_intersection;
    if (m > 0)
    {
        top_intersection = (m*l + c);
        bottom_intersection = (m*r + c);
    }
    // otherwise it's on the right
    else
    {
        top_intersection = (m*r + c);
        bottom_intersection = (m*l + c);
    }

    // work out the top and bottom extents for the triangle
    float toptrianglepoint, bottomtrianglepoint;
    if (y0 < y1)
    {
        toptrianglepoint = p1.Y;
        bottomtrianglepoint = p2.Y;
    }
    else
    {
        toptrianglepoint = p2.Y;
        bottomtrianglepoint = p1.Y;
    }

    // and calculate the overlap between those two bounds
    float topoverlap = top_intersection > toptrianglepoint ? top_intersection : toptrianglepoint;
    float botoverlap = bottom_intersection < bottomtrianglepoint ? bottom_intersection : bottomtrianglepoint;

    // (topoverlap<botoverlap) :
    // if the intersection isn't the right way up then we have no overlap

    // (!((botoverlap<t) || (topoverlap>b)) :
    // If the bottom overlap is higher than the top of the rectangle or the top overlap is
    // lower than the bottom of the rectangle we don't have intersection. So return the negative
    // of that. Much faster than checking each of the points is within the bounds of the rectangle.
    return (topoverlap < botoverlap) && (!((botoverlap < t) || (topoverlap > b)));*/
}

bool CollisionsHelper::IsPointInTriangle(const Vector2& point, const Vector2& a, const Vector2& b, const Vector2& c)
{
    const Vector2 an = a - point;
    const Vector2 bn = b - point;
    const Vector2 cn = c - point;

    const bool orientation = Vector2::Cross(an, bn) > 0;

    if (Vector2::Cross(bn, cn) > 0 != orientation)
        return false;
    return Vector2::Cross(cn, an) > 0 == orientation;
}
