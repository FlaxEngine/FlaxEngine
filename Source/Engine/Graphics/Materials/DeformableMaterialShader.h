// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "MaterialShader.h"

/// <summary>
/// Represents material that can be used to render objects that can be deformed.
/// </summary>
class DeformableMaterialShader : public MaterialShader
{
private:
    struct Cache
    {
        PipelineStateCache Default;
        PipelineStateCache Depth;
        PipelineStateCache Distortion;
#if GPU_ENABLE_DEVELOPMENT
        PipelineStateCache QuadOverdraw;
        PipelineStateCache Wireframe;
#endif

        FORCE_INLINE PipelineStateCache* GetPS(const DrawPass pass)
        {
            switch (pass)
            {
            case DrawPass::Depth:
                return &Depth;
            case DrawPass::GBuffer:
            case DrawPass::GBuffer | DrawPass::GlobalSurfaceAtlas:
            case DrawPass::GlobalSurfaceAtlas:
            case DrawPass::Forward:
                return &Default;
            case DrawPass::Distortion:
                return &Distortion;
#if GPU_ENABLE_DEVELOPMENT
            case DrawPass::QuadOverdraw:
                return &QuadOverdraw;
            case DrawPass::Wireframe:
                return &Wireframe;
#endif
            default:
                return nullptr;
            }
        }

        FORCE_INLINE void Release()
        {
            Default.Release();
            Depth.Release();
            Distortion.Release();
#if GPU_ENABLE_DEVELOPMENT
            QuadOverdraw.Release();
            Wireframe.Release();
#endif
        }
    };

private:
    Cache _cache;

public:
    DeformableMaterialShader(const StringView& name)
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
