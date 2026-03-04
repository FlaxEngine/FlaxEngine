// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Graphics/GPUPipelineState.h"
#include "GPUShaderProgramWebGPU.h"
#include "GPUDeviceWebGPU.h"

#if GRAPHICS_API_WEBGPU

/// <summary>
/// Graphics pipeline state object for Web GPU backend.
/// </summary>
class GPUPipelineStateWebGPU : public GPUResourceWebGPU<GPUPipelineState>
{
public:
    // Batches render context state for the pipeline state. Used as a key for caching created pipelines.
    struct PipelineKey
    {
        union
        {
            struct
            {
                uint16 DepthStencilFormat : 7;
                uint16 MultiSampleCount : 3;
                uint16 RenderTargetCount : 3;
                uint8 RenderTargetFormats[GPU_MAX_RT_BINDED];
                class GPUVertexLayoutWebGPU* VertexLayout;
            };
            uint64 Packed[2];
        };

        FORCE_INLINE bool operator==(const PipelineKey& other) const
        {
            return Platform::MemoryCompare(&Packed, &other.Packed, sizeof(Packed)) == 0;
        }
    };

    // Batches bind group description for the pipeline state. Used as a key for caching created bind groups.
    typedef GPUBindGroupKeyWebGPU BindGroupKey;

private:
#if GPU_ENABLE_RESOURCE_NAMING
    DebugName _debugName;
#endif
    WGPUDepthStencilState _depthStencilDesc;
    WGPUFragmentState _fragmentDesc;
    WGPUBlendState _blendState;
    WGPUColorTargetState _colorTargets[GPU_MAX_RT_BINDED];
    WGPUVertexBufferLayout _vertexBuffers[GPU_MAX_VB_BINDED];
    Dictionary<PipelineKey, WGPURenderPipeline> _pipelines;
    Dictionary<BindGroupKey, WGPUBindGroup> _bindGroups;
    GPUBindGroupCacheWebGPU _bindGroupCache;

public:
    GPUShaderProgramVSWebGPU* VS = nullptr;
    GPUShaderProgramPSWebGPU* PS = nullptr;
    WGPURenderPipelineDescriptor PipelineDesc;
    WGPUBindGroupLayout BindGroupLayouts[GPUBindGroupsWebGPU::GraphicsMax] = {};
    SpirvShaderDescriptorInfo* BindGroupDescriptors[GPUBindGroupsWebGPU::GraphicsMax] = {};

public:
    GPUPipelineStateWebGPU(GPUDeviceWebGPU* device)
        : GPUResourceWebGPU<GPUPipelineState>(device, StringView::Empty)
    {
    }

public:
    // Gets the pipeline for the given rendering state. Pipelines are cached and reused for the same key.
    WGPURenderPipeline GetPipeline(const PipelineKey& key, const GPUContextBindingsWebGPU& bindings);

    // Gets the bind group for the given key (unhashed). Bind groups are cached and reused for the same key.
    FORCE_INLINE WGPUBindGroup GetBindGroup(BindGroupKey& key)
    {
#if GPU_ENABLE_RESOURCE_NAMING
        StringAnsiView debugName(_debugName.Get(), _debugName.Count() - 1);
#else
        StringAnsiView debugName;
#endif
        return _bindGroupCache.Get(_device->Device, key, debugName);
    }

private:
    void InitLayout(const GPUContextBindingsWebGPU& bindings);

public:
    // [GPUPipelineState]
    bool IsValid() const override;
    bool Init(const Description& desc) override;

protected:
    // [GPUResourceWebGPU]
    void OnReleaseGPU() final override;
};

uint32 GetHash(const GPUPipelineStateWebGPU::PipelineKey& key);
uint32 GetHash(const GPUBindGroupKeyWebGPU& key);

#endif
