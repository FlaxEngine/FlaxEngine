// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "MaterialShader.h"

/// <summary>
/// Represents material that can be used to render GUI.
/// </summary>
class GUIMaterialShader : public MaterialShader
{
private:
    struct Cache
    {
        GPUPipelineState* Depth = nullptr;
        GPUPipelineState* NoDepth = nullptr;

        FORCE_INLINE void Release()
        {
            SAFE_DELETE_GPU_RESOURCE(Depth);
            SAFE_DELETE_GPU_RESOURCE(NoDepth);
        }
    };

private:
    Cache _cache;

public:
    /// <summary>
    /// Init
    /// </summary>
    /// <param name="name">Material resource name</param>
    GUIMaterialShader(const StringView& name)
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
