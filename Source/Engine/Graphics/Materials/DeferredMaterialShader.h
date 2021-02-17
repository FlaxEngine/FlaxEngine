// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "MaterialShader.h"

/// <summary>
/// Represents material that can be used to render objects to GBuffer.
/// </summary>
class DeferredMaterialShader : public MaterialShader
{
private:

    struct Cache
    {
        PipelineStateCache Default;
        PipelineStateCache DefaultSkinned;
        PipelineStateCache DefaultLightmap;
        PipelineStateCache Depth;
        PipelineStateCache DepthSkinned;
        PipelineStateCache MotionVectors;
        PipelineStateCache MotionVectorsSkinned;
        PipelineStateCache MotionVectorsSkinnedPerBone;

        FORCE_INLINE PipelineStateCache* GetPS(const DrawPass pass, const bool useLightmap, const bool useSkinning, const bool perBoneMotionBlur)
        {
            switch (pass)
            {
            case DrawPass::Depth:
                return useSkinning ? &DepthSkinned : &Depth;
            case DrawPass::GBuffer:
                return useLightmap ? &DefaultLightmap : (useSkinning ? &DefaultSkinned : &Default);
            case DrawPass::MotionVectors:
                return useSkinning ? (perBoneMotionBlur ? &MotionVectorsSkinnedPerBone : &MotionVectorsSkinned) : &MotionVectors;
            default:
                return nullptr;
            }
        }

        FORCE_INLINE void Release()
        {
            Default.Release();
            DefaultSkinned.Release();
            DefaultLightmap.Release();
            Depth.Release();
            DepthSkinned.Release();
            MotionVectors.Release();
            MotionVectorsSkinned.Release();
        }
    };

private:

    Cache _cache;
    Cache _cacheInstanced;

public:

    DeferredMaterialShader(const String& name)
        : MaterialShader(name)
    {
    }

public:

    // [MaterialShader]
    DrawPass GetDrawModes() const override;
    bool CanUseLightmap() const override;
    bool CanUseInstancing(InstancingHandler& handler) const override;
    void Bind(BindParameters& params) override;
    void Unload() override;

protected:

    // [MaterialShader]
    bool Load() override;
};
