// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/GPUContext.h"
#include "IShaderResourceDX12.h"
#include "DescriptorHeapDX12.h"
#include "../IncludeDirectXHeaders.h"

#if GRAPHICS_API_DIRECTX12

class GPUDeviceDX12;
class GPUPipelineStateDX12;
class GPUBufferDX12;
class GPUSamplerDX12;
class GPUConstantBufferDX12;
class GPUTextureViewDX12;
class GPUVertexLayoutDX12;

/// <summary>
/// Size of the resource barriers buffer size (will be flushed on overflow)
/// </summary>
#define DX12_RB_BUFFER_SIZE 16

/// <summary>
/// GPU Commands Context implementation for DirectX 12
/// </summary>
class GPUContextDX12 : public GPUContext
{
    friend GPUDeviceDX12;

public:

    typedef DescriptorHeapRingBufferDX12::Allocation Descriptor;

private:

    GPUDeviceDX12* _device;
    ID3D12GraphicsCommandList* _commandList;
    ID3D12CommandAllocator* _currentAllocator;
    GPUPipelineStateDX12* _currentState;
    GPUShaderProgramCS* _currentCompute;

    int32 _swapChainsUsed;
    int32 _vbCount;
    int32 _rtCount;
    int32 _rbBufferSize;

    uint32 _srMaskDirtyGraphics;
    uint32 _srMaskDirtyCompute;
    uint32 _stencilRef;
    D3D_PRIMITIVE_TOPOLOGY _primitiveTopology;

    int32 _isCompute : 1;
    int32 _rtDirtyFlag : 1;
    int32 _psDirtyFlag : 1;
    int32 _cbGraphicsDirtyFlag : 1;
    int32 _cbComputeDirtyFlag : 1;
    int32 _samplersDirtyFlag : 1;

    GPUTextureViewDX12* _rtDepth;
    GPUTextureViewDX12* _rtHandles[GPU_MAX_RT_BINDED];
    IShaderResourceDX12* _srHandles[GPU_MAX_SR_BINDED];
    IShaderResourceDX12* _uaHandles[GPU_MAX_UA_BINDED];
    GPUVertexLayoutDX12* _vertexLayout;
    GPUBufferDX12* _ibHandle;
    GPUBufferDX12* _vbHandles[GPU_MAX_VB_BINDED];
    D3D12_INDEX_BUFFER_VIEW _ibView;
    D3D12_VERTEX_BUFFER_VIEW _vbViews[GPU_MAX_VB_BINDED];
    D3D12_RESOURCE_BARRIER _rbBuffer[DX12_RB_BUFFER_SIZE];
    GPUConstantBufferDX12* _cbHandles[GPU_MAX_CB_BINDED];
    GPUSamplerDX12* _samplers[GPU_MAX_SAMPLER_BINDED - GPU_STATIC_SAMPLERS_COUNT];

public:

    GPUContextDX12(GPUDeviceDX12* device, D3D12_COMMAND_LIST_TYPE type);
    ~GPUContextDX12();

public:

    FORCE_INLINE ID3D12GraphicsCommandList* GetCommandList() const
    {
        return _commandList;
    }

    uint64 FrameFenceValues[2];

public:

    /// <summary>
    /// Adds the transition barrier for the given resource (or subresource). Supports batching barriers.
    /// </summary>
    /// <param name="resource">The resource.</param>
    /// <param name="before">The 'before' state.</param>
    /// <param name="after">The 'after' state.</param>
    /// <param name="subresourceIndex">The index of the subresource.</param>
    void AddTransitionBarrier(ResourceOwnerDX12* resource, const D3D12_RESOURCE_STATES before, const D3D12_RESOURCE_STATES after, const int32 subresourceIndex);

    /// <summary>
    /// Adds the UAV barrier. Supports batching barriers.
    /// </summary>
    void AddUAVBarrier();

    /// <summary>
    /// Set DirectX 12 resource state using resource barrier
    /// </summary>
    /// <param name="resource">Resource to use</param>
    /// <param name="after">The target state to resource have.</param>
    /// <param name="subresourceIndex">The subresource index. Use -1 to apply for the whole resource.</param>
    void SetResourceState(ResourceOwnerDX12* resource, D3D12_RESOURCE_STATES after, int32 subresourceIndex = -1);

    /// <summary>
    /// Reset commands list and request new allocator for commands
    /// </summary>
    void Reset();

    /// <summary>
    /// Flush existing commands to the GPU
    /// </summary>
    /// <param name="waitForCompletion">True if CPU should wait for GPU to wait job</param>
    /// <returns>Next fence value used for CPU/GPU work synchronization</returns>
    uint64 Execute(bool waitForCompletion = false);

    /// <summary>
    /// External event called by swap chain after using context end
    /// </summary>
    void OnSwapChainFlush();

    /// <summary>
    /// Flush pending resource barriers
    /// </summary>
    FORCE_INLINE void FlushResourceBarriers()
    {
        flushRBs();
    }

protected:

    void GetActiveHeapDescriptor(const D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle, Descriptor& descriptor);

private:

    void flushSRVs();
    void flushRTVs();
    void flushUAVs();
    void flushCBs();
    void flushSamplers();
    void flushRBs();
    void flushPS();
    void OnDrawCall();

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
    void SetViewport(const Viewport& viewport) override;
    void SetScissor(const Rectangle& scissorRect) override;
    GPUPipelineState* GetState() const override;
    void SetState(GPUPipelineState* state) override;
    void ClearState() override;
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
    void SetResourceState(GPUResource* resource, uint64 state, int32 subresource) override;
    void ForceRebindDescriptors() override;
};

#endif
