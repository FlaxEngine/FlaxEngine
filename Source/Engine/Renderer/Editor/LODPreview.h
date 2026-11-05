// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/Config.h"

#if GPU_ENABLE_DEVELOPMENT

#include "Engine/Content/AssetReference.h"
#include "Engine/Content/Assets/Material.h"
#include "Engine/Graphics/Materials/IMaterial.h"

class GPUPipelineState;

/// <summary>
/// Rendering Level Of Detail number as colors to debug LOD switches in editor.
/// </summary>
class LODPreviewMaterialShader : public IMaterial
{
private:

    AssetReference<Material> _material;

public:

    LODPreviewMaterialShader();
    virtual ~LODPreviewMaterialShader()
    {
    }

public:

    // [IMaterial]
    const MaterialInfo& GetInfo() const override;
    GPUShader* GetShader() const override;
    bool IsReady() const override;
    bool CanUseInstancing(const RenderContext& renderContext, InstancingHandler& handler) const override;
    DrawPass GetDrawModes() const override;
    void Bind(BindParameters& params) override;
};

#endif
