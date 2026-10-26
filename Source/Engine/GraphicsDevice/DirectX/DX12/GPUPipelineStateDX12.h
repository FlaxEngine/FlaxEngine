// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_DIRECTX12

#include "Engine/Graphics/GPUPipelineState.h"
#include "GPUDeviceDX12.h"
#include "GPUPipelineStateKeyDX12.h"
#include "Types.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "../IncludeDirectXHeaders.h"
#ifdef __ID3D12Device2_FWD_DEFINED__
#define GPU_D3D12_PSO_STREAM 1
#else
#define GPU_D3D12_PSO_STREAM 0
#endif

class GPUTextureViewDX12;
class GPUVertexLayoutDX12;

/// <summary>
/// Graphics pipeline state object for DirectX 12 backend.
/// </summary>
class GPUPipelineStateDX12 : public GPUResourceDX12<GPUPipelineState>
{
private:
    Dictionary<GPUPipelineStateKeyDX12, ID3D12PipelineState*> _states;
#if GPU_D3D12_PSO_STREAM
    D3D12_DEPTH_STENCIL_DESC1 _depthStencil;
#else
    D3D12_DEPTH_STENCIL_DESC _depthStencil;
#endif
    D3D12_RASTERIZER_DESC _rasterizer;
    D3D12_BLEND_DESC _blend;
    D3D12_PRIMITIVE_TOPOLOGY_TYPE _primitiveTopology;
#if GPU_ALLOW_TESSELLATION_SHADERS
    D3D12_SHADER_BYTECODE _shaderHS, _shaderDS;
#endif
#if GPU_ALLOW_GEOMETRY_SHADERS
    D3D12_SHADER_BYTECODE _shaderGS;
#endif
    D3D12_SHADER_BYTECODE _shaderVS, _shaderPS;
#if GPU_ENABLE_DEVELOPMENT && COMPILE_WITH_PSO_PRECACHING
    Array<GPUPipelineStateKeyDX12, InlinedAllocation<2>> _precachedPSOs;
#endif

public:
    GPUPipelineStateDX12(GPUDeviceDX12* device);

public:
    D3D_PRIMITIVE_TOPOLOGY PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    DxShaderHeader Header;
    GPUVertexLayoutDX12* VertexBufferLayout = nullptr;
    GPUVertexLayoutDX12* VertexInputLayout = nullptr;

    /// <summary>
    /// Gets DirectX 12 graphics pipeline state object for the given rendering state. Uses depth buffer and render targets formats and multi-sample levels to setup a proper PSO. Uses caching.
    /// </summary>
    /// <param name="key">The PSO dynamic key.</param>
    /// <param name="precaching">True if action is called during precaching of PSO, instead of during rendering.</param>
    /// <returns>DirectX 12 graphics pipeline state object</returns>
    ID3D12PipelineState* GetState(const GPUPipelineStateKeyDX12& key, bool precaching = false);

public:
    // [GPUPipelineState]
    bool IsValid() const override;
    bool Init(const Description& desc) override;
#if COMPILE_WITH_PSO_PRECACHING
    void PrecachePSO(const PrecacheContext& context) override;
#endif

protected:
    // [GPUResourceDX12]
    void OnReleaseGPU() override;
};

#endif
