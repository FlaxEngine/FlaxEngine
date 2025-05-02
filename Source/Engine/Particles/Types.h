// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config.h"

/// <summary>
/// The particles simulation execution mode.
/// </summary>
API_ENUM() enum class ParticlesSimulationMode
{
    /// <summary>
    /// The default model. Select the best simulation mode based on a target platform.
    /// </summary>
    Default = 0,

    /// <summary>
    /// Runs particles simulation on a CPU (always supported).
    /// </summary>
    CPU = 1,

    /// <summary>
    /// Runs particles simulation on a GPU (if supported).
    /// </summary>
    GPU = 2,
};

/// <summary>
/// The particles simulation space modes.
/// </summary>
API_ENUM() enum class ParticlesSimulationSpace
{
    /// <summary>
    /// Simulates particles in the world space.
    /// </summary>
    World = 0,

    /// <summary>
    /// Simulates particles in the local space of the actor.
    /// </summary>
    Local = 1,
};

/// <summary>
/// The sprite rendering facing modes.
/// </summary>
API_ENUM() enum class ParticleSpriteFacingMode
{
    /// <summary>
    /// Particles will face camera position.
    /// </summary>
    FaceCameraPosition,

    /// <summary>
    /// Particles will face camera plane.
    /// </summary>
    FaceCameraPlane,

    /// <summary>
    /// Particles will orient along velocity vector.
    /// </summary>
    AlongVelocity,

    /// <summary>
    /// Particles will use the custom vector for facing.
    /// </summary>
    CustomFacingVector,

    /// <summary>
    /// Particles will use the custom fixed axis for facing up.
    /// </summary>
    FixedAxis,
};

/// <summary>
/// The model particle rendering facing modes.
/// </summary>
API_ENUM() enum class ParticleModelFacingMode
{
    /// <summary>
    /// Particles will face camera position.
    /// </summary>
    FaceCameraPosition,

    /// <summary>
    /// Particles will face camera plane.
    /// </summary>
    FaceCameraPlane,

    /// <summary>
    /// Particles will orient along velocity vector.
    /// </summary>
    AlongVelocity,
};

/// <summary>
/// The particles sorting modes.
/// </summary>
API_ENUM() enum class ParticleSortMode
{
    /// <summary>
    /// Do not perform additional sorting prior to rendering.
    /// </summary>
    None,

    /// <summary>
    /// Sorts particles by depth to the view's near plane.
    /// </summary>
    ViewDepth,

    /// <summary>
    /// Sorts particles by distance to the view's origin.
    /// </summary>
    ViewDistance,

    /// <summary>
    /// The custom sorting according to a per particle attribute. Lower values are rendered before higher values.
    /// </summary>
    CustomAscending,

    /// <summary>
    /// The custom sorting according to a per particle attribute. Higher values are rendered before lower values.
    /// </summary>
    CustomDescending,
};
