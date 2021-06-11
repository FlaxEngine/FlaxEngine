// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "MaterialShader.h"

/// <summary>
/// Represents material that can be used to render terrain.
/// </summary>
class TerrainMaterialShader : public MaterialShader
{
private:

    struct Cache
    {
        PipelineStateCache Default;
        PipelineStateCache DefaultLightmap;
        PipelineStateCache Depth;

        FORCE_INLINE PipelineStateCache* GetPS(const DrawPass pass, const bool useLightmap)
        {
            switch (pass)
            {
            case DrawPass::Depth:
                return &Depth;
            case DrawPass::GBuffer:
                return useLightmap ? &DefaultLightmap : &Default;
            default:
                return nullptr;
            }
        }

        FORCE_INLINE void Release()
        {
            Default.Release();
            DefaultLightmap.Release();
            Depth.Release();
        }
    };

private:

    Cache _cache;

public:

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="name">Material resource name</param>
    TerrainMaterialShader(const StringView& name)
        : MaterialShader(name)
    {
    }

public:

    // [MaterialShader]
    DrawPass GetDrawModes() const override;
    bool CanUseLightmap() const override;
    void Bind(BindParameters& params) override;
    void Unload() override;

protected:

    // [MaterialShader]
    bool Load() override;
};
