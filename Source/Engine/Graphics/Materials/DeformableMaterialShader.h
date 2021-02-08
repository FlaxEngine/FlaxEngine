// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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

        FORCE_INLINE PipelineStateCache* GetPS(const DrawPass pass)
        {
            switch (pass)
            {
            case DrawPass::Depth:
                return &Depth;
            case DrawPass::GBuffer:
            case DrawPass::Forward:
                return &Default;
            default:
                return nullptr;
            }
        }

        FORCE_INLINE void Release()
        {
            Default.Release();
            Depth.Release();
        }
    };

private:

    Cache _cache;
    DrawPass _drawModes = DrawPass::None;

public:

    DeformableMaterialShader(const String& name)
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
