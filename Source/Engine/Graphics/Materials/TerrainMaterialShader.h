// Copyright (c) Wojciech Figat. All rights reserved.

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
#if GPU_ENABLE_DEVELOPMENT
        PipelineStateCache QuadOverdraw;
#endif

        FORCE_INLINE PipelineStateCache* GetPS(const DrawPass pass, const bool useLightmap)
        {
            switch (pass)
            {
            case DrawPass::Depth:
                return &Depth;
            case DrawPass::GBuffer:
            case DrawPass::GBuffer | DrawPass::GlobalSurfaceAtlas:
            case DrawPass::GlobalSurfaceAtlas:
                return useLightmap ? &DefaultLightmap : &Default;
#if GPU_ENABLE_DEVELOPMENT
            case DrawPass::QuadOverdraw:
                return &QuadOverdraw;
#endif
            default:
                return nullptr;
            }
        }

        FORCE_INLINE void Release()
        {
            Default.Release();
            DefaultLightmap.Release();
            Depth.Release();
#if GPU_ENABLE_DEVELOPMENT
            QuadOverdraw.Release();
#endif
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
