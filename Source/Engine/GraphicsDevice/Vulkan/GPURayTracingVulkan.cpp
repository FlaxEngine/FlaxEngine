// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_VULKAN

#include "GPURayTracingVulkan.h"

#if GPU_VK_ALLOW_RAYTRACING

#include "GPUContextVulkan.h"
#include "GPUBufferVulkan.h"
#include "CmdBufferVulkan.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Memory/Memory.h"
#include "Engine/Graphics/GPUBufferDescription.h"
#include "Engine/Graphics/PixelFormatExtensions.h"

GPUAccelerationStructureViewVulkan::GPUAccelerationStructureViewVulkan()
    : GPUResourceView(SpawnParams(Guid::New(), GPUResourceView::TypeInitializer))
{
}

void GPUAccelerationStructureViewVulkan::Init(GPUDeviceVulkan* device, VkAccelerationStructureKHR handle)
{
    _device = device;
    _handle = handle;
}

void GPUAccelerationStructureViewVulkan::Release()
{
    _handle = VK_NULL_HANDLE;
}

void GPUAccelerationStructureViewVulkan::DescriptorAsAccelerationStructure(GPUContextVulkan* context, VkAccelerationStructureKHR& handle)
{
    handle = _handle;
}

VkDeviceAddress GPUAccelerationStructureVulkan::GetBufferDeviceAddress(VkBuffer buffer) const
{
    VkBufferDeviceAddressInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    info.buffer = buffer;
    return vkGetBufferDeviceAddressKHR(_device->Device, &info);
}

VkBuffer GPUAccelerationStructureVulkan::CreateRtBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocation* allocation, VkDeviceSize* outSize, bool hostVisible)
{
    if (size == 0)
        return VK_NULL_HANDLE;

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    // Host-visible buffers (e.g. the TLAS instance upload buffer) must be mappable; device-only buffers
    // (acceleration structure storage, scratch) cannot be mapped and are kept in GPU memory.
    allocInfo.usage = hostVisible ? VMA_MEMORY_USAGE_CPU_TO_GPU : VMA_MEMORY_USAGE_GPU_ONLY;
#if defined(VMA_ALLOCATION_CREATE_BUFFER_DEVICE_ADDRESS_BIT)
    allocInfo.flags = VMA_ALLOCATION_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
#endif

    VkBuffer buffer = VK_NULL_HANDLE;
    const VkResult result = vmaCreateBuffer(_device->Allocator, &bufferInfo, &allocInfo, &buffer, allocation, nullptr);
    if (result != VK_SUCCESS)
    {
        LOG(Error, "Failed to allocate Vulkan ray tracing buffer ({0} bytes).", (uint64)size);
        return VK_NULL_HANDLE;
    }
    if (outSize)
        *outSize = size;
    return buffer;
}

void GPUAccelerationStructureVulkan::DestroyRtBuffer(VkBuffer& buffer, VmaAllocation& allocation, VkAccelerationStructureKHR& as)
{
    if (as != VK_NULL_HANDLE)
    {
        vkDestroyAccelerationStructureKHR(_device->Device, as, nullptr);
        as = VK_NULL_HANDLE;
    }
    if (buffer != VK_NULL_HANDLE)
    {
        _device->DeferredDeletionQueue.EnqueueResource(DeferredDeletionQueueVulkan::Buffer, buffer, allocation);
        buffer = VK_NULL_HANDLE;
        allocation = VK_NULL_HANDLE;
    }
}

void GPUAccelerationStructureVulkan::ReleaseGPUResources()
{
    _built = false;
    _view.Release();
    DestroyRtBuffer(_blasBuffer, _blasAllocation, _blas);
    DestroyRtBuffer(_tlasBuffer, _tlasAllocation, _tlas);
    _blasSize = 0;
    _tlasSize = 0;
    SAFE_DELETE_GPU_RESOURCE(_vbGeom);
    SAFE_DELETE_GPU_RESOURCE(_ibGeom);
}

void GPUAccelerationStructureVulkan::Build(GPUContextVulkan* context)
{
    if (!vkCreateAccelerationStructureKHR || !vkCmdBuildAccelerationStructuresKHR)
    {
        LOG(Warning, "Ray tracing is not available on this Vulkan device.");
        return;
    }
    if (_geometry.VertexBuffer == nullptr || _geometry.IndexBuffer == nullptr || _geometry.VertexCount == 0 || _geometry.IndexCount == 0)
    {
        LOG(Warning, "Cannot build acceleration structure without geometry.");
        return;
    }

    ReleaseGPUResources();

    const uint32 vbSize = _geometry.VertexBuffer->GetSize();
    const uint32 ibSize = _geometry.IndexBuffer->GetSize();
    const GPUBufferFlags geomFlags = GPUBufferFlags::ShaderResource | GPUBufferFlags::ShaderDeviceAddress;
    _vbGeom = _device->CreateBuffer(TEXT("RT.GeometryVB"));
    _ibGeom = _device->CreateBuffer(TEXT("RT.GeometryIB"));
    if (_vbGeom->Init(GPUBufferDescription::Raw(Math::AlignUp<uint32>(vbSize, 4), geomFlags)) ||
        _ibGeom->Init(GPUBufferDescription::Raw(Math::AlignUp<uint32>(ibSize, 4), geomFlags)))
    {
        LOG(Error, "Failed to create ray tracing geometry buffers.");
        return;
    }
    context->CopyBuffer(_vbGeom, _geometry.VertexBuffer, vbSize, 0, 0);
    context->CopyBuffer(_ibGeom, _geometry.IndexBuffer, ibSize, 0, 0);

    auto vbVulkan = (GPUBufferVulkan*)_vbGeom;
    auto ibVulkan = (GPUBufferVulkan*)_ibGeom;
    const VkBuffer vbHandle = vbVulkan->GetHandle();
    const VkBuffer ibHandle = ibVulkan->GetHandle();
    const VkDeviceAddress vbAddress = GetBufferDeviceAddress(vbHandle);
    const VkDeviceAddress ibAddress = GetBufferDeviceAddress(ibHandle);
    if (vbAddress == 0 || ibAddress == 0)
    {
        LOG(Error, "Failed to get device addresses for ray tracing geometry buffers.");
        return;
    }

    const uint32 ibStride = _geometry.IndexBuffer->GetStride() == 4 ? 4 : 2;
    const VkIndexType ibIndexType = ibStride == 4 ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;
    VkFormat vbFormat = VK_FORMAT_R32G32B32_SFLOAT;
    switch (_geometry.VertexFormat)
    {
    case PixelFormat::R32G32B32_Float:
        vbFormat = VK_FORMAT_R32G32B32_SFLOAT;
        break;
    case PixelFormat::R16G16B16A16_Float:
        vbFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
        break;
    case PixelFormat::R16G16B16A16_SNorm:
        vbFormat = VK_FORMAT_R16G16B16A16_SNORM;
        break;
    case PixelFormat::R16G16B16A16_UNorm:
        vbFormat = VK_FORMAT_R16G16B16A16_UNORM;
        break;
    default:
        LOG(Warning, "Ray tracing acceleration structure: unsupported vertex position format {0}, falling back to R32G32B32_Float.", (int32)_geometry.VertexFormat);
        break;
    }

    VkAccelerationStructureGeometryKHR geometry = {};
    geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    geometry.geometry.triangles.vertexFormat = vbFormat;
    geometry.geometry.triangles.vertexData.deviceAddress = vbAddress + _geometry.VertexBufferOffset;
    geometry.geometry.triangles.vertexStride = _geometry.VertexStride;
    geometry.geometry.triangles.maxVertex = _geometry.VertexCount - 1;
    geometry.geometry.triangles.indexType = ibIndexType;
    geometry.geometry.triangles.indexData.deviceAddress = ibAddress + _geometry.IndexBufferOffset;

    VkAccelerationStructureBuildGeometryInfoKHR blasBuildInfo = {};
    blasBuildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    blasBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    blasBuildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    blasBuildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    blasBuildInfo.geometryCount = 1;
    blasBuildInfo.pGeometries = &geometry;

    const uint32 triangleCount = _geometry.IndexCount / 3;

    VkAccelerationStructureBuildSizesInfoKHR blasSizes = {};
    blasSizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizesKHR(_device->Device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &blasBuildInfo, &triangleCount, &blasSizes);

    _blasBuffer = CreateRtBuffer(blasSizes.accelerationStructureSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, &_blasAllocation, &_blasSize);
    VmaAllocation blasScratchAllocation = nullptr;
    VkBuffer blasScratch = CreateRtBuffer(blasSizes.buildScratchSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, &blasScratchAllocation);
    if (_blasBuffer == VK_NULL_HANDLE || blasScratch == VK_NULL_HANDLE)
        return;

    VkAccelerationStructureCreateInfoKHR blasCreateInfo = {};
    blasCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    blasCreateInfo.buffer = _blasBuffer;
    blasCreateInfo.size = blasSizes.accelerationStructureSize;
    blasCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    if (vkCreateAccelerationStructureKHR(_device->Device, &blasCreateInfo, nullptr, &_blas) != VK_SUCCESS)
    {
        LOG(Error, "Failed to create bottom-level acceleration structure.");
        return;
    }

    blasBuildInfo.dstAccelerationStructure = _blas;
    blasBuildInfo.scratchData.deviceAddress = GetBufferDeviceAddress(blasScratch);

    VkAccelerationStructureBuildRangeInfoKHR blasRange = {};
    blasRange.primitiveCount = triangleCount;

    const VkAccelerationStructureBuildRangeInfoKHR* blasRanges = &blasRange;
    auto cmd = context->GetCmdBufferManager()->GetCmdBuffer()->GetHandle();
    vkCmdBuildAccelerationStructuresKHR(cmd, 1, &blasBuildInfo, &blasRanges);

    VkMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);

    VkAccelerationStructureDeviceAddressInfoKHR blasAddressInfo = {};
    blasAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    blasAddressInfo.accelerationStructure = _blas;
    const VkDeviceAddress blasDeviceAddress = vkGetAccelerationStructureDeviceAddressKHR(_device->Device, &blasAddressInfo);

    VkTransformMatrixKHR transform = {};
    const Matrix& m = _geometry.Transform;
    transform.matrix[0][0] = m.M11; transform.matrix[0][1] = m.M21; transform.matrix[0][2] = m.M31; transform.matrix[0][3] = m.M41;
    transform.matrix[1][0] = m.M12; transform.matrix[1][1] = m.M22; transform.matrix[1][2] = m.M32; transform.matrix[1][3] = m.M42;
    transform.matrix[2][0] = m.M13; transform.matrix[2][1] = m.M23; transform.matrix[2][2] = m.M33; transform.matrix[2][3] = m.M43;

    VkAccelerationStructureInstanceKHR instance = {};
    instance.transform = transform;
    instance.instanceCustomIndex = 0;
    instance.mask = 0xFF;
    instance.instanceShaderBindingTableRecordOffset = 0;
    instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    instance.accelerationStructureReference = blasDeviceAddress;

    VmaAllocation instanceAllocation = nullptr;
    VkBuffer instanceBuffer = CreateRtBuffer(sizeof(VkAccelerationStructureInstanceKHR), VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, &instanceAllocation, nullptr, true);
    if (instanceBuffer == VK_NULL_HANDLE)
        return;

    void* mapped = nullptr;
    if (vmaMapMemory(_device->Allocator, instanceAllocation, &mapped) != VK_SUCCESS || mapped == nullptr)
    {
        LOG(Error, "Failed to map ray tracing instance buffer.");
        _device->DeferredDeletionQueue.EnqueueResource(DeferredDeletionQueueVulkan::Buffer, instanceBuffer, instanceAllocation);
        return;
    }
    Platform::MemoryCopy(mapped, &instance, sizeof(instance));
    vmaUnmapMemory(_device->Allocator, instanceAllocation);

    VkAccelerationStructureGeometryKHR instanceGeometry = {};
    instanceGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    instanceGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    instanceGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    instanceGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
    instanceGeometry.geometry.instances.data.deviceAddress = GetBufferDeviceAddress(instanceBuffer);

    VkAccelerationStructureBuildGeometryInfoKHR tlasBuildInfo = {};
    tlasBuildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    tlasBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    tlasBuildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    tlasBuildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    tlasBuildInfo.geometryCount = 1;
    tlasBuildInfo.pGeometries = &instanceGeometry;

    VkAccelerationStructureBuildSizesInfoKHR tlasSizes = {};
    tlasSizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    const uint32 instanceCount = 1;
    vkGetAccelerationStructureBuildSizesKHR(_device->Device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &tlasBuildInfo, &instanceCount, &tlasSizes);

    _tlasBuffer = CreateRtBuffer(tlasSizes.accelerationStructureSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, &_tlasAllocation, &_tlasSize);
    VmaAllocation tlasScratchAllocation = nullptr;
    VkBuffer tlasScratch = CreateRtBuffer(tlasSizes.buildScratchSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, &tlasScratchAllocation);
    if (_tlasBuffer == VK_NULL_HANDLE || tlasScratch == VK_NULL_HANDLE)
        return;

    VkAccelerationStructureCreateInfoKHR tlasCreateInfo = {};
    tlasCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    tlasCreateInfo.buffer = _tlasBuffer;
    tlasCreateInfo.size = tlasSizes.accelerationStructureSize;
    tlasCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    if (vkCreateAccelerationStructureKHR(_device->Device, &tlasCreateInfo, nullptr, &_tlas) != VK_SUCCESS)
    {
        LOG(Error, "Failed to create top-level acceleration structure.");
        return;
    }

    tlasBuildInfo.dstAccelerationStructure = _tlas;
    tlasBuildInfo.scratchData.deviceAddress = GetBufferDeviceAddress(tlasScratch);

    VkAccelerationStructureBuildRangeInfoKHR tlasRange = {};
    tlasRange.primitiveCount = 1;
    const VkAccelerationStructureBuildRangeInfoKHR* tlasRanges = &tlasRange;
    vkCmdBuildAccelerationStructuresKHR(cmd, 1, &tlasBuildInfo, &tlasRanges);

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);

    _view.Init(_device, _tlas);

    _device->DeferredDeletionQueue.EnqueueResource(DeferredDeletionQueueVulkan::Buffer, blasScratch, blasScratchAllocation);
    _device->DeferredDeletionQueue.EnqueueResource(DeferredDeletionQueueVulkan::Buffer, tlasScratch, tlasScratchAllocation);
    _device->DeferredDeletionQueue.EnqueueResource(DeferredDeletionQueueVulkan::Buffer, instanceBuffer, instanceAllocation);

    _built = true;
}

void GPUAccelerationStructureVulkan::DeleteObjectNow()
{
    ReleaseGPUResources();
    Delete(this);
}

GPUAccelerationStructure* GPUDeviceVulkan::CreateAccelerationStructure(const StringView& name)
{
    if (!Limits.HasRayTracing)
        return nullptr;
    return New<GPUAccelerationStructureVulkan>(this);
}

void GPUContextVulkan::BuildAccelerationStructure(GPUAccelerationStructure* accelerationStructure)
{
    if (accelerationStructure)
        ((GPUAccelerationStructureVulkan*)accelerationStructure)->Build(this);
}

#else

#include "GPUContextVulkan.h"
#include "Engine/Graphics/GPUAccelerationStructure.h"

GPUAccelerationStructure* GPUDeviceVulkan::CreateAccelerationStructure(const StringView& name)
{
    return nullptr;
}

void GPUContextVulkan::BuildAccelerationStructure(GPUAccelerationStructure* accelerationStructure)
{
}

#endif

#endif
