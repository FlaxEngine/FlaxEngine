// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "MaterialShader.h"

/// <summary>
/// Represents material that can be used to render objects with Forward Rendering.
/// </summary>
class ForwardMaterialShader : public MaterialShader
{
private:

    struct Cache
    {
        PipelineStateCache Default;
        PipelineStateCache DefaultSkinned;
        PipelineStateCache Depth;
        PipelineStateCache DepthSkinned;
        PipelineStateCache Distortion;
        PipelineStateCache DistortionSkinned;

        FORCE_INLINE PipelineStateCache* GetPS(const DrawPass pass, const bool useSkinning)
        {
            switch (pass)
            {
            case DrawPass::Depth:
                return useSkinning ? &DepthSkinned : &Depth;
            case DrawPass::Distortion:
                return useSkinning ? &DistortionSkinned : &Distortion;
            case DrawPass::Forward:
                return useSkinning ? &DefaultSkinned : &Default;
            default:
                return nullptr;
            }
        }

        FORCE_INLINE void Release()
        {
            Default.Release();
            DefaultSkinned.Release();
            Depth.Release();
            DepthSkinned.Release();
            Distortion.Release();
            DistortionSkinned.Release();
        }
    };

private:

    Cache _cache;
    Cache _cacheInstanced;
    DrawPass _drawModes = DrawPass::None;

public:

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="name">Material resource name</param>
    ForwardMaterialShader(const String& name)
        : MaterialShader(name)
    {
    }

public:

    // [MaterialShader]
    DrawPass GetDrawModes() const override;
    bool CanUseInstancing(InstancingHandler& handler) const override;
    void Bind(BindParameters& params) override;
    void Unload() override;

protected:

    // [MaterialShader]
    bool Load() override;
};
