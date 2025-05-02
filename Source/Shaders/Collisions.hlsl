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

#endif
