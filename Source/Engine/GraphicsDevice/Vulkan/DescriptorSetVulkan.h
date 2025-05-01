// Copyright (c) Wojciech Figat. All rights reserved.

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
        Vertex,
        // Pixel shader stage
        Pixel,
#if GPU_ALLOW_GEOMETRY_SHADERS
        // Geometry shader stage
        Geometry,
#endif
#if GPU_ALLOW_TESSELLATION_SHADERS
        // Hull shader stage
        Hull,
        // Domain shader stage
        Domain,
#endif
        // Graphics pipeline stages count
        GraphicsStagesCount,
        // Compute pipeline slot
        Compute = 0,
        // The maximum amount of slots for all stages
        Max = GraphicsStagesCount,
    };

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

    uint32 Hash = 0;
    uint32 SetLayoutsHash = 0;
    uint32 LayoutTypes[VULKAN_DESCRIPTOR_TYPE_END];
    Array<SetLayout> SetLayouts;

public:
    DescriptorSetLayoutInfoVulkan()
    {
        Platform::MemoryClear(LayoutTypes, sizeof(LayoutTypes));
    }

    void AddBindingsForStage(VkShaderStageFlagBits stageFlags, DescriptorSet::Stage descSet, const SpirvShaderDescriptorInfo* descriptorInfo);

    void CopyFrom(const DescriptorSetLayoutInfoVulkan& info)
    {
        Platform::MemoryCopy(LayoutTypes, info.LayoutTypes, sizeof(LayoutTypes));
        Hash = info.Hash;
        SetLayoutsHash = info.SetLayoutsHash;
        SetLayouts = info.SetLayouts;
    }

    bool operator==(const DescriptorSetLayoutInfoVulkan& other) const;

    friend inline uint32 GetHash(const DescriptorSetLayoutInfoVulkan& key)
    {
        return key.Hash;
    }
};

class DescriptorSetLayoutVulkan : public DescriptorSetLayoutInfoVulkan
{
public:
    typedef Array<VkDescriptorSetLayout, FixedAllocation<DescriptorSet::Max>> HandlesArray;

    GPUDeviceVulkan* Device;
    HandlesArray Handles;
    VkDescriptorSetAllocateInfo AllocateInfo;

    DescriptorSetLayoutVulkan(GPUDeviceVulkan* device);
    ~DescriptorSetLayoutVulkan();

    void Compile();

    friend inline uint32 GetHash(const DescriptorSetLayoutVulkan& key)
    {
        return key.Hash;
    }
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
        return _descriptorSetsMax > _allocatedDescriptorSetsCount + layout.SetLayouts.Count();
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

public:
    DescriptorPoolSetContainerVulkan(GPUDeviceVulkan* device);
    ~DescriptorPoolSetContainerVulkan();

public:
    TypedDescriptorPoolSetVulkan* AcquireTypedPoolSet(const DescriptorSetLayoutVulkan& layout);
    void Reset();

    mutable uint64 Refs = 0;
    mutable uint64 LastFrameUsed;
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

    DescriptorPoolSetContainerVulkan* AcquirePoolSetContainer();
    void GC();
};

class PipelineLayoutVulkan
{
public:
    GPUDeviceVulkan* Device;
    VkPipelineLayout Handle;
    DescriptorSetLayoutVulkan DescriptorSetLayout;

    PipelineLayoutVulkan(GPUDeviceVulkan* device, const DescriptorSetLayoutInfoVulkan& layout);
    ~PipelineLayoutVulkan();
};

struct DescriptorSetWriteContainerVulkan
{
    Array<VkDescriptorImageInfo> DescriptorImageInfo;
    Array<VkDescriptorBufferInfo> DescriptorBufferInfo;
    Array<VkBufferView> DescriptorTexelBufferView;
    Array<VkWriteDescriptorSet> DescriptorWrites;
    Array<byte> BindingToDynamicOffset;

    void Release()
    {
        DescriptorImageInfo.Resize(0);
        DescriptorBufferInfo.Resize(0);
        DescriptorTexelBufferView.Resize(0);
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
    uint32 SetupDescriptorWrites(const SpirvShaderDescriptorInfo& info, VkWriteDescriptorSet* writeDescriptors, VkDescriptorImageInfo* imageInfo, VkDescriptorBufferInfo* bufferInfo, VkBufferView* texelBufferView, byte* bindingToDynamicOffset);

    bool WriteUniformBuffer(uint32 descriptorIndex, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range, uint32 index = 0) const
    {
        ASSERT(descriptorIndex < WritesCount);
        ASSERT(WriteDescriptors[descriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        auto* bufferInfo = const_cast<VkDescriptorBufferInfo*>(WriteDescriptors[descriptorIndex].pBufferInfo + index);
        bool edited = DescriptorSet::CopyAndReturnNotEqual(bufferInfo->buffer, buffer);
        edited |= DescriptorSet::CopyAndReturnNotEqual(bufferInfo->offset, offset);
        edited |= DescriptorSet::CopyAndReturnNotEqual(bufferInfo->range, range);
        return edited;
    }

    bool WriteDynamicUniformBuffer(uint32 descriptorIndex, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range, uint32 dynamicOffset, uint32 index = 0) const
    {
        ASSERT(descriptorIndex < WritesCount);
        ASSERT(WriteDescriptors[descriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
        auto* bufferInfo = const_cast<VkDescriptorBufferInfo*>(WriteDescriptors[descriptorIndex].pBufferInfo + index);
        bool edited = DescriptorSet::CopyAndReturnNotEqual(bufferInfo->buffer, buffer);
        edited |= DescriptorSet::CopyAndReturnNotEqual(bufferInfo->offset, offset);
        edited |= DescriptorSet::CopyAndReturnNotEqual(bufferInfo->range, range);
        const byte dynamicOffsetIndex = BindingToDynamicOffset[descriptorIndex];
        DynamicOffsets[dynamicOffsetIndex] = dynamicOffset;
        return edited;
    }

    bool WriteSampler(uint32 descriptorIndex, VkSampler sampler, uint32 index = 0) const
    {
        ASSERT(descriptorIndex < WritesCount);
        ASSERT(WriteDescriptors[descriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER || WriteDescriptors[descriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        auto* imageInfo = const_cast<VkDescriptorImageInfo*>(WriteDescriptors[descriptorIndex].pImageInfo + index);
        bool edited = DescriptorSet::CopyAndReturnNotEqual(imageInfo->sampler, sampler);
        return edited;
    }

    bool WriteImage(uint32 descriptorIndex, VkImageView imageView, VkImageLayout layout, uint32 index = 0) const
    {
        ASSERT(descriptorIndex < WritesCount);
        ASSERT(WriteDescriptors[descriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE || WriteDescriptors[descriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        auto* imageInfo = const_cast<VkDescriptorImageInfo*>(WriteDescriptors[descriptorIndex].pImageInfo + index);
        bool edited = DescriptorSet::CopyAndReturnNotEqual(imageInfo->imageView, imageView);
        edited |= DescriptorSet::CopyAndReturnNotEqual(imageInfo->imageLayout, layout);
        return edited;
    }

    bool WriteStorageImage(uint32 descriptorIndex, VkImageView imageView, VkImageLayout layout, uint32 index = 0) const
    {
        ASSERT(descriptorIndex < WritesCount);
        ASSERT(WriteDescriptors[descriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        auto* imageInfo = const_cast<VkDescriptorImageInfo*>(WriteDescriptors[descriptorIndex].pImageInfo + index);
        bool edited = DescriptorSet::CopyAndReturnNotEqual(imageInfo->imageView, imageView);
        edited |= DescriptorSet::CopyAndReturnNotEqual(imageInfo->imageLayout, layout);
        return edited;
    }

    bool WriteStorageTexelBuffer(uint32 descriptorIndex, VkBufferView bufferView, uint32 index = 0) const
    {
        ASSERT(descriptorIndex < WritesCount);
        ASSERT(WriteDescriptors[descriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER);
        auto* bufferInfo = const_cast<VkBufferView*>(WriteDescriptors[descriptorIndex].pTexelBufferView + index);
        *bufferInfo = bufferView;
        return true;
    }

    bool WriteStorageBuffer(uint32 descriptorIndex, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range, uint32 index = 0) const
    {
        ASSERT(descriptorIndex < WritesCount);
        ASSERT(WriteDescriptors[descriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER || WriteDescriptors[descriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC);
        auto* bufferInfo = const_cast<VkDescriptorBufferInfo*>(WriteDescriptors[descriptorIndex].pBufferInfo + index);
        bool edited = DescriptorSet::CopyAndReturnNotEqual(bufferInfo->buffer, buffer);
        edited |= DescriptorSet::CopyAndReturnNotEqual(bufferInfo->offset, offset);
        edited |= DescriptorSet::CopyAndReturnNotEqual(bufferInfo->range, range);
        return edited;
    }

    bool WriteUniformTexelBuffer(uint32 descriptorIndex, VkBufferView view, uint32 index = 0) const
    {
        ASSERT(descriptorIndex < WritesCount);
        ASSERT(WriteDescriptors[descriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER);
        auto* bufferInfo = const_cast<VkBufferView*>(WriteDescriptors[descriptorIndex].pTexelBufferView + index);
        return DescriptorSet::CopyAndReturnNotEqual(*bufferInfo, view);
    }

    void SetDescriptorSet(VkDescriptorSet descriptorSet) const
    {
        for (uint32 i = 0; i < WritesCount; i++)
            WriteDescriptors[i].dstSet = descriptorSet;
    }
};

#endif
