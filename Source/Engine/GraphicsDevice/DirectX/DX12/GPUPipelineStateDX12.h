// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_DIRECTX12

#include "Engine/Graphics/GPUPipelineState.h"
#include "GPUDeviceDX12.h"
#include "Types.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "../IncludeDirectXHeaders.h"

class GPUTextureViewDX12;
class GPUVertexLayoutDX12;

struct GPUPipelineStateKeyDX12
{
    int32 RTsCount;
    MSAALevel MSAA;
    GPUVertexLayout* VertexLayout;
    PixelFormat DepthFormat;
    PixelFormat RTVsFormats[GPU_MAX_RT_BINDED];

    bool operator==(const GPUPipelineStateKeyDX12& other) const
    {
        return Platform::MemoryCompare((void*)this, &other, sizeof(GPUPipelineStateKeyDX12)) == 0;
    }

    friend inline uint32 GetHash(const GPUPipelineStateKeyDX12& key)
    {
        uint32 hash = (int32)key.MSAA * 11;
        CombineHash(hash, (uint32)key.DepthFormat * 93473262);
        CombineHash(hash, key.RTsCount * 136);
        CombineHash(hash, GetHash(key.VertexLayout));
        CombineHash(hash, (uint32)key.RTVsFormats[0]);
        CombineHash(hash, (uint32)key.RTVsFormats[1]);
        CombineHash(hash, (uint32)key.RTVsFormats[2]);
        CombineHash(hash, (uint32)key.RTVsFormats[3]);
        CombineHash(hash, (uint32)key.RTVsFormats[4]);
        CombineHash(hash, (uint32)key.RTVsFormats[5]);
        static_assert(GPU_MAX_RT_BINDED == 6, "Update hash combine code to match RT count (manually inlined loop).");
        return hash;
    }
};

/// <summary>
/// Graphics pipeline state object for DirectX 12 backend.
/// </summary>
class GPUPipelineStateDX12 : public GPUResourceDX12<GPUPipelineState>
{
private:
    Dictionary<GPUPipelineStateKeyDX12, ID3D12PipelineState*> _states;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC _desc;

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
    /// <param name="depth">The depth buffer (can be null).</param>
    /// <param name="rtCount">The render targets count (can be 0).</param>
    /// <param name="rtHandles">The render target handles array.</param>
    /// <param name="vertexLayout">The vertex buffers layout.</param>
    /// <returns>DirectX 12 graphics pipeline state object</returns>
    ID3D12PipelineState* GetState(GPUTextureViewDX12* depth, int32 rtCount, GPUTextureViewDX12** rtHandles, GPUVertexLayoutDX12* vertexLayout);

public:
    // [GPUPipelineState]
    bool IsValid() const override;
    bool Init(const Description& desc) override;

protected:
    // [GPUResourceDX12]
    void OnReleaseGPU() override;
};

#endif
