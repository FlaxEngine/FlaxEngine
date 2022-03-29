// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "RendererPass.h"

/// <summary>
/// Global Surface Atlas rendering pass. Captures scene geometry into a single atlas texture which contains surface diffuse color, normal vector, emission light, and calculates direct+indirect lighting. Used by Global Illumination and Reflections.
/// </summary>
class FLAXENGINE_API GlobalSurfaceAtlasPass : public RendererPass<GlobalSurfaceAtlasPass>
{
public:
    // Binding data for the GPU.
    struct BindingData
    {
        GPUTexture* Dummy; // TODO: add textures
    };

private:
    bool _supported = false;
    AssetReference<Shader> _shader;
    GPUPipelineState* _psDebug = nullptr;
    GPUConstantBuffer* _cb0 = nullptr;

public:
    /// <summary>
    /// Renders the Global Surface Atlas.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="context">The GPU context.</param>
    /// <param name="result">The result Global Surface Atlas data for binding to the shaders.</param>
    /// <returns>True if failed to render (platform doesn't support it, out of video memory, disabled feature or effect is not ready), otherwise false.</returns>
    bool Render(RenderContext& renderContext, GPUContext* context, BindingData& result);

    /// <summary>
    /// Renders the debug view.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="context">The GPU context.</param>
    /// <param name="output">The output buffer.</param>
    void RenderDebug(RenderContext& renderContext, GPUContext* context, GPUTexture* output);

private:
#if COMPILE_WITH_DEV_ENV
    void OnShaderReloading(Asset* obj);
#endif

public:
    // [RendererPass]
    String ToString() const override;
    bool Init() override;
    void Dispose() override;

protected:
    // [RendererPass]
    bool setupResources() override;
};
