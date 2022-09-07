// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#if USE_EDITOR

#include "Engine/Content/AssetReference.h"
#include "Engine/Content/Assets/Material.h"
#include "Engine/Graphics/Materials/IMaterial.h"
#include "Engine/Content/Assets/Shader.h"

class GPUPipelineState;

/// <summary>
/// Rendering material shaders complexity to visualize performance of pixels rendering in editor.
/// </summary>
class MaterialComplexityMaterialShader
{
private:

    class WrapperShader : public IMaterial
    {
    public:
        MaterialInfo Info;
        MaterialDomain Domain;
        AssetReference<Material> MaterialAsset;
        const MaterialInfo& GetInfo() const override;
        GPUShader* GetShader() const override;
        bool IsReady() const override;
        bool CanUseInstancing(InstancingHandler& handler) const override;
        DrawPass GetDrawModes() const override;
        void Bind(BindParameters& params) override;
    };

    WrapperShader _wrappers[5];
    AssetReference<Shader> _shader;
    GPUPipelineState* _ps = nullptr;

public:

    MaterialComplexityMaterialShader();
    void DebugOverrideDrawCallsMaterial(RenderContext& renderContext, GPUContext* context, GPUTextureView* lightBuffer);
    void Draw(RenderContext& renderContext, GPUContext* context, GPUTextureView* lightBuffer);

private:

    void DebugOverrideDrawCallsMaterial(struct DrawCall& drawCall, const bool isReady[ARRAY_COUNT(_wrappers) + 1]);
};

#endif
