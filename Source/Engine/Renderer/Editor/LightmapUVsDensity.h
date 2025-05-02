// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if USE_EDITOR

#include "Engine/Content/AssetReference.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Content/Assets/Texture.h"
#include "Engine/Graphics/Materials/IMaterial.h"

class GPUPipelineState;

/// <summary>
/// Lightmap UVs Density rendering for profiling and debugging in editor.
/// </summary>
class LightmapUVsDensityMaterialShader : public IMaterial
{
private:
    AssetReference<Shader> _shader;
    AssetReference<Texture> _gridTexture;
    GPUPipelineState* _ps = nullptr;
    MaterialInfo _info;

public:
    LightmapUVsDensityMaterialShader();
    virtual ~LightmapUVsDensityMaterialShader()
    {
    }

private:
#if COMPILE_WITH_DEV_ENV
    void OnShaderReloading(Asset* obj);
#endif

public:
    // [IMaterial]
    const MaterialInfo& GetInfo() const override;
    GPUShader* GetShader() const override;
    bool IsReady() const override;
    DrawPass GetDrawModes() const override;
    void Bind(BindParameters& params) override;
};

#endif
