// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "MaterialShader.h"

/// <summary>
/// Represents material that can be used to render post-process effects.
/// </summary>
class PostFxMaterialShader : public MaterialShader
{
private:
    struct Cache
    {
        GPUPipelineState* Default = nullptr;

        FORCE_INLINE void Release()
        {
            SAFE_DELETE_GPU_RESOURCE(Default);
        }
    };

private:
    Cache _cache;

public:
    /// <summary>
    /// Init
    /// </summary>
    /// <param name="name">Material resource name</param>
    PostFxMaterialShader(const StringView& name)
        : MaterialShader(name)
    {
    }

public:
    // [MaterialShader]
    void Bind(BindParameters& params) override;
    void Unload() override;

protected:
    // [MaterialShader]
    bool Load() override;
};
