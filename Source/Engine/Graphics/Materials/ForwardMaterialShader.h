// Copyright (c) Wojciech Figat. All rights reserved.

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
#if GPU_ENABLE_DEVELOPMENT
        PipelineStateCache QuadOverdraw;
        PipelineStateCache QuadOverdrawSkinned;
        PipelineStateCache Wireframe;
        PipelineStateCache WireframeSkinned;
#endif

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
#if GPU_ENABLE_DEVELOPMENT
            case DrawPass::QuadOverdraw:
                return useSkinning ? &QuadOverdrawSkinned : &QuadOverdraw;
            case DrawPass::Wireframe:
                return useSkinning ? &WireframeSkinned : &Wireframe;
#endif
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
#if GPU_ENABLE_DEVELOPMENT
            QuadOverdraw.Release();
            QuadOverdrawSkinned.Release();
            Wireframe.Release();
            WireframeSkinned.Release();
#endif
        }
    };

private:
    Cache _cache;
    Cache _cacheInstanced;

public:
    /// <summary>
    /// Init
    /// </summary>
    /// <param name="name">Material resource name</param>
    ForwardMaterialShader(const StringView& name)
        : MaterialShader(name)
    {
    }

public:
    // [MaterialShader]
    DrawPass GetDrawModes() const override;
    bool CanUseInstancing(const RenderContext& renderContext, InstancingHandler& handler) const override;
    void Bind(BindParameters& params) override;
    void Unload() override;

protected:
    // [MaterialShader]
    bool Load() override;
};
