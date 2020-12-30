// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/Platform.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "IncludeVulkanHeaders.h"
#include "Types.h"
#include "Config.h"
#include "Engine/Platform/CriticalSection.h"

#if GRAPHICS_API_VULKAN

#define VULKAN_DESCRIPTOR_TYPE_BEGIN VK_DESCRIPTOR_TYPE_SAMPLER
#define VULKAN_DESCRIPTOR_TYPE_END VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT

class GPUDeviceVulkan;
class CmdBufferVulkan;
class GPUContextVulkan;

namespace DescriptorSet
{
    enum Stage
    {
        // Vertex shader stage
        Vertex = 0,

        // Pixel shader stage
        Pixel = 1,

        // Geometry shader stage
        Geometry = 2,

        // Hull shader stage
        Hull = 3,

        // Domain shader stage
        Domain = 4,

        // Graphics pipeline stages count
        GraphicsStagesCount = 5,

        // Compute pipeline slot
        Compute = 0,

        // The maximum amount of slots for all stages
        Max = 5,
    };

    inline Stage GetSetForFrequency(ShaderStage stage)
    {
        switch (stage)
        {
        case ShaderStage::Vertex:
            return Vertex;
        case ShaderStage::Hull:
            return Hull;
        case ShaderStage::Domain:
            return Domain;
        case ShaderStage::Pixel:
            return Pixel;
        case ShaderStage::Geometry:
            return Geometry;
        case ShaderStage::Compute:
            return Compute;
        default:
        CRASH;
            return Max;
        }
    }

    inline ShaderStage GetFrequencyForGfxSet(Stage stage)
    {
        switch (stage)
        {
        case Vertex:
            return ShaderStage::Vertex;
        case Hull:
            return ShaderStage::Hull;
        case Domain:
            return ShaderStage::Domain;
        case Pixel:
            return ShaderStage::Pixel;
        case Geometry:
            return ShaderStage::Geometry;
        default:
        CRASH;
            return static_cast<ShaderStage>(ShaderStage_Count);
        }
    }

    template<typename T>
    inline bool CopyAndReturnNotEqual(T& a, T b)
    {
        const bool result = a != b;
        a = b;
        return result;
    }
};

class DescriptorSetLayoutInfoVulkan
{
public:

    struct SetLayout
    {
        Array<VkDescriptorSetLayoutBinding> LayoutBindings;
    };

protected:

    uint32 _layoutTypes[VULKAN_DESCRIPTOR_TYPE_END];
    Array<SetLayout> _setLayouts;
    uint32 _hash = 0;
    uint32 _typesUsageID = ~0;

    void CacheTypesUsageID();
    void AddDescriptor(int32 descriptorSetIndex, const VkDescriptorSetLayoutBinding& descriptor);

public:

    DescriptorSetLayoutInfoVulkan()
    {
        Platform::MemoryClear(_layoutTypes, sizeof(_layoutTypes));
    }

public:

    inline uint32 GetTypesUsed(VkDescriptorType type) const
    {
        return _layoutTypes[type];
    }

    const Array<SetLayout>& GetLayouts() const
    {
        return _setLayouts;
    }

    inline const uint32* GetLayoutTypes() const
    {
        return _layoutTypes;
    }

    inline uint32 GetTypesUsageID() const
    {
        return _typesUsageID;
    }

public:

    void AddBindingsForStage(VkShaderStageFlagBits stageFlags, DescriptorSet::Stage descSet, const SpirvShaderDescriptorInfo* descriptorInfo);

    void CopyFrom(const DescriptorSetLayoutInfoVulkan& info)
    {
        Platform::MemoryCopy(_layoutTypes, info._layoutTypes, sizeof(_layoutTypes));
        _hash = info._hash;
        _typesUsageID = info._typesUsageID;
        _setLayouts = info._setLayouts;
    }

    inline bool operator ==(const DescriptorSetLayoutInfoVulkan& other) const
    {
        if (other._setLayouts.Count() != _setLayouts.Count())
            return false;
        if (other._typesUsageID != _typesUsageID)
            return false;

        for (int32 index = 0; index < other._setLayouts.Count(); index++)
        {
            const int32 bindingsCount = _setLayouts[index].LayoutBindings.Count();
            if (other._setLayouts[index].LayoutBindings.Count() != bindingsCount)
                return false;

            if (bindingsCount != 0 && Platform::MemoryCompare(other._setLayouts[index].LayoutBindings.Get(), _setLayouts[index].LayoutBindings.Get(), bindingsCount * sizeof(VkDescriptorSetLayoutBinding)))
                return false;
        }

        return true;
    }

    friend inline uint32 GetHash(const DescriptorSetLayoutInfoVulkan& key)
    {
        return key._hash;
    }
};

class DescriptorSetLayoutVulkan : public DescriptorSetLayoutInfoVulkan
{
public:

    typedef Array<VkDescriptorSetLayout, FixedAllocation<DescriptorSet::Max>> DescriptorSetLayoutHandlesArray;

private:

    GPUDeviceVulkan* _device;
    DescriptorSetLayoutHandlesArray _handles;
    VkDescriptorSetAllocateInfo _allocateInfo;

public:

    DescriptorSetLayoutVulkan(GPUDeviceVulkan* device);
    ~DescriptorSetLayoutVulkan();

public:

    inline const DescriptorSetLayoutHandlesArray& GetHandles() const
    {
        return _handles;
    }

    inline const VkDescriptorSetAllocateInfo& GetAllocateInfo() const
    {
        return _allocateInfo;
    }

    friend inline uint32 GetHash(const DescriptorSetLayoutVulkan& key)
    {
        return key._hash;
    }

    void Compile();
};

class DescriptorPoolVulkan
{
private:

    GPUDeviceVulkan* _device;
    VkDescriptorPool _handle;

    uint32 _descriptorSetsMax;
    uint32 _allocatedDescriptorSetsCount;
    uint32 _allocatedDescriptorSetsCountMax;

    const DescriptorSetLayoutVulkan& _layout;

public:

    DescriptorPoolVulkan(GPUDeviceVulkan* device, const DescriptorSetLayoutVulkan& layout);
    ~DescriptorPoolVulkan();

public:

    inline VkDescriptorPool GetHandle() const
    {
        return _handle;
    }

    inline bool IsEmpty() const
    {
        return _allocatedDescriptorSetsCount == 0;
    }

    inline bool CanAllocate(const DescriptorSetLayoutVulkan& layout) const
    {
        return _descriptorSetsMax > _allocatedDescriptorSetsCount + layout.GetLayouts().Count();
    }

    inline uint32 GetAllocatedDescriptorSetsCount() const
    {
        return _allocatedDescriptorSetsCount;
    }

public:

    void Track(const DescriptorSetLayoutVulkan& layout);
    void TrackRemoveUsage(const DescriptorSetLayoutVulkan& layout);
    void Reset();
    bool AllocateDescriptorSets(const VkDescriptorSetAllocateInfo& descriptorSetAllocateInfo, VkDescriptorSet* result);
};

class DescriptorPoolSetContainerVulkan;

class TypedDescriptorPoolSetVulkan
{
    friend DescriptorPoolSetContainerVulkan;

private:

    GPUDeviceVulkan* _device;
    const DescriptorPoolSetContainerVulkan* _owner;
    const DescriptorSetLayoutVulkan& _layout;

    class PoolList
    {
    public:

        DescriptorPoolVulkan* Element;
        PoolList* Next;

        PoolList(DescriptorPoolVulkan* element, PoolList* next = nullptr)
        {
            Element = element;
            Next = next;
        }
    };

    PoolList* _poolListHead = nullptr;
    PoolList* _poolListCurrent = nullptr;

public:

    TypedDescriptorPoolSetVulkan(GPUDeviceVulkan* device, const DescriptorPoolSetContainerVulkan* owner, const DescriptorSetLayoutVulkan& layout)
        : _device(device)
        , _owner(owner)
        , _layout(layout)
    {
        PushNewPool();
    };

    ~TypedDescriptorPoolSetVulkan();

    bool AllocateDescriptorSets(const DescriptorSetLayoutVulkan& layout, VkDescriptorSet* outSets);

    const DescriptorPoolSetContainerVulkan* GetOwner() const
    {
        return _owner;
    }

private:

    DescriptorPoolVulkan* GetFreePool(bool forceNewPool = false);
    DescriptorPoolVulkan* PushNewPool();
    void Reset();
};

class DescriptorPoolSetContainerVulkan
{
private:

    GPUDeviceVulkan* _device;
    Dictionary<uint32, TypedDescriptorPoolSetVulkan*> _typedDescriptorPools;
    uint64 _lastFrameUsed;
    bool _used;

public:

    DescriptorPoolSetContainerVulkan(GPUDeviceVulkan* device);

    ~DescriptorPoolSetContainerVulkan();

public:

    TypedDescriptorPoolSetVulkan* AcquireTypedPoolSet(const DescriptorSetLayoutVulkan& layout);
    void Reset();
    void SetUsed(bool used);

    bool IsUnused() const
    {
        return !_used;
    }

    uint64 GetLastFrameUsed() const
    {
        return _lastFrameUsed;
    }
};

class DescriptorPoolsManagerVulkan
{
private:

    GPUDeviceVulkan* _device = nullptr;
    CriticalSection _locker;
    Array<DescriptorPoolSetContainerVulkan*> _poolSets;

public:

    DescriptorPoolsManagerVulkan(GPUDeviceVulkan* device);
    ~DescriptorPoolsManagerVulkan();

    DescriptorPoolSetContainerVulkan& AcquirePoolSetContainer();
    void ReleasePoolSet(DescriptorPoolSetContainerVulkan& poolSet);
    void GC();
};

class PipelineLayoutVulkan
{
private:

    GPUDeviceVulkan* _device;
    VkPipelineLayout _handle;
    DescriptorSetLayoutVulkan _descriptorSetLayout;

public:

    PipelineLayoutVulkan(GPUDeviceVulkan* device, const DescriptorSetLayoutInfoVulkan& layout);
    ~PipelineLayoutVulkan();

public:

    inline VkPipelineLayout GetHandle() const
    {
        return _handle;
    }

public:

    inline const DescriptorSetLayoutVulkan& GetDescriptorSetLayout() const
    {
        return _descriptorSetLayout;
    }

    inline bool HasDescriptors() const
    {
        return _descriptorSetLayout.GetLayouts().HasItems();
    }
};

struct DescriptorSetWriteContainerVulkan
{
    Array<VkDescriptorImageInfo> DescriptorImageInfo;
    Array<VkDescriptorBufferInfo> DescriptorBufferInfo;
    Array<VkWriteDescriptorSet> DescriptorWrites;
    Array<byte> BindingToDynamicOffset;

    void Release()
    {
        DescriptorImageInfo.Resize(0);
        DescriptorBufferInfo.Resize(0);
        DescriptorWrites.Resize(0);
        BindingToDynamicOffset.Resize(0);
    }
};

class DescriptorSetWriterVulkan
{
public:

    VkWriteDescriptorSet* WriteDescriptors = nullptr;
    byte* BindingToDynamicOffset = nullptr;
    uint32* DynamicOffsets = nullptr;
    uint32 WritesCount = 0;

public:

    uint32 SetupDescriptorWrites(const SpirvShaderDescriptorInfo& info, VkWriteDescriptorSet* writeDescriptors, VkDescriptorImageInfo* imageInfo, VkDescriptorBufferInfo* bufferInfo, byte* bindingToDynamicOffset);

    bool WriteUniformBuffer(uint32 descriptorIndex, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range) const
    {
        ASSERT(descriptorIndex < WritesCount);
        ASSERT(WriteDescriptors[descriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        VkDescriptorBufferInfo* bufferInfo = const_cast<VkDescriptorBufferInfo*>(WriteDescriptors[descriptorIndex].pBufferInfo);
        ASSERT(bufferInfo);
        bool edited = DescriptorSet::CopyAndReturnNotEqual(bufferInfo->buffer, buffer);
        edited |= DescriptorSet::CopyAndReturnNotEqual(bufferInfo->offset, offset);
        edited |= DescriptorSet::CopyAndReturnNotEqual(bufferInfo->range, range);
        return edited;
    }

    bool WriteDynamicUniformBuffer(uint32 descriptorIndex, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range, uint32 dynamicOffset) const
    {
        ASSERT(descriptorIndex < WritesCount);
        ASSERT(WriteDescriptors[descriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
        VkDescriptorBufferInfo* bufferInfo = const_cast<VkDescriptorBufferInfo*>(WriteDescriptors[descriptorIndex].pBufferInfo);
        ASSERT(bufferInfo);
        bool edited = DescriptorSet::CopyAndReturnNotEqual(bufferInfo->buffer, buffer);
        edited |= DescriptorSet::CopyAndReturnNotEqual(bufferInfo->offset, offset);
        edited |= DescriptorSet::CopyAndReturnNotEqual(bufferInfo->range, range);
        const byte dynamicOffsetIndex = BindingToDynamicOffset[descriptorIndex];
        DynamicOffsets[dynamicOffsetIndex] = dynamicOffset;
        return edited;
    }

    bool WriteSampler(uint32 descriptorIndex, VkSampler sampler) const
    {
        ASSERT(descriptorIndex < WritesCount);
        ASSERT(WriteDescriptors[descriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER || WriteDescriptors[descriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        VkDescriptorImageInfo* imageInfo = const_cast<VkDescriptorImageInfo*>(WriteDescriptors[descriptorIndex].pImageInfo);
        ASSERT(imageInfo);
        bool edited = DescriptorSet::CopyAndReturnNotEqual(imageInfo->sampler, sampler);
        return edited;
    }

    bool WriteImage(uint32 descriptorIndex, VkImageView imageView, VkImageLayout layout) const
    {
        ASSERT(descriptorIndex < WritesCount);
        ASSERT(WriteDescriptors[descriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE || WriteDescriptors[descriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        VkDescriptorImageInfo* imageInfo = const_cast<VkDescriptorImageInfo*>(WriteDescriptors[descriptorIndex].pImageInfo);
        ASSERT(imageInfo);
        bool edited = DescriptorSet::CopyAndReturnNotEqual(imageInfo->imageView, imageView);
        edited |= DescriptorSet::CopyAndReturnNotEqual(imageInfo->imageLayout, layout);
        return edited;
    }

    bool WriteStorageImage(uint32 descriptorIndex, VkImageView imageView, VkImageLayout layout) const
    {
        ASSERT(descriptorIndex < WritesCount);
        ASSERT(WriteDescriptors[descriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        VkDescriptorImageInfo* imageInfo = const_cast<VkDescriptorImageInfo*>(WriteDescriptors[descriptorIndex].pImageInfo);
        ASSERT(imageInfo);
        bool edited = DescriptorSet::CopyAndReturnNotEqual(imageInfo->imageView, imageView);
        edited |= DescriptorSet::CopyAndReturnNotEqual(imageInfo->imageLayout, layout);
        return edited;
    }

    bool WriteStorageTexelBuffer(uint32 descriptorIndex, const VkBufferView* bufferView) const
    {
        ASSERT(descriptorIndex < WritesCount);
        ASSERT(WriteDescriptors[descriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER);
        WriteDescriptors[descriptorIndex].pTexelBufferView = bufferView;
        return true;
    }

    bool WriteStorageBuffer(uint32 descriptorIndex, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range) const
    {
        ASSERT(descriptorIndex < WritesCount);
        ASSERT(WriteDescriptors[descriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER || WriteDescriptors[descriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC);
        VkDescriptorBufferInfo* bufferInfo = const_cast<VkDescriptorBufferInfo*>(WriteDescriptors[descriptorIndex].pBufferInfo);
        ASSERT(bufferInfo);
        bool edited = DescriptorSet::CopyAndReturnNotEqual(bufferInfo->buffer, buffer);
        edited |= DescriptorSet::CopyAndReturnNotEqual(bufferInfo->offset, offset);
        edited |= DescriptorSet::CopyAndReturnNotEqual(bufferInfo->range, range);
        return edited;
    }

    bool WriteUniformTexelBuffer(uint32 descriptorIndex, const VkBufferView* view) const
    {
        ASSERT(descriptorIndex < WritesCount);
        ASSERT(WriteDescriptors[descriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER);
        return DescriptorSet::CopyAndReturnNotEqual(WriteDescriptors[descriptorIndex].pTexelBufferView, view);
    }

    void SetDescriptorSet(VkDescriptorSet descriptorSet) const
    {
        for (uint32 i = 0; i < WritesCount; i++)
        {
            WriteDescriptors[i].dstSet = descriptorSet;
        }
    }
};

#endif
