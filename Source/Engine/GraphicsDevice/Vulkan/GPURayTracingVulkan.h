// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_VULKAN

#include "Engine/Graphics/GPUAccelerationStructure.h"
#include "Engine/Graphics/GPUResource.h"
#include "GPUDeviceVulkan.h"
#include "IncludeVulkanHeaders.h"

class GPUContextVulkan;

#if defined(VK_KHR_acceleration_structure) && defined(VK_KHR_buffer_device_address)
#define GPU_VK_ALLOW_RAYTRACING 1
#else
#define GPU_VK_ALLOW_RAYTRACING 0
#endif

#if GPU_VK_ALLOW_RAYTRACING

/// <summary>
/// Top-level acceleration structure view for Vulkan (VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR).
/// </summary>
class GPUAccelerationStructureViewVulkan : public GPUResourceView, public DescriptorOwnerResourceVulkan
{
private:
    GPUDeviceVulkan* _device = nullptr;
    VkAccelerationStructureKHR _handle = VK_NULL_HANDLE;

public:
    GPUAccelerationStructureViewVulkan();

    void Init(GPUDeviceVulkan* device, VkAccelerationStructureKHR handle);
    void Release();

public:
    // [GPUResourceView]
    void* GetNativePtr() const override
    {
        return (void*)(DescriptorOwnerResourceVulkan*)this;
    }

    // [DescriptorOwnerResourceVulkan]
    void DescriptorAsAccelerationStructure(GPUContextVulkan* context, VkAccelerationStructureKHR& handle) override;
#if !BUILD_RELEASE
    bool HasSRV() const override { return true; }
#endif
};

/// <summary>
/// Vulkan hardware ray tracing acceleration structure (bottom-level + single-instance top-level).
/// </summary>
class GPUAccelerationStructureVulkan : public GPUAccelerationStructure
{
private:
    GPUDeviceVulkan* _device;
    GPUAccelerationStructureGeometry _geometry;
    bool _built = false;

    VkBuffer _blasBuffer = VK_NULL_HANDLE;
    VmaAllocation _blasAllocation = VK_NULL_HANDLE;
    VkAccelerationStructureKHR _blas = VK_NULL_HANDLE;
    VkDeviceSize _blasSize = 0;

    VkBuffer _tlasBuffer = VK_NULL_HANDLE;
    VmaAllocation _tlasAllocation = VK_NULL_HANDLE;
    VkAccelerationStructureKHR _tlas = VK_NULL_HANDLE;
    VkDeviceSize _tlasSize = 0;

    GPUAccelerationStructureViewVulkan _view;

    GPUBuffer* _vbGeom = nullptr;
    GPUBuffer* _ibGeom = nullptr;

public:
    explicit GPUAccelerationStructureVulkan(GPUDeviceVulkan* device)
        : _device(device)
    {
    }

    ~GPUAccelerationStructureVulkan()
    {
        ReleaseGPUResources();
    }

    void Build(GPUContextVulkan* context);

public:
    // [GPUAccelerationStructure]
    void SetGeometry(const GPUAccelerationStructureGeometry& geometry) override
    {
        _geometry = geometry;
        _built = false;
    }
    bool IsBuilt() const override
    {
        return _built;
    }
    GPUResourceView* GetView() const override
    {
        return _built ? (GPUResourceView*)&_view : nullptr;
    }
    void DeleteObjectNow() override;

private:
    VkDeviceAddress GetBufferDeviceAddress(VkBuffer buffer) const;
    VkBuffer CreateRtBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocation* allocation, VkDeviceSize* outSize = nullptr, bool hostVisible = false);
    void DestroyRtBuffer(VkBuffer& buffer, VmaAllocation& allocation, VkAccelerationStructureKHR& as);
    void ReleaseGPUResources();
};

#endif

#endif
