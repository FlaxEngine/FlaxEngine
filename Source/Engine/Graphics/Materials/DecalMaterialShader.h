// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "MaterialShader.h"

/// <summary>
/// Represents material that can be used to render decals.
/// </summary>
class DecalMaterialShader : public MaterialShader
{
private:
    struct Cache
    {
        GPUPipelineState* Inside = nullptr;
        GPUPipelineState* Outside = nullptr;

        FORCE_INLINE void Release()
        {
            SAFE_DELETE_GPU_RESOURCE(Inside);
            SAFE_DELETE_GPU_RESOURCE(Outside);
        }
    };

private:
    Cache _cache;

public:
    /// <summary>
    /// Init
    /// </summary>
    /// <param name="name">Material resource name</param>
    DecalMaterialShader(const StringView& name)
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
