// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "FlaxEngine.Gen.h"

class GPUTexture;

/// <summary>
/// Structure that contains precomputed data for atmosphere rendering.
/// </summary>
struct AtmosphereCache
{
    GPUTexture* Transmittance;
    GPUTexture* Irradiance;
    GPUTexture* Inscatter;
};

/// <summary>
/// PBR atmosphere cache data rendering service.
/// </summary>
class FLAXENGINE_API AtmospherePreCompute
{
public:

    /// <summary>
    /// Gets the atmosphere cache textures.
    /// </summary>
    /// <param name="cache">Result cache</param>
    /// <returns>True if context is ready for usage.</returns>
    static bool GetCache(AtmosphereCache* cache);
};
