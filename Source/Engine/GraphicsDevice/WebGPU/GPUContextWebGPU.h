// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/GPUContext.h"
#include "Engine/Core/Math/Vector4.h"
#include "GPUDeviceWebGPU.h"
#include "GPUPipelineStateWebGPU.h"

#if GRAPHICS_API_WEBGPU

class GPUBufferWebGPU;
class GPUSamplerWebGPU;
class GPUTextureViewWebGPU;
class GPUVertexLayoutWebGPU;
class GPUConstantBufferWebGPU;

/// <summary>
/// GPU Context for Web GPU backend.
/// </summary>
class GPUContextWebGPU : public GPUContext
{
private:
    struct BufferBind
    {
        WGPUBuffer Buffer;
        uint32 Size, Offset;
    };

    struct PendingClear
    {
        GPUTextureViewWebGPU* View;
        union
        {
            float RGBA[4];
            struct
            {
                float Depth;
                uint32 Stencil;
            };
        };
    };

    GPUDeviceWebGPU* _device;
    uint32 _minUniformBufferOffsetAlignment;
    int32 _activeOcclusionQuerySet = -1;
    int32 _pendingOcclusionQuerySet = -1;
    uint32 _usedQuerySets = 0;
    Array<WGPUPassTimestampWrites> _pendingTimestampWrites;

    // State tracking
    uint32 _renderPassDirty : 1;
    uint32 _pipelineDirty : 1;
    uint32 _bindGroupDirty : 1;
    uint32 _vertexBufferDirty : 1;
    uint32 _indexBufferDirty : 1;
    uint32 _indexBuffer32Bit : 1;
    uint32 _blendFactorDirty : 1;
    uint32 _blendFactorSet : 1;
    uint32 _renderTargetCount : 4;
    uint32 _vertexBufferCount : 4;
    uint32 _stencilRef;
    Float4 _blendFactor;
    Viewport _viewport;
    Rectangle _scissorRect;
    WGPURenderPassEncoder _renderPass;
    GPUTextureViewWebGPU* _depthStencil;
    GPUTextureViewWebGPU* _renderTargets[GPU_MAX_RT_BINDED];
    GPUConstantBufferWebGPU* _constantBuffers[GPU_MAX_CB_BINDED];
    GPUSamplerWebGPU* _samplers[GPU_MAX_SAMPLER_BINDED];
    BufferBind _indexBuffer;
    BufferBind _vertexBuffers[GPU_MAX_VB_BINDED];
    GPUPipelineStateWebGPU* _pipelineState;
    GPUPipelineStateWebGPU::PipelineKey _pipelineKey;
    Array<PendingClear, FixedAllocation<16>> _pendingClears;
    GPUResourceView* _shaderResources[GPU_MAX_SR_BINDED];
    GPUResourceView* _storageResources[GPU_MAX_SR_BINDED];
    GPUResourceView** _resourceTables[(int32)SpirvShaderResourceBindingType::MAX];
#if ENABLE_ASSERTION
    uint32 _resourceTableSizes[(int32)SpirvShaderResourceBindingType::MAX];
#endif

public:
    GPUContextWebGPU(GPUDeviceWebGPU* device);
    ~GPUContextWebGPU();

public:
    // Handle to the WebGPU command encoder object.
    WGPUCommandEncoder Encoder = nullptr;

private:
    void WriteTimestamp(GPUQuerySetWebGPU* set, uint32 index);
    bool FindClear(const GPUTextureViewWebGPU* view, PendingClear& clear);
    void ManualClear(const PendingClear& clear);
    void OnDrawCall();
    WGPUComputePassEncoder OnDispatch(GPUShaderProgramCS* shader);
    void EndRenderPass();
    void EndComputePass(WGPUComputePassEncoder computePass);
    void FlushRenderPass();
    void FlushBindGroup();
    void FlushTimestamps(int32 skipLast = 0);
    constexpr static int32 DynamicOffsetsMax = 4;
    void BuildBindGroup(uint32 groupIndex, const SpirvShaderDescriptorInfo& descriptors, GPUPipelineStateWebGPU::BindGroupKey& key, uint32 dynamicOffsets[DynamicOffsetsMax], uint32& dynamicOffsetsCount);

public:
    // [GPUContext]
    void FrameBegin() override;
    void FrameEnd() override;
#if GPU_ALLOW_PROFILE_EVENTS
    void EventBegin(const Char* name) override;
    void EventEnd() override;
#endif
    void* GetNativePtr() const override;
    bool IsDepthBufferBinded() override;
    void Clear(GPUTextureView* rt, const Color& color) override;
    void ClearDepth(GPUTextureView* depthBuffer, float depthValue, uint8 stencilValue) override;
    void ClearUA(GPUBuffer* buf, const Float4& value) override;
    void ClearUA(GPUBuffer* buf, const uint32 value[4]) override;
    void ClearUA(GPUTexture* texture, const uint32 value[4]) override;
    void ClearUA(GPUTexture* texture, const Float4& value) override;
    void ResetRenderTarget() override;
    void SetRenderTarget(GPUTextureView* rt) override;
    void SetRenderTarget(GPUTextureView* depthBuffer, GPUTextureView* rt) override;
    void SetRenderTarget(GPUTextureView* depthBuffer, const Span<GPUTextureView*>& rts) override;
    void SetBlendFactor(const Float4& value) override;
    void SetStencilRef(uint32 value) override;
    void ResetSR() override;
    void ResetUA() override;
    void ResetCB() override;
    void BindCB(int32 slot, GPUConstantBuffer* cb) override;
    void BindSR(int32 slot, GPUResourceView* view) override;
    void BindUA(int32 slot, GPUResourceView* view) override;
    void BindVB(const Span<GPUBuffer*>& vertexBuffers, const uint32* vertexBuffersOffsets = nullptr, GPUVertexLayout* vertexLayout = nullptr) override;
    void BindIB(GPUBuffer* indexBuffer) override;
    void BindSampler(int32 slot, GPUSampler* sampler) override;
    void UpdateCB(GPUConstantBuffer* cb, const void* data) override;
    void Dispatch(GPUShaderProgramCS* shader, uint32 threadGroupCountX, uint32 threadGroupCountY, uint32 threadGroupCountZ) override;
    void DispatchIndirect(GPUShaderProgramCS* shader, GPUBuffer* bufferForArgs, uint32 offsetForArgs) override;
    void ResolveMultisample(GPUTexture* sourceMultisampleTexture, GPUTexture* destTexture, int32 sourceSubResource, int32 destSubResource, PixelFormat format) override;
    void DrawInstanced(uint32 verticesCount, uint32 instanceCount, int32 startInstance, int32 startVertex) override;
    void DrawIndexedInstanced(uint32 indicesCount, uint32 instanceCount, int32 startInstance, int32 startVertex, int32 startIndex) override;
    void DrawInstancedIndirect(GPUBuffer* bufferForArgs, uint32 offsetForArgs) override;
    void DrawIndexedInstancedIndirect(GPUBuffer* bufferForArgs, uint32 offsetForArgs) override;
    uint64 BeginQuery(GPUQueryType type) override;
    void EndQuery(uint64 queryID) override;
    void SetViewport(const Viewport& viewport) override;
    void SetScissor(const Rectangle& scissorRect) override;
    void SetDepthBounds(float minDepth, float maxDepth) override;
    GPUPipelineState* GetState() const override;
    void SetState(GPUPipelineState* state) override;
    void ResetState() override;
    void FlushState() override;
    void Flush() override;
    void UpdateBuffer(GPUBuffer* buffer, const void* data, uint32 size, uint32 offset) override;
    void CopyBuffer(GPUBuffer* dstBuffer, GPUBuffer* srcBuffer, uint32 size, uint32 dstOffset, uint32 srcOffset) override;
    void UpdateTexture(GPUTexture* texture, int32 arrayIndex, int32 mipIndex, const void* data, uint32 rowPitch, uint32 slicePitch) override;
    void CopyTexture(GPUTexture* dstResource, uint32 dstSubresource, uint32 dstX, uint32 dstY, uint32 dstZ, GPUTexture* srcResource, uint32 srcSubresource) override;
    void ResetCounter(GPUBuffer* buffer) override;
    void CopyCounter(GPUBuffer* dstBuffer, uint32 dstOffset, GPUBuffer* srcBuffer) override;
    void CopyResource(GPUResource* dstResource, GPUResource* srcResource) override;
    void CopySubresource(GPUResource* dstResource, uint32 dstSubresource, GPUResource* srcResource, uint32 srcSubresource) override;
};

#endif
