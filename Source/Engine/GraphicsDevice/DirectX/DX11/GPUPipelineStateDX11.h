// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/GPUPipelineState.h"
#include "GPUShaderProgramDX11.h"
#include "GPUDeviceDX11.h"

#if GRAPHICS_API_DIRECTX11

/// <summary>
/// Graphics pipeline state object for DirectX 11 backend.
/// </summary>
class GPUPipelineStateDX11 : public GPUResourceDX11<GPUPipelineState>
{
public:

    int32 RasterizerStateIndex;
    ID3D11DepthStencilState* DepthStencilState = nullptr;
    ID3D11BlendState* BlendState = nullptr;
    GPUShaderProgramVSDX11* VS = nullptr;
#if GPU_ALLOW_TESSELLATION_SHADERS
    GPUShaderProgramHSDX11* HS = nullptr;
    GPUShaderProgramDSDX11* DS = nullptr;
#endif
#if GPU_ALLOW_GEOMETRY_SHADERS
    GPUShaderProgramGSDX11* GS = nullptr;
#endif
    GPUShaderProgramPSDX11* PS = nullptr;
    D3D11_PRIMITIVE_TOPOLOGY PrimitiveTopology;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUPipelineStateDX11"/> class.
    /// </summary>
    /// <param name="device">The device.</param>
    GPUPipelineStateDX11(GPUDeviceDX11* device)
        : GPUResourceDX11<GPUPipelineState>(device, StringView::Empty)
    {
    }

public:

    // [GPUResourceDX11]
    ID3D11Resource* GetResource() final override;

    // [GPUPipelineState]
    bool IsValid() const override;
    bool Init(const Description& desc) override;

protected:

    // [GPUResourceDX11]
    void OnReleaseGPU() final override;
};

#endif
