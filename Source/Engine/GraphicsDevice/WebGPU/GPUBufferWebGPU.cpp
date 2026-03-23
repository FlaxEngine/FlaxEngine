// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_WEBGPU

#include "GPUBufferWebGPU.h"
#include "RenderToolsWebGPU.h"
#include "Engine/Core/Log.h"

GPUBufferWebGPU::GPUBufferWebGPU(GPUDeviceWebGPU* device, const StringView& name)
    : GPUResourceWebGPU(device, name)
{
}

GPUBufferView* GPUBufferWebGPU::View() const
{
    return (GPUBufferView*)&_view;
}

void* GPUBufferWebGPU::Map(GPUResourceMapMode mode)
{
    ASSERT(!_mapped);
    WGPUMapMode mapMode = 0;
    if (EnumHasAnyFlags(mode, GPUResourceMapMode::Read))
        mapMode |= WGPUMapMode_Read;
    if (EnumHasAnyFlags(mode, GPUResourceMapMode::Write))
        mapMode |= WGPUMapMode_Write;
    AsyncCallbackWebGPU<WGPUBufferMapCallbackInfo> mapRequest(WGPU_BUFFER_MAP_CALLBACK_INFO_INIT);
    mapRequest.Info.callback = [](WGPUMapAsyncStatus status, WGPUStringView message, WGPU_NULLABLE void* userdata1, WGPU_NULLABLE void* userdata2)
    {
        AsyncCallbackDataWebGPU& userData = *reinterpret_cast<AsyncCallbackDataWebGPU*>(userdata1);
        userData.Call(status == WGPUMapAsyncStatus_Success, status, message);
    };
    wgpuBufferMapAsync(Buffer, mapMode, 0, _desc.Size, mapRequest.Info);
    auto mapRequestResult = mapRequest.Wait(_device->WebGPUInstance);
    if (mapRequestResult == WGPUWaitStatus_TimedOut)
    {
        LOG(Error, "WebGPU buffer map request has timed out after {}s", (int32)mapRequest.Data.WaitTime);
        return nullptr;
    }
    if (mapRequestResult == WGPUWaitStatus_Error)
        return nullptr;
    _mapped = true;
    if (EnumHasNoneFlags(mode, GPUResourceMapMode::Write))
        return (void*)wgpuBufferGetConstMappedRange(Buffer, 0, _desc.Size);
    return wgpuBufferGetMappedRange(Buffer, 0, _desc.Size);
}

void GPUBufferWebGPU::Unmap()
{
    ASSERT(_mapped);
    _mapped = false;
    wgpuBufferUnmap(Buffer);
}

bool GPUBufferWebGPU::OnInit()
{
    // Create buffer
    WGPUBufferDescriptor bufferDesc = WGPU_BUFFER_DESCRIPTOR_INIT;
#if GPU_ENABLE_RESOURCE_NAMING
    _name.Set(_namePtr, _nameSize);
    bufferDesc.label = { _name.Get(), (size_t)_name.Length() };
#endif
    if (EnumHasAllFlags(_desc.Flags, GPUBufferFlags::IndexBuffer))
        bufferDesc.usage |= WGPUBufferUsage_Index;
    else if (EnumHasAllFlags(_desc.Flags, GPUBufferFlags::VertexBuffer))
        bufferDesc.usage |= WGPUBufferUsage_Vertex;
    else if (EnumHasAllFlags(_desc.Flags, GPUBufferFlags::Argument))
        bufferDesc.usage |= WGPUBufferUsage_Indirect;
    if (IsUnorderedAccess() || IsShaderResource()) // SRV buffers need to be bind as read-only storage
        bufferDesc.usage |= WGPUBufferUsage_Storage;
    switch (_desc.Usage)
    {
    case GPUResourceUsage::Default:
        bufferDesc.usage |= WGPUBufferUsage_CopyDst;
        if (IsUnorderedAccess())
            bufferDesc.usage |= WGPUBufferUsage_CopySrc; // eg. GPU particles copy particle counter between buffers
        break;
    case GPUResourceUsage::Dynamic:
        if (bufferDesc.usage == 0) // WebGPU doesn't allow to map-write Index/Vertex/Storage buffers
            bufferDesc.usage = WGPUBufferUsage_MapWrite;
        else
            bufferDesc.usage |= WGPUBufferUsage_CopyDst;
        break;
    case GPUResourceUsage::StagingUpload:
        bufferDesc.usage |= WGPUBufferUsage_MapWrite | WGPUBufferUsage_CopySrc;
        break;
    case GPUResourceUsage::StagingReadback:
        bufferDesc.usage |= WGPUBufferUsage_MapRead;
        break;
    case GPUResourceUsage::Staging:
        bufferDesc.usage |= WGPUBufferUsage_MapRead | WGPUBufferUsage_MapWrite | WGPUBufferUsage_CopySrc;
        break;
    }
    bufferDesc.size = (_desc.Size + 3) & ~0x3; // Align up to the multiple of 4 bytes
    bufferDesc.mappedAtCreation = _desc.InitData != nullptr && (bufferDesc.usage & WGPUBufferUsage_MapWrite);
    Buffer = wgpuDeviceCreateBuffer(_device->Device, &bufferDesc);
    if (!Buffer)
        return true;
    _memoryUsage = bufferDesc.size;
    Usage = bufferDesc.usage;
    _view.Ptr.ObjectVersion = _device->GetObjectVersion();

    // Initialize with a data if provided
    if (bufferDesc.mappedAtCreation)
    {
        //wgpuBufferWriteMappedRange(Buffer, 0, _desc.InitData, _desc.Size);
        Platform::MemoryCopy(wgpuBufferGetMappedRange(Buffer, 0, _desc.Size), _desc.InitData, _desc.Size);
        wgpuBufferUnmap(Buffer);
    }
    else if (_desc.InitData)
    {
        wgpuQueueWriteBuffer(_device->Queue, Buffer, 0, _desc.InitData, _desc.Size);
    }

    // Create view
    _view.Set(this, Buffer);

    return false;
}

void GPUBufferWebGPU::OnReleaseGPU()
{
    if (Buffer)
    {
        wgpuBufferDestroy(Buffer);
        wgpuBufferRelease(Buffer);
        Buffer = nullptr;
    }
#if GPU_ENABLE_RESOURCE_NAMING
    _name.Clear();
#endif
    _view.Ptr.ViewVersion++;

    // Base
    GPUBuffer::OnReleaseGPU();
}

#endif
