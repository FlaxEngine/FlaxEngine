// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

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
    struct Key
    {
        union
        {
            struct
            {
                uint16 DepthStencilFormat : 7;
                uint16 MultiSampleCount : 3;
                uint16 RenderTargetCount : 3;
                uint16 VertexBufferCount : 3;
                uint8 RenderTargetFormats[GPU_MAX_RT_BINDED];
                struct WGPUVertexBufferLayout* VertexBuffers[GPU_MAX_VB_BINDED];
            };
            uint64 Packed[4];
        };

        FORCE_INLINE bool operator==(const Key& other) const
        {
            return Platform::MemoryCompare(&Packed, &other.Packed, sizeof(Packed)) == 0;
        }
    };

private:
#if GPU_ENABLE_RESOURCE_NAMING
    DebugName _debugName;
#endif
    WGPUDepthStencilState _depthStencilDesc;
    WGPUFragmentState _fragmentDesc;
    WGPUBlendState _blendState;
    WGPUColorTargetState _colorTargets[GPU_MAX_RT_BINDED];
    WGPUVertexBufferLayout _vertexBuffers[GPU_MAX_VB_BINDED];
    Dictionary<Key, WGPURenderPipeline> _pipelines;

public:
    GPUShaderProgramVSWebGPU* VS = nullptr;
    GPUShaderProgramPSWebGPU* PS = nullptr;
    WGPURenderPipelineDescriptor PipelineDesc;

public:
    GPUPipelineStateWebGPU(GPUDeviceWebGPU* device)
        : GPUResourceWebGPU<GPUPipelineState>(device, StringView::Empty)
    {
    }

public:
    WGPURenderPipeline GetPipeline(const Key& key);

public:
    // [GPUPipelineState]
    bool IsValid() const override;
    bool Init(const Description& desc) override;

protected:
    // [GPUResourceWebGPU]
    void OnReleaseGPU() final override;
};

uint32 GetHash(const GPUPipelineStateWebGPU::Key& key);

#endif
