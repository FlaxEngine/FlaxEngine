// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX11

#include "GPUPipelineStateDX11.h"

void GPUPipelineStateDX11::OnReleaseGPU()
{
    BlendState = nullptr;
    VS = nullptr;
    HS = nullptr;
    DS = nullptr;
    GS = nullptr;
    PS = nullptr;
}

ID3D11Resource* GPUPipelineStateDX11::GetResource()
{
    return nullptr;
}

bool GPUPipelineStateDX11::IsValid() const
{
    return !!_memoryUsage;
}

bool GPUPipelineStateDX11::Init(const Description& desc)
{
    ASSERT(!IsValid());

    // Cache shaders
    VS = (GPUShaderProgramVSDX11*)desc.VS;
    HS = desc.HS ? (GPUShaderProgramHSDX11*)desc.HS : nullptr;
    DS = desc.DS ? (GPUShaderProgramDSDX11*)desc.DS : nullptr;
    GS = desc.GS ? (GPUShaderProgramGSDX11*)desc.GS : nullptr;
    PS = desc.PS ? (GPUShaderProgramPSDX11*)desc.PS : nullptr;

    // Primitive Topology
    const static D3D11_PRIMITIVE_TOPOLOGY D3D11_primTypes[] =
    {
        D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED,
        D3D11_PRIMITIVE_TOPOLOGY_POINTLIST,
        D3D11_PRIMITIVE_TOPOLOGY_LINELIST,
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
    };
    PrimitiveTopology = D3D11_primTypes[static_cast<int32>(desc.PrimitiveTopology)];
    if (HS)
        PrimitiveTopology = (D3D11_PRIMITIVE_TOPOLOGY)((int32)D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST + (HS->GetControlPointsCount() - 1));

    // States
    DepthStencilStateIndex = static_cast<int32>(desc.DepthFunc) + (desc.DepthEnable ? 0 : 9) + (desc.DepthWriteEnable ? 0 : 18);
    RasterizerStateIndex = static_cast<int32>(desc.CullMode) + (desc.Wireframe ? 0 : 3) + (desc.DepthClipEnable ? 0 : 6);
    BlendState = _device->GetBlendState(desc.BlendMode);

    // Calculate approx. memory usage (just to set sth)
    _memoryUsage = sizeof(D3D11_DEPTH_STENCIL_DESC) + sizeof(D3D11_RASTERIZER_DESC) + sizeof(D3D11_BLEND_DESC);

    return GPUPipelineState::Init(desc);
}

#endif
