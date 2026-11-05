// Copyright (c) Wojciech Figat. All rights reserved.

#ifndef __COLLISIONS__
#define __COLLISIONS__

bool RayHitSphere(float3 r, float3 sphereCenter, float sphereRadius)
{
    float3 closestPointOnRay = max(0, dot(sphereCenter, r)) * r;
    float3 centerToRay = closestPointOnRay - sphereCenter;
    return dot(centerToRay, centerToRay) <= (sphereRadius * sphereRadius);
}

bool RayHitRect(float3 r, float3 rectCenter, float3 rectX, float3 rectY, float3 rectZ, float rectExtentX, float rectExtentY)
{
    float3 pointOnPlane = r * max(0, dot(rectZ, rectCenter) / dot(rectZ, r));
    bool inExtentX = abs(dot(rectX, pointOnPlane - rectCenter)) <= rectExtentX;
    bool inExtentY = abs(dot(rectY, pointOnPlane - rectCenter)) <= rectExtentY;
    return inExtentX && inExtentY;
}

// Determines whether there is an intersection between a ray (rPos and rDir) and a triangle (v0, v1, v2).
// Returns true on intersection and outputs the distance along the ray to the intersection point.
// This method tests if the ray intersects either the front or back of the triangle.
bool RayIntersectsTriangle(float3 rPos, float3 rDir, float3 v0, float3 v1, float3 v2, out float distance)
{
    // [https://stackoverflow.com/a/42752998]
    float3 edgeAB = v1 - v0;
    float3 edgeAC = v2 - v0;
    float3 triFaceVector = cross(edgeAB, edgeAC);
    float3 vertRayOffset = rPos - v0;
    float3 rayOffsetPerp = cross(vertRayOffset, rDir);
    float determinant = -dot(rDir, triFaceVector);
    float invDet = 1.0f / determinant;
    distance = dot(vertRayOffset, triFaceVector) * invDet;
    float u = dot(edgeAC, rayOffsetPerp) * invDet;
    float v = -dot(edgeAB, rayOffsetPerp) * invDet;
    float w = 1.0f - u - v;
    return abs(determinant) >= 1E-8 && distance > 0 && u >= 0 && v >= 0 && w >= 0;
}

// Hits axis-aligned box (boxMin, boxMax) with a line (lineStart, lineEnd).
// Returns the intersections on the line (x - closest, y - furthest).
// Line hits the box if: intersections.x < intersections.y.
// Hit point is: hitPoint = lineStart + (lineEnd - lineStart) * intersections.x/y.
float2 LineHitBox(float3 lineStart, float3 lineEnd, float3 boxMin, float3 boxMax)
{
    float3 invDirection = 1.0f / (lineEnd - lineStart);
    float3 enterIntersection = (boxMin - lineStart) * invDirection;
    float3 exitIntersection = (boxMax - lineStart) * invDirection;
    float3 minIntersections = min(enterIntersection, exitIntersection);
    float3 maxIntersections = max(enterIntersection, exitIntersection);
    float2 intersections;
    intersections.x = max(minIntersections.x, max(minIntersections.y, minIntersections.z));
    intersections.y = min(maxIntersections.x, min(maxIntersections.y, maxIntersections.z));
    return saturate(intersections);
}

// Determines whether there is an intersection between a box and a sphere.
bool BoxIntersectsSphere(float3 boxMin, float3 boxMax, float3 sphereCenter, float sphereRadius)
{
    const float3 clampedCenter = clamp(sphereCenter, boxMin, boxMax);
    return distance(sphereCenter, clampedCenter) <= sphereRadius;
}

// Calculates unsigned distance from point to the AABB. If point is inside it, returns 0.
float PointDistanceBox(float3 boxMin, float3 boxMax, float3 pos)
{
    float3 clampedPos = clamp(pos, boxMin, boxMax);
    return length(clampedPos - pos);
}

float dot2(float3 v)
{
    return dot(v, v);
}

// Calculates squared distance from point to the triangle.
float DistancePointToTriangle2(float3 p, float3 v1, float3 v2, float3 v3)
{
    // [Inigo Quilez, https://iquilezles.org/articles/triangledistance/]
    float3 v21 = v2 - v1; float3 p1 = p - v1;
    float3 v32 = v3 - v2; float3 p2 = p - v2;
    float3 v13 = v1 - v3; float3 p3 = p - v3;
    float3 nor = cross(v21, v13);
    return // inside/outside test
            (sign(dot(cross(v21, nor), p1)) +
            sign(dot(cross(v32, nor), p2)) +
            sign(dot(cross(v13, nor), p3)) < 2.0)
            ?
            // 3 edges
            min(min(
            dot2(v21 * saturate(dot(v21, p1) / dot2(v21)) - p1),
            dot2(v32 * saturate(dot(v32, p2) / dot2(v32)) - p2)),
            dot2(v13 * saturate(dot(v13, p3) / dot2(v13)) - p3))
            :
            // 1 face
            dot(nor, p1) * dot(nor, p1) / dot2(nor);
}

#endif
