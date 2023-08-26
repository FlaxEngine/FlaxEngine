// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

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
    /// Sweep and prune.
    /// </summary>
    SweepAndPrune = 0,

    /// <summary>
    /// Multi box pruning.
    /// </summary>
    MultiboxPruning = 1,

    /// <summary>
    /// Automatic box pruning.
    /// </summary>
    AutomaticBoxPruning = 2,

    /// <summary>
    /// Parallel automatic box pruning.
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
    /// Projected Gauss-Seidel iterative solver.
    /// </summary>
    ProjectedGaussSeidelIterativeSolver = 0,

    /// <summary>
    /// Default Temporal Gauss-Seidel solver.
    /// </summary>
    DefaultTemporalGaussSeidelSolver = 1,
};

/// <summary>
/// Physics simulation settings container.
/// </summary>
API_CLASS(sealed, Namespace="FlaxEditor.Content.Settings", NoConstructor) class FLAXENGINE_API PhysicsSettings : public SettingsBase
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(PhysicsSettings);
public:
    /// <summary>
    /// The default gravity force value (in cm^2/s).
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
    PhysicsBroadPhaseType BroadPhaseType = PhysicsBroadPhaseType::SweepAndPrune;

    /// <summary>
    /// The solver type to use in the simulation.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(72), EditorDisplay(\"Simulation\")")
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
    /// If enabled, any Raycast or other scene query that intersects with a Collider marked as a Trigger will returns with a hit. Individual raycasts can override this behavior.
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
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) final override;
};
