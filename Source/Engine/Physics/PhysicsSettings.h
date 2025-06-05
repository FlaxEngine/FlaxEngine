// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config/Settings.h"
#include "Engine/Core/Math/Vector3.h"
#include "Types.h"

/// <summary>
/// Broad phase algorithm used in the simulation.
/// <see href="https://nvidia-omniverse.github.io/PhysX/physx/5.1.0/_build/physx/latest/struct_px_broad_phase_type.html"/>
/// </summary>
API_ENUM() enum class PhysicsBroadPhaseType
{
    /// <summary>
    /// 3-axes sweep-and-prune. Good generic choice with great performance when many objects are sleeping.
    /// </summary>
    SweepAndPrune = 0,

    /// <summary>
    /// Alternative broad phase algorithm that does not suffer from the same performance issues as SAP when all objects are moving or when inserting large numbers of objects. 
    /// </summary>
    MultiBoxPruning = 1,

    /// <summary>
    /// Revisited implementation of MBP, which automatically manages broad-phase regions.
    /// </summary>
    AutomaticBoxPruning = 2,

    /// <summary>
    /// Parallel implementation of ABP. It can often be the fastest (CPU) broadphase, but it can use more memory than ABP.
    /// </summary>
    ParallelAutomaticBoxPruning = 3,
};

/// <summary>
/// The type of solver used in the simulation.
/// <see href="https://nvidia-omniverse.github.io/PhysX/physx/5.1.0/_build/physx/latest/struct_px_solver_type.html"/>
/// </summary>
API_ENUM() enum class PhysicsSolverType
{
    /// <summary>
    /// The iterative sequential impulse solver.
    /// </summary>
    ProjectedGaussSeidelIterativeSolver = 0,

    /// <summary>
    /// Non linear iterative solver. This kind of solver can lead to improved convergence and handle large mass ratios, long chains and jointed systems better. It is slightly more expensive than the default solver and can introduce more energy to correct joint and contact errors.
    /// </summary>
    TemporalGaussSeidelSolver = 1,
};

/// <summary>
/// Physics simulation settings container.
/// </summary>
API_CLASS(sealed, Namespace="FlaxEditor.Content.Settings", NoConstructor) class FLAXENGINE_API PhysicsSettings : public SettingsBase
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(PhysicsSettings);

public:
    /// <summary>
    /// The default gravity value (in cm/(s^2)).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(0), EditorDisplay(\"Simulation\")")
    Vector3 DefaultGravity = Vector3(0, -981.0f, 0);

    /// <summary>
    /// Minimum relative velocity required for an object to bounce. A typical value for simulation stability is about 0.2 * gravity
    /// </summary>
    API_FIELD(Attributes="EditorOrder(30), EditorDisplay(\"Simulation\")")
    float BounceThresholdVelocity = 200.0f;

    /// <summary>
    /// Default friction combine mode, controls how friction is computed for multiple materials.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(40), EditorDisplay(\"Simulation\")")
    PhysicsCombineMode FrictionCombineMode = PhysicsCombineMode::Average;

    /// <summary>
    /// Default restitution combine mode, controls how restitution is computed for multiple materials.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(40), EditorDisplay(\"Simulation\")")
    PhysicsCombineMode RestitutionCombineMode = PhysicsCombineMode::Average;

    /// <summary>
    /// If true CCD will be ignored. This is an optimization when CCD is never used which removes the need for physics to check it internally.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(70), EditorDisplay(\"Simulation\")")
    bool DisableCCD = false;

    /// <summary>
    /// Broad phase algorithm to use in the simulation.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(71), EditorDisplay(\"Simulation\")")
    PhysicsBroadPhaseType BroadPhaseType = PhysicsBroadPhaseType::ParallelAutomaticBoxPruning;

    /// <summary>
    /// Enables enhanced determinism in the simulation. This has a performance impact.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(71), EditorDisplay(\"Simulation\")")
    bool EnableEnhancedDeterminism = false;

    /// <summary>
    /// The solver type to use in the simulation.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(73), EditorDisplay(\"Simulation\")")
    PhysicsSolverType SolverType = PhysicsSolverType::ProjectedGaussSeidelIterativeSolver;

    /// <summary>
    /// The maximum allowed delta time (in seconds) for the physics simulation step.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1000), EditorDisplay(\"Framerate\")")
    float MaxDeltaTime = 0.1f;

    /// <summary>
    /// Whether to substep the physics simulation.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1005), EditorDisplay(\"Framerate\")")
    bool EnableSubstepping = false;

    /// <summary>
    /// Delta time (in seconds) for an individual simulation substep.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1010), EditorDisplay(\"Framerate\")")
    float SubstepDeltaTime = 0.00833333333f;

    /// <summary>
    /// The maximum number of substeps for physics simulation.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1020), EditorDisplay(\"Framerate\")")
    int32 MaxSubsteps = 5;

    /// <summary>
    /// Enables support for cooking physical collision shapes geometry at runtime. Use it to enable generating runtime terrain collision or convex mesh colliders.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1100), EditorDisplay(\"Other\")")
    bool SupportCookingAtRuntime = false;

    /// <summary>
    /// Triangles from triangle meshes (CSG) with an area less than or equal to this value will be removed from physics collision data. Set to less than or equal 0 to disable.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1110), EditorDisplay(\"Other\")")
    float TriangleMeshTriangleMinAreaThreshold = 5.0f;

    /// <summary>
    /// If enabled, any Raycast or other scene query that intersects with a Collider marked as a Trigger will return with a hit. Individual raycasts can override this behavior.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1200), EditorDisplay(\"Other\")")
    bool QueriesHitTriggers = true;

    /// <summary>
    /// The collision layers masks. Used to define layer-based collision detection.
    /// </summary>
    uint32 LayerMasks[32];

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="PhysicsSettings"/> class.
    /// </summary>
    PhysicsSettings();

    /// <summary>
    /// Gets the instance of the settings asset (default value if missing). Object returned by this method is always loaded with valid data to use.
    /// </summary>
    static PhysicsSettings* Get();

    // [SettingsBase]
    void Apply() override;
#if USE_EDITOR
    void Serialize(SerializeStream& stream, const void* otherObj) override;
#endif
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) final override;
};
