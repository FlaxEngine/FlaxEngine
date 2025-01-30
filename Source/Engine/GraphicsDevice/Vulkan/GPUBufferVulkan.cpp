// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if GRAPHICS_API_VULKAN

#include "GPUBufferVulkan.h"
#include "GPUContextVulkan.h"
#include "RenderToolsVulkan.h"
#include "Engine/Core/Log.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Graphics/Async/Tasks/GPUUploadBufferTask.h"

void GPUBufferViewVulkan::Init(GPUDeviceVulkan* device, GPUBufferVulkan* owner, VkBuffer buffer, VkDeviceSize size, VkBufferUsageFlags usage, PixelFormat format)
{
    ASSERT(View == VK_NULL_HANDLE);

    _parent = owner;
    Device = device;
    Owner = owner;
    Buffer = buffer;
    Size = size;

    if ((owner->IsShaderResource() && !(owner->GetDescription().Flags & GPUBufferFlags::Structured)) || (usage & VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT) == VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT)
    {
        VkBufferViewCreateInfo viewInfo;
        RenderToolsVulkan::ZeroStruct(viewInfo, VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO);
        viewInfo.buffer = Buffer;
        viewInfo.format = RenderToolsVulkan::ToVulkanFormat(format);
        viewInfo.offset = 0;
        viewInfo.range = Size;
        if (viewInfo.format == VK_FORMAT_UNDEFINED)
            return; // Skip for structured buffers that use custom structure type and have unknown format
        VALIDATE_VULKAN_RESULT(vkCreateBufferView(device->Device, &viewInfo, nullptr, &View));
    }
}

void GPUBufferViewVulkan::Release()
{
    if (View != VK_NULL_HANDLE)
    {
        Device->DeferredDeletionQueue.EnqueueResource(DeferredDeletionQueueVulkan::BufferView, View);
        View = VK_NULL_HANDLE;
    }
#if BUILD_DEBUG
    Device = nullptr;
    Owner = nullptr;
    Buffer = VK_NULL_HANDLE;
#endif
}

void GPUBufferViewVulkan::DescriptorAsUniformTexelBuffer(GPUContextVulkan* context, VkBufferView& bufferView)
{
    ASSERT_LOW_LAYER(View != VK_NULL_HANDLE);
    bufferView = View;
    context->AddBufferBarrier(Owner, VK_ACCESS_SHADER_READ_BIT);
}

void GPUBufferViewVulkan::DescriptorAsStorageBuffer(GPUContextVulkan* context, VkBuffer& buffer, VkDeviceSize& offset, VkDeviceSize& range)
{
    ASSERT_LOW_LAYER(Buffer);
    buffer = Buffer;
    offset = 0;
    range = Size;
    context->AddBufferBarrier(Owner, VK_ACCESS_SHADER_READ_BIT);
}

void GPUBufferViewVulkan::DescriptorAsStorageTexelBuffer(GPUContextVulkan* context, VkBufferView& bufferView)
{
    ASSERT_LOW_LAYER(View != VK_NULL_HANDLE);
    bufferView = View;
    context->AddBufferBarrier(Owner, VK_ACCESS_SHADER_READ_BIT);
}

GPUBufferView* GPUBufferVulkan::View() const
{
    return (GPUBufferView*)&_view;
}

void* GPUBufferVulkan::Map(GPUResourceMapMode mode)
{
    void* mapped = nullptr;
    const VkResult result = vmaMapMemory(_device->Allocator, _allocation, (void**)&mapped);
    LOG_VULKAN_RESULT(result);
    return mapped;
}

void GPUBufferVulkan::Unmap()
{
    vmaUnmapMemory(_device->Allocator, _allocation);
}

bool GPUBufferVulkan::OnInit()
{
    const bool useSRV = IsShaderResource();
    const bool useUAV = IsUnorderedAccess();

    // Setup buffer description
    VkBufferCreateInfo bufferInfo;
    RenderToolsVulkan::ZeroStruct(bufferInfo, VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO);
    bufferInfo.size = _desc.Size;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if (useSRV && !(_desc.Flags & GPUBufferFlags::Structured))
        bufferInfo.usage |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
    if (useUAV || EnumHasAnyFlags(_desc.Flags, GPUBufferFlags::RawBuffer | GPUBufferFlags::Structured))
        bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    if (useUAV && useSRV)
        bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
    if (EnumHasAnyFlags(_desc.Flags, GPUBufferFlags::Argument))
        bufferInfo.usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    if (EnumHasAnyFlags(_desc.Flags, GPUBufferFlags::Argument) && useUAV)
        bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT; // For some reason, glslang marks indirect uav buffers (UpdateProbesInitArgs, IndirectArgsBuffer) as Storage Texel Buffers
    if (EnumHasAnyFlags(_desc.Flags, GPUBufferFlags::VertexBuffer))
        bufferInfo.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (EnumHasAnyFlags(_desc.Flags, GPUBufferFlags::IndexBuffer))
        bufferInfo.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (IsStaging() || EnumHasAnyFlags(_desc.Flags, GPUBufferFlags::UnorderedAccess))
        bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    // Create buffer
    VmaAllocationCreateInfo allocInfo = {};
    switch (_desc.Usage)
    {
    case GPUResourceUsage::Dynamic:
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        break;
    case GPUResourceUsage::StagingUpload:
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        break;
    case GPUResourceUsage::StagingReadback:
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
        break;
    case GPUResourceUsage::Staging:
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_COPY;
        break;
    default:
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    }
    const VkResult result = vmaCreateBuffer(_device->Allocator, &bufferInfo, &allocInfo, &_buffer, &_allocation, nullptr);
    LOG_VULKAN_RESULT_WITH_RETURN(result);
#if GPU_ENABLE_RESOURCE_NAMING
    VK_SET_DEBUG_NAME(_device, _buffer, VK_OBJECT_TYPE_BUFFER, GetName());
#endif
    _memoryUsage = _desc.Size;
    Access = 0;

    // Check if set initial data
    if (_desc.InitData)
    {
        if (IsDynamic() || IsStaging())
        {
            // Faster path using Map/Unmap sequence
            SetData(_desc.InitData, _desc.Size);
        }
        else if (_device->IsRendering() && IsInMainThread())
        {
            // Upload resource data now
            _device->GetMainContext()->UpdateBuffer(this, _desc.InitData, _desc.Size);
        }
        else
        {
            // Create async resource copy task
            auto copyTask = ::New<GPUUploadBufferTask>(this, 0, Span<byte>((const byte*)_desc.InitData, _desc.Size), true);
            ASSERT(copyTask->HasReference(this));
            copyTask->Start();
        }
    }

    // Check if need to use a counter
    if (EnumHasAnyFlags(_desc.Flags, GPUBufferFlags::Counter | GPUBufferFlags::Append))
    {
#if GPU_ENABLE_RESOURCE_NAMING
        String name = String(GetName()) + TEXT(".Counter");
        Counter = ::New<GPUBufferVulkan>(_device, name);
#else
		Counter = ::New<GPUBufferVulkan>(_device, StringView::Empty);
#endif
        if (Counter->Init(GPUBufferDescription::Raw(4, GPUBufferFlags::UnorderedAccess)))
        {
            LOG(Error, "Cannot create counter buffer.");
            return true;
        }
    }
    // Check if need to bind buffer to the shaders
    else if (useSRV || useUAV)
    {
        // Create buffer view
        _view.Init(_device, this, _buffer, _desc.Size, bufferInfo.usage, _desc.Format);
    }

    return false;
}

void GPUBufferVulkan::OnReleaseGPU()
{
    _view.Release();
    SAFE_DELETE_GPU_RESOURCE(Counter);
    if (_allocation != VK_NULL_HANDLE)
    {
        _device->DeferredDeletionQueue.EnqueueResource(DeferredDeletionQueueVulkan::Buffer, _buffer, _allocation);
        _buffer = VK_NULL_HANDLE;
        _allocation = VK_NULL_HANDLE;
    }

    // Base
    GPUBuffer::OnReleaseGPU();
}

#endif
