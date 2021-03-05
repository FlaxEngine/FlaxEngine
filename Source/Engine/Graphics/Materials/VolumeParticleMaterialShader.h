// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "MaterialShader.h"

/// <summary>
/// Represents material that can be used to render particles.
/// </summary>
class VolumeParticleMaterialShader : public MaterialShader
{
private:

    GPUPipelineState* _psVolumetricFog = nullptr;

public:

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="name">Material resource name</param>
    VolumeParticleMaterialShader(const String& name)
        : MaterialShader(name)
    {
    }

public:

    // [MaterialShader]
    DrawPass GetDrawModes() const override;
    void Bind(BindParameters& params) override;
    void Unload() override;

protected:

    // [MaterialShader]
    bool Load() override;
};
