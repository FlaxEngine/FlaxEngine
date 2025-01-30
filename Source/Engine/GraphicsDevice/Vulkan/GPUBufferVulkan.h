// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_VULKAN

#include "Engine/Graphics/GPUBuffer.h"
#include "GPUDeviceVulkan.h"

/// <summary>
/// The buffer view for Vulkan backend.
/// </summary>
class GPUBufferViewVulkan : public GPUBufferView, public DescriptorOwnerResourceVulkan
{
public:
    GPUBufferViewVulkan()
    {
    }

#if BUILD_DEBUG
    ~GPUBufferViewVulkan()
    {
        ASSERT(View == VK_NULL_HANDLE);
    }
#endif

public:
    GPUDeviceVulkan* Device = nullptr;
    GPUBufferVulkan* Owner = nullptr;
    VkBuffer Buffer = VK_NULL_HANDLE;
    VkBufferView View = VK_NULL_HANDLE;
    VkDeviceSize Size = 0;

public:
    void Init(GPUDeviceVulkan* device, GPUBufferVulkan* owner, VkBuffer buffer, VkDeviceSize size, VkBufferUsageFlags usage, PixelFormat format);
    void Release();

public:
    // [GPUResourceView]
    void* GetNativePtr() const override
    {
        return (void*)(DescriptorOwnerResourceVulkan*)this;
    }

    // [DescriptorOwnerResourceVulkan]
    void DescriptorAsUniformTexelBuffer(GPUContextVulkan* context, VkBufferView& bufferView) override;
    void DescriptorAsStorageBuffer(GPUContextVulkan* context, VkBuffer& buffer, VkDeviceSize& offset, VkDeviceSize& range) override;
    void DescriptorAsStorageTexelBuffer(GPUContextVulkan* context, VkBufferView& bufferView) override;
#if !BUILD_RELEASE
    bool HasSRV() const override { return ((GPUBuffer*)_parent)->IsShaderResource(); }
    bool HasUAV() const override { return ((GPUBuffer*)_parent)->IsUnorderedAccess(); }
#endif
};

/// <summary>
/// GPU buffer for Vulkan backend.
/// </summary>
class GPUBufferVulkan : public GPUResourceVulkan<GPUBuffer>
{
private:
    VkBuffer _buffer = VK_NULL_HANDLE;
    VmaAllocation _allocation = VK_NULL_HANDLE;
    GPUBufferViewVulkan _view;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="GPUBufferVulkan"/> class.
    /// </summary>
    /// <param name="device">The device.</param>
    /// <param name="name">The name.</param>
    GPUBufferVulkan(GPUDeviceVulkan* device, const StringView& name)
        : GPUResourceVulkan<GPUBuffer>(device, name)
    {
    }

public:
    /// <summary>
    /// Gets the Vulkan buffer handle.
    /// </summary>
    FORCE_INLINE VkBuffer GetHandle() const
    {
        return _buffer;
    }

    /// <summary>
    /// Gets the Vulkan memory allocation handle.
    /// </summary>
    FORCE_INLINE VmaAllocation GetAllocation() const
    {
        return _allocation;
    }

    /// <summary>
    /// The current buffer access flags.
    /// </summary>
    VkAccessFlags Access;

    /// <summary>
    /// The counter buffer attached to the Append/Counter buffers.
    /// </summary>
    GPUBufferVulkan* Counter = nullptr;

public:
    // [GPUBuffer]
    GPUBufferView* View() const override;
    void* Map(GPUResourceMapMode mode) override;
    void Unmap() override;

protected:
    // [GPUBuffer]
    bool OnInit() override;
    void OnReleaseGPU() override;
};

#endif
