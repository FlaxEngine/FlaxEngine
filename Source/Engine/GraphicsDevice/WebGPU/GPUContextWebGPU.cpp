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
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Viewport.h"
#include "Engine/Core/Math/Rectangle.h"
#include "Engine/Engine/Engine.h"
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
    _minUniformBufferOffsetAlignment = device->MinUniformBufferOffsetAlignment;

    // Setup descriptor handles tables lookup cache
    _resourceTables[(int32)SpirvShaderResourceBindingType::INVALID] = nullptr;
    _resourceTables[(int32)SpirvShaderResourceBindingType::CB] = nullptr;
    _resourceTables[(int32)SpirvShaderResourceBindingType::SAMPLER] = nullptr;
    _resourceTables[(int32)SpirvShaderResourceBindingType::SRV] = _shaderResources;
    _resourceTables[(int32)SpirvShaderResourceBindingType::UAV] = _storageResources;
#if ENABLE_ASSERTION
    _resourceTableSizes[(int32)SpirvShaderResourceBindingType::INVALID] = 0;
    _resourceTableSizes[(int32)SpirvShaderResourceBindingType::CB] = GPU_MAX_CB_BINDED;
    _resourceTableSizes[(int32)SpirvShaderResourceBindingType::SAMPLER] = GPU_MAX_SAMPLER_BINDED;
    _resourceTableSizes[(int32)SpirvShaderResourceBindingType::SRV] = GPU_MAX_SR_BINDED;
    _resourceTableSizes[(int32)SpirvShaderResourceBindingType::UAV] = GPU_MAX_UA_BINDED;
#endif
}

GPUContextWebGPU::~GPUContextWebGPU()
{
    if (Encoder)
        Flush();
    CHECK(Encoder == nullptr);
}

void GPUContextWebGPU::FrameBegin()
{
    // Base
    GPUContext::FrameBegin();

    // Setup
    _usedQuerySets = 0;
    _renderPassDirty = false;
    _pipelineDirty = false;
    _bindGroupDirty = false;
    _viewportDirty = false;
    _scissorRectDirty = false;
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

#if GPU_ALLOW_PROFILE_EVENTS

#include "Engine/Utilities/StringConverter.h"

void GPUContextWebGPU::EventBegin(const Char* name)
{
    // Cannot insert commands in encoder during render pass
    if (_renderPass)
        EndRenderPass();

    StringAsANSI<> nameAnsi(name);
    wgpuCommandEncoderPushDebugGroup(Encoder, { nameAnsi.Get(), (size_t)nameAnsi.Length() });
}

void GPUContextWebGPU::EventEnd()
{
    // Cannot insert commands in encoder during render pass
    if (_renderPass)
        EndRenderPass();

    wgpuCommandEncoderPopDebugGroup(Encoder);
}

#endif

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
    _bindGroupDirty = true;
    Platform::MemoryClear(_shaderResources, sizeof(_shaderResources));
}

void GPUContextWebGPU::ResetUA()
{
    _bindGroupDirty = true;
    Platform::MemoryClear(_storageResources, sizeof(_storageResources));
}

void GPUContextWebGPU::ResetCB()
{
    _bindGroupDirty = true;
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
    _pipelineKey.VertexLayout = (GPUVertexLayoutWebGPU*)(vertexLayout ? vertexLayout : GPUVertexLayout::Get(vertexBuffers));
    for (int32 i = 0; i < vertexBuffers.Length(); i++)
    {
        auto vbWebGPU = (GPUBufferWebGPU*)vertexBuffers.Get()[i];
        if (vbWebGPU)
            _vertexBuffers[i] = { vbWebGPU->Buffer, vbWebGPU->GetSize(), vertexBuffersOffsets ? vertexBuffersOffsets[i] : 0 };
        else
            _vertexBuffers[i] = { _device->DefaultBuffer, 64, 0 }; // Fallback
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
        uint32 alignedSize = Math::AlignUp<uint32>(size, 16); // Uniform buffers must be aligned to 16 bytes
        auto allocation = _device->DataUploader.Allocate(alignedSize, WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst, _minUniformBufferOffsetAlignment);
        cbWebGPU->Allocation = allocation;
        cbWebGPU->AllocationSize = alignedSize;
        // TODO: consider holding CPU-side staging buffer and copying data to the GPU buffer in a single batch for all uniforms (before flushing the active command encoder)
        wgpuQueueWriteBuffer(_device->Queue, allocation.Buffer, allocation.Offset, data, size);
        _bindGroupDirty = true;
    }
}

void GPUContextWebGPU::Dispatch(GPUShaderProgramCS* shader, uint32 threadGroupCountX, uint32 threadGroupCountY, uint32 threadGroupCountZ)
{
    auto computePass = OnDispatch(shader);
    wgpuComputePassEncoderDispatchWorkgroups(computePass, threadGroupCountX, threadGroupCountY, threadGroupCountZ);
    EndComputePass(computePass);
    RENDER_STAT_DISPATCH_CALL();
}

void GPUContextWebGPU::DispatchIndirect(GPUShaderProgramCS* shader, GPUBuffer* bufferForArgs, uint32 offsetForArgs)
{
    ASSERT(bufferForArgs && EnumHasAnyFlags(bufferForArgs->GetFlags(), GPUBufferFlags::Argument));
    auto bufferForArgsWebGPU = (GPUBufferWebGPU*)bufferForArgs;
    auto computePass = OnDispatch(shader);
    wgpuComputePassEncoderDispatchWorkgroupsIndirect(computePass, bufferForArgsWebGPU->Buffer, offsetForArgs);
    EndComputePass(computePass);
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
    auto query = _device->AllocateQuery(type);
    if (query.Raw)
    {
        ASSERT_LOW_LAYER(query.Set < WEBGPU_MAX_QUERY_SETS);
        auto set = _device->QuerySets[query.Set];
        if (set->Type == GPUQueryType::Timer)
        {
            // Put a new timestamp write
            WriteTimestamp(set, query.Index);
        }
        else if (_activeOcclusionQuerySet == query.Set && _renderPass)
        {
            // Begin occlusion query on the active set
            wgpuRenderPassEncoderBeginOcclusionQuery(_renderPass, query.Index);
        }
        else
        {
            // Set the next pending occlusion query set to use for the next pass (or frame)
            _pendingOcclusionQuerySet = query.Set;
        }

        // Mark query set as used (to be resolved on the frame end)
        static_assert(sizeof(_usedQuerySets) * 8 >= WEBGPU_MAX_QUERY_SETS, "Not enough bits in flags of used queries set.");
        _usedQuerySets |= 1u << query.Set;

    }
    return query.Raw;
}

void GPUContextWebGPU::EndQuery(uint64 queryID)
{
    if (queryID)
    {
        GPUQueryWebGPU query;
        query.Raw = queryID;
        auto set = _device->QuerySets[query.Set];
        if (set->Type == GPUQueryType::Timer)
        {
            // Put a new timestamp write
            WriteTimestamp(set, query.Index + 1);
        }
        else if (_activeOcclusionQuerySet == query.Set && _renderPass)
        {
            // End occlusion query on the active set
            wgpuRenderPassEncoderEndOcclusionQuery(_renderPass);
        }
    }
}

void GPUContextWebGPU::SetViewport(const Viewport& viewport)
{
    _viewport = viewport;
    _viewportDirty = true;
}

void GPUContextWebGPU::SetScissor(const Rectangle& scissorRect)
{
    _scissorRect = scissorRect;
    _scissorRectDirty = true;
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
        EndRenderPass();

    // Flush pending actions
    FlushTimestamps();
    _pendingTimestampWrites.Clear();

    // Resolve used queries
    for (uint32 setIndex = 0; setIndex < _device->QuerySetsCount; setIndex++)
    {
        if (_usedQuerySets & (1u << setIndex))
            _device->QuerySets[setIndex]->Resolve(Encoder);
    }
    _usedQuerySets = 0;

    // End commands recording
    WGPUCommandBufferDescriptor commandBufferDesc = WGPU_COMMAND_BUFFER_DESCRIPTOR_INIT;
    WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(Encoder, &commandBufferDesc);
    wgpuCommandEncoderRelease(Encoder);
    Encoder = nullptr;
    if (commandBuffer)
    {
        wgpuQueueSubmit(_device->Queue, 1, &commandBuffer);
        wgpuCommandBufferRelease(commandBuffer);
        _device->QueueSubmits++;
    }
}

void GPUContextWebGPU::UpdateBuffer(GPUBuffer* buffer, const void* data, uint32 size, uint32 offset)
{
    if (size == 0)
        return;
    ASSERT(data);
    ASSERT(buffer && buffer->GetSize() >= size + offset);
    auto bufferWebGPU = (GPUBufferWebGPU*)buffer;
    if (bufferWebGPU->Usage & WGPUBufferUsage_MapWrite)
    {
        CRASH; // TODO: impl this (map if not mapped yet and memcpy)
    }
    else if (bufferWebGPU->IsDynamic())
    {
        // Cannot insert copy commands in encoder during render pass
        if (_renderPass)
            EndRenderPass();

        // Synchronous upload via shared buffer
        auto sizeAligned = (size + 3) & ~0x3; // Number of bytes must be a multiple of 4 for both wgpuQueueWriteBuffer and wgpuCommandEncoderCopyBufferToBuffer
        auto allocation = _device->DataUploader.Allocate(sizeAligned, WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst);
        wgpuQueueWriteBuffer(_device->Queue, allocation.Buffer, allocation.Offset, data, sizeAligned);
        wgpuCommandEncoderCopyBufferToBuffer(Encoder, allocation.Buffer, allocation.Offset, bufferWebGPU->Buffer, offset, sizeAligned);
    }
    else
    {
        // Efficient upload via queue
        wgpuQueueWriteBuffer(_device->Queue, bufferWebGPU->Buffer, offset, data, size);
    }
}

void GPUContextWebGPU::CopyBuffer(GPUBuffer* dstBuffer, GPUBuffer* srcBuffer, uint32 size, uint32 dstOffset, uint32 srcOffset)
{
    // Cannot insert copy commands in encoder during render pass
    if (_renderPass)
        EndRenderPass();

    ASSERT(dstBuffer && srcBuffer);
    auto srcBufferWebGPU = (GPUBufferWebGPU*)srcBuffer;
    auto dstBufferWebGPU = (GPUBufferWebGPU*)dstBuffer;
    auto copySize = (size + 3) & ~0x3; // Number of bytes must be a multiple of 4 for wgpuCommandEncoderCopyBufferToBuffer
    wgpuCommandEncoderCopyBufferToBuffer(Encoder, srcBufferWebGPU->Buffer, srcOffset, dstBufferWebGPU->Buffer, dstOffset, copySize);
}

void GPUContextWebGPU::UpdateTexture(GPUTexture* texture, int32 arrayIndex, int32 mipIndex, const void* data, uint32 rowPitch, uint32 slicePitch)
{
    ASSERT(texture && texture->IsAllocated() && data);
    auto textureWebGPU = (GPUTextureWebGPU*)texture;
    ASSERT_LOW_LAYER(textureWebGPU->Texture && wgpuTextureGetUsage(textureWebGPU->Texture) & WGPUTextureUsage_CopyDst);

    int32 mipWidth, mipHeight, mipDepth;
    texture->GetMipSize(mipIndex, mipWidth, mipHeight, mipDepth);

    const int32 blockSize = PixelFormatExtensions::ComputeBlockSize(textureWebGPU->Format());
    mipWidth = Math::Max(mipWidth, blockSize);
    mipHeight = Math::Max(mipHeight, blockSize);
    if (textureWebGPU->Dimensions() == TextureDimensions::VolumeTexture)
        mipDepth = Math::Max(mipDepth, blockSize);

    WGPUTexelCopyTextureInfo copyInfo = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
    copyInfo.texture = textureWebGPU->Texture;
    copyInfo.mipLevel = mipIndex;
    copyInfo.origin.z = arrayIndex;
    copyInfo.aspect = WGPUTextureAspect_All;
    WGPUTexelCopyBufferLayout dataLayout = WGPU_TEXEL_COPY_BUFFER_LAYOUT_INIT;
    dataLayout.bytesPerRow = rowPitch;
    dataLayout.rowsPerImage = mipHeight;
    WGPUExtent3D writeSize = { (uint32_t)mipWidth, (uint32_t)mipHeight, (uint32_t)mipDepth };
    wgpuQueueWriteTexture(_device->Queue, &copyInfo, data, slicePitch, &dataLayout, &writeSize);
}

void GPUContextWebGPU::CopyTexture(GPUTexture* dstResource, uint32 dstSubresource, uint32 dstX, uint32 dstY, uint32 dstZ, GPUTexture* srcResource, uint32 srcSubresource)
{
    ASSERT(dstResource && srcResource);
    auto srcTextureWebGPU = (GPUTextureWebGPU*)srcResource;
    auto dstTextureWebGPU = (GPUTextureWebGPU*)dstResource;
    ASSERT_LOW_LAYER(dstTextureWebGPU->Texture && srcTextureWebGPU->Texture);

    const int32 srcMipIndex = srcSubresource % srcTextureWebGPU->MipLevels();
    const int32 dstMipIndex = dstSubresource % srcTextureWebGPU->MipLevels();
    const int32 srcArrayIndex = srcSubresource / srcTextureWebGPU->ArraySize();
    const int32 dstArrayIndex = srcSubresource / srcTextureWebGPU->ArraySize();

    int32 srcMipWidth, srcMipHeight, srcMipDepth;
    srcTextureWebGPU->GetMipSize(srcMipIndex, srcMipWidth, srcMipHeight, srcMipDepth);

    if (dstTextureWebGPU->Usage & WGPUTextureUsage_CopyDst && srcTextureWebGPU->Usage & WGPUTextureUsage_CopySrc)
    {
        // Direct copy
        WGPUTexelCopyTextureInfo srcInfo = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
        srcInfo.texture = srcTextureWebGPU->Texture;
        srcInfo.mipLevel = srcMipIndex;
        srcInfo.origin.z = srcArrayIndex;
        srcInfo.aspect = WGPUTextureAspect_All;
        WGPUTexelCopyTextureInfo dstInfo = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
        dstInfo.texture = dstTextureWebGPU->Texture;
        dstInfo.mipLevel = dstMipIndex;
        dstInfo.origin = { dstX, dstY, dstZ + dstArrayIndex };
        dstInfo.aspect = WGPUTextureAspect_All;
        WGPUExtent3D copySize = { (uint32_t)srcMipWidth, (uint32_t)srcMipHeight, (uint32_t)srcMipDepth };
        wgpuCommandEncoderCopyTextureToTexture(Encoder, &srcInfo, &dstInfo, &copySize);
    }
    else if (dstTextureWebGPU->Usage & WGPUTextureUsage_RenderAttachment && srcTextureWebGPU->Usage & WGPUTextureUsage_TextureBinding)
    {
        // Copy via drawing
        ResetRenderTarget();
        SetViewportAndScissors(srcMipWidth, srcMipHeight);
        SetState(_device->GetCopyLinearPS());
        if (srcSubresource == 0 && dstSubresource == 0)
        {
            SetRenderTarget(dstTextureWebGPU->View(0));
            BindSR(0, srcTextureWebGPU->View(0));
        }
        else
        {
            ASSERT(dstTextureWebGPU->HasPerMipViews() && srcResource->HasPerMipViews());
            SetRenderTarget(dstTextureWebGPU->View(dstArrayIndex, dstMipIndex));
            BindSR(0, srcTextureWebGPU->View(srcArrayIndex, srcMipIndex));
        }
        DrawFullscreenTriangle();
    }
    else
    {
        LOG(Fatal, "Cannot copy texture {} to {}", srcTextureWebGPU->GetDescription().ToString(), dstTextureWebGPU->GetDescription().ToString());
    }
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
    // Cannot insert copy commands in encoder during render pass
    if (_renderPass)
        EndRenderPass();

    ASSERT(dstResource && srcResource);
    auto dstTexture = Cast<GPUTexture>(dstResource);
    auto srcTexture = Cast<GPUTexture>(srcResource);
    if (srcTexture && dstTexture)
    {
        // Texture -> Texture
        ASSERT(srcTexture->MipLevels() == dstTexture->MipLevels());
        ASSERT(srcTexture->ArraySize() == dstTexture->ArraySize());
        for (int32 arraySlice = 0; arraySlice < srcTexture->ArraySize(); arraySlice++)
        {
            for (int32 mipLevel = 0; mipLevel < srcTexture->MipLevels(); mipLevel++)
            {
                uint32 subresource = arraySlice * srcTexture->MipLevels() + mipLevel;
                CopyTexture(dstTexture, subresource, 0, 0, 0, srcTexture, subresource);
            }
        }
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
    // Cannot insert copy commands in encoder during render pass
    if (_renderPass)
        EndRenderPass();

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

void GPUContextWebGPU::WriteTimestamp(GPUQuerySetWebGPU* set, uint32 index)
{
    WGPUPassTimestampWrites write = WGPU_PASS_TIMESTAMP_WRITES_INIT;
    write.querySet = set->Set;
    write.beginningOfPassWriteIndex = index;
    write.endOfPassWriteIndex = 0; // makePassTimestampWrites doesn't pass undefined properly thus it has to be a valid query (index 0 is left as dummy)
    _pendingTimestampWrites.Add(write);
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
        EndRenderPass();

    // Clear with a render pass
    WGPURenderPassColorAttachment colorAttachment;
    WGPURenderPassDepthStencilAttachment depthStencilAttachment;
    WGPURenderPassDescriptor renderPassDesc = WGPU_RENDER_PASS_DESCRIPTOR_INIT;
    if (((GPUTextureWebGPU*)clear.View->GetParent())->IsDepthStencil())
    {
        renderPassDesc.depthStencilAttachment = &depthStencilAttachment;
        depthStencilAttachment = WGPU_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_INIT;
        depthStencilAttachment.view = clear.View->ViewRender;
        depthStencilAttachment.depthLoadOp = WGPULoadOp_Clear;
        depthStencilAttachment.depthStoreOp = WGPUStoreOp_Store;
        depthStencilAttachment.depthClearValue = clear.Depth;
        depthStencilAttachment.stencilClearValue = clear.Stencil;
        if (clear.View->HasStencil)
        {
            depthStencilAttachment.stencilLoadOp = WGPULoadOp_Clear;
            depthStencilAttachment.stencilStoreOp = WGPUStoreOp_Store;
        }
    }
    else
    {
        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &colorAttachment;
        colorAttachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
        colorAttachment.view = clear.View->ViewRender;
        colorAttachment.depthSlice = clear.View->DepthSlice;
        colorAttachment.loadOp = WGPULoadOp_Clear;
        colorAttachment.storeOp = WGPUStoreOp_Store;
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
        if (clear.View != _depthStencil && !SpanContains(renderTargets, clear.View))
        {
            ManualClear(clear);
            _pendingClears.RemoveAt(i);
        }
    }

    // Check if need to start a new render pass
    if (_renderPassDirty || !_renderPass)
    {
        FlushRenderPass();
    }

    // Flush rendering states
    if (_pipelineDirty)
    {
        _pipelineDirty = false;
        WGPURenderPipeline pipeline = _pipelineState ? _pipelineState->GetPipeline(_pipelineKey, { _shaderResources }) : nullptr;
        wgpuRenderPassEncoderSetPipeline(_renderPass, pipeline);
        RENDER_STAT_PS_STATE_CHANGE();

        // Invalidate bind groups (layout might change)
        _bindGroupDirty = true;
    }
    if (_scissorRectDirty)
    {
        _scissorRectDirty = false;
        wgpuRenderPassEncoderSetScissorRect(_renderPass, (uint32_t)_scissorRect.GetX(), (uint32_t)_scissorRect.GetY(), (uint32_t)_scissorRect.GetWidth(), (uint32_t)_scissorRect.GetHeight());
    }
    if (_viewportDirty)
    {
        _viewportDirty = false;
        wgpuRenderPassEncoderSetViewport(_renderPass, _viewport.X, _viewport.Y, _viewport.Width, _viewport.Height, _viewport.MinDepth, _viewport.MaxDepth);
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
        FlushBindGroup();
    }
}

WGPUComputePassEncoder GPUContextWebGPU::OnDispatch(GPUShaderProgramCS* shader)
{
    // End existing render pass (if any)
    if (_renderPass)
        EndRenderPass();

    // Flush pending clears
    FlushState();

    // Start a new compute pass
    WGPUComputePassDescriptor computePassDesc = WGPU_COMPUTE_PASS_DESCRIPTOR_INIT;
    FlushTimestamps(1);
    if (_pendingTimestampWrites.HasItems())
        computePassDesc.timestampWrites = &_pendingTimestampWrites.Last();
    _pendingTimestampWrites.Clear();
    auto computePass = wgpuCommandEncoderBeginComputePass(Encoder, &computePassDesc);
    ASSERT(computePass);

    // Set pipeline
    GPUPipelineStateWebGPU::BindGroupKey key;
    auto shaderWebGPU = (GPUShaderProgramCSWebGPU*)shader;
    WGPUComputePipeline pipeline = shaderWebGPU->GetPipeline(_device->Device, { _shaderResources }, key.Layout);
    wgpuComputePassEncoderSetPipeline(computePass, pipeline);

    // Set bind group
    uint32 dynamicOffsets[DynamicOffsetsMax];
    uint32 dynamicOffsetsCount = 0;
    BuildBindGroup(0, shaderWebGPU->DescriptorInfo, key, dynamicOffsets, dynamicOffsetsCount);
    WGPUBindGroup bindGroup = shaderWebGPU->GetBindGroup(_device->Device, key);
    wgpuComputePassEncoderSetBindGroup(computePass, 0, bindGroup, dynamicOffsetsCount, dynamicOffsets);

    return computePass;
}

void GPUContextWebGPU::EndRenderPass()
{
    wgpuRenderPassEncoderEnd(_renderPass);
    wgpuRenderPassEncoderRelease(_renderPass);
    _renderPass = nullptr;
}

void GPUContextWebGPU::EndComputePass(WGPUComputePassEncoder computePass)
{
    wgpuComputePassEncoderEnd(computePass);
    wgpuComputePassEncoderRelease(computePass);
    computePass = nullptr;
}

void GPUContextWebGPU::FlushRenderPass()
{
    _renderPassDirty = false;

    // End existing pass (if any)
    if (_renderPass)
        EndRenderPass();

    // Start a new render pass
    WGPURenderPassColorAttachment colorAttachments[GPU_MAX_RT_BINDED];
    WGPURenderPassDepthStencilAttachment depthStencilAttachment;
    WGPURenderPassDescriptor renderPassDesc = WGPU_RENDER_PASS_DESCRIPTOR_INIT;
    renderPassDesc.colorAttachmentCount = _renderTargetCount;
    renderPassDesc.colorAttachments = colorAttachments;
    PendingClear clear;
    _pipelineKey.MultiSampleCount = 1;
    _pipelineKey.RenderTargetCount = _renderTargetCount;
    GPUTextureViewSizeWebGPU attachmentSize;
    for (int32 i = 0; i < renderPassDesc.colorAttachmentCount; i++)
    {
        auto& colorAttachment = colorAttachments[i];
        colorAttachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
        auto renderTarget = _renderTargets[i];
        colorAttachment.view = renderTarget->ViewRender;
        colorAttachment.depthSlice = renderTarget->DepthSlice;
        colorAttachment.loadOp = WGPULoadOp_Load;
        colorAttachment.storeOp = WGPUStoreOp_Store;
        if (FindClear(renderTarget, clear))
        {
            colorAttachment.loadOp = WGPULoadOp_Clear;
            colorAttachment.clearValue = { clear.RGBA[0], clear.RGBA[1], clear.RGBA[2], clear.RGBA[3] };
        }
        _pipelineKey.MultiSampleCount = (int32)renderTarget->GetMSAA();
        _pipelineKey.RenderTargetFormats[i] = renderTarget->Format;
        attachmentSize.Set(renderTarget->RenderSize);
    }
    if (_depthStencil)
    {
        auto renderTarget = _depthStencil;
        renderPassDesc.depthStencilAttachment = &depthStencilAttachment;
        depthStencilAttachment = WGPU_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_INIT;
        depthStencilAttachment.view = renderTarget->ViewRender;
        depthStencilAttachment.depthLoadOp = renderTarget->ReadOnly ? WGPULoadOp_Undefined : WGPULoadOp_Load;
        depthStencilAttachment.depthStoreOp = renderTarget->ReadOnly ? WGPUStoreOp_Undefined : WGPUStoreOp_Store;
        depthStencilAttachment.depthReadOnly = renderTarget->ReadOnly;
        if (renderTarget->HasStencil)
        {
            depthStencilAttachment.stencilLoadOp = renderTarget->ReadOnly ? WGPULoadOp_Undefined : WGPULoadOp_Load;
            depthStencilAttachment.stencilStoreOp = renderTarget->ReadOnly ? WGPUStoreOp_Undefined : WGPUStoreOp_Store;
            depthStencilAttachment.depthReadOnly = renderTarget->ReadOnly;
            depthStencilAttachment.stencilReadOnly = renderTarget->ReadOnly;
        }
        else
        {
            depthStencilAttachment.stencilClearValue = 0;
            depthStencilAttachment.stencilLoadOp = WGPULoadOp_Undefined;
            depthStencilAttachment.stencilStoreOp = WGPUStoreOp_Undefined;
            depthStencilAttachment.stencilReadOnly = true;
        }
        if (!renderTarget->ReadOnly && FindClear(renderTarget, clear))
        {
            depthStencilAttachment.depthLoadOp = WGPULoadOp_Clear;
            depthStencilAttachment.depthClearValue = clear.Depth;
            if (renderTarget->HasStencil)
            {
                depthStencilAttachment.stencilLoadOp = WGPULoadOp_Clear;
                depthStencilAttachment.stencilClearValue = clear.Stencil;
            }
        }
        else
        {
            depthStencilAttachment.depthClearValue = 0.0f;
            depthStencilAttachment.stencilClearValue = 0;
        }
        _pipelineKey.DepthStencilFormat = renderTarget->Format;
        attachmentSize.Set(renderTarget->RenderSize);
    }
    else
    {
        _pipelineKey.DepthStencilFormat = WGPUTextureFormat_Undefined;
    }
    if (_pendingOcclusionQuerySet != _activeOcclusionQuerySet)
    {
        _activeOcclusionQuerySet = _pendingOcclusionQuerySet;
        renderPassDesc.occlusionQuerySet = _device->QuerySets[_activeOcclusionQuerySet]->Set;
    }
    FlushTimestamps(1);
    if (_pendingTimestampWrites.HasItems())
        renderPassDesc.timestampWrites = &_pendingTimestampWrites.Last();
    _pendingTimestampWrites.Clear();
    ASSERT(attachmentSize.Packed != 0);
    _renderPass = wgpuCommandEncoderBeginRenderPass(Encoder, &renderPassDesc);
    ASSERT(_renderPass);

    // Discard texture clears (done manually or via render pass)
    _pendingClears.Clear();

    // Apply pending state
    if (_stencilRef != 0)
        wgpuRenderPassEncoderSetStencilReference(_renderPass, _stencilRef);

    // Auto-dirty pipeline when new render pass starts
    if (_pipelineState)
        _pipelineDirty = true;
    _indexBufferDirty = true;
    _vertexBufferDirty = true;
    _bindGroupDirty = true;
    if (_blendFactorSet)
        _blendFactorDirty = true;
    _scissorRectDirty |= _scissorRect != Rectangle(0, 0, attachmentSize.Width, attachmentSize.Height);
    _viewportDirty |= _viewport != Viewport(Float2(attachmentSize.Width, attachmentSize.Height));
    ASSERT_LOW_LAYER(_scissorRect.GetWidth() <= attachmentSize.Width && _scissorRect.GetHeight() <= attachmentSize.Height);
}

void GPUContextWebGPU::FlushBindGroup()
{
    _bindGroupDirty = false;

    // Each shader stage (Vertex, Pixel) uses a separate bind group
    GPUPipelineStateWebGPU::BindGroupKey key;
    uint32 dynamicOffsets[DynamicOffsetsMax];
    for (uint32 groupIndex = 0; groupIndex < GPUBindGroupsWebGPU::GraphicsMax; groupIndex++)
    {
        auto descriptors = _pipelineState->BindGroupDescriptors[groupIndex];
        key.Layout = _pipelineState->BindGroupLayouts[groupIndex];
        if (!descriptors || !key.Layout)
            continue;

        // Build descriptors
        uint32 dynamicOffsetsCount = 0;
        BuildBindGroup(groupIndex, *descriptors, key, dynamicOffsets, dynamicOffsetsCount);

        // Bind group
        WGPUBindGroup bindGroup = _pipelineState->GetBindGroup(key);
        wgpuRenderPassEncoderSetBindGroup(_renderPass, groupIndex, bindGroup, dynamicOffsetsCount, dynamicOffsets);
    }
}

void GPUContextWebGPU::FlushTimestamps(int32 skipLast)
{
    for (int32 i = 0; i < _pendingTimestampWrites.Count() - skipLast; i++)
    {
        // WebGPU timestamps have very bad API design made for single-file examples, not real game engines so drain writes here with dummy render passes
        // Also, webgpu.h wrapper doesn't pass timestampWrites as array but just a single item...
        WGPURenderPassDescriptor dummyDesc = WGPU_RENDER_PASS_DESCRIPTOR_INIT;
        if (!_device->DefaultRenderTarget)
        {
            _device->DefaultRenderTarget = (GPUTextureWebGPU*)_device->CreateTexture(TEXT("DefaultRenderTarget"));
            _device->DefaultRenderTarget->Init(GPUTextureDescription::New2D(1, 1, PixelFormat::R8G8B8A8_UNorm, GPUTextureFlags::RenderTarget));
        }
        WGPURenderPassColorAttachment dummyAttachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
        dummyAttachment.view = ((GPUTextureViewWebGPU*)_device->DefaultRenderTarget->View(0))->ViewRender;
        dummyAttachment.loadOp = WGPULoadOp_Clear;
        dummyAttachment.storeOp = WGPUStoreOp_Discard;
        dummyDesc.colorAttachmentCount = 1;
        dummyDesc.colorAttachments = &dummyAttachment;
        dummyDesc.timestampWrites = &_pendingTimestampWrites[i];
        auto renderPass = wgpuCommandEncoderBeginRenderPass(Encoder, &dummyDesc);
        wgpuRenderPassEncoderEnd(renderPass);
        wgpuRenderPassEncoderRelease(renderPass);
    }
}

void GPUContextWebGPU::BuildBindGroup(uint32 groupIndex, const SpirvShaderDescriptorInfo& descriptors, GPUPipelineStateWebGPU::BindGroupKey& key, uint32 dynamicOffsets[DynamicOffsetsMax], uint32& dynamicOffsetsCount)
{
    // Build descriptors for the bind group
    auto entriesCount = descriptors.DescriptorTypesCount;
    static_assert(ARRAY_COUNT(key.Entries) == SpirvShaderDescriptorInfo::MaxDescriptors, "Invalid size of bind group entries array.");
    static_assert(ARRAY_COUNT(key.Versions) == SpirvShaderDescriptorInfo::MaxDescriptors, "Invalid size of bind group versions array.");
    key.EntriesCount = entriesCount;
    auto entriesPtr = key.Entries;
    auto versionsPtr = key.Versions;
    Platform::MemoryClear(entriesPtr, entriesCount * sizeof(WGPUBindGroupEntry));
    Platform::MemoryClear(versionsPtr, ((entriesCount + 3) & ~0x3) * sizeof(uint8));
    for (int32 index = 0; index < entriesCount; index++)
    {
        auto& descriptor = descriptors.DescriptorTypes[index];
        auto& entry = entriesPtr[index];
        entry.binding = descriptor.Binding;
        entry.size = WGPU_WHOLE_SIZE;
        switch (descriptor.DescriptorType)
        {
        case VK_DESCRIPTOR_TYPE_SAMPLER:
        {
            GPUSamplerWebGPU* sampler = _samplers[descriptor.Slot];
            if (!sampler)
                sampler = _device->DefaultSamplers[0]; // Fallback
            entry.sampler = sampler->Sampler;
            break;
        }
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        {
            ASSERT_LOW_LAYER(descriptor.BindingType == SpirvShaderResourceBindingType::SRV);
            auto view = _shaderResources[descriptor.Slot];
            auto ptr = view ? (GPUResourceViewPtrWebGPU*)view->GetNativePtr() : nullptr;
            if (ptr && ptr->TextureView)
            {
                entry.textureView = ptr->TextureView->View;
                versionsPtr[index] = ptr->Version;
            }
            if (!entry.textureView)
            {
                // Fallback
                auto defaultTexture = _device->DefaultTexture[(int32)descriptor.ResourceType];
                if (!defaultTexture)
                {
                    LOG(Error, "Missing default resource {} at slot {} of binding space {}", (int32)descriptor.ResourceType, descriptor.Slot, (int32)descriptor.BindingType);
                    CRASH;
                }
                switch (descriptor.ResourceType)
                {
                case SpirvShaderResourceType::Texture3D:
                    view = defaultTexture->ViewVolume();
                    break;
                case SpirvShaderResourceType::Texture1DArray:
                case SpirvShaderResourceType::Texture2DArray:
                    view = defaultTexture->ViewArray();
                    break;
                default:
                    view = defaultTexture->View(0);
                    break;
                }
                ptr = (GPUResourceViewPtrWebGPU*)view->GetNativePtr();
                entry.textureView = ptr->TextureView->View;
            }
            break;
        }
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
        {
            ASSERT(descriptor.Slot < _resourceTableSizes[(int32)descriptor.BindingType]);
            GPUResourceView* view = _resourceTables[(int32)descriptor.BindingType][descriptor.Slot];
            auto ptr = view ? (GPUResourceViewPtrWebGPU*)view->GetNativePtr() : nullptr;
            if (ptr && ptr->BufferView)
            {
                entry.buffer = ptr->BufferView->Buffer;
                entry.size = ((GPUBufferWebGPU*)view->GetParent())->GetSize();
                versionsPtr[index] = (uint64)ptr->Version;
            }
            if (!entry.buffer)
                entry.buffer = _device->DefaultBuffer; // Fallback
            break;
        }
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        {
            GPUConstantBufferWebGPU* uniform = _constantBuffers[descriptor.Slot];
            if (uniform && uniform->Allocation.Buffer)
            {
                entry.buffer = uniform->Allocation.Buffer;
                entry.size = uniform->AllocationSize;
                if (descriptor.DescriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                    entry.offset = uniform->Allocation.Offset;
                else
                    dynamicOffsets[dynamicOffsetsCount++] = uniform->Allocation.Offset;
            }
            else
                LOG(Fatal, "Missing constant buffer at slot {}", descriptor.Slot);
            break;
        }
        default:
#if GPU_ENABLE_DIAGNOSTICS
            LOG(Fatal, "Unknown descriptor type: {} used as {}", (uint32)descriptor.DescriptorType, (uint32)descriptor.BindingType);
#else
            CRASH;
#endif
            return;
        }
    }

#if BUILD_DEBUG
    // Validate
    for (int32 i = 0; i < entriesCount; i++)
    {
        auto& e = entriesPtr[i];
        if ((e.buffer != nullptr) + (e.sampler != nullptr) + (e.textureView != nullptr) != 1)
        {
            LOG(Error, "Invalid binding in group {} at index {} ({})", groupIndex, i, _pipelineState->GetName());
            LOG(Error, " > sampler: {}", (uint32)e.sampler);
            LOG(Error, " > textureView: {}", (uint32)e.textureView);
            LOG(Error, " > buffer: {}", (uint32)e.buffer);
        }
    }
    ASSERT(dynamicOffsetsCount <= DynamicOffsetsMax);
#endif
}

#endif
