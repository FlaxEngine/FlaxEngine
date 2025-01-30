// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_VULKAN

#include "Engine/Core/Collections/Array.h"
#include "Engine/Graphics/Textures/GPUTexture.h"
#include "GPUDeviceVulkan.h"
#include "ResourceOwnerVulkan.h"

/// <summary>
/// The texture view for Vulkan backend.
/// </summary>
class GPUTextureViewVulkan : public GPUTextureView, public DescriptorOwnerResourceVulkan
{
public:
    GPUTextureViewVulkan()
    {
    }

    GPUTextureViewVulkan(const GPUTextureViewVulkan& other)
        : GPUTextureViewVulkan()
    {
#if !BUILD_RELEASE
        CRASH; // Not used
#endif
    }

    GPUTextureViewVulkan& operator=(const GPUTextureViewVulkan& other)
    {
#if !BUILD_RELEASE
        CRASH; // Not used
#endif
        return *this;
    }

#if !BUILD_RELEASE
    ~GPUTextureViewVulkan()
    {
        ASSERT(View == VK_NULL_HANDLE);
    }
#endif

public:
    GPUDeviceVulkan* Device = nullptr;
    ResourceOwnerVulkan* Owner = nullptr;
    VkImage Image = VK_NULL_HANDLE;
    VkImageView View = VK_NULL_HANDLE;
    VkImageView ViewFramebuffer = VK_NULL_HANDLE;
    VkImageView ViewSRV = VK_NULL_HANDLE;
    VkExtent3D Extent;
    uint32 Layers;
    VkImageViewCreateInfo Info;
    int32 SubresourceIndex;
    VkImageLayout LayoutRTV;
    VkImageLayout LayoutSRV;
#if VULKAN_USE_DEBUG_DATA
    PixelFormat Format;
    bool ReadOnlyDepth;
#endif

public:
    void Init(GPUDeviceVulkan* device, ResourceOwnerVulkan* owner, VkImage image, int32 totalMipLevels, PixelFormat format, MSAALevel msaa, VkExtent3D extent, VkImageViewType viewType, int32 mipLevels = 1, int32 firstMipIndex = 0, int32 arraySize = 1, int32 firstArraySlice = 0, bool readOnlyDepth = false);

    VkImageView GetFramebufferView();

    void Release();

public:
    // [GPUResourceView]
    void* GetNativePtr() const override
    {
        return (void*)(DescriptorOwnerResourceVulkan*)this;
    }

    // [DescriptorOwnerResourceVulkan]
    void DescriptorAsImage(GPUContextVulkan* context, VkImageView& imageView, VkImageLayout& layout) override;
    void DescriptorAsStorageImage(GPUContextVulkan* context, VkImageView& imageView, VkImageLayout& layout) override;
#if !BUILD_RELEASE
    bool HasSRV() const override { return ((GPUTexture*)_parent)->IsShaderResource(); }
    bool HasUAV() const override { return ((GPUTexture*)_parent)->IsUnorderedAccess(); }
#endif
};

/// <summary>
/// Texture object for Vulkan backend.
/// </summary>
class GPUTextureVulkan : public GPUResourceVulkan<GPUTexture>, public ResourceOwnerVulkan, public DescriptorOwnerResourceVulkan
{
private:
    VkImage _image = VK_NULL_HANDLE;
    VmaAllocation _allocation = VK_NULL_HANDLE;
    GPUTextureViewVulkan _handleArray;
    GPUTextureViewVulkan _handleVolume;
    GPUTextureViewVulkan _handleUAV;
    GPUTextureViewVulkan _handleReadOnlyDepth;
    Array<GPUTextureViewVulkan> _handlesPerSlice; // [slice]
    Array<Array<GPUTextureViewVulkan>> _handlesPerMip; // [slice][mip]

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="GPUTextureVulkan"/> class.
    /// </summary>
    /// <param name="device">The device.</param>
    /// <param name="name">The resource name.</param>
    GPUTextureVulkan(GPUDeviceVulkan* device, const StringView& name)
        : GPUResourceVulkan<GPUTexture>(device, name)
    {
    }

public:
    /// <summary>
    /// Gets the Vulkan image handle.
    /// </summary>
    FORCE_INLINE VkImage GetHandle() const
    {
        return _image;
    }

    /// <summary>
    /// The Vulkan staging buffer (used by the staging textures for memory transfers).
    /// </summary>
    GPUBufferVulkan* StagingBuffer = nullptr;

    /// <summary>
    /// The default aspect mask flags for the texture (all planes).
    /// </summary>
    VkImageAspectFlags DefaultAspectMask;

private:
    void initHandles();

public:
    // [GPUTexture]
    GPUTextureView* View(int32 arrayOrDepthIndex) const override
    {
        return (GPUTextureView*)&_handlesPerSlice[arrayOrDepthIndex];
    }
    GPUTextureView* View(int32 arrayOrDepthIndex, int32 mipMapIndex) const override
    {
        return (GPUTextureView*)&_handlesPerMip[arrayOrDepthIndex][mipMapIndex];
    }
    GPUTextureView* ViewArray() const override
    {
        ASSERT(ArraySize() > 1);
        return (GPUTextureView*)&_handleArray;
    }
    GPUTextureView* ViewVolume() const override
    {
        ASSERT(IsVolume());
        return (GPUTextureView*)&_handleVolume;
    }
    GPUTextureView* ViewReadOnlyDepth() const override
    {
        ASSERT(_desc.Flags & GPUTextureFlags::ReadOnlyDepthView);
        return (GPUTextureView*)&_handleReadOnlyDepth;
    }
    void* GetNativePtr() const override
    {
        return (void*)_image;
    }
    bool GetData(int32 arrayIndex, int32 mipMapIndex, TextureMipData& data, uint32 mipRowPitch) override;

    // [ResourceOwnerVulkan]
    GPUResource* AsGPUResource() const override
    {
        return (GPUResource*)this;
    }

    // [DescriptorOwnerResourceVulkan]
    void DescriptorAsStorageImage(GPUContextVulkan* context, VkImageView& imageView, VkImageLayout& layout) override;

protected:
    // [GPUTexture]
    bool OnInit() override;
    void OnResidentMipsChanged() override;
    void OnReleaseGPU() override;
};

#endif
