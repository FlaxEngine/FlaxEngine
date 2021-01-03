// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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

#endif
