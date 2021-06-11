// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if GRAPHICS_API_VULKAN

#include "GPUTextureVulkan.h"
#include "GPUBufferVulkan.h"
#include "GPUContextVulkan.h"
#include "RenderToolsVulkan.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#include "Engine/Graphics/Textures/TextureData.h"

void GPUTextureViewVulkan::Init(GPUDeviceVulkan* device, ResourceOwnerVulkan* owner, VkImage image, int32 totalMipLevels, PixelFormat format, MSAALevel msaa, VkExtent3D extent, VkImageViewType viewType, int32 mipLevels, int32 firstMipIndex, int32 arraySize, int32 firstArraySlice, bool readOnlyDepth)
{
    ASSERT(View == VK_NULL_HANDLE);

    GPUTextureView::Init(owner->AsGPUResource(), format, msaa);

    Device = device;
    Owner = owner;
    Image = image;
    Extent.width = Math::Max<uint32_t>(1, extent.width >> firstMipIndex);
    Extent.height = Math::Max<uint32_t>(1, extent.height >> firstMipIndex);
    Extent.depth = Math::Max<uint32_t>(1, extent.depth >> firstMipIndex);
    Layers = arraySize;

    RenderToolsVulkan::ZeroStruct(Info, VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
    Info.image = image;
    Info.viewType = viewType;
    Info.format = RenderToolsVulkan::ToVulkanFormat(format);
    Info.components =
    {
        VK_COMPONENT_SWIZZLE_R,
        VK_COMPONENT_SWIZZLE_G,
        VK_COMPONENT_SWIZZLE_B,
        VK_COMPONENT_SWIZZLE_A
    };

    auto& range = Info.subresourceRange;
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = firstMipIndex;
    range.levelCount = mipLevels;
    range.baseArrayLayer = firstArraySlice;
    range.layerCount = arraySize;

    if (mipLevels != 1 || arraySize != 1)
    {
        SubresourceIndex = -1;
    }
    else
    {
        SubresourceIndex = RenderTools::CalcSubresourceIndex(firstMipIndex, firstArraySlice, totalMipLevels);
    }

    if (PixelFormatExtensions::IsDepthStencil(format))
    {
        range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (PixelFormatExtensions::HasStencil(format))
        {
            range.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        LayoutRTV = readOnlyDepth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        LayoutSRV = readOnlyDepth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    else
    {
        LayoutRTV = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        LayoutSRV = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    VALIDATE_VULKAN_RESULT(vkCreateImageView(device->Device, &Info, nullptr, &View));
}

VkImageView GPUTextureViewVulkan::GetFramebufferView()
{
    if (ViewFramebuffer)
        return ViewFramebuffer;

    if (Info.viewType == VK_IMAGE_VIEW_TYPE_3D)
    {
        // Special case:
        // Render Target Handle to a 3D Volume texture.
        // Use it as Texture2D Array with layers.
        VkImageViewCreateInfo createInfo = Info;
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        createInfo.subresourceRange.layerCount = Extent.depth;
        Layers = Extent.depth;
        VALIDATE_VULKAN_RESULT(vkCreateImageView(Device->Device, &createInfo, nullptr, &ViewFramebuffer));
    }
    else if (Info.subresourceRange.levelCount != 1)
    {
        // Special case:
        // Render Target Handle can be created for full texture including its mip maps but framebuffer image view can use only a single surface
        // Use an additional view for that case with modified level count to 1.
        VkImageViewCreateInfo createInfo = Info;
        createInfo.subresourceRange.levelCount = 1;
        VALIDATE_VULKAN_RESULT(vkCreateImageView(Device->Device, &createInfo, nullptr, &ViewFramebuffer));
    }
    else
    {
        ViewFramebuffer = View;
    }

    return ViewFramebuffer;
}

void GPUTextureViewVulkan::Release()
{
    if (View != VK_NULL_HANDLE)
    {
        if (ViewFramebuffer != View && ViewFramebuffer != VK_NULL_HANDLE)
        {
            Device->OnImageViewDestroy(ViewFramebuffer);
            Device->DeferredDeletionQueue.EnqueueResource(DeferredDeletionQueueVulkan::Type::ImageView, ViewFramebuffer);
        }

        Device->OnImageViewDestroy(View);
        Device->DeferredDeletionQueue.EnqueueResource(DeferredDeletionQueueVulkan::Type::ImageView, View);

        View = VK_NULL_HANDLE;
        ViewFramebuffer = VK_NULL_HANDLE;

#if BUILD_DEBUG
        Device = nullptr;
        Owner = nullptr;
        Image = VK_NULL_HANDLE;
#endif
    }
}

void GPUTextureViewVulkan::DescriptorAsImage(GPUContextVulkan* context, VkImageView& imageView, VkImageLayout& layout)
{
    imageView = View;
    layout = LayoutSRV;

    context->AddImageBarrier(this, LayoutSRV);
}

void GPUTextureViewVulkan::DescriptorAsStorageImage(GPUContextVulkan* context, VkImageView& imageView, VkImageLayout& layout)
{
    imageView = View;
    layout = VK_IMAGE_LAYOUT_GENERAL;

    context->AddImageBarrier(this, VK_IMAGE_LAYOUT_GENERAL);
}

bool GPUTextureVulkan::GetData(int32 arrayOrDepthSliceIndex, int32 mipMapIndex, TextureMipData& data, uint32 mipRowPitch)
{
    if (!IsStaging())
    {
        LOG(Warning, "Texture::GetData is valid only for staging resources.");
        return true;
    }

    GPUDeviceLock lock(_device);

    // Internally it's a buffer, so adapt resource index and offset
    const uint32 subresource = mipMapIndex + arrayOrDepthSliceIndex * MipLevels();
    // TODO: rowAlign/sliceAlign on Vulkan texture ???
    int32 offsetInBytes = ComputeBufferOffset(subresource, 1, 1);
    int32 lengthInBytes = ComputeSubresourceSize(subresource, 1, 1);
    int32 rowPitch = ComputeRowPitch(mipMapIndex, 1);
    int32 depthPicth = ComputeSlicePitch(mipMapIndex, 1);

    // Map the staging resource mip map for reading
    auto allocation = StagingBuffer->GetAllocation();
    void* mapped;
    const VkResult result = vmaMapMemory(_device->Allocator, allocation, (void**)&mapped);
    LOG_VULKAN_RESULT_WITH_RETURN(result);

    // Shift mapped buffer to the beginning of the mip data start
    mapped = (void*)((byte*)mapped + offsetInBytes);

    // Check if target row pitch is the same
    if (mipRowPitch == rowPitch || mipRowPitch == 0)
    {
        // Init mip info
        data.Lines = depthPicth / rowPitch;
        data.DepthPitch = depthPicth;
        data.RowPitch = rowPitch;

        // Copy data
        data.Data.Copy((byte*)mapped, depthPicth);
    }
    else
    {
        // Init mip info
        data.Lines = depthPicth / rowPitch;
        data.DepthPitch = mipRowPitch * data.Lines;
        data.RowPitch = mipRowPitch;

        // Copy data
        data.Data.Allocate(data.DepthPitch);
        for (uint32 i = 0; i < data.Lines; i++)
        {
            Platform::MemoryCopy(data.Data.Get() + data.RowPitch * i, ((byte*)mapped) + rowPitch * i, data.RowPitch);
        }
    }

    // Unmap resource
    vmaUnmapMemory(_device->Allocator, allocation);

    return false;
}

void GPUTextureVulkan::DescriptorAsStorageImage(GPUContextVulkan* context, VkImageView& imageView, VkImageLayout& layout)
{
    ASSERT(_handleUAV.Owner == this);
    imageView = _handleUAV.View;
    layout = VK_IMAGE_LAYOUT_GENERAL;

    context->AddImageBarrier(this, VK_IMAGE_LAYOUT_GENERAL);
}

bool GPUTextureVulkan::OnInit()
{
    // Check if texture should have optimal CPU read/write access
    if (IsStaging())
    {
        // TODO: rowAlign/sliceAlign on Vulkan texture ???
        const int32 totalSize = ComputeBufferTotalSize(1, 1);
        StagingBuffer = (GPUBufferVulkan*)_device->CreateBuffer(TEXT("Texture.StagingBuffer"));
        if (StagingBuffer->Init(GPUBufferDescription::Buffer(totalSize, GPUBufferFlags::None, PixelFormat::Unknown, nullptr, 0, _desc.Usage)))
        {
            Delete(StagingBuffer);
            return true;
        }
        _memoryUsage = 1;
        return false;
    }

    const bool useSRV = IsShaderResource();
    const bool useDSV = IsDepthStencil();
    const bool useRTV = IsRenderTarget();
    const bool useUAV = IsUnorderedAccess();

    const bool optimalTiling = true;
    PixelFormat format = _desc.Format;
    if (useDSV)
        format = PixelFormatExtensions::FindDepthStencilFormat(format);
    _desc.Format = _device->GetClosestSupportedPixelFormat(format, _desc.Flags, optimalTiling);

    // Setup texture description
    VkImageCreateInfo imageInfo;
    RenderToolsVulkan::ZeroStruct(imageInfo, VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO);
    imageInfo.imageType = IsVolume() ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
    imageInfo.format = RenderToolsVulkan::ToVulkanFormat(Format());
    imageInfo.mipLevels = MipLevels();
    imageInfo.arrayLayers = ArraySize();
    imageInfo.extent.width = Width();
    imageInfo.extent.height = Height();
    imageInfo.extent.depth = Depth();
    imageInfo.flags = IsCubeMap() ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
    if (IsSRGB())
        imageInfo.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
#if VK_KHR_maintenance1
    if (_device->OptionalDeviceExtensions.HasKHRMaintenance1 && imageInfo.imageType == VK_IMAGE_TYPE_3D)
        imageInfo.flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT_KHR;
#endif
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (useSRV)
        imageInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if (useDSV)
        imageInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    if (useRTV)
        imageInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (useUAV)
        imageInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    imageInfo.tiling = optimalTiling ? VK_IMAGE_TILING_OPTIMAL : VK_IMAGE_TILING_LINEAR;
    imageInfo.samples = (VkSampleCountFlagBits)MultiSampleLevel();
    // TODO: set initialLayout to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL for IsRegularTexture() ???

    // Create texture
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    const VkResult result = vmaCreateImage(_device->Allocator, &imageInfo, &allocInfo, &_image, &_allocation, nullptr);
    LOG_VULKAN_RESULT_WITH_RETURN(result);
#if GPU_ENABLE_RESOURCE_NAMING
    VK_SET_DEBUG_NAME(_device, _image, VK_OBJECT_TYPE_IMAGE, GetName());
#endif
    //LOG(Warning, "VULKAN TEXTURE -> 0x{0:x} - {1}", (int)_image, GetName());

    // Set state
    initResource(VK_IMAGE_LAYOUT_UNDEFINED, _desc.MipLevels, _desc.ArraySize, true);
    _memoryUsage = calculateMemoryUsage();
    if (PixelFormatExtensions::IsDepthStencil(format))
    {
        DefaultAspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (PixelFormatExtensions::HasStencil(format))
            DefaultAspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    else
    {
        DefaultAspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    // Initialize handles to the resource
    if (IsRegularTexture())
    {
        // 'Regular' texture is using only one handle (texture/cubemap)
        _handlesPerSlice.Resize(1, false);
    }
    else
    {
        // Create all handles
        initHandles();
    }

    return false;
}

void GPUTextureVulkan::initHandles()
{
    // Cache properties
    const int32 arraySize = ArraySize();
    const int32 mipLevels = MipLevels();
    const bool isArray = arraySize > 1;
    const bool isCubeMap = IsCubeMap();
    const bool isVolume = IsVolume();
    const auto format = Format();
    const auto msaa = MultiSampleLevel();
    VkExtent3D extent;
    extent.width = Width();
    extent.height = Height();
    extent.depth = Depth();

    // Create resource views
    if (isVolume)
    {
        // Create handle for whole 3d texture
        _handleVolume.Init(_device, this, _image, mipLevels, format, msaa, extent, VK_IMAGE_VIEW_TYPE_3D, mipLevels, 0);

        // Init per slice views
        _handlesPerSlice.Resize(Depth(), false);
        if (_desc.HasPerSliceViews())
        {
            for (int32 sliceIndex = 0; sliceIndex < Depth(); sliceIndex++)
            {
                _handlesPerSlice[sliceIndex].Init(_device, this, _image, mipLevels, format, msaa, extent, VK_IMAGE_VIEW_TYPE_2D, mipLevels, 0, 1, sliceIndex);
            }
        }
    }
    else if (isArray)
    {
        // Resize handles
        _handlesPerSlice.Resize(ArraySize(), false);

        // Create per array slice handles
        for (int32 arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
        {
            _handlesPerSlice[arrayIndex].Init(_device, this, _image, mipLevels, format, msaa, extent, VK_IMAGE_VIEW_TYPE_2D, mipLevels, 0, 1, arrayIndex);
        }

        // Create whole array handle
        if (isCubeMap)
        {
            _handleArray.Init(_device, this, _image, mipLevels, format, msaa, extent, VK_IMAGE_VIEW_TYPE_CUBE, mipLevels, 0, arraySize, 0);
        }
        else
        {
            _handleArray.Init(_device, this, _image, mipLevels, format, msaa, extent, VK_IMAGE_VIEW_TYPE_2D_ARRAY, mipLevels, 0, arraySize, 0);
        }
    }
    else
    {
        // Create single handle for the whole texture
        _handlesPerSlice.Resize(1, false);
        if (isCubeMap)
        {
            _handlesPerSlice[0].Init(_device, this, _image, mipLevels, format, msaa, extent, VK_IMAGE_VIEW_TYPE_CUBE, mipLevels, 0, arraySize);
        }
        else
        {
            _handlesPerSlice[0].Init(_device, this, _image, mipLevels, format, msaa, extent, VK_IMAGE_VIEW_TYPE_2D, mipLevels);
        }
    }

    // Init per mip map handles
    if (HasPerMipViews())
    {
        // Init handles
        _handlesPerMip.Resize(arraySize, false);
        for (int32 arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
        {
            auto& slice = _handlesPerMip[arrayIndex];
            slice.Resize(mipLevels, false);

            for (int32 mipIndex = 0; mipIndex < mipLevels; mipIndex++)
            {
                slice[mipIndex].Init(_device, this, _image, mipLevels, format, msaa, extent, VK_IMAGE_VIEW_TYPE_2D, 1, mipIndex, 1, arrayIndex);
            }
        }
    }

    // UAV texture
    if (IsUnorderedAccess())
    {
        if (isVolume)
        {
            _handleUAV.Init(_device, this, _image, mipLevels, format, msaa, extent, VK_IMAGE_VIEW_TYPE_3D, 1, 0, 1, 0);
        }
        else if (isArray)
        {
            _handleUAV.Init(_device, this, _image, mipLevels, format, msaa, extent, VK_IMAGE_VIEW_TYPE_2D_ARRAY, 1, 0, arraySize, 0);
        }
        else
        {
            _handleUAV.Init(_device, this, _image, mipLevels, format, msaa, extent, VK_IMAGE_VIEW_TYPE_2D, 1, 0, 1, 0);
        }
    }

    // Read-only depth-stencil
    if (_desc.Flags & GPUTextureFlags::ReadOnlyDepthView)
    {
        _handleReadOnlyDepth.Init(_device, this, _image, mipLevels, format, msaa, extent, VK_IMAGE_VIEW_TYPE_2D, mipLevels, 0, 1, 0, true);
    }
}

void GPUTextureVulkan::onResidentMipsChanged()
{
    // We support changing resident mip maps only for regular textures (render targets and depth buffers don't use that feature at all)
    ASSERT(IsRegularTexture() && _handlesPerSlice.Count() == 1);
    ASSERT(!IsVolume());

    // Change view
    auto& handle = _handlesPerSlice[0];
    handle.Release();
    VkExtent3D extent;
    extent.width = Width();
    extent.height = Height();
    extent.depth = Depth();
    const int32 firstMipIndex = MipLevels() - ResidentMipLevels();
    const int32 mipLevels = ResidentMipLevels();
    if (IsCubeMap())
    {
        handle.Init(_device, this, _image, mipLevels, Format(), MultiSampleLevel(), extent, VK_IMAGE_VIEW_TYPE_CUBE, mipLevels, firstMipIndex, ArraySize());
    }
    else
    {
        handle.Init(_device, this, _image, mipLevels, Format(), MultiSampleLevel(), extent, VK_IMAGE_VIEW_TYPE_2D, mipLevels, firstMipIndex, ArraySize());
    }
}

void GPUTextureVulkan::OnReleaseGPU()
{
    _handleArray.Release();
    _handleVolume.Release();
    _handleUAV.Release();
    _handleReadOnlyDepth.Release();
    for (int32 i = 0; i < _handlesPerMip.Count(); i++)
    {
        for (int32 j = 0; j < _handlesPerMip[i].Count(); j++)
        {
            _handlesPerMip[i][j].Release();
        }
    }
    for (int32 i = 0; i < _handlesPerSlice.Count(); i++)
    {
        _handlesPerSlice[i].Release();
    }
    _handlesPerMip.Resize(0, false);
    _handlesPerSlice.Resize(0, false);
    if (_image != VK_NULL_HANDLE)
    {
        _device->DeferredDeletionQueue.EnqueueResource(DeferredDeletionQueueVulkan::Type::Image, _image, _allocation);
        _image = VK_NULL_HANDLE;
        _allocation = VK_NULL_HANDLE;
    }
    SAFE_DELETE_GPU_RESOURCE(StagingBuffer);
    State.Release();

    // Base
    GPUTexture::OnReleaseGPU();
}

#endif
