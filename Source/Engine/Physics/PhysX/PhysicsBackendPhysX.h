// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_PHYSX

#include "Engine/Physics/PhysicsBackend.h"
#include "Types.h"

/// <summary>
/// Implementation of the physical simulation using PhysX.
/// </summary>
class PhysicsBackendPhysX
{
public:
    static PxPhysics* GetPhysics();
#if COMPILE_WITH_PHYSICS_COOKING
    static PxCooking* GetCooking();
#endif
    static PxMaterial* GetDefaultMaterial();
    static void SimulationStepDone(PxScene* scene, float dt);
};

#endif
