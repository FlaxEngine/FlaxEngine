// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"

/// <summary>
/// The engine utility for large worlds support. Contains constants and tools for using 64-bit precision coordinates in various game systems (eg. scene rendering).
/// </summary>
API_CLASS(Static) class FLAXENGINE_API LargeWorlds
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(LargeWorlds);

    /// <summary>
    /// Enables large worlds usage in the engine. If true, scene rendering and other systems will use origin shifting to achieve higher precision in large worlds.
    /// </summary>
    API_FIELD() static bool Enable;

    /// <summary>
    /// Defines the size of a single chunk. Large world (64-bit) gets divided into smaller chunks so all the math operations (32-bit) can be performed relative to the chunk origin without precision loss.
    /// </summary>
    API_FIELD() static constexpr Real ChunkSize = 8192;

    /// <summary>
    /// Updates the large world origin to match the input position. The origin is snapped to the best matching chunk location.
    /// </summary>
    /// <param name="origin">The current origin of the large world. Gets updated with the input position.</param>
    /// <param name="position">The current input position (eg. render view location or auto listener position).</param>
    /// <remarks>Used only if LargeWorlds::Enabled is true.</remarks>
    API_FUNCTION() static void UpdateOrigin(API_PARAM(Ref) Vector3& origin, const Vector3& position);
};
