#pragma once

#include "Engine/Graphics/RenderView.h"
#include "RendererPass.h"
#include "Engine/Content/Assets/Shader.h"

/// <summary>
/// Custom Depth Pass.
/// </summary>
class CustomDepthPass : public RendererPass<CustomDepthPass>
{
public:
    void Render(RenderContext& renderContext);
    void Clear(RenderContext& renderContext);
public:
    // [RendererPass]
    String ToString() const override;
};
