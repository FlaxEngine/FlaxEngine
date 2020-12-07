// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config/Settings.h"
#include "Engine/Core/Math/Vector3.h"
#include "Types.h"

// Default values for the physics settings

#define PhysicsSettings_DefaultGravity Vector3(0, -981.0f, 0)
#define PhysicsSettings_TriangleMeshTriangleMinAreaThreshold (5.0f)
#define PhysicsSettings_BounceThresholdVelocity (200.0f)
#define PhysicsSettings_FrictionCombineMode PhysicsCombineMode::Average
#define PhysicsSettings_RestitutionCombineMode PhysicsCombineMode::Average
#define PhysicsSettings_DisableCCD false
#define PhysicsSettings_EnableAdaptiveForce false
#define PhysicsSettings_MaxDeltaTime (1.0f / 10.0f)
#define PhysicsSettings_EnableSubstepping false
#define PhysicsSettings_SubstepDeltaTime (1.0f / 120.0f)
#define PhysicsSettings_MaxSubsteps 5
#define PhysicsSettings_QueriesHitTriggers true
#define PhysicsSettings_SupportCookingAtRuntime false

/// <summary>
/// Physics simulation settings container.
/// </summary>
/// <seealso cref="Settings{PhysicsSettings}" />
class PhysicsSettings : public Settings<PhysicsSettings>
{
public:

    /// <summary>
    /// Initializes a new instance of the <see cref="PhysicsSettings"/> class.
    /// </summary>
    PhysicsSettings()
    {
        for (int32 i = 0; i < 32; i++)
            LayerMasks[i] = MAX_uint32;
    }

public:

    /// <summary>
    /// The default gravity force value (in cm^2/s).
    /// </summary>
    Vector3 DefaultGravity = PhysicsSettings_DefaultGravity;

    /// <summary>
    /// Triangles from triangle meshes (CSG) with an area less than or equal to this value will be removed from physics collision data. Set to less than or equal 0 to disable.
    /// </summary>
    float TriangleMeshTriangleMinAreaThreshold = PhysicsSettings_TriangleMeshTriangleMinAreaThreshold;

    /// <summary>
    /// Minimum relative velocity required for an object to bounce. A typical value for simulation stability is about 0.2 * gravity
    /// </summary>
    float BounceThresholdVelocity = PhysicsSettings_BounceThresholdVelocity;

    /// <summary>
    /// Default friction combine mode, controls how friction is computed for multiple materials.
    /// </summary>
    PhysicsCombineMode FrictionCombineMode = PhysicsSettings_FrictionCombineMode;

    /// <summary>
    /// Default restitution combine mode, controls how restitution is computed for multiple materials.
    /// </summary>
    PhysicsCombineMode RestitutionCombineMode = PhysicsSettings_RestitutionCombineMode;

    /// <summary>
    /// If true CCD will be ignored. This is an optimization when CCD is never used which removes the need for physx to check it internally.
    /// </summary>
    bool DisableCCD = PhysicsSettings_DisableCCD;

    /// <summary>
    /// Enables adaptive forces to accelerate convergence of the solver. Can improve physics simulation performance but lead to artifacts.
    /// </summary>
    bool EnableAdaptiveForce = PhysicsSettings_EnableAdaptiveForce;

    /// <summary>
    /// The maximum allowed delta time (in seconds) for the physics simulation step.
    /// </summary>
    float MaxDeltaTime = PhysicsSettings_MaxDeltaTime;

    /// <summary>
    /// Whether to substep the physics simulation.
    /// </summary>
    bool EnableSubstepping = PhysicsSettings_EnableSubstepping;

    /// <summary>
    /// Delta time (in seconds) for an individual simulation substep.
    /// </summary>
    float SubstepDeltaTime = PhysicsSettings_SubstepDeltaTime;

    /// <summary>
    /// The maximum number of substeps for physics simulation.
    /// </summary>
    int32 MaxSubsteps = PhysicsSettings_MaxSubsteps;

    /// <summary>
    /// If enabled, any Raycast or other scene query that intersects with a Collider marked as a Trigger will returns with a hit. Individual raycasts can override this behavior.
    /// </summary>
    bool QueriesHitTriggers = PhysicsSettings_QueriesHitTriggers;

    /// <summary>
    /// Enables support for cooking physical collision shapes geometry at runtime. Use it to enable generating runtime terrain collision or convex mesh colliders.
    /// </summary>
    bool SupportCookingAtRuntime = PhysicsSettings_SupportCookingAtRuntime;

public:

    /// <summary>
    /// The collision layers masks. Used to define layer-based collision detection.
    /// </summary>
    uint32 LayerMasks[32];

public:

    // [Settings]
    void Apply() override;
    void RestoreDefault() override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) final override;
};
