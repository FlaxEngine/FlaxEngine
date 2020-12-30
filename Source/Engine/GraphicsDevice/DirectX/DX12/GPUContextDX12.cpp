// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX12

#include "GPUContextDX12.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Viewport.h"
#include "Engine/Core/Math/Rectangle.h"
#include "GPUShaderDX12.h"
#include "GPUPipelineStateDX12.h"
#include "UploadBufferDX12.h"
#include "GPUTextureDX12.h"
#include "GPUBufferDX12.h"
#include "CommandQueueDX12.h"
#include "DescriptorHeapDX12.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/GraphicsDevice/DirectX/RenderToolsDX.h"
#include "Engine/Debug/Exceptions/NotImplementedException.h"
#include "GPUShaderProgramDX12.h"
#include "CommandSignatureDX12.h"
#include "Engine/Profiler/RenderStats.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Threading/Threading.h"
#if USE_PIX && GPU_ALLOW_PROFILE_EVENTS
#include <pix3.h>
#endif

#define DX12_ENABLE_RESOURCE_BARRIERS_BATCHING 1
#define DX12_ENABLE_RESOURCE_BARRIERS_DEBUGGING 0

inline bool operator!=(const D3D12_VERTEX_BUFFER_VIEW& l, const D3D12_VERTEX_BUFFER_VIEW& r)
{
    return l.SizeInBytes != r.SizeInBytes || l.StrideInBytes != r.StrideInBytes || l.BufferLocation != r.BufferLocation;
}

inline bool operator!=(const D3D12_INDEX_BUFFER_VIEW& l, const D3D12_INDEX_BUFFER_VIEW& r)
{
    return l.SizeInBytes != r.SizeInBytes || l.Format != r.Format || l.BufferLocation != r.BufferLocation;
}

// Ensure to match the indirect commands arguments layout
static_assert(sizeof(GPUDispatchIndirectArgs) == sizeof(D3D12_DISPATCH_ARGUMENTS), "Wrong size of GPUDrawIndirectArgs.");
static_assert(OFFSET_OF(GPUDispatchIndirectArgs, ThreadGroupCountX) == OFFSET_OF(D3D12_DISPATCH_ARGUMENTS, ThreadGroupCountX), "Wrong offset for GPUDrawIndirectArgs::ThreadGroupCountX");
static_assert(OFFSET_OF(GPUDispatchIndirectArgs, ThreadGroupCountY) == OFFSET_OF(D3D12_DISPATCH_ARGUMENTS, ThreadGroupCountY),"Wrong offset for GPUDrawIndirectArgs::ThreadGroupCountY");
static_assert(OFFSET_OF(GPUDispatchIndirectArgs, ThreadGroupCountZ) == OFFSET_OF(D3D12_DISPATCH_ARGUMENTS, ThreadGroupCountZ), "Wrong offset for GPUDrawIndirectArgs::ThreadGroupCountZ");
//
static_assert(sizeof(GPUDrawIndirectArgs) == sizeof(D3D12_DRAW_ARGUMENTS), "Wrong size of GPUDrawIndirectArgs.");
static_assert(OFFSET_OF(GPUDrawIndirectArgs, VerticesCount) == OFFSET_OF(D3D12_DRAW_ARGUMENTS, VertexCountPerInstance), "Wrong offset for GPUDrawIndirectArgs::VerticesCount");
static_assert(OFFSET_OF(GPUDrawIndirectArgs, InstanceCount) == OFFSET_OF(D3D12_DRAW_ARGUMENTS, InstanceCount),"Wrong offset for GPUDrawIndirectArgs::InstanceCount");
static_assert(OFFSET_OF(GPUDrawIndirectArgs, StartVertex) == OFFSET_OF(D3D12_DRAW_ARGUMENTS, StartVertexLocation), "Wrong offset for GPUDrawIndirectArgs::StartVertex");
static_assert(OFFSET_OF(GPUDrawIndirectArgs, StartInstance) == OFFSET_OF(D3D12_DRAW_ARGUMENTS, StartInstanceLocation), "Wrong offset for GPUDrawIndirectArgs::StartInstance");
//
static_assert(sizeof(GPUDrawIndexedIndirectArgs) == sizeof(D3D12_DRAW_INDEXED_ARGUMENTS), "Wrong size of GPUDrawIndexedIndirectArgs.");
static_assert(OFFSET_OF(GPUDrawIndexedIndirectArgs, IndicesCount) == OFFSET_OF(D3D12_DRAW_INDEXED_ARGUMENTS, IndexCountPerInstance), "Wrong offset for GPUDrawIndexedIndirectArgs::IndicesCount");
static_assert(OFFSET_OF(GPUDrawIndexedIndirectArgs, InstanceCount) == OFFSET_OF(D3D12_DRAW_INDEXED_ARGUMENTS, InstanceCount),"Wrong offset for GPUDrawIndexedIndirectArgs::InstanceCount");
static_assert(OFFSET_OF(GPUDrawIndexedIndirectArgs, StartIndex) == OFFSET_OF(D3D12_DRAW_INDEXED_ARGUMENTS, StartIndexLocation), "Wrong offset for GPUDrawIndexedIndirectArgs::StartIndex");
static_assert(OFFSET_OF(GPUDrawIndexedIndirectArgs, StartVertex) == OFFSET_OF(D3D12_DRAW_INDEXED_ARGUMENTS, BaseVertexLocation), "Wrong offset for GPUDrawIndexedIndirectArgs::StartVertex");
static_assert(OFFSET_OF(GPUDrawIndexedIndirectArgs, StartInstance) == OFFSET_OF(D3D12_DRAW_INDEXED_ARGUMENTS, StartInstanceLocation), "Wrong offset for GPUDrawIndexedIndirectArgs::StartInstance");

GPUContextDX12::GPUContextDX12(GPUDeviceDX12* device, D3D12_COMMAND_LIST_TYPE type)
    : GPUContext(device)
    , _device(device)
    , _commandList(nullptr)
    , _currentAllocator(nullptr)
    , _currentState(nullptr)
    , _currentCompute(nullptr)
    , _swapChainsUsed(0)
    , _vbCount(0)
    , _rtCount(0)
    , _rbBufferSize(0)
    , _srMaskDirtyGraphics(0)
    , _srMaskDirtyCompute(0)
    , _uaMaskDirtyGraphics(0)
    , _uaMaskDirtyCompute(0)
    , _isCompute(0)
    , _rtDirtyFlag(0)
    , _psDirtyFlag(0)
    , _cbDirtyFlag(0)
    , _rtDepth(nullptr)
    , _ibHandle(nullptr)
{
    _currentAllocator = _device->GetCommandQueue()->RequestAllocator();
    VALIDATE_DIRECTX_RESULT(device->GetDevice()->CreateCommandList(0, type, _currentAllocator, nullptr, IID_PPV_ARGS(&_commandList)));
#if GPU_ENABLE_RESOURCE_NAMING
    _commandList->SetName(TEXT("GPUContextDX12::CommandList"));
#endif
}

GPUContextDX12::~GPUContextDX12()
{
    DX_SAFE_RELEASE_CHECK(_commandList, 0);
}

void GPUContextDX12::AddTransitionBarrier(ResourceOwnerDX12* resource, const D3D12_RESOURCE_STATES before, const D3D12_RESOURCE_STATES after, const int32 subresourceIndex)
{
    // Check if need to flush barriers
    if (_rbBufferSize == DX12_RB_BUFFER_SIZE)
        flushRBs();

#if DX12_ENABLE_RESOURCE_BARRIERS_DEBUGGING
    String resourceName;
    const auto gpuResource = resource->AsGPUResource();
    if (gpuResource)
        resourceName = gpuResource->GetName();
    else
        resourceName = StringUtils::ToString((uint32)(uint64)resource->GetResource());
    const auto info = String::Format(TEXT("[DX12 Resource Barrier]: 0x{0:x} -> 0x{1:x}: {2} (subresource: {3})"), before, after, resourceName, subresourceIndex);
    Log::Logger::Write(LogType::Info, info);
#endif

#if DX12_ENABLE_RESOURCE_BARRIERS_BATCHING

    // Enqueue barrier
    D3D12_RESOURCE_BARRIER& barrier = _rbBuffer[_rbBufferSize];
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = resource->GetResource();
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter = after;
    barrier.Transition.Subresource = subresourceIndex;
    _rbBufferSize++;

#else

    D3D12_RESOURCE_BARRIER barrier;
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = resource->GetResource();
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter = after;
    barrier.Transition.Subresource = subresourceIndex;
    _commandList->ResourceBarrier(1, &barrier);

#endif
}

void GPUContextDX12::SetResourceState(ResourceOwnerDX12* resource, D3D12_RESOURCE_STATES after, int32 subresourceIndex)
{
    // Check if resource is missing
    auto nativeResource = resource->GetResource();
    if (nativeResource == nullptr)
        return;

    auto& state = resource->State;
    if (subresourceIndex == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES && !state.AreAllSubresourcesSame())
    {
        // Slow path because we have to transition the entire resource with multiple subresources that aren't in the same state
        const uint32 subresourceCount = resource->GetSubresourcesCount();
        for (uint32 i = 0; i < subresourceCount; i++)
        {
            const D3D12_RESOURCE_STATES before = state.GetSubresourceState(i);
            if (before != after)
            {
                AddTransitionBarrier(resource, before, after, i);
                state.SetSubresourceState(i, after);
            }
        }
        ASSERT(state.CheckResourceState(after));
        state.SetResourceState(after);
    }
    else
    {
        const D3D12_RESOURCE_STATES before = state.GetSubresourceState(subresourceIndex);
        if (ResourceStateDX12::IsTransitionNeeded(before, after))
        {
            AddTransitionBarrier(resource, before, after, subresourceIndex);
            state.SetSubresourceState(subresourceIndex, after);
        }
    }
}

void GPUContextDX12::Reset()
{
    // The command list persists, but we must request a new allocator
    ASSERT(_commandList != nullptr);
    if (_currentAllocator == nullptr)
    {
        _currentAllocator = _device->GetCommandQueue()->RequestAllocator();
        _commandList->Reset(_currentAllocator, nullptr);
    }

    // Setup initial state
    _currentState = nullptr;
    _rtDirtyFlag = false;
    _cbDirtyFlag = false;
    _rtCount = 0;
    _rtDepth = nullptr;
    _srMaskDirtyGraphics = 0;
    _srMaskDirtyCompute = 0;
    _uaMaskDirtyGraphics = 0;
    _uaMaskDirtyCompute = 0;
    _psDirtyFlag = false;
    _isCompute = false;
    _currentCompute = nullptr;
    _rbBufferSize = 0;
    _vbCount = 0;
    Platform::MemoryClear(_rtHandles, sizeof(_rtHandles));
    Platform::MemoryClear(_srHandles, sizeof(_srHandles));
    Platform::MemoryClear(_uaHandles, sizeof(_uaHandles));
    Platform::MemoryClear(_vbHandles, sizeof(_vbHandles));
    Platform::MemoryClear(&_ibHandle, sizeof(_ibHandle));
    Platform::MemoryClear(&_cbHandles, sizeof(_cbHandles));
    _swapChainsUsed = 0;

    // Bind Root Signature
    _commandList->SetGraphicsRootSignature(_device->GetRootSignature());
    _commandList->SetComputeRootSignature(_device->GetRootSignature());

    // Bind heaps
    ID3D12DescriptorHeap* ppHeaps[] = { _device->RingHeap_CBV_SRV_UAV.GetHeap() };
    _commandList->SetDescriptorHeaps(ARRAY_COUNT(ppHeaps), ppHeaps);
}

uint64 GPUContextDX12::Execute(bool waitForCompletion)
{
    ASSERT(_currentAllocator != nullptr);
    auto queue = _device->GetCommandQueue();

    // Flush reaming and buffered commands
    FlushState();
    _currentState = nullptr;

    // Execute commands
    const uint64 fenceValue = queue->ExecuteCommandList(_commandList);

    // Cleanup used allocator
    queue->DiscardAllocator(fenceValue, _currentAllocator);
    _currentAllocator = nullptr;

    // Wait for GPU if need to
    if (waitForCompletion)
        queue->WaitForFence(fenceValue);

    return fenceValue;
}

void GPUContextDX12::OnSwapChainFlush()
{
    _swapChainsUsed++;

    // Flush per-window (excluding the main window)
    if (_swapChainsUsed > 1)
    {
        // Flush GPU commands
        Flush();
    }
}

void GPUContextDX12::GetActiveHeapDescriptor(const D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle, DescriptorHeapRingBufferDX12::Allocation& descriptor)
{
    // Allocate data for the table
    descriptor = _device->RingHeap_CBV_SRV_UAV.AllocateTable(1);

    // Copy descriptor
    _device->GetDevice()->CopyDescriptorsSimple(1, descriptor.CPU, cpuHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void GPUContextDX12::flushSRVs()
{
    uint32 srMask;
    if (_isCompute)
    {
        // Skip if no compute shader binded or it doesn't use shader resources
        if (_srMaskDirtyCompute == 0 || _currentCompute == nullptr || (srMask = _currentCompute->GetBindings().UsedSRsMask) == 0)
            return;

        // Bind all required slots and mark them as not dirty
        _srMaskDirtyCompute &= ~srMask;
    }
    else
    {
        // Skip if no state binded or it doesn't use shader resources
        if (_srMaskDirtyGraphics == 0 || _currentState == nullptr || (srMask = _currentState->GetUsedSRsMask()) == 0)
            return;

        // Bind all required slots and mark them as not dirty
        _srMaskDirtyGraphics &= ~srMask;
    }

    // Count SRVs required to be bind to the pipeline (the index of the most significant bit that's set)
    const uint32 srCount = Math::FloorLog2(srMask) + 1;
    ASSERT(srCount <= GPU_MAX_SR_BINDED);

    // Fill table with source descriptors
    D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptorRangeStarts[GPU_MAX_SR_BINDED];
    for (uint32 i = 0; i < srCount; i++)
    {
        const auto handle = _srHandles[i];
        if (handle != nullptr)
        {
            srcDescriptorRangeStarts[i] = handle->SRV();

            D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            if (handle->IsDepthStencilResource())
                state |= D3D12_RESOURCE_STATE_DEPTH_READ;

            SetResourceState(handle->GetResourceOwner(), state, handle->SubresourceIndex);
        }
        else
        {
            srcDescriptorRangeStarts[i] = _device->NullSRV();
        }
    }

    // Allocate data for the table
    auto allocation = _device->RingHeap_CBV_SRV_UAV.AllocateTable(srCount);

    // Copy descriptors
    _device->GetDevice()->CopyDescriptors(
        1, &allocation.CPU, &srCount,
        srCount, srcDescriptorRangeStarts, nullptr,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Flush SRV descriptors table
    if (_isCompute)
        _commandList->SetComputeRootDescriptorTable(2, allocation.GPU);
    else
        _commandList->SetGraphicsRootDescriptorTable(2, allocation.GPU);
}

void GPUContextDX12::flushRTVs()
{
    // Check if update render target bindings
    if (_rtDirtyFlag)
    {
        // Clear flag
        _rtDirtyFlag = false;

        // Get render targets CPU descriptors
        D3D12_CPU_DESCRIPTOR_HANDLE rtCPU[GPU_MAX_RT_BINDED];
        for (int32 i = 0; i < _rtCount; i++)
        {
            const auto handle = _rtHandles[i];
            SetResourceState(handle->GetResourceOwner(), D3D12_RESOURCE_STATE_RENDER_TARGET, handle->SubresourceIndex);
            rtCPU[i] = handle->RTV();
        }

        // Get depth buffer handle
        D3D12_CPU_DESCRIPTOR_HANDLE depthBuffer;
        if (_rtDepth)
        {
            depthBuffer = _rtDepth->DSV();
            SetResourceState(_rtDepth->GetResourceOwner(), D3D12_RESOURCE_STATE_DEPTH_WRITE, _rtDepth->SubresourceIndex);
        }
        else
        {
            depthBuffer.ptr = 0;
        }

        // Flush output merger render targets
        _commandList->OMSetRenderTargets(_rtCount, rtCPU, false, depthBuffer.ptr != 0 ? &depthBuffer : nullptr);
    }
}

void GPUContextDX12::flushUAVs()
{
    uint32 uaMask;
    if (_isCompute)
    {
        // Skip if no compute shader binded or it doesn't use shader resources
        if (_uaMaskDirtyCompute == 0 || _currentCompute == nullptr || (uaMask = _currentCompute->GetBindings().UsedUAsMask) == 0)
            return;

        // Bind all dirty slots and all used slots
        uaMask |= _uaMaskDirtyCompute;
        _uaMaskDirtyCompute = 0;
    }
    else
    {
        // Skip if no state binded or it doesn't use shader resources
        if (_uaMaskDirtyGraphics == 0 || _currentState == nullptr || (uaMask = _currentState->GetUsedUAsMask()) == 0)
            return;

        // Bind all dirty slots and all used slots
        uaMask |= _uaMaskDirtyGraphics;
        _uaMaskDirtyGraphics = 0;
    }

    // Count UAVs required to be bind to the pipeline (the index of the most significant bit that's set)
    const uint32 uaCount = Math::FloorLog2(uaMask) + 1;
    ASSERT(uaCount <= GPU_MAX_UA_BINDED);

    // Fill table with source descriptors
    D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptorRangeStarts[GPU_MAX_UA_BINDED];
    for (uint32 i = 0; i < uaCount; i++)
    {
        const auto handle = _uaHandles[i];
        if (handle != nullptr)
        {
            srcDescriptorRangeStarts[i] = handle->UAV();
            SetResourceState(handle->GetResourceOwner(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        }
        else
        {
            srcDescriptorRangeStarts[i] = _device->NullUAV();
        }
    }

    // Allocate data for the table
    auto allocation = _device->RingHeap_CBV_SRV_UAV.AllocateTable(uaCount);

    // Copy descriptors
    _device->GetDevice()->CopyDescriptors(
        1, &allocation.CPU, &uaCount,
        uaCount, srcDescriptorRangeStarts, nullptr,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Flush UAV descriptors table
    if (_isCompute)
        _commandList->SetComputeRootDescriptorTable(3, allocation.GPU);
    else
        _commandList->SetGraphicsRootDescriptorTable(3, allocation.GPU);
}

void GPUContextDX12::flushCBs()
{
    // Check if need to flush constant buffers
    if (_cbDirtyFlag)
    {
        // Clear flag
        _cbDirtyFlag = false;

        // Flush with the driver
        for (uint32 slotIndex = 0; slotIndex < ARRAY_COUNT(_cbHandles); slotIndex++)
        {
            auto cb = _cbHandles[slotIndex];
            if (cb)
            {
                ASSERT(cb->GPUAddress != 0);
                if (_isCompute)
                    _commandList->SetComputeRootConstantBufferView(slotIndex, cb->GPUAddress);
                else
                    _commandList->SetGraphicsRootConstantBufferView(slotIndex, cb->GPUAddress);
            }
        }
    }
}

void GPUContextDX12::flushRBs()
{
#if DX12_ENABLE_RESOURCE_BARRIERS_BATCHING

    // Check if flush buffer
    if (_rbBufferSize > 0)
    {
#if DX12_ENABLE_RESOURCE_BARRIERS_DEBUGGING
		const auto info = String::Format(TEXT("[DX12 Resource Barrier]: Flush {0} barriers"), _rbBufferSize);
		Log::Logger::Write(LogType::Info, info);
#endif

        // Flush resource barriers
        _commandList->ResourceBarrier(_rbBufferSize, _rbBuffer);

        // Clear size
        _rbBufferSize = 0;
    }

#endif
}

void GPUContextDX12::flushPS()
{
    if (_psDirtyFlag && _currentState && (_rtDepth || _rtCount))
    {
        // Clear flag
        _psDirtyFlag = false;

        // Change state
        ASSERT(_currentState->IsValid());
        _commandList->SetPipelineState(_currentState->GetState(_rtDepth, _rtCount, _rtHandles));
        _commandList->IASetPrimitiveTopology(_currentState->PrimitiveTopologyType);

        RENDER_STAT_PS_STATE_CHANGE();
    }
}

void GPUContextDX12::onDrawCall()
{
    // Ensure state of the vertex and index buffers
    for (int32 i = 0; i < _vbCount; i++)
    {
        const auto vbDX12 = _vbHandles[i];
        if (vbDX12)
        {
            SetResourceState(vbDX12, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        }
    }
    if (_ibHandle)
    {
        SetResourceState(_ibHandle, D3D12_RESOURCE_STATE_INDEX_BUFFER);
    }

    // Flush
    flushSRVs();
    flushRTVs();
    flushUAVs();
    flushRBs();
    flushPS();
    flushCBs();
}

void GPUContextDX12::FrameBegin()
{
    // Base
    GPUContext::FrameBegin();

    // Prepare command list
    Reset();
}

void GPUContextDX12::FrameEnd()
{
    // Base
    GPUContext::FrameEnd();

    // Execute command (but don't wait for them)
    Execute(false);
}

#if GPU_ALLOW_PROFILE_EVENTS

void GPUContextDX12::EventBegin(const Char* name)
{
#if USE_PIX
    PIXBeginEvent(_commandList, 0, name);
#endif
}

void GPUContextDX12::EventEnd()
{
#if USE_PIX
    PIXEndEvent(_commandList);
#endif
}

#endif

void* GPUContextDX12::GetNativePtr() const
{
    return (void*)_commandList;
}

bool GPUContextDX12::IsDepthBufferBinded()
{
    return _rtDepth != nullptr;
}

void GPUContextDX12::Clear(GPUTextureView* rt, const Color& color)
{
    auto rtDX12 = static_cast<GPUTextureViewDX12*>(rt);

    if (rtDX12)
    {
        SetResourceState(rtDX12->GetResourceOwner(), D3D12_RESOURCE_STATE_RENDER_TARGET, rtDX12->SubresourceIndex);
        flushRBs();

        _commandList->ClearRenderTargetView(rtDX12->RTV(), color.Raw, 0, nullptr);
    }
}

void GPUContextDX12::ClearDepth(GPUTextureView* depthBuffer, float depthValue)
{
    auto depthBufferDX12 = static_cast<GPUTextureViewDX12*>(depthBuffer);

    if (depthBufferDX12)
    {
        SetResourceState(depthBufferDX12->GetResourceOwner(), D3D12_RESOURCE_STATE_DEPTH_WRITE, depthBufferDX12->SubresourceIndex);
        flushRBs();

        _commandList->ClearDepthStencilView(depthBufferDX12->DSV(), D3D12_CLEAR_FLAG_DEPTH, depthValue, 0xff, 0, nullptr);
    }
}

void GPUContextDX12::ClearUA(GPUBuffer* buf, const Vector4& value)
{
    ASSERT(buf != nullptr && buf->IsUnorderedAccess());

    auto bufDX12 = reinterpret_cast<GPUBufferDX12*>(buf);

    SetResourceState(bufDX12, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    flushRBs();

    auto uav = ((GPUBufferViewDX12*)bufDX12->View())->UAV();
    Descriptor desc;
    GetActiveHeapDescriptor(uav, desc);
    _commandList->ClearUnorderedAccessViewFloat(desc.GPU, uav, bufDX12->GetResource(), value.Raw, 0, nullptr);
}

void GPUContextDX12::ResetRenderTarget()
{
    if (_rtDepth != nullptr || _rtCount != 0 || _rtDepth)
    {
        _rtDirtyFlag = false;
        _psDirtyFlag = true;
        _rtCount = 0;
        _rtDepth = nullptr;
        Platform::MemoryClear(_rtHandles, sizeof(_rtHandles));

        D3D12_CPU_DESCRIPTOR_HANDLE rtCPU[GPU_MAX_RT_BINDED];
        Platform::MemoryClear(rtCPU, sizeof(rtCPU));
        _commandList->OMSetRenderTargets(0, rtCPU, false, nullptr);
    }
}

void GPUContextDX12::SetRenderTarget(GPUTextureView* rt)
{
    GPUTextureViewDX12* rtDX12 = static_cast<GPUTextureViewDX12*>(rt);

    if (_rtDepth != nullptr || _rtCount != 1 || _rtHandles[0] != rtDX12)
    {
        _rtDirtyFlag = true;
        _psDirtyFlag = true;
        _rtCount = 1;
        _rtDepth = nullptr;
        _rtHandles[0] = rtDX12;
    }
}

void GPUContextDX12::SetRenderTarget(GPUTextureView* depthBuffer, GPUTextureView* rt)
{
    auto rtDX12 = static_cast<GPUTextureViewDX12*>(rt);
    auto depthBufferDX12 = static_cast<GPUTextureViewDX12*>(depthBuffer);
    auto rtCount = rtDX12 ? 1 : 0;

    if (_rtDepth != depthBufferDX12 || _rtCount != rtCount || _rtHandles[0] != rtDX12)
    {
        _rtDirtyFlag = true;
        _psDirtyFlag = true;
        _rtCount = rtCount;
        _rtDepth = depthBufferDX12;
        _rtHandles[0] = rtDX12;
    }
}

void GPUContextDX12::SetRenderTarget(GPUTextureView* depthBuffer, const Span<GPUTextureView*>& rts)
{
    ASSERT(Math::IsInRange(rts.Length(), 1, GPU_MAX_RT_BINDED));

    const auto depthBufferDX12 = static_cast<GPUTextureViewDX12*>(depthBuffer);

    GPUTextureViewDX12* rtvs[GPU_MAX_RT_BINDED];
    for (int32 i = 0; i < rts.Length(); i++)
    {
        rtvs[i] = static_cast<GPUTextureViewDX12*>(rts[i]);
    }
    const int32 rtvsSize = sizeof(GPUTextureViewDX12*) * rts.Length();

    if (_rtDepth != depthBufferDX12 || _rtCount != rts.Length() || Platform::MemoryCompare(_rtHandles, rtvs, rtvsSize) != 0)
    {
        _rtDirtyFlag = true;
        _psDirtyFlag = true;
        _rtCount = rts.Length();
        _rtDepth = depthBufferDX12;
        Platform::MemoryCopy(_rtHandles, rtvs, rtvsSize);
    }
}

void GPUContextDX12::SetRenderTarget(GPUTextureView* rt, GPUBuffer* uaOutput)
{
    auto uaOutputDX12 = uaOutput ? (GPUBufferViewDX12*)uaOutput->View() : nullptr;

    // Set render target normally
    SetRenderTarget(nullptr, rt);

    // Bind UAV output to the last slot
    const int32 slot = ARRAY_COUNT(_uaHandles) - 1;
    IShaderResourceDX12** lastSlot = &_uaHandles[slot];
    if (*lastSlot != uaOutputDX12)
    {
        *lastSlot = uaOutputDX12;
        _srMaskDirtyGraphics |= 1 << slot;
        _srMaskDirtyCompute |= 1 << slot;
    }
}

void GPUContextDX12::ResetSR()
{
    for (int32 slot = 0; slot < GPU_MAX_SR_BINDED; slot++)
    {
        if (_srHandles[slot])
        {
            _srMaskDirtyGraphics |= 1 << slot;
            _srMaskDirtyCompute |= 1 << slot;
            _srHandles[slot] = nullptr;
        }
    }
}

void GPUContextDX12::ResetUA()
{
    for (uint32 slot = 0; slot < ARRAY_COUNT(_uaHandles); slot++)
    {
        if (_uaHandles[slot])
        {
            _srMaskDirtyGraphics |= 1 << slot;
            _srMaskDirtyCompute |= 1 << slot;
            _uaHandles[slot] = nullptr;
            break;
        }
    }
}

void GPUContextDX12::ResetCB()
{
    _cbDirtyFlag = false;
    Platform::MemoryClear(_cbHandles, sizeof(_cbHandles));
}

void GPUContextDX12::BindCB(int32 slot, GPUConstantBuffer* cb)
{
    ASSERT(slot >= 0 && slot < GPU_MAX_CB_BINDED);

    auto cbDX12 = static_cast<GPUConstantBufferDX12*>(cb);

    if (_cbHandles[slot] != cbDX12)
    {
        _cbDirtyFlag = true;
        _cbHandles[slot] = cbDX12;
    }
}

void GPUContextDX12::BindSR(int32 slot, GPUResourceView* view)
{
    ASSERT(slot >= 0 && slot < GPU_MAX_SR_BINDED);

    auto handle = view ? (IShaderResourceDX12*)view->GetNativePtr() : nullptr;

    if (_srHandles[slot] != handle)
    {
        _srMaskDirtyGraphics |= 1 << slot;
        _srMaskDirtyCompute |= 1 << slot;
        _srHandles[slot] = handle;
    }
}

void GPUContextDX12::BindUA(int32 slot, GPUResourceView* view)
{
    ASSERT(slot >= 0 && slot < GPU_MAX_UA_BINDED);

    auto handle = view ? (IShaderResourceDX12*)view->GetNativePtr() : nullptr;

    if (_uaHandles[slot] != handle)
    {
        _uaMaskDirtyGraphics |= 1 << slot;
        _uaMaskDirtyCompute |= 1 << slot;
        _uaHandles[slot] = handle;
    }
}

void GPUContextDX12::BindVB(const Span<GPUBuffer*>& vertexBuffers, const uint32* vertexBuffersOffsets)
{
    ASSERT(vertexBuffers.Length() >= 0 && vertexBuffers.Length() <= GPU_MAX_VB_BINDED);

    bool vbEdited = _vbCount != vertexBuffers.Length();
    D3D12_VERTEX_BUFFER_VIEW views[GPU_MAX_VB_BINDED];
    for (int32 i = 0; i < vertexBuffers.Length(); i++)
    {
        const auto vbDX12 = static_cast<GPUBufferDX12*>(vertexBuffers[i]);
        if (vbDX12)
        {
            vbDX12->GetVBView(views[i]);
            if (vertexBuffersOffsets)
            {
                views[i].BufferLocation += (D3D12_GPU_VIRTUAL_ADDRESS)vertexBuffersOffsets[i];
                views[i].SizeInBytes -= vertexBuffersOffsets[i];
            }
            vbEdited |= views[i] != _vbViews[i];
        }
        else
        {
            Platform::MemoryClear(&views[i], sizeof(D3D12_VERTEX_BUFFER_VIEW));
        }
        vbEdited |= vbDX12 != _vbHandles[i];
        _vbHandles[i] = vbDX12;
    }
    if (vbEdited)
    {
        _vbCount = vertexBuffers.Length();
        Platform::MemoryCopy(_vbViews, views, sizeof(views));
#if PLATFORM_XBOX_SCARLETT
        if (vertexBuffers.Length() == 0)
            return;
#endif
        _commandList->IASetVertexBuffers(0, vertexBuffers.Length(), views);
    }
}

void GPUContextDX12::BindIB(GPUBuffer* indexBuffer)
{
    const auto ibDX12 = static_cast<GPUBufferDX12*>(indexBuffer);
    D3D12_INDEX_BUFFER_VIEW view;
    ibDX12->GetIBView(view);
    if (ibDX12 != _ibHandle || _ibView != view)
    {
        _ibHandle = ibDX12;
        _commandList->IASetIndexBuffer(&view);
        _ibView = view;
    }
}

void GPUContextDX12::UpdateCB(GPUConstantBuffer* cb, const void* data)
{
    ASSERT(data && cb);
    auto cbDX12 = static_cast<GPUConstantBufferDX12*>(cb);
    const uint32 size = cbDX12->GetSize();
    if (size == 0)
        return;

    // Allocate bytes for the buffer
    DynamicAllocation allocation = _device->UploadBuffer->Allocate(size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    // Copy data
    Platform::MemoryCopy(allocation.CPUAddress, data, allocation.Size);

    // Cache GPU address of the allocation
    cbDX12->GPUAddress = allocation.GPUAddress;

    // Mark CB slot as dirty if this CB is binded to the pipeline
    for (uint32 i = 0; i < ARRAY_COUNT(_cbHandles); i++)
    {
        if (_cbHandles[i] == cbDX12)
        {
            _cbDirtyFlag = true;
            break;
        }
    }
}

void GPUContextDX12::Dispatch(GPUShaderProgramCS* shader, uint32 threadGroupCountX, uint32 threadGroupCountY, uint32 threadGroupCountZ)
{
    _isCompute = 1;
    _currentCompute = shader;

    flushSRVs();
    flushUAVs();
    flushRBs();
    flushCBs();

    auto shaderDX12 = (GPUShaderProgramCSDX12*)shader;
    auto computeState = shaderDX12->GetOrCreateState();

    _commandList->SetPipelineState(computeState);
    RENDER_STAT_PS_STATE_CHANGE();

    _commandList->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
    RENDER_STAT_DISPATCH_CALL();

    _isCompute = 0;
    _currentCompute = nullptr;

    // Restore previous state on next draw call
    _psDirtyFlag = true;
}

void GPUContextDX12::DispatchIndirect(GPUShaderProgramCS* shader, GPUBuffer* bufferForArgs, uint32 offsetForArgs)
{
    _isCompute = 1;
    _currentCompute = shader;

    auto bufferForArgsDX12 = (GPUBufferDX12*)bufferForArgs;
    SetResourceState(bufferForArgsDX12, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

    flushSRVs();
    flushUAVs();
    flushRBs();
    flushCBs();

    auto shaderDX12 = (GPUShaderProgramCSDX12*)shader;
    auto computeState = shaderDX12->GetOrCreateState();

    _commandList->SetPipelineState(computeState);
    RENDER_STAT_PS_STATE_CHANGE();

    auto signature = _device->DispatchIndirectCommandSignature->GetSignature();
    _commandList->ExecuteIndirect(signature, 1, bufferForArgsDX12->GetResource(), (UINT64)offsetForArgs, nullptr, 0);
    RENDER_STAT_DISPATCH_CALL();

    _isCompute = 0;
    _currentCompute = nullptr;

    // Restore previous state on next draw call
    _psDirtyFlag = true;
}

void GPUContextDX12::ResolveMultisample(GPUTexture* sourceMultisampleTexture, GPUTexture* destTexture, int32 sourceSubResource, int32 destSubResource, PixelFormat format)
{
    ASSERT(sourceMultisampleTexture && sourceMultisampleTexture->IsMultiSample());
    ASSERT(destTexture && !destTexture->IsMultiSample());

    auto dstResourceDX12 = static_cast<GPUTextureDX12*>(destTexture);
    auto srcResourceDX12 = static_cast<GPUTextureDX12*>(sourceMultisampleTexture);

    SetResourceState(srcResourceDX12, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
    SetResourceState(dstResourceDX12, D3D12_RESOURCE_STATE_RESOLVE_DEST);
    flushRBs();

    auto formatDXGI = RenderToolsDX::ToDxgiFormat(format == PixelFormat::Unknown ? destTexture->Format() : format);
    _commandList->ResolveSubresource(dstResourceDX12->GetResource(), destSubResource, srcResourceDX12->GetResource(), destSubResource, formatDXGI);
}

void GPUContextDX12::DrawInstanced(uint32 verticesCount, uint32 instanceCount, int32 startInstance, int32 startVertex)
{
    onDrawCall();
    _commandList->DrawInstanced(verticesCount, instanceCount, startVertex, startInstance);
    RENDER_STAT_DRAW_CALL(verticesCount * instanceCount, verticesCount * instanceCount / 3);
}

void GPUContextDX12::DrawIndexedInstanced(uint32 indicesCount, uint32 instanceCount, int32 startInstance, int32 startVertex, int32 startIndex)
{
    onDrawCall();
    _commandList->DrawIndexedInstanced(indicesCount, instanceCount, startIndex, startVertex, startInstance);
    RENDER_STAT_DRAW_CALL(0, indicesCount / 3 * instanceCount);
}

void GPUContextDX12::DrawInstancedIndirect(GPUBuffer* bufferForArgs, uint32 offsetForArgs)
{
    ASSERT(bufferForArgs && bufferForArgs->GetFlags() & GPUBufferFlags::Argument);

    auto bufferForArgsDX12 = (GPUBufferDX12*)bufferForArgs;
    auto signature = _device->DrawIndirectCommandSignature->GetSignature();
    SetResourceState(bufferForArgsDX12, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

    onDrawCall();
    _commandList->ExecuteIndirect(signature, 1, bufferForArgsDX12->GetResource(), (UINT64)offsetForArgs, nullptr, 0);
    RENDER_STAT_DRAW_CALL(0, 0);
}

void GPUContextDX12::DrawIndexedInstancedIndirect(GPUBuffer* bufferForArgs, uint32 offsetForArgs)
{
    ASSERT(bufferForArgs && bufferForArgs->GetFlags() & GPUBufferFlags::Argument);

    auto bufferForArgsDX12 = (GPUBufferDX12*)bufferForArgs;
    auto signature = _device->DrawIndexedIndirectCommandSignature->GetSignature();
    SetResourceState(bufferForArgsDX12, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

    onDrawCall();
    _commandList->ExecuteIndirect(signature, 1, bufferForArgsDX12->GetResource(), (UINT64)offsetForArgs, nullptr, 0);
    RENDER_STAT_DRAW_CALL(0, 0);
}

void GPUContextDX12::SetViewport(const Viewport& viewport)
{
    _commandList->RSSetViewports(1, (D3D12_VIEWPORT*)&viewport);
}

void GPUContextDX12::SetScissor(const Rectangle& scissorRect)
{
    D3D12_RECT rect;
    rect.left = (LONG)scissorRect.GetLeft();
    rect.right = (LONG)scissorRect.GetRight();
    rect.top = (LONG)scissorRect.GetTop();
    rect.bottom = (LONG)scissorRect.GetBottom();
    _commandList->RSSetScissorRects(1, &rect);
}

GPUPipelineState* GPUContextDX12::GetState() const
{
    return _currentState;
}

void GPUContextDX12::SetState(GPUPipelineState* state)
{
    if (_currentState != state)
    {
        _currentState = static_cast<GPUPipelineStateDX12*>(state);
        _psDirtyFlag = true;
    }
}

void GPUContextDX12::ClearState()
{
    if (!_commandList)
        return;

    ResetRenderTarget();
    ResetSR();
    ResetUA();
    ResetCB();
    SetState(nullptr);

    FlushState();

    //_commandList->ClearState(nullptr);
}

void GPUContextDX12::FlushState()
{
    // Flush
    flushCBs();
    flushSRVs();
    flushRTVs();
    flushUAVs();
    flushRBs();
    //flushPS();
}

void GPUContextDX12::Flush()
{
    if (!_currentAllocator)
        return;

    // Flush GPU commands
    Execute();
    Reset();
}

void GPUContextDX12::UpdateBuffer(GPUBuffer* buffer, const void* data, uint32 size, uint32 offset)
{
    ASSERT(data);
    ASSERT(buffer && buffer->GetSize() >= size);

    auto bufferDX12 = (GPUBufferDX12*)buffer;

    SetResourceState(bufferDX12, D3D12_RESOURCE_STATE_COPY_DEST);
    flushRBs();

    _device->UploadBuffer->UploadBuffer(this, bufferDX12->GetResource(), offset, data, size);
}

void GPUContextDX12::CopyBuffer(GPUBuffer* dstBuffer, GPUBuffer* srcBuffer, uint32 size, uint32 dstOffset, uint32 srcOffset)
{
    ASSERT(dstBuffer && srcBuffer);

    auto dstBufferDX12 = static_cast<GPUBufferDX12*>(dstBuffer);
    auto srcBufferDX12 = static_cast<GPUBufferDX12*>(srcBuffer);

    SetResourceState(dstBufferDX12, D3D12_RESOURCE_STATE_COPY_DEST);
    SetResourceState(srcBufferDX12, D3D12_RESOURCE_STATE_COPY_SOURCE);
    flushRBs();

    _commandList->CopyBufferRegion(dstBufferDX12->GetResource(), dstOffset, srcBufferDX12->GetResource(), srcOffset, size);
}

void GPUContextDX12::UpdateTexture(GPUTexture* texture, int32 arrayIndex, int32 mipIndex, const void* data, uint32 rowPitch, uint32 slicePitch)
{
    ASSERT(texture && texture->IsAllocated() && data);

    auto textureDX12 = static_cast<GPUTextureDX12*>(texture);

    SetResourceState(textureDX12, D3D12_RESOURCE_STATE_COPY_DEST);
    flushRBs();

    _device->UploadBuffer->UploadTexture(this, textureDX12->GetResource(), data, rowPitch, slicePitch, mipIndex, arrayIndex);
}

void GPUContextDX12::CopyTexture(GPUTexture* dstResource, uint32 dstSubresource, uint32 dstX, uint32 dstY, uint32 dstZ, GPUTexture* srcResource, uint32 srcSubresource)
{
    auto dstTextureDX12 = (GPUTextureDX12*)dstResource;
    auto srcTextureDX12 = (GPUTextureDX12*)srcResource;

    SetResourceState(dstTextureDX12, D3D12_RESOURCE_STATE_COPY_DEST);
    SetResourceState(srcTextureDX12, D3D12_RESOURCE_STATE_COPY_SOURCE);
    flushRBs();

    // Get destination copy location
    D3D12_TEXTURE_COPY_LOCATION dst;
    dst.pResource = dstTextureDX12->GetResource();
    dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst.SubresourceIndex = dstSubresource;

    // Get source copy location
    D3D12_TEXTURE_COPY_LOCATION src;
    src.pResource = srcTextureDX12->GetResource();
    src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    src.SubresourceIndex = srcSubresource;

    // Copy
    _commandList->CopyTextureRegion(&dst, dstX, dstY, dstZ, &src, nullptr);
}

void GPUContextDX12::ResetCounter(GPUBuffer* buffer)
{
    ASSERT(buffer);

    auto bufferDX12 = static_cast<GPUBufferDX12*>(buffer);
    auto counter = bufferDX12->GetCounter();
    ASSERT(counter != nullptr);

    SetResourceState(counter, D3D12_RESOURCE_STATE_COPY_DEST);
    flushRBs();

    uint32 value = 0;
    _device->UploadBuffer->UploadBuffer(this, counter->GetResource(), 0, &value, 4);

    SetResourceState(counter, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
}

void GPUContextDX12::CopyCounter(GPUBuffer* dstBuffer, uint32 dstOffset, GPUBuffer* srcBuffer)
{
    ASSERT(dstBuffer && srcBuffer);

    auto dstBufferDX12 = static_cast<GPUBufferDX12*>(dstBuffer);
    auto srcBufferDX12 = static_cast<GPUBufferDX12*>(srcBuffer);
    auto counter = srcBufferDX12->GetCounter();
    ASSERT(counter != nullptr);

    SetResourceState(dstBufferDX12, D3D12_RESOURCE_STATE_COPY_DEST);
    SetResourceState(counter, D3D12_RESOURCE_STATE_COPY_SOURCE);
    flushRBs();

    _commandList->CopyBufferRegion(dstBufferDX12->GetResource(), dstOffset, counter->GetResource(), 0, 4);
}

void GPUContextDX12::CopyResource(GPUResource* dstResource, GPUResource* srcResource)
{
    ASSERT(dstResource && srcResource);

    auto dstResourceDX12 = dynamic_cast<ResourceOwnerDX12*>(dstResource);
    auto srcResourceDX12 = dynamic_cast<ResourceOwnerDX12*>(srcResource);

    SetResourceState(dstResourceDX12, D3D12_RESOURCE_STATE_COPY_DEST);
    SetResourceState(srcResourceDX12, D3D12_RESOURCE_STATE_COPY_SOURCE);
    flushRBs();

    auto srcType = srcResource->GetObjectType();
    auto dstType = dstResource->GetObjectType();

    // Buffer -> Buffer
    if (srcType == GPUResource::ObjectType::Buffer && dstType == GPUResource::ObjectType::Buffer)
    {
        _commandList->CopyResource(dstResourceDX12->GetResource(), srcResourceDX12->GetResource());
    }
        // Texture -> Texture
    else if (srcType == GPUResource::ObjectType::Texture && dstType == GPUResource::ObjectType::Texture)
    {
        auto dstTextureDX12 = static_cast<GPUTextureDX12*>(dstResource);
        auto srcTextureDX12 = static_cast<GPUTextureDX12*>(srcResource);

        if (dstTextureDX12->IsStaging())
        {
            // Staging Texture -> Staging Texture
            if (srcTextureDX12->IsStaging())
            {
                const int32 size = dstTextureDX12->ComputeBufferTotalSize(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

                // Map the staging resources
                D3D12_RANGE range;
                range.Begin = 0;
                range.End = size;
                void* srcMapped;
                HRESULT mapResult = srcTextureDX12->GetResource()->Map(0, &range, &srcMapped);
                LOG_DIRECTX_RESULT(mapResult);
                void* dstMapped;
                mapResult = dstTextureDX12->GetResource()->Map(0, &range, &dstMapped);
                LOG_DIRECTX_RESULT(mapResult);

                // Copy data
                Platform::MemoryCopy(dstMapped, srcMapped, size);

                // Unmap buffers
                srcTextureDX12->GetResource()->Unmap(0, nullptr);
                dstTextureDX12->GetResource()->Unmap(0, nullptr);
            }
            else
            {
                for (int32 arraySlice = 0; arraySlice < srcTextureDX12->ArraySize(); arraySlice++)
                {
                    for (int32 mipLevel = 0; mipLevel < srcTextureDX12->MipLevels(); mipLevel++)
                    {
                        const int32 subresource = RenderToolsDX::CalcSubresourceIndex(mipLevel, arraySlice, srcTextureDX12->MipLevels());
                        const int32 copyOffset = dstTextureDX12->ComputeBufferOffset(subresource, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

                        D3D12_TEXTURE_COPY_LOCATION dstLocation;
                        dstLocation.pResource = dstTextureDX12->GetResource();
                        dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
                        dstLocation.PlacedFootprint.Offset = copyOffset;
                        dstLocation.PlacedFootprint.Footprint.Width = dstTextureDX12->CalculateMipSize(dstTextureDX12->Width(), mipLevel);
                        dstLocation.PlacedFootprint.Footprint.Height = dstTextureDX12->CalculateMipSize(dstTextureDX12->Height(), mipLevel);
                        dstLocation.PlacedFootprint.Footprint.Depth = dstTextureDX12->CalculateMipSize(dstTextureDX12->Depth(), mipLevel);
                        dstLocation.PlacedFootprint.Footprint.Format = RenderToolsDX::ToDxgiFormat(dstTextureDX12->Format());
                        dstLocation.PlacedFootprint.Footprint.RowPitch = dstTextureDX12->ComputeRowPitch(mipLevel, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

                        D3D12_TEXTURE_COPY_LOCATION srcLocation;
                        srcLocation.pResource = srcTextureDX12->GetResource();
                        srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                        srcLocation.SubresourceIndex = subresource;

                        _commandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
                    }
                }
            }
        }
        else
        {
            _commandList->CopyResource(dstResourceDX12->GetResource(), srcResourceDX12->GetResource());
        }
    }
    else
    {
        // Not implemented
        CRASH;
    }
}

void GPUContextDX12::CopySubresource(GPUResource* dstResource, uint32 dstSubresource, GPUResource* srcResource, uint32 srcSubresource)
{
    ASSERT(dstResource && srcResource);

    auto srcType = srcResource->GetObjectType();
    auto dstType = dstResource->GetObjectType();

    // Buffer -> Buffer
    if (srcType == GPUResource::ObjectType::Buffer && dstType == GPUResource::ObjectType::Buffer)
    {
        auto dstBufferDX12 = dynamic_cast<ResourceOwnerDX12*>(dstResource);
        auto srcBufferDX12 = dynamic_cast<ResourceOwnerDX12*>(srcResource);

        SetResourceState(dstBufferDX12, D3D12_RESOURCE_STATE_COPY_DEST);
        SetResourceState(srcBufferDX12, D3D12_RESOURCE_STATE_COPY_SOURCE);
        flushRBs();

        uint64 bytesCount = srcResource->GetMemoryUsage();
        _commandList->CopyBufferRegion(dstBufferDX12->GetResource(), 0, srcBufferDX12->GetResource(), 0, bytesCount);
    }
        // Texture -> Texture
    else if (srcType == GPUResource::ObjectType::Texture && dstType == GPUResource::ObjectType::Texture)
    {
        auto dstTextureDX12 = static_cast<GPUTextureDX12*>(dstResource);
        auto srcTextureDX12 = static_cast<GPUTextureDX12*>(srcResource);

        SetResourceState(dstTextureDX12, D3D12_RESOURCE_STATE_COPY_DEST);
        SetResourceState(srcTextureDX12, D3D12_RESOURCE_STATE_COPY_SOURCE);
        flushRBs();

        if (srcTextureDX12->IsStaging() || dstTextureDX12->IsStaging())
        {
            Log::NotImplementedException(TEXT("Copy region of staging resources is not supported yet."));
        }

        // Create copy locations structures
        D3D12_TEXTURE_COPY_LOCATION dstLocation;
        dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLocation.pResource = dstTextureDX12->GetResource();
        dstLocation.SubresourceIndex = dstSubresource;
        D3D12_TEXTURE_COPY_LOCATION srcLocation;
        srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        srcLocation.pResource = srcTextureDX12->GetResource();
        srcLocation.SubresourceIndex = srcSubresource;

        _commandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
    }
    else
    {
        Log::NotImplementedException(TEXT("Cannot copy data between buffer and texture."));
    }
}

#endif
