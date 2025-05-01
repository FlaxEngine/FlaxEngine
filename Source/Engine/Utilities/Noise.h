// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Math/Vector3.h"

/// <summary>
/// Collection of various noise functions (eg. Perlin, Worley, Voronoi).
/// </summary>
API_CLASS(Static, Namespace="FlaxEngine.Utilities") class FLAXENGINE_API Noise
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(Noise);
public:
    /// <summary>
    /// Classic Perlin noise.
    /// </summary>
    /// <param name="p">Point on a 2D grid to sample noise at.</param>
    /// <returns>Noise value (range 0-1).</returns>
    API_FUNCTION() static float PerlinNoise(const Float2& p);

    /// <summary>
    /// Classic Perlin noise with periodic variant (tiling).
    /// </summary>
    /// <param name="p">Point on a 2D grid to sample noise at.</param>
    /// <param name="rep">Periodic variant of the noise - period of the noise.</param>
    /// <returns>Noise value (range 0-1).</returns>
    API_FUNCTION() static float PerlinNoise(const Float2& p, const Float2& rep);

    /// <summary>
    /// Simplex noise.
    /// </summary>
    /// <param name="p">Point on a 2D grid to sample noise at.</param>
    /// <returns>Noise value (range 0-1).</returns>
    API_FUNCTION() static float SimplexNoise(const Float2& p);

    /// <summary>
    /// Worley noise (cellar noise with standard 3x3 search window for F1 and F2 values).
    /// </summary>
    /// <param name="p">Point on a 2D grid to sample noise at.</param>
    /// <returns>Noise value with: F1 and F2 feature points.</returns>
    API_FUNCTION() static Float2 WorleyNoise(const Float2& p);

    /// <summary>
    /// Voronoi noise (X=minDistToCell, Y=randomColor, Z=minEdgeDistance).
    /// </summary>
    /// <param name="p">Point on a 2D grid to sample noise at.</param>
    /// <returns>Noise result with: X=minDistToCell, Y=randomColor, Z=minEdgeDistance.</returns>
    API_FUNCTION() static Float3 VoronoiNoise(const Float2& p);

    /// <summary>
    /// Custom noise function (3D -> 1D).
    /// </summary>
    /// <param name="p">Point on a 3D grid to sample noise at.</param>
    /// <returns>Noise result.</returns>
    API_FUNCTION() static float CustomNoise(const Float3& p);

    /// <summary>
    /// Custom noise function (3D -> 3D).
    /// </summary>
    /// <param name="p">Point on a 3D grid to sample noise at.</param>
    /// <returns>Noise result.</returns>
    API_FUNCTION() static Float3 CustomNoise3D(const Float3& p);

    /// <summary>
    /// Custom noise function for forces.
    /// </summary>
    /// <param name="p">Point on a 3D grid to sample noise at.</param>
    /// <param name="octaves">Noise octaves count.</param>
    /// <param name="roughness">Noise roughness (in range 0-1).</param>
    /// <returns>Noise result.</returns>
    API_FUNCTION() static Float3 CustomNoise3D(const Float3& p, int32 octaves, float roughness);
};
