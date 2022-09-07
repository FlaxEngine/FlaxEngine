// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_PHYSX

#include "Engine/Physics/Types.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector4.h"
#include "Engine/Core/Math/Quaternion.h"
#include "Engine/Core/Math/BoundingBox.h"
#include <ThirdParty/PhysX/foundation/PxVec2.h>
#include <ThirdParty/PhysX/foundation/PxVec3.h>
#include <ThirdParty/PhysX/foundation/PxVec4.h>
#include <ThirdParty/PhysX/foundation/PxQuat.h>
#include <ThirdParty/PhysX/foundation/PxBounds3.h>
#include <ThirdParty/PhysX/characterkinematic/PxExtended.h>
#include <ThirdParty/PhysX/PxShape.h>
#include <ThirdParty/PhysX/PxQueryReport.h>

namespace physx
{
    class PxScene;
    class PxConvexMesh;
    class PxTriangleMesh;
    class PxCooking;
    class PxPhysics;
    class PxVec3;
    class PxVec4;
    class PxTransform;
    class PxJoint;
    class PxMat44;
    class PxCpuDispatcher;
    class PxGpuDispatcher;
    class PxSimulationEventCallback;
    struct PxActiveTransform;
    class PxActor;
    class PxRigidActor;
    class PxRigidDynamic;
    class PxRigidStatic;
    class PxFoundation;
    class PxShape;
    class PxGeometry;
    class PxGeometryHolder;
    class PxProfileZoneManager;
    class PxMaterial;
    class PxPvd;
    class PxBase;
    class PxTolerancesScale;
    class PxBaseTask;
    class PxControllerManager;
    class PxController;
    class PxCapsuleController;
    class PxQueryFilterCallback;
    class PxControllerFilterCallback;
    class PxHeightField;
    struct PxFilterData;
    struct PxRaycastHit;
    struct PxSweepHit;
}

using namespace physx;

#define RELEASE_PHYSX(x) if(x) { (x)->release(); x = nullptr; }

#if USE_LARGE_WORLDS

inline PxVec2 C2P(const Vector2& v)
{
    return PxVec2((float)v.X, (float)v.Y);
}

inline PxVec3 C2P(const Vector3& v)
{
    return PxVec3((float)v.X, (float)v.Y, (float)v.Z);
}

inline PxVec4 C2P(const Vector4& v)
{
    return PxVec4((float)v.X, (float)v.Y, (float)v.Z, (float)v.W);
}

inline PxBounds3 C2P(const BoundingBox& v)
{
    return PxBounds3(C2P(v.Minimum), C2P(v.Maximum));
}

inline Vector2 P2C(const PxVec2& v)
{
    return Vector2(v.x, v.y);
}

inline Vector3 P2C(const PxVec3& v)
{
    return Vector3(v.x, v.y, v.z);
}

inline Vector4 P2C(const PxVec4& v)
{
    return Vector4(v.x, v.y, v.z, v.w);
}

inline BoundingBox P2C(const PxBounds3& v)
{
    return BoundingBox(P2C(v.minimum), P2C(v.maximum));
}

inline Vector3 P2C(const PxExtendedVec3& v)
{
#ifdef PX_BIG_WORLDS
    return *(Vector3*)&v;
#else
    return Vector3(v.x, v.y, v.z);
#endif
}

#else

inline PxVec2 C2P(const Vector2& v)
{
    return *(PxVec2*)&v;
}

inline PxVec3 C2P(const Vector3& v)
{
    return *(PxVec3*)&v;
}

inline PxVec4 C2P(const Vector4& v)
{
    return *(PxVec4*)&v;
}

inline PxBounds3 C2P(const BoundingBox& v)
{
    return *(PxBounds3*)&v;
}

inline Vector2 P2C(const PxVec2& v)
{
    return *(Vector2*)&v;
}

inline Vector3 P2C(const PxVec3& v)
{
    return *(Vector3*)&v;
}

inline Vector4 P2C(const PxVec4& v)
{
    return *(Vector4*)&v;
}

inline BoundingBox P2C(const PxBounds3& v)
{
    return *(BoundingBox*)&v;
}

inline Vector3 P2C(const PxExtendedVec3& v)
{
#ifdef PX_BIG_WORLDS
    return Vector3((float)v.x, (float)v.y, (float)v.z);
#else
	return *(Vector3*)&v;
#endif
}

#endif

inline PxQuat& C2P(const Quaternion& v)
{
    return *(PxQuat*)&v;
}

inline Quaternion& P2C(const PxQuat& v)
{
    return *(Quaternion*)&v;
}

inline float M2ToCm2(float v)
{
    return v * (100.0f * 100.0f);
}

inline float Cm2ToM2(float v)
{
    return v / (100.0f * 100.0f);
}

inline float KgPerM3ToKgPerCm3(float v)
{
    return v / (100.0f * 100.0f * 100.0f);
}

inline float RpmToRadPerS(float v)
{
    return v * (PI / 30.0f);
}

inline float RadPerSToRpm(float v)
{
    return v * (30.0f / PI);
}

inline void P2C(const PxRaycastHit& hit, RayCastHit& result)
{
    result.Point = P2C(hit.position);
    result.Normal = P2C(hit.normal);
    result.Distance = hit.distance;
    result.Collider = hit.shape ? static_cast<PhysicsColliderActor*>(hit.shape->userData) : nullptr;
    result.FaceIndex = hit.faceIndex;
    result.UV.X = hit.u;
    result.UV.Y = hit.v;
}

inline void P2C(const PxSweepHit& hit, RayCastHit& result)
{
    result.Point = P2C(hit.position);
    result.Normal = P2C(hit.normal);
    result.Distance = hit.distance;
    result.Collider = hit.shape ? static_cast<PhysicsColliderActor*>(hit.shape->userData) : nullptr;
    result.FaceIndex = hit.faceIndex;
    result.UV = Vector2::Zero;
}

extern PxShapeFlags GetShapeFlags(bool isTrigger, bool isEnabled);

#endif
