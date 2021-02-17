// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

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

inline PxVec2& C2P(const Vector2& v)
{
    return *(PxVec2*)&v;
}

inline PxVec3& C2P(const Vector3& v)
{
    return *(PxVec3*)&v;
}

inline PxVec4& C2P(const Vector4& v)
{
    return *(PxVec4*)&v;
}

inline PxQuat& C2P(const Quaternion& v)
{
    return *(PxQuat*)&v;
}

inline PxBounds3& C2P(const BoundingBox& v)
{
    return *(PxBounds3*)&v;
}

inline Vector2& P2C(const PxVec2& v)
{
    return *(Vector2*)&v;
}

inline Vector3& P2C(const PxVec3& v)
{
    return *(Vector3*)&v;
}

inline Vector4& P2C(const PxVec4& v)
{
    return *(Vector4*)&v;
}

inline Quaternion& P2C(const PxQuat& v)
{
    return *(Quaternion*)&v;
}

inline BoundingBox& P2C(const PxBounds3& v)
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

extern PxShapeFlags GetShapeFlags(bool isTrigger, bool isEnabled);
