// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_NULL

#include "Engine/Graphics/GPUContext.h"

/// <summary>
/// GPU Context for Null backend.
/// </summary>
class GPUContextNull : public GPUContext
{
public:

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUContextNull"/> class.
    /// </summary>
    /// <param name="device">The device.</param>
    GPUContextNull(GPUDevice* device)
        : GPUContext(device)
    {
    }

public:

    // [GPUContext]
#if GPU_ALLOW_PROFILE_EVENTS
    void EventBegin(const Char* name) override
    {
    }

    void EventEnd() override
    {
    }
#endif
    void* GetNativePtr() const override
    {
        return nullptr;
    }

    bool IsDepthBufferBinded() override
    {
        return false;
    }

    void Clear(GPUTextureView* rt, const Color& color) override
    {
    }

    void ClearDepth(GPUTextureView* depthBuffer, float depthValue, uint8 stencilValue) override
    {
    }

    void ClearUA(GPUBuffer* buf, const Float4& value) override
    {
    }

    void ClearUA(GPUBuffer* buf, const uint32 value[4]) override
    {
    }

    void ClearUA(GPUTexture* texture, const uint32 value[4]) override
    {
    }

    void ClearUA(GPUTexture* texture, const Float4& value) override
    {
    }

    void ResetRenderTarget() override
    {
    }

    void SetRenderTarget(GPUTextureView* rt) override
    {
    }

    void SetRenderTarget(GPUTextureView* depthBuffer, GPUTextureView* rt) override
    {
    }

    void SetRenderTarget(GPUTextureView* depthBuffer, const Span<GPUTextureView*>& rts) override
    {
    }

    void SetBlendFactor(const Float4& value) override
    {
    }

    void SetStencilRef(uint32 value) override
    {
    }

    void ResetSR() override
    {
    }

    void ResetUA() override
    {
    }

    void ResetCB() override
    {
    }

    void BindCB(int32 slot, GPUConstantBuffer* cb) override
    {
    }

    void BindSR(int32 slot, GPUResourceView* view) override
    {
    }

    void BindUA(int32 slot, GPUResourceView* view) override
    {
    }

    void BindVB(const Span<GPUBuffer*>& vertexBuffers, const uint32* vertexBuffersOffsets = nullptr, GPUVertexLayout* vertexLayout = nullptr) override
    {
    }

    void BindIB(GPUBuffer* indexBuffer) override
    {
    }

    void BindSampler(int32 slot, GPUSampler* sampler) override
    {
    }

    void UpdateCB(GPUConstantBuffer* cb, const void* data) override
    {
    }

    void Dispatch(GPUShaderProgramCS* shader, uint32 threadGroupCountX, uint32 threadGroupCountY, uint32 threadGroupCountZ) override
    {
    }

    void DispatchIndirect(GPUShaderProgramCS* shader, GPUBuffer* bufferForArgs, uint32 offsetForArgs) override
    {
    }

    void ResolveMultisample(GPUTexture* sourceMultisampleTexture, GPUTexture* destTexture, int32 sourceSubResource, int32 destSubResource, PixelFormat format) override
    {
    }

    void DrawInstanced(uint32 verticesCount, uint32 instanceCount, int32 startInstance, int32 startVertex) override
    {
    }

    void DrawIndexedInstanced(uint32 indicesCount, uint32 instanceCount, int32 startInstance, int32 startVertex, int32 startIndex) override
    {
    }

    void DrawInstancedIndirect(GPUBuffer* bufferForArgs, uint32 offsetForArgs) override
    {
    }

    void DrawIndexedInstancedIndirect(GPUBuffer* bufferForArgs, uint32 offsetForArgs) override
    {
    }

    void SetViewport(const Viewport& viewport) override
    {
    }

    void SetScissor(const Rectangle& scissorRect) override
    {
    }

    GPUPipelineState* GetState() const override
    {
        return nullptr;
    }

    void SetState(GPUPipelineState* state) override
    {
    }

    void ClearState() override
    {
    }

    void FlushState() override
    {
    }

    void Flush() override
    {
    }

    void UpdateBuffer(GPUBuffer* buffer, const void* data, uint32 size, uint32 offset) override
    {
    }

    void CopyBuffer(GPUBuffer* dstBuffer, GPUBuffer* srcBuffer, uint32 size, uint32 dstOffset, uint32 srcOffset) override
    {
    }

    void UpdateTexture(GPUTexture* texture, int32 arrayIndex, int32 mipIndex, const void* data, uint32 rowPitch, uint32 slicePitch) override
    {
    }

    void CopyTexture(GPUTexture* dstResource, uint32 dstSubresource, uint32 dstX, uint32 dstY, uint32 dstZ, GPUTexture* srcResource, uint32 srcSubresource) override
    {
    }

    void ResetCounter(GPUBuffer* buffer) override
    {
    }

    void CopyCounter(GPUBuffer* dstBuffer, uint32 dstOffset, GPUBuffer* srcBuffer) override
    {
    }

    void CopyResource(GPUResource* dstResource, GPUResource* srcResource) override
    {
    }

    void CopySubresource(GPUResource* dstResource, uint32 dstSubresource, GPUResource* srcResource, uint32 srcSubresource) override
    {
    }
};

#endif
