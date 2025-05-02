// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "MaterialShader.h"

/// <summary>
/// Represents material that can be used to render particles.
/// </summary>
class ParticleMaterialShader : public MaterialShader
{
private:
    struct Cache
    {
        PipelineStateCache Default;
        PipelineStateCache Depth;
        PipelineStateCache Distortion;
#if USE_EDITOR
        PipelineStateCache QuadOverdraw;
#endif

        FORCE_INLINE PipelineStateCache* GetPS(const DrawPass pass)
        {
            switch (pass)
            {
            case DrawPass::Depth:
                return &Depth;
            case DrawPass::Distortion:
                return &Distortion;
            case DrawPass::Forward:
                return &Default;
#if USE_EDITOR
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
            Depth.Release();
            Distortion.Release();
#if USE_EDITOR
            QuadOverdraw.Release();
#endif
        }
    };

private:
    Cache _cacheSprite;
    Cache _cacheModel;
    Cache _cacheRibbon;
    PipelineStateCache _cacheVolumetricFog;
    DrawPass _drawModes = DrawPass::None;

public:
    /// <summary>
    /// Init
    /// </summary>
    /// <param name="name">Material resource name</param>
    ParticleMaterialShader(const StringView& name)
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
