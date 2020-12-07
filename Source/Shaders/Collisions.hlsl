// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

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

// Computes where a ray hits a sphere (which is centered at the origin).
// \param[in]  rayOrigin    The start position of the ray.
// \param[in]  rayDirection The normalized direction of the ray.
// \param[in]  radius       The radius of the sphere.
// \param[out] enter        The ray parameter where the ray enters the sphere.
//                          0 if the ray is already in the sphere.
// \param[out] exit         The ray parameter where the ray exits the sphere.
// \return  0 or a positive value if the ray hits the sphere. A negative value
//          if the ray does not touch the sphere.
float HitSphere(float3 rayOrigin, float3 rayDirection, float radius, out float enter, out float exit)
{
	// Solve the equation:  ||rayOrigin + distance * rayDirection|| = r
	//
	// This is a straight-forward quadratic equation:
	//   ||O + d * D|| = r
	//   =>  (O + d * D)2 = r2  where V2 means V.V
	//   =>  d2 * D2 + 2 * d * (O.D) + O2 - r2 = 0
	// D2 is 1 because the rayDirection is normalized.
	//   =>  d = -O.D + sqrt((O.D)2 - O2 + r2)

	float OD = dot(rayOrigin, rayDirection);
	float OO = dot(rayOrigin, rayOrigin);
	float radicand = OD * OD - OO + radius * radius;
	enter = max(0, -OD - sqrt(radicand));
	exit = -OD + sqrt(radicand);

	// If radicand is negative then we do not have a result - no hit.
	return radicand;  
}

// Clips a ray to an AABB.  Does not handle rays parallel to any of the planes.
//
// @param rayOrigin - The origin of the ray in world space.
// @param rayEnd - The end of the ray in world space.  
// @param boxMin - The minimum extrema of the box.
// @param boxMax - The maximum extrema of the box.
// @return - Returns the closest intersection along the ray in x, and furthest in y.  
//			If the ray did not intersect the box, then the furthest intersection <= the closest intersection.
//			The intersections will always be in the range [0,1], which corresponds to [rayOrigin, rayEnd] in worldspace.
//			To find the world space position of either intersection, simply plug it back into the ray equation:
//			WorldPos = RayOrigin + (rayEnd - RayOrigin) * Intersection;
float2 LineBoxIntersect(float3 rayOrigin, float3 rayEnd, float3 boxMin, float3 boxMax)
{
	float3 invRayDir = 1.0f / (rayEnd - rayOrigin);

	//find the ray intersection with each of the 3 planes defined by the minimum extrema.
	float3 planeIntersections1 = (boxMin - rayOrigin) * invRayDir;
	//find the ray intersection with each of the 3 planes defined by the maximum extrema.
	float3 planeIntersections2 = (boxMax - rayOrigin) * invRayDir;
	//get the closest of these intersections along the ray
	float3 closestPlaneIntersections = min(planeIntersections1, planeIntersections2);
	//get the furthest of these intersections along the ray
	float3 furthestPlaneIntersections = max(planeIntersections1, planeIntersections2);

	float2 boxIntersections;
	//find the furthest near intersection
	boxIntersections.x = max(closestPlaneIntersections.x, max(closestPlaneIntersections.y, closestPlaneIntersections.z));
	//find the closest far intersection
	boxIntersections.y = min(furthestPlaneIntersections.x, min(furthestPlaneIntersections.y, furthestPlaneIntersections.z));
	//clamp the intersections to be between rayOrigin and RayEnd on the ray
	return saturate(boxIntersections);
}

#endif
