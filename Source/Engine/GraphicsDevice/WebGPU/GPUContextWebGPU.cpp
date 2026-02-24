// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_WEBGPU

#include "GPUContextWebGPU.h"
#include "GPUShaderWebGPU.h"
#include "GPUShaderProgramWebGPU.h"
#include "GPUPipelineStateWebGPU.h"
#include "GPUTextureWebGPU.h"
#include "GPUBufferWebGPU.h"
#include "GPUSamplerWebGPU.h"
#include "GPUVertexLayoutWebGPU.h"
#include "RenderToolsWebGPU.h"
#include "Engine/Core/Math/Viewport.h"
#include "Engine/Core/Math/Rectangle.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Profiler/RenderStats.h"
#include "Engine/Graphics/PixelFormatExtensions.h"

// Ensure to match the indirect commands arguments layout
static_assert(sizeof(GPUDispatchIndirectArgs) == sizeof(uint32) * 3, "Wrong size of GPUDrawIndirectArgs.");
static_assert(OFFSET_OF(GPUDispatchIndirectArgs, ThreadGroupCountX) == sizeof(uint32) * 0, "Wrong offset for GPUDrawIndirectArgs::ThreadGroupCountX");
static_assert(OFFSET_OF(GPUDispatchIndirectArgs, ThreadGroupCountY) == sizeof(uint32) * 1, "Wrong offset for GPUDrawIndirectArgs::ThreadGroupCountY");
static_assert(OFFSET_OF(GPUDispatchIndirectArgs, ThreadGroupCountZ) == sizeof(uint32) * 2, "Wrong offset for GPUDrawIndirectArgs::ThreadGroupCountZ");
//
static_assert(sizeof(GPUDrawIndirectArgs) == sizeof(uint32) * 4, "Wrong size of GPUDrawIndirectArgs.");
static_assert(OFFSET_OF(GPUDrawIndirectArgs, VerticesCount) == sizeof(uint32) * 0, "Wrong offset for GPUDrawIndirectArgs::VerticesCount");
static_assert(OFFSET_OF(GPUDrawIndirectArgs, InstanceCount) == sizeof(uint32) * 1, "Wrong offset for GPUDrawIndirectArgs::InstanceCount");
static_assert(OFFSET_OF(GPUDrawIndirectArgs, StartVertex) == sizeof(uint32) * 2, "Wrong offset for GPUDrawIndirectArgs::StartVertex");
static_assert(OFFSET_OF(GPUDrawIndirectArgs, StartInstance) == sizeof(uint32) * 3, "Wrong offset for GPUDrawIndirectArgs::StartInstance");
//
static_assert(sizeof(GPUDrawIndexedIndirectArgs) == sizeof(uint32) * 5, "Wrong size of GPUDrawIndexedIndirectArgs.");
static_assert(OFFSET_OF(GPUDrawIndexedIndirectArgs, IndicesCount) == sizeof(uint32) * 0, "Wrong offset for GPUDrawIndexedIndirectArgs::IndicesCount");
static_assert(OFFSET_OF(GPUDrawIndexedIndirectArgs, InstanceCount) == sizeof(uint32) * 1, "Wrong offset for GPUDrawIndexedIndirectArgs::InstanceCount");
static_assert(OFFSET_OF(GPUDrawIndexedIndirectArgs, StartIndex) == sizeof(uint32) * 2, "Wrong offset for GPUDrawIndexedIndirectArgs::StartIndex");
static_assert(OFFSET_OF(GPUDrawIndexedIndirectArgs, StartVertex) == sizeof(uint32) * 3, "Wrong offset for GPUDrawIndexedIndirectArgs::StartVertex");
static_assert(OFFSET_OF(GPUDrawIndexedIndirectArgs, StartInstance) == sizeof(uint32) * 4, "Wrong offset for GPUDrawIndexedIndirectArgs::StartInstance");

GPUContextWebGPU::GPUContextWebGPU(GPUDeviceWebGPU* device)
    : GPUContext(device)
    , _device(device)
{
    _vertexBufferNullLayout = WGPU_VERTEX_BUFFER_LAYOUT_INIT;
    _minUniformBufferOffsetAlignment = device->MinUniformBufferOffsetAlignment;
}

GPUContextWebGPU::~GPUContextWebGPU()
{
    CHECK(Encoder == nullptr);
}

void GPUContextWebGPU::FrameBegin()
{
    // Base
    GPUContext::FrameBegin();

    // Setup
    _renderPassDirty = false;
    _pipelineDirty = false;
    _bindGroupDirty = false;
    _indexBufferDirty = false;
    _vertexBufferDirty = false;
    _indexBuffer32Bit = false;
    _blendFactorDirty = false;
    _blendFactorSet = false;
    _renderTargetCount = 0;
    _vertexBufferCount = 0;
    _stencilRef = 0;
    _blendFactor = Float4::One;
    _viewport = Viewport(Float2::Zero);
    _scissorRect = Rectangle::Empty;
    _renderPass = nullptr;
    _depthStencil = nullptr;
    _pipelineState = nullptr;
    Platform::MemoryClear(&_pipelineKey, sizeof(_pipelineKey));
    Platform::MemoryClear(&_indexBuffer, sizeof(_indexBuffer));
    Platform::MemoryClear(&_vertexBuffers, sizeof(_vertexBuffers));
    Platform::MemoryClear(&_renderTargets, sizeof(_renderTargets));
    Platform::MemoryClear(&_constantBuffers, sizeof(_constantBuffers));
    Platform::MemoryClear(&_shaderResources, sizeof(_shaderResources));
    Platform::MemoryClear(&_storageResources, sizeof(_storageResources));
    _pendingClears.Clear();

    // Create command encoder
    WGPUCommandEncoderDescriptor encoderDesc = WGPU_COMMAND_ENCODER_DESCRIPTOR_INIT;
    Encoder = wgpuDeviceCreateCommandEncoder(_device->Device, &encoderDesc);
    ASSERT(Encoder);

    // Bind static samplers
    for (int32 i = 0; i < ARRAY_COUNT(_device->DefaultSamplers); i++)
        _samplers[i] = _device->DefaultSamplers[i];
}

void GPUContextWebGPU::FrameEnd()
{
    // Base
    GPUContext::FrameEnd();

    // Flush command encoder to the command buffer and submit them on a queue
    Flush();
}

void* GPUContextWebGPU::GetNativePtr() const
{
    return Encoder;
}

bool GPUContextWebGPU::IsDepthBufferBinded()
{
    return _depthStencil != nullptr;
}

void GPUContextWebGPU::Clear(GPUTextureView* rt, const Color& color)
{
    auto& clear = _pendingClears.AddOne();
    clear.View = (GPUTextureViewWebGPU*)rt;
    Platform::MemoryCopy(clear.RGBA, color.Raw, sizeof(color.Raw));
}

void GPUContextWebGPU::ClearDepth(GPUTextureView* depthBuffer, float depthValue, uint8 stencilValue)
{
    auto& clear = _pendingClears.AddOne();
    clear.View = (GPUTextureViewWebGPU*)depthBuffer;
    clear.Depth = depthValue;
    clear.Stencil = stencilValue;
}

void GPUContextWebGPU::ClearUA(GPUBuffer* buf, const Float4& value)
{
    MISSING_CODE("GPUContextWebGPU::ClearUA");
}

void GPUContextWebGPU::ClearUA(GPUBuffer* buf, const uint32 value[4])
{
    MISSING_CODE("GPUContextWebGPU::ClearUA");
}

void GPUContextWebGPU::ClearUA(GPUTexture* texture, const uint32 value[4])
{
    MISSING_CODE("GPUContextWebGPU::ClearUA");
}

void GPUContextWebGPU::ClearUA(GPUTexture* texture, const Float4& value)
{
    MISSING_CODE("GPUContextWebGPU::ClearUA");
}

void GPUContextWebGPU::ResetRenderTarget()
{
    if (_renderTargetCount != 0 || _depthStencil)
    {
        _renderPassDirty = true;
        _renderTargetCount = 0;
        _depthStencil = nullptr;
    }
}

void GPUContextWebGPU::SetRenderTarget(GPUTextureView* rt)
{
    auto rtWebGPU = (GPUTextureViewWebGPU*)rt;
    int32 newRtCount = rtWebGPU ? 1 : 0;
    if (_renderTargetCount != newRtCount || _renderTargets[0] != rtWebGPU || _depthStencil != nullptr)
    {
        _renderPassDirty = true;
        _renderTargetCount = newRtCount;
        _depthStencil = nullptr;
        _renderTargets[0] = rtWebGPU;
    }
}

void GPUContextWebGPU::SetRenderTarget(GPUTextureView* depthBuffer, GPUTextureView* rt)
{
    auto depthBufferGPU = (GPUTextureViewWebGPU*)depthBuffer;
    auto rtWebGPU = (GPUTextureViewWebGPU*)rt;
    int32 newRtCount = rtWebGPU ? 1 : 0;
    if (_renderTargetCount != newRtCount || _renderTargets[0] != rtWebGPU || _depthStencil != depthBufferGPU)
    {
        _renderPassDirty = true;
        _renderTargetCount = newRtCount;
        _depthStencil = depthBufferGPU;
        _renderTargets[0] = rtWebGPU;
    }
}

void GPUContextWebGPU::SetRenderTarget(GPUTextureView* depthBuffer, const Span<GPUTextureView*>& rts)
{
    ASSERT(Math::IsInRange(rts.Length(), 1, GPU_MAX_RT_BINDED));
    auto depthBufferGPU = (GPUTextureViewWebGPU*)depthBuffer;
    if (_renderTargetCount != rts.Length() || _depthStencil != depthBufferGPU || Platform::MemoryCompare(_renderTargets, rts.Get(), rts.Length() * sizeof(void*)) != 0)
    {
        _renderPassDirty = true;
        _renderTargetCount = rts.Length();
        _depthStencil = depthBufferGPU;
        Platform::MemoryCopy(_renderTargets, rts.Get(), rts.Length() * sizeof(void*));
    }
}

void GPUContextWebGPU::SetBlendFactor(const Float4& value)
{
    if (_blendFactor != value)
    {
        _blendFactorDirty = true;
        _blendFactor = value;
        _blendFactorSet = value != Float4::One;
    }
}

void GPUContextWebGPU::SetStencilRef(uint32 value)
{
    if (_stencilRef != value)
    {
        _stencilRef = value;
        if (_renderPass)
            wgpuRenderPassEncoderSetStencilReference(_renderPass, value);
    }
}

void GPUContextWebGPU::ResetSR()
{
    Platform::MemoryClear(_shaderResources, sizeof(_shaderResources));
}

void GPUContextWebGPU::ResetUA()
{
    Platform::MemoryClear(_storageResources, sizeof(_storageResources));
}

void GPUContextWebGPU::ResetCB()
{
    _bindGroupDirty = false;
    Platform::MemoryClear(_constantBuffers, sizeof(_constantBuffers));
}

void GPUContextWebGPU::BindCB(int32 slot, GPUConstantBuffer* cb)
{
    ASSERT(slot >= 0 && slot < GPU_MAX_CB_BINDED);
    auto cbWebGPU = (GPUConstantBufferWebGPU*)cb;
    if (_constantBuffers[slot] != cbWebGPU)
    {
        _bindGroupDirty = true;
        _constantBuffers[slot] = cbWebGPU;
    }
}

void GPUContextWebGPU::BindSR(int32 slot, GPUResourceView* view)
{
    ASSERT(slot >= 0 && slot < GPU_MAX_SR_BINDED);
    if (_shaderResources[slot] != view)
    {
        _bindGroupDirty = true;
        _shaderResources[slot] = view;
        if (view)
            *view->LastRenderTime = _lastRenderTime;
    }
}

void GPUContextWebGPU::BindUA(int32 slot, GPUResourceView* view)
{
    ASSERT(slot >= 0 && slot < GPU_MAX_UA_BINDED);
    if (_storageResources[slot] != view)
    {
        _bindGroupDirty = true;
        _storageResources[slot] = view;
        if (view)
            *view->LastRenderTime = _lastRenderTime;
    }
}

void GPUContextWebGPU::BindVB(const Span<GPUBuffer*>& vertexBuffers, const uint32* vertexBuffersOffsets, GPUVertexLayout* vertexLayout)
{
    ASSERT(vertexBuffers.Length() <= GPU_MAX_VB_BINDED);
    _vertexBufferDirty = true;
    _vertexBufferCount = vertexBuffers.Length();
    _pipelineKey.VertexBufferCount = vertexBuffers.Length();
    for (int32 i = 0; i < vertexBuffers.Length(); i++)
    {
        auto vbWebGPU = (GPUBufferWebGPU*)vertexBuffers.Get()[i];
        _vertexBuffers[i].Buffer = vbWebGPU ? vbWebGPU->Buffer : nullptr;
        _vertexBuffers[i].Offset = vertexBuffersOffsets ? vertexBuffersOffsets[i] : 0;
        _vertexBuffers[i].Size = vbWebGPU ? vbWebGPU->GetSize() : 0;
        _pipelineKey.VertexBuffers[i] = vbWebGPU && vbWebGPU->GetVertexLayout() ? &((GPUVertexLayoutWebGPU*)vbWebGPU->GetVertexLayout())->Layout : &_vertexBufferNullLayout;
    }
}

void GPUContextWebGPU::BindIB(GPUBuffer* indexBuffer)
{
    auto ibWebGPU = (GPUBufferWebGPU*)indexBuffer;
    _indexBufferDirty = true;
    _indexBuffer32Bit = indexBuffer->GetFormat() == PixelFormat::R32_UInt;
    _indexBuffer.Buffer = ibWebGPU->Buffer;
    _indexBuffer.Offset = 0;
    _indexBuffer.Size = indexBuffer->GetSize();
}

void GPUContextWebGPU::BindSampler(int32 slot, GPUSampler* sampler)
{
    ASSERT(slot >= 0 && slot < GPU_MAX_SAMPLER_BINDED);
    auto samplerWebGPU = (GPUSamplerWebGPU*)sampler;
    if (_samplers[slot] != samplerWebGPU)
    {
        _bindGroupDirty = true;
        _samplers[slot] = samplerWebGPU;
    }
}

void GPUContextWebGPU::UpdateCB(GPUConstantBuffer* cb, const void* data)
{
    ASSERT(data && cb);
    auto cbWebGPU = static_cast<GPUConstantBufferWebGPU*>(cb);
    const uint32 size = cbWebGPU->GetSize();
    if (size != 0)
    {
        // Allocate a chunk of memory in a shared page allocator
        auto allocation = _device->DataUploader.Allocate(size, _minUniformBufferOffsetAlignment, WGPUBufferUsage_Uniform);
        cbWebGPU->Allocation = allocation;
        // TODO: consider holding CPU-side staging buffer and copying data to the GPU buffer in a single batch for all uniforms (before flushing the active command encoder)
        wgpuQueueWriteBuffer(_device->Queue, allocation.Buffer, allocation.Offset, data, size);
        _bindGroupDirty = true;
    }
}

void GPUContextWebGPU::Dispatch(GPUShaderProgramCS* shader, uint32 threadGroupCountX, uint32 threadGroupCountY, uint32 threadGroupCountZ)
{
    OnDispatch(shader);
    MISSING_CODE("GPUContextWebGPU::Dispatch");
    RENDER_STAT_DISPATCH_CALL();
}

void GPUContextWebGPU::DispatchIndirect(GPUShaderProgramCS* shader, GPUBuffer* bufferForArgs, uint32 offsetForArgs)
{
    ASSERT(bufferForArgs && EnumHasAnyFlags(bufferForArgs->GetFlags(), GPUBufferFlags::Argument));
    auto bufferForArgsWebGPU = (GPUBufferWebGPU*)bufferForArgs;
    OnDispatch(shader);
    MISSING_CODE("GPUContextWebGPU::Dispatch");
    RENDER_STAT_DISPATCH_CALL();
}

void GPUContextWebGPU::ResolveMultisample(GPUTexture* sourceMultisampleTexture, GPUTexture* destTexture, int32 sourceSubResource, int32 destSubResource, PixelFormat format)
{
    ASSERT(sourceMultisampleTexture && sourceMultisampleTexture->IsMultiSample());
    ASSERT(destTexture && !destTexture->IsMultiSample());

    // TODO: do it via a render pass (see WGPURenderPassColorAttachment::resolveTarget)
    MISSING_CODE("GPUContextWebGPU::ResolveMultisample");
}

void GPUContextWebGPU::DrawInstanced(uint32 verticesCount, uint32 instanceCount, int32 startInstance, int32 startVertex)
{
    OnDrawCall();
    wgpuRenderPassEncoderDraw(_renderPass, verticesCount, instanceCount, startVertex, startInstance);
    RENDER_STAT_DRAW_CALL(verticesCount * instanceCount, verticesCount * instanceCount / 3);
}

void GPUContextWebGPU::DrawIndexedInstanced(uint32 indicesCount, uint32 instanceCount, int32 startInstance, int32 startVertex, int32 startIndex)
{
    OnDrawCall();
    wgpuRenderPassEncoderDrawIndexed(_renderPass, indicesCount, instanceCount, startIndex, startVertex, startInstance);
    RENDER_STAT_DRAW_CALL(indicesCount * instanceCount, indicesCount / 3 * instanceCount);
}

void GPUContextWebGPU::DrawInstancedIndirect(GPUBuffer* bufferForArgs, uint32 offsetForArgs)
{
    ASSERT(bufferForArgs && EnumHasAnyFlags(bufferForArgs->GetFlags(), GPUBufferFlags::Argument));
    const auto bufferForArgsWebGPU = static_cast<GPUBufferWebGPU*>(bufferForArgs);
    OnDrawCall();
    wgpuRenderPassEncoderDrawIndirect(_renderPass, bufferForArgsWebGPU->Buffer, offsetForArgs);
    RENDER_STAT_DRAW_CALL(0, 0);
}

void GPUContextWebGPU::DrawIndexedInstancedIndirect(GPUBuffer* bufferForArgs, uint32 offsetForArgs)
{
    ASSERT(bufferForArgs && EnumHasAnyFlags(bufferForArgs->GetFlags(), GPUBufferFlags::Argument));
    const auto bufferForArgsWebGPU = static_cast<GPUBufferWebGPU*>(bufferForArgs);
    OnDrawCall();
    wgpuRenderPassEncoderDrawIndexedIndirect(_renderPass, bufferForArgsWebGPU->Buffer, offsetForArgs);
    RENDER_STAT_DRAW_CALL(0, 0);
}

uint64 GPUContextWebGPU::BeginQuery(GPUQueryType type)
{
    // TODO: impl timer/occlusion queries
    return 0;
}

void GPUContextWebGPU::EndQuery(uint64 queryID)
{
}

void GPUContextWebGPU::SetViewport(const Viewport& viewport)
{
    _viewport = viewport;
    if (_renderPass)
        wgpuRenderPassEncoderSetViewport(_renderPass, viewport.X, viewport.Y, viewport.Width, viewport.Height, viewport.MinDepth, viewport.MaxDepth);
}

void GPUContextWebGPU::SetScissor(const Rectangle& scissorRect)
{
    _scissorRect = scissorRect;
    if (_renderPass)
        wgpuRenderPassEncoderSetScissorRect(_renderPass, (uint32_t)scissorRect.GetX(), (uint32_t)scissorRect.GetY(), (uint32_t)scissorRect.GetWidth(), (uint32_t)scissorRect.GetHeight());
}

void GPUContextWebGPU::SetDepthBounds(float minDepth, float maxDepth)
{
}

GPUPipelineState* GPUContextWebGPU::GetState() const
{
    return _pipelineState;
}

void GPUContextWebGPU::SetState(GPUPipelineState* state)
{
    if (_pipelineState != state)
    {
        _pipelineState = (GPUPipelineStateWebGPU*)state;
        _pipelineDirty = true;
    }
}

void GPUContextWebGPU::ResetState()
{
    if (!Encoder)
        return;

    ResetRenderTarget();
    ResetSR();
    ResetUA();
    ResetCB();
    SetState(nullptr);

    FlushState();
}

void GPUContextWebGPU::FlushState()
{
    // Flush pending clears
    for (auto& clear : _pendingClears)
        ManualClear(clear);
    _pendingClears.Clear();
}

void GPUContextWebGPU::Flush()
{
    if (!Encoder)
        return;
    PROFILE_CPU();

    // End existing pass (if any)
    if (_renderPass)
    {
        wgpuRenderPassEncoderEnd(_renderPass);
        wgpuRenderPassEncoderRelease(_renderPass);
    }

    // End commands recording
    WGPUCommandBufferDescriptor commandBufferDesc = WGPU_COMMAND_BUFFER_DESCRIPTOR_INIT;
    WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(Encoder, &commandBufferDesc);
    wgpuCommandEncoderRelease(Encoder);
    if (commandBuffer)
    {
        wgpuQueueSubmit(_device->Queue, 1, &commandBuffer);
        wgpuCommandBufferRelease(commandBuffer);
    }
}

void GPUContextWebGPU::UpdateBuffer(GPUBuffer* buffer, const void* data, uint32 size, uint32 offset)
{
    ASSERT(data);
    ASSERT(buffer && buffer->GetSize() >= size + offset);
    auto bufferWebGPU = (GPUBufferWebGPU*)buffer;
    if (bufferWebGPU->IsDynamic())
    {
        // Synchronous upload via shared buffer
        // TODO: test using map/unmap sequence
        auto allocation = _device->DataUploader.Allocate(size - offset);
        wgpuQueueWriteBuffer(_device->Queue, allocation.Buffer, allocation.Offset, data, size);
        wgpuCommandEncoderCopyBufferToBuffer(Encoder, allocation.Buffer, allocation.Offset, bufferWebGPU->Buffer, offset, size);
    }
    else
    {
        // Efficient upload via queue
        wgpuQueueWriteBuffer(_device->Queue, bufferWebGPU->Buffer, offset, data, size);
    }
}

void GPUContextWebGPU::CopyBuffer(GPUBuffer* dstBuffer, GPUBuffer* srcBuffer, uint32 size, uint32 dstOffset, uint32 srcOffset)
{
    ASSERT(dstBuffer && srcBuffer);
    auto srcBufferWebGPU = (GPUBufferWebGPU*)srcBuffer;
    auto dstBufferWebGPU = (GPUBufferWebGPU*)dstBuffer;
    wgpuCommandEncoderCopyBufferToBuffer(Encoder, srcBufferWebGPU->Buffer, srcOffset, dstBufferWebGPU->Buffer, dstOffset, size);
}

void GPUContextWebGPU::UpdateTexture(GPUTexture* texture, int32 arrayIndex, int32 mipIndex, const void* data, uint32 rowPitch, uint32 slicePitch)
{
    ASSERT(texture && texture->IsAllocated() && data);
    auto textureWebGPU = (GPUTextureWebGPU*)texture;
    ASSERT_LOW_LAYER(textureWebGPU->Texture && wgpuTextureGetUsage(textureWebGPU->Texture) & WGPUTextureUsage_CopyDst);
    ASSERT(!texture->IsVolume()); // TODO: impl uploading volume textures (handle write size properly)

    int32 mipWidth, mipHeight, mipDepth;
    texture->GetMipSize(mipIndex, mipWidth, mipHeight, mipDepth);

    WGPUTexelCopyTextureInfo copyInfo = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
    copyInfo.texture = textureWebGPU->Texture;
    copyInfo.mipLevel = mipIndex;
    copyInfo.aspect = WGPUTextureAspect_All;
    WGPUTexelCopyBufferLayout dataLayout = WGPU_TEXEL_COPY_BUFFER_LAYOUT_INIT;
    dataLayout.bytesPerRow = rowPitch;
    dataLayout.rowsPerImage = mipHeight;
    WGPUExtent3D writeSize = { (uint32_t)mipWidth, (uint32_t)mipHeight, 1 };
    wgpuQueueWriteTexture(_device->Queue, &copyInfo, data, slicePitch, &dataLayout, &writeSize);
}

void GPUContextWebGPU::CopyTexture(GPUTexture* dstResource, uint32 dstSubresource, uint32 dstX, uint32 dstY, uint32 dstZ, GPUTexture* srcResource, uint32 srcSubresource)
{
    ASSERT(dstResource && srcResource);
    auto srcTextureWebGPU = (GPUTextureWebGPU*)srcResource;
    auto dstTextureWebGPU = (GPUTextureWebGPU*)dstResource;
    ASSERT_LOW_LAYER(dstTextureWebGPU->Texture && wgpuTextureGetUsage(dstTextureWebGPU->Texture) & WGPUTextureUsage_CopyDst);
    ASSERT_LOW_LAYER(srcTextureWebGPU->Texture && wgpuTextureGetUsage(srcTextureWebGPU->Texture) & WGPUTextureUsage_CopySrc);

    // TODO: handle array/depth slices
    const int32 srcMipIndex = srcSubresource % srcTextureWebGPU->MipLevels();
    const int32 dstMipIndex = dstSubresource % srcTextureWebGPU->MipLevels();

    int32 srcMipWidth, srcMipHeight, srcMipDepth;
    srcTextureWebGPU->GetMipSize(srcMipIndex, srcMipWidth, srcMipHeight, srcMipDepth);

    WGPUTexelCopyTextureInfo srcInfo = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
    srcInfo.texture = srcTextureWebGPU->Texture;
    srcInfo.mipLevel = srcMipIndex;
    srcInfo.aspect = WGPUTextureAspect_All;
    WGPUTexelCopyTextureInfo dstInfo = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
    dstInfo.texture = dstTextureWebGPU->Texture;
    dstInfo.mipLevel = dstMipIndex;
    dstInfo.origin = { dstX, dstY, dstZ };
    dstInfo.aspect = WGPUTextureAspect_All;
    WGPUExtent3D copySize = { (uint32_t)srcMipWidth, (uint32_t)srcMipHeight, (uint32_t)srcMipDepth };
    wgpuCommandEncoderCopyTextureToTexture(Encoder, &srcInfo, &dstInfo, &copySize);
}

void GPUContextWebGPU::ResetCounter(GPUBuffer* buffer)
{
    MISSING_CODE("GPUContextWebGPU::ResetCounter");
}

void GPUContextWebGPU::CopyCounter(GPUBuffer* dstBuffer, uint32 dstOffset, GPUBuffer* srcBuffer)
{
    MISSING_CODE("GPUContextWebGPU::CopyCounter");
}

void GPUContextWebGPU::CopyResource(GPUResource* dstResource, GPUResource* srcResource)
{
    ASSERT(dstResource && srcResource);
    auto dstTexture = Cast<GPUTexture>(dstResource);
    auto srcTexture = Cast<GPUTexture>(srcResource);
    if (srcTexture && dstTexture)
    {
        // Texture -> Texture
        ASSERT(srcTexture->MipLevels() == dstTexture->MipLevels());
        ASSERT(srcTexture->ArraySize() == 1); // TODO: implement copying texture arrays
        for (int32 mipLevel = 0; mipLevel < srcTexture->MipLevels(); mipLevel++)
            CopyTexture(dstTexture, mipLevel, 0, 0, 0, srcTexture, mipLevel);
    }
    else if (srcTexture)
    {
        // Texture -> Buffer
        auto srcTextureWebGPU = (GPUTextureWebGPU*)srcResource;
        auto dstBufferWebGPU = (GPUBufferWebGPU*)dstResource;
        MISSING_CODE("GPUContextWebGPU::CopyResource: texture -> buffer"); // TODO: impl this
    }
    else if (dstTexture)
    {
        // Buffer -> Texture
        auto srcBufferWebGPU = (GPUBufferWebGPU*)srcResource;
        auto dstTextureWebGPU = (GPUTextureWebGPU*)dstResource;
        MISSING_CODE("GPUContextWebGPU::CopyResource: buffer -> texture"); // TODO: impl this
    }
    else
    {
        // Buffer -> Buffer
        auto srcBufferWebGPU = (GPUBufferWebGPU*)srcResource;
        auto dstBufferWebGPU = (GPUBufferWebGPU*)dstResource;
        uint64 size = Math::Min(srcBufferWebGPU->GetSize(), dstBufferWebGPU->GetSize());
        wgpuCommandEncoderCopyBufferToBuffer(Encoder, srcBufferWebGPU->Buffer, 0, dstBufferWebGPU->Buffer, 0, size);
    }
}

void GPUContextWebGPU::CopySubresource(GPUResource* dstResource, uint32 dstSubresource, GPUResource* srcResource, uint32 srcSubresource)
{
    ASSERT(dstResource && srcResource);
    auto dstTexture = Cast<GPUTexture>(dstResource);
    auto srcTexture = Cast<GPUTexture>(srcResource);
    if (srcTexture && dstTexture)
    {
        // Texture -> Texture
        CopyTexture(dstTexture, dstSubresource, 0, 0, 0, srcTexture, srcSubresource);
    }
    else if (srcTexture)
    {
        // Texture -> Buffer
        auto srcTextureWebGPU = (GPUTextureWebGPU*)srcResource;
        auto dstBufferWebGPU = (GPUBufferWebGPU*)dstResource;
        MISSING_CODE("GPUContextWebGPU::CopyResource: texture -> buffer"); // TODO: impl this
    }
    else if (dstTexture)
    {
        // Buffer -> Texture
        auto srcBufferWebGPU = (GPUBufferWebGPU*)srcResource;
        auto dstTextureWebGPU = (GPUTextureWebGPU*)dstResource;
        MISSING_CODE("GPUContextWebGPU::CopyResource: buffer -> texture"); // TODO: impl this
    }
    else
    {
        // Buffer -> Buffer
        ASSERT(dstSubresource == 0 && srcSubresource == 0);
        auto srcBufferWebGPU = (GPUBufferWebGPU*)srcResource;
        auto dstBufferWebGPU = (GPUBufferWebGPU*)dstResource;
        uint64 size = Math::Min(srcBufferWebGPU->GetSize(), dstBufferWebGPU->GetSize());
        wgpuCommandEncoderCopyBufferToBuffer(Encoder, srcBufferWebGPU->Buffer, 0, dstBufferWebGPU->Buffer, 0, size);
    }
}

bool GPUContextWebGPU::FindClear(const GPUTextureViewWebGPU* view, PendingClear& clear)
{
    for (auto& e : _pendingClears)
    {
        if (e.View == view)
        {
            clear = e;
            return true;
        }
    }
    return false;
}

void GPUContextWebGPU::ManualClear(const PendingClear& clear)
{
    // End existing pass (if any)
    if (_renderPass)
    {
        wgpuRenderPassEncoderEnd(_renderPass);
        wgpuRenderPassEncoderRelease(_renderPass);
        _renderPass = nullptr;
    }

    // Clear with a render pass
    WGPURenderPassColorAttachment colorAttachment;
    WGPURenderPassDepthStencilAttachment depthStencilAttachment;
    WGPURenderPassDescriptor renderPassDesc = WGPU_RENDER_PASS_DESCRIPTOR_INIT;
    if (((GPUTextureWebGPU*)clear.View->GetParent())->IsDepthStencil())
    {
        renderPassDesc.depthStencilAttachment = &depthStencilAttachment;
        depthStencilAttachment = WGPU_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_INIT;
        depthStencilAttachment.view = clear.View->View;
        depthStencilAttachment.depthLoadOp = WGPULoadOp_Clear;
        depthStencilAttachment.depthStoreOp = WGPUStoreOp_Store;
        depthStencilAttachment.depthClearValue = clear.Depth;
        depthStencilAttachment.stencilClearValue = clear.Stencil;
    }
    else
    {
        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &colorAttachment;
        colorAttachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
        colorAttachment.view = clear.View->View;
        colorAttachment.depthSlice = clear.View->DepthSlice;
        colorAttachment.loadOp = WGPULoadOp_Clear;
        colorAttachment.storeOp = WGPUStoreOp_Store;
        if (clear.View->HasStencil)
        {
            depthStencilAttachment.stencilLoadOp = WGPULoadOp_Clear;
            depthStencilAttachment.stencilStoreOp = WGPUStoreOp_Store;
            depthStencilAttachment.stencilClearValue = clear.Stencil;
        }
        colorAttachment.clearValue = { clear.RGBA[0], clear.RGBA[1], clear.RGBA[2], clear.RGBA[3] };
    }
    auto renderPass = wgpuCommandEncoderBeginRenderPass(Encoder, &renderPassDesc);
    wgpuRenderPassEncoderEnd(renderPass);
    wgpuRenderPassEncoderRelease(renderPass);
}

void GPUContextWebGPU::OnDrawCall()
{
    // Clear textures that are not bind to the render pass
    auto renderTargets = ToSpan(_renderTargets, _renderTargetCount);
    for (int32 i = _pendingClears.Count() - 1; i >= 0; i--)
    {
        auto clear = _pendingClears[i];
        if (clear.View != _depthStencil && SpanContains(renderTargets, clear.View))
        {
            ManualClear(clear);
            _pendingClears.RemoveAt(i);
        }
    }

    // Check if need to start a new render pass
    if (_renderPassDirty)
    {
        _renderPassDirty = false;

        // End existing pass (if any)
        if (_renderPass)
        {
            wgpuRenderPassEncoderEnd(_renderPass);
            wgpuRenderPassEncoderRelease(_renderPass);
        }

        // Start a new render pass
        WGPURenderPassColorAttachment colorAttachments[GPU_MAX_RT_BINDED];
        WGPURenderPassDepthStencilAttachment depthStencilAttachment = WGPU_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_INIT;
        WGPURenderPassDescriptor renderPassDesc = WGPU_RENDER_PASS_DESCRIPTOR_INIT;
        renderPassDesc.colorAttachmentCount = _renderTargetCount;
        renderPassDesc.colorAttachments = colorAttachments;
        PendingClear clear;
        _pipelineKey.MultiSampleCount = 1;
        _pipelineKey.RenderTargetCount = _renderTargetCount;
        for (int32 i = 0; i < renderPassDesc.colorAttachmentCount; i++)
        {
            auto& colorAttachment = colorAttachments[i];
            colorAttachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
            auto renderTarget = _renderTargets[i];
            colorAttachment.view = renderTarget->View;
            colorAttachment.depthSlice = renderTarget->DepthSlice;
            colorAttachment.loadOp = WGPULoadOp_Load;
            colorAttachment.storeOp = WGPUStoreOp_Store;
            if (FindClear(_depthStencil, clear))
            {
                colorAttachment.loadOp = WGPULoadOp_Clear;
                colorAttachment.clearValue = { clear.RGBA[0], clear.RGBA[1], clear.RGBA[2], clear.RGBA[3] };
            }
            _pipelineKey.MultiSampleCount = (int32)renderTarget->GetMSAA();
            _pipelineKey.RenderTargetFormats[i] = renderTarget->Format;
        }
        if (_depthStencil)
        {
            renderPassDesc.depthStencilAttachment = &depthStencilAttachment;
            depthStencilAttachment.view = _depthStencil->View;
            depthStencilAttachment.depthLoadOp = WGPULoadOp_Load;
            depthStencilAttachment.depthStoreOp = _depthStencil->ReadOnly ? WGPUStoreOp_Discard : WGPUStoreOp_Store;
            depthStencilAttachment.depthReadOnly = _depthStencil->ReadOnly;
            if (_depthStencil->HasStencil)
            {
                depthStencilAttachment.stencilLoadOp = WGPULoadOp_Load;
                depthStencilAttachment.stencilStoreOp = _depthStencil->ReadOnly ? WGPUStoreOp_Discard : WGPUStoreOp_Store;
                depthStencilAttachment.depthReadOnly = _depthStencil->ReadOnly;
            }
            else
            {
                depthStencilAttachment.stencilClearValue = 0;
                depthStencilAttachment.stencilLoadOp = WGPULoadOp_Clear;
                depthStencilAttachment.stencilStoreOp = WGPUStoreOp_Discard;
                depthStencilAttachment.stencilReadOnly = true;
            }
            if (FindClear(_depthStencil, clear))
            {
                depthStencilAttachment.depthLoadOp = WGPULoadOp_Clear;
                depthStencilAttachment.depthClearValue = clear.Depth;
                if (_depthStencil->HasStencil)
                {
                    depthStencilAttachment.stencilLoadOp = WGPULoadOp_Clear;
                    depthStencilAttachment.stencilClearValue = clear.Stencil;
                }
            }
            _pipelineKey.DepthStencilFormat = _depthStencil->Format;
        }
        else
        {
            _pipelineKey.DepthStencilFormat = WGPUTextureFormat_Undefined;
        }
        _renderPass = wgpuCommandEncoderBeginRenderPass(Encoder, &renderPassDesc);
        ASSERT(_renderPass);

        // Discard texture clears (done manually or via render pass)
        _pendingClears.Clear();

        // Apply pending state
        if (_stencilRef != 0)
            wgpuRenderPassEncoderSetStencilReference(_renderPass, _stencilRef);
        auto scissorRect = _scissorRect;
        // TODO: skip calling this if scissorRect is default (0, 0, attachment width, attachment  height)
        wgpuRenderPassEncoderSetScissorRect(_renderPass, (uint32_t)scissorRect.GetX(), (uint32_t)scissorRect.GetY(), (uint32_t)scissorRect.GetWidth(), (uint32_t)scissorRect.GetHeight());
        auto viewport = _viewport;
        // TODO: skip calling this if viewport is default (0, 0, attachment width, attachment  height, 0, 1)
        wgpuRenderPassEncoderSetViewport(_renderPass, viewport.X, viewport.Y, viewport.Width, viewport.Height, viewport.MinDepth, viewport.MaxDepth);

        // Auto-dirty pipeline when new render pass starts
        if (_pipelineState)
            _pipelineDirty = true;
        _indexBufferDirty = true;
        _vertexBufferDirty = true;
        _bindGroupDirty = true;
        if (_blendFactorSet)
            _blendFactorDirty = true;
    }

    // Flush rendering states
    if (_pipelineDirty)
    {
        _pipelineDirty = false;
        WGPURenderPipeline pipeline = _pipelineState ? _pipelineState->GetPipeline(_pipelineKey) : nullptr;
        wgpuRenderPassEncoderSetPipeline(_renderPass, pipeline);
        RENDER_STAT_PS_STATE_CHANGE();
    }
    if (_indexBufferDirty && _indexBuffer.Buffer)
    {
        _indexBufferDirty = false;
        wgpuRenderPassEncoderSetIndexBuffer(_renderPass, _indexBuffer.Buffer, _indexBuffer32Bit ? WGPUIndexFormat_Uint32 : WGPUIndexFormat_Uint16, _indexBuffer.Offset, _indexBuffer.Size);
    }
    if (_vertexBufferDirty)
    {
        _vertexBufferDirty = false;
        for (int32 i = 0; i < _vertexBufferCount; i++)
        {
            auto vb = _vertexBuffers[i];
            wgpuRenderPassEncoderSetVertexBuffer(_renderPass, i, vb.Buffer, vb.Offset, vb.Size);
        }
    }
    if (_blendFactorDirty)
    {
        _blendFactorDirty = false;
        WGPUColor color = { _blendFactor.X, _blendFactor.Y, _blendFactor.Z, _blendFactor.W };
        wgpuRenderPassEncoderSetBlendConstant(_renderPass, &color);
    }
    if (_bindGroupDirty)
    {
        _bindGroupDirty = false;
        // TODO: bind _samplers
        // TODO: bind _constantBuffers
        // TODO: bind _shaderResources
    }
}

void GPUContextWebGPU::OnDispatch(GPUShaderProgramCS* shader)
{
    // TODO: add compute shaders support
}

#endif
