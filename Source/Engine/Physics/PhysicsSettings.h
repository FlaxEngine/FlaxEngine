// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config/Settings.h"
#include "Engine/Core/Math/Vector3.h"
#include "Types.h"

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
    API_FIELD(Attributes="EditorOrder(0), DefaultValue(typeof(Vector3), \"0,-981.0,0\"), EditorDisplay(\"Simulation\")")
    Vector3 DefaultGravity = Vector3(0, -981.0f, 0);

    /// <summary>
    /// If enabled, any Raycast or other scene query that intersects with a Collider marked as a Trigger will returns with a hit. Individual raycasts can override this behavior.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10), DefaultValue(true), EditorDisplay(\"Framerate\")")
    bool QueriesHitTriggers = true;

    /// <summary>
    /// Triangles from triangle meshes (CSG) with an area less than or equal to this value will be removed from physics collision data. Set to less than or equal 0 to disable.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(20), DefaultValue(5.0f), EditorDisplay(\"Simulation\")")
    float TriangleMeshTriangleMinAreaThreshold = 5.0f;

    /// <summary>
    /// Minimum relative velocity required for an object to bounce. A typical value for simulation stability is about 0.2 * gravity
    /// </summary>
    API_FIELD(Attributes="EditorOrder(30), DefaultValue(200.0f), EditorDisplay(\"Simulation\")")
    float BounceThresholdVelocity = 200.0f;

    /// <summary>
    /// Default friction combine mode, controls how friction is computed for multiple materials.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(40), DefaultValue(PhysicsCombineMode.Average), EditorDisplay(\"Simulation\")")
    PhysicsCombineMode FrictionCombineMode = PhysicsCombineMode::Average;

    /// <summary>
    /// Default restitution combine mode, controls how restitution is computed for multiple materials.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(40), DefaultValue(PhysicsCombineMode.Average), EditorDisplay(\"Simulation\")")
    PhysicsCombineMode RestitutionCombineMode = PhysicsCombineMode::Average;

    /// <summary>
    /// If true CCD will be ignored. This is an optimization when CCD is never used which removes the need for physx to check it internally.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(70), DefaultValue(false), EditorDisplay(\"Simulation\")")
    bool DisableCCD = false;

    /// <summary>
    /// Enables adaptive forces to accelerate convergence of the solver. Can improve physics simulation performance but lead to artifacts.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(80), DefaultValue(false), EditorDisplay(\"Simulation\")")
    bool EnableAdaptiveForce = false;

    /// <summary>
    /// The maximum allowed delta time (in seconds) for the physics simulation step.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1000), DefaultValue(1.0f / 10.0f), EditorDisplay(\"Framerate\")")
    float MaxDeltaTime = 1.0f / 10.0f;

    /// <summary>
    /// Whether to substep the physics simulation.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1005), DefaultValue(false), EditorDisplay(\"Framerate\")")
    bool EnableSubstepping = false;

    /// <summary>
    /// Delta time (in seconds) for an individual simulation substep.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1010), DefaultValue(1.0f / 120.0f), EditorDisplay(\"Framerate\")")
    float SubstepDeltaTime = 1.0f / 120.0f;

    /// <summary>
    /// The maximum number of substeps for physics simulation.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1020), DefaultValue(5), EditorDisplay(\"Framerate\")")
    int32 MaxSubsteps = 5;

    /// <summary>
    /// Enables support for cooking physical collision shapes geometry at runtime. Use it to enable generating runtime terrain collision or convex mesh colliders.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1100), DefaultValue(false), EditorDisplay(\"Other\")")
    bool SupportCookingAtRuntime = false;

public:

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
