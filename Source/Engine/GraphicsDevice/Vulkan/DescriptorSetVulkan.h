// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/Platform.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "IncludeVulkanHeaders.h"
#include "Types.h"
#include "Config.h"
#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
#include "Engine/Platform/CriticalSection.h"
#endif

#if GRAPHICS_API_VULKAN

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
        NumGfxStages = 5,

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

    uint32 LayoutTypes[VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT];
    Array<SetLayout> SetLayouts;

    uint32 _hash = 0;

#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
    uint32 _typesUsageID = ~0;

    void CacheTypesUsageID();
#endif
    void AddDescriptor(int32 descriptorSetIndex, const VkDescriptorSetLayoutBinding& descriptor);

public:

    DescriptorSetLayoutInfoVulkan()
    {
        Platform::MemoryClear(LayoutTypes, sizeof(LayoutTypes));
    }

public:

    inline uint32 GetTypesUsed(VkDescriptorType type) const
    {
        return LayoutTypes[type];
    }

    const Array<SetLayout>& GetLayouts() const
    {
        return SetLayouts;
    }

    inline const uint32* GetLayoutTypes() const
    {
        return LayoutTypes;
    }

#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
    inline uint32 GetTypesUsageID() const
    {
        return _typesUsageID;
    }
#endif

public:

    void AddBindingsForStage(VkShaderStageFlagBits stageFlags, DescriptorSet::Stage descSet, const SpirvShaderDescriptorInfo* descriptorInfo);

    void CopyFrom(const DescriptorSetLayoutInfoVulkan& info)
    {
        Platform::MemoryCopy(LayoutTypes, info.LayoutTypes, sizeof(LayoutTypes));
        _hash = info._hash;
#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
        _typesUsageID = info._typesUsageID;
#endif
        SetLayouts = info.SetLayouts;
    }

    inline bool operator ==(const DescriptorSetLayoutInfoVulkan& other) const
    {
        if (other.SetLayouts.Count() != SetLayouts.Count())
        {
            return false;
        }

#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
        if (other._typesUsageID != _typesUsageID)
        {
            return false;
        }
#endif

        for (int32 index = 0; index < other.SetLayouts.Count(); index++)
        {
            const int32 numBindings = SetLayouts[index].LayoutBindings.Count();
            if (other.SetLayouts[index].LayoutBindings.Count() != numBindings)
            {
                return false;
            }

            if (numBindings != 0 && Platform::MemoryCompare(other.SetLayouts[index].LayoutBindings.Get(), SetLayouts[index].LayoutBindings.Get(), numBindings * sizeof(VkDescriptorSetLayoutBinding)))
            {
                return false;
            }
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
#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
    VkDescriptorSetAllocateInfo _allocateInfo;
#endif

public:

    DescriptorSetLayoutVulkan(GPUDeviceVulkan* device);
    ~DescriptorSetLayoutVulkan();

public:

    void Compile();

    inline const DescriptorSetLayoutHandlesArray& GetHandles() const
    {
        return _handles;
    }

#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
    inline const VkDescriptorSetAllocateInfo& GetAllocateInfo() const
    {
        return _allocateInfo;
    }
#endif

    friend inline uint32 GetHash(const DescriptorSetLayoutVulkan& key)
    {
        return key._hash;
    }
};

class DescriptorPoolVulkan
{
private:

    GPUDeviceVulkan* _device;
    VkDescriptorPool _handle;

    uint32 MaxDescriptorSets;
    uint32 NumAllocatedDescriptorSets;
    uint32 PeakAllocatedDescriptorSets;

#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
    const DescriptorSetLayoutVulkan& Layout;
#else
	int32 MaxAllocatedTypes[VK_DESCRIPTOR_TYPE_RANGE_SIZE];
	int32 NumAllocatedTypes[VK_DESCRIPTOR_TYPE_RANGE_SIZE];
	int32 PeakAllocatedTypes[VK_DESCRIPTOR_TYPE_RANGE_SIZE];
#endif

public:

#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
    DescriptorPoolVulkan(GPUDeviceVulkan* device, const DescriptorSetLayoutVulkan& layout);
#else
	DescriptorPoolVulkan(GPUDeviceVulkan* device);
#endif

    ~DescriptorPoolVulkan();

public:

    inline VkDescriptorPool GetHandle() const
    {
        return _handle;
    }

    inline bool IsEmpty() const
    {
        return NumAllocatedDescriptorSets == 0;
    }

    inline bool CanAllocate(const DescriptorSetLayoutVulkan& layout) const
    {
#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
        return MaxDescriptorSets > NumAllocatedDescriptorSets + layout.GetLayouts().Count();
#else
		for (uint32 typeIndex = VK_DESCRIPTOR_TYPE_BEGIN_RANGE; typeIndex < VK_DESCRIPTOR_TYPE_END_RANGE; typeIndex++)
		{
			if (NumAllocatedTypes[typeIndex] + (int32)layout.GetTypesUsed((VkDescriptorType)typeIndex) > MaxAllocatedTypes[typeIndex])
			{
				return false;
			}
		}
		return true;
#endif
    }

    void TrackAddUsage(const DescriptorSetLayoutVulkan& layout);

    void TrackRemoveUsage(const DescriptorSetLayoutVulkan& layout);

#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
    void Reset();

    bool AllocateDescriptorSets(const VkDescriptorSetAllocateInfo& descriptorSetAllocateInfo, VkDescriptorSet* result);

    inline uint32 GetNumAllocatedDescriptorSets() const
    {
        return NumAllocatedDescriptorSets;
    }
#endif
};

#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER

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

#endif

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
    Array<uint8> BindingToDynamicOffsetMap;

    void Release()
    {
        DescriptorImageInfo.Resize(0);
        DescriptorBufferInfo.Resize(0);
        DescriptorWrites.Resize(0);
        BindingToDynamicOffsetMap.Resize(0);
    }
};

#if !VULKAN_USE_DESCRIPTOR_POOL_MANAGER

class DescriptorSetsVulkan
{
public:

	typedef Array<VkDescriptorSet, FixedAllocation<DescriptorSet::NumGfxStages>> DescriptorSetArray;

private:

	GPUDeviceVulkan* _device;
	DescriptorPoolVulkan* _pool;
	const DescriptorSetLayoutVulkan* _layout;
	DescriptorSetArray _sets;

public:

	DescriptorSetsVulkan(GPUDeviceVulkan* device, const DescriptorSetLayoutVulkan& layout, GPUContextVulkan* context);
	~DescriptorSetsVulkan();

public:

	inline const DescriptorSetArray& GetHandles() const
	{
		return _sets;
	}

	inline void Bind(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout, VkPipelineBindPoint bindPoint, const Array<uint32>& dynamicOffsets) const
	{
		vkCmdBindDescriptorSets(cmdBuffer, bindPoint, pipelineLayout, 0, _sets.Count(), _sets.Get(), dynamicOffsets.Count(), dynamicOffsets.Get());
	}
};

class DescriptorSetRingBufferVulkan
{
private:

	GPUDeviceVulkan* _device;
	DescriptorSetsVulkan* _currDescriptorSets;

	struct DescriptorSetsPair
	{
		uint64 FenceCounter;
		DescriptorSetsVulkan* DescriptorSets;

		DescriptorSetsPair()
			: FenceCounter(0)
			, DescriptorSets(nullptr)
		{
		}
	};

	struct DescriptorSetsEntry
	{
		CmdBufferVulkan* CmdBuffer;
		Array<DescriptorSetsPair> Pairs;

		DescriptorSetsEntry(CmdBufferVulkan* cmdBuffer)
			: CmdBuffer(cmdBuffer)
		{
		}

		~DescriptorSetsEntry()
		{
			for (auto& pair : Pairs)
			{
				Delete(pair.DescriptorSets);
			}
		}
	};

	Array<DescriptorSetsEntry*> DescriptorSetsEntries;

public:

	DescriptorSetRingBufferVulkan(GPUDeviceVulkan* device);

	virtual ~DescriptorSetRingBufferVulkan()
	{
	}

public:

	void Reset()
	{
		_currDescriptorSets = nullptr;
	}

	void Release()
	{
		DescriptorSetsEntries.ClearDelete();
	}

	void Set(DescriptorSetsVulkan* newDescriptorSets)
	{
		_currDescriptorSets = newDescriptorSets;
	}

	inline void Bind(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout, VkPipelineBindPoint bindPoint, const Array<uint32>& dynamicOffsets)
	{
		ASSERT(_currDescriptorSets);
		_currDescriptorSets->Bind(cmdBuffer, pipelineLayout, bindPoint, dynamicOffsets);
	}

	DescriptorSetsVulkan* RequestDescriptorSets(GPUContextVulkan* context, CmdBufferVulkan* cmdBuffer, const PipelineLayoutVulkan* layout);
};

#endif

class DescriptorSetWriterVulkan
{
public:

    VkWriteDescriptorSet* WriteDescriptors;
    uint8* BindingToDynamicOffsetMap;
    uint32* DynamicOffsets;
    uint32 NumWrites;

public:

    DescriptorSetWriterVulkan()
        : WriteDescriptors(nullptr)
        , BindingToDynamicOffsetMap(nullptr)
        , DynamicOffsets(nullptr)
        , NumWrites(0)
    {
    }

public:

    uint32 SetupDescriptorWrites(const SpirvShaderDescriptorInfo& info, VkWriteDescriptorSet* writeDescriptors, VkDescriptorImageInfo* imageInfo, VkDescriptorBufferInfo* bufferInfo, uint8* bindingToDynamicOffsetMap);

    bool WriteUniformBuffer(uint32 descriptorIndex, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range) const
    {
        ASSERT(descriptorIndex < NumWrites);
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
        ASSERT(descriptorIndex < NumWrites);
        ASSERT(WriteDescriptors[descriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
        VkDescriptorBufferInfo* bufferInfo = const_cast<VkDescriptorBufferInfo*>(WriteDescriptors[descriptorIndex].pBufferInfo);
        ASSERT(bufferInfo);
        bool edited = DescriptorSet::CopyAndReturnNotEqual(bufferInfo->buffer, buffer);
        edited |= DescriptorSet::CopyAndReturnNotEqual(bufferInfo->offset, offset);
        edited |= DescriptorSet::CopyAndReturnNotEqual(bufferInfo->range, range);
        const uint8 dynamicOffsetIndex = BindingToDynamicOffsetMap[descriptorIndex];
        DynamicOffsets[dynamicOffsetIndex] = dynamicOffset;
        return edited;
    }

    bool WriteSampler(uint32 descriptorIndex, VkSampler sampler) const
    {
        ASSERT(descriptorIndex < NumWrites);
        ASSERT(WriteDescriptors[descriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER || WriteDescriptors[descriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        VkDescriptorImageInfo* imageInfo = const_cast<VkDescriptorImageInfo*>(WriteDescriptors[descriptorIndex].pImageInfo);
        ASSERT(imageInfo);
        bool edited = DescriptorSet::CopyAndReturnNotEqual(imageInfo->sampler, sampler);
        return edited;
    }

    bool WriteImage(uint32 descriptorIndex, VkImageView imageView, VkImageLayout layout) const
    {
        ASSERT(descriptorIndex < NumWrites);
        ASSERT(WriteDescriptors[descriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE || WriteDescriptors[descriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        VkDescriptorImageInfo* imageInfo = const_cast<VkDescriptorImageInfo*>(WriteDescriptors[descriptorIndex].pImageInfo);
        ASSERT(imageInfo);
        bool edited = DescriptorSet::CopyAndReturnNotEqual(imageInfo->imageView, imageView);
        edited |= DescriptorSet::CopyAndReturnNotEqual(imageInfo->imageLayout, layout);
        return edited;
    }

    bool WriteStorageImage(uint32 descriptorIndex, VkImageView imageView, VkImageLayout layout) const
    {
        ASSERT(descriptorIndex < NumWrites);
        ASSERT(WriteDescriptors[descriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        VkDescriptorImageInfo* imageInfo = const_cast<VkDescriptorImageInfo*>(WriteDescriptors[descriptorIndex].pImageInfo);
        ASSERT(imageInfo);
        bool edited = DescriptorSet::CopyAndReturnNotEqual(imageInfo->imageView, imageView);
        edited |= DescriptorSet::CopyAndReturnNotEqual(imageInfo->imageLayout, layout);
        return edited;
    }

    bool WriteStorageTexelBuffer(uint32 descriptorIndex, const VkBufferView* bufferView) const
    {
        ASSERT(descriptorIndex < NumWrites);
        ASSERT(WriteDescriptors[descriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER);
        WriteDescriptors[descriptorIndex].pTexelBufferView = bufferView;
        return true;
    }

    bool WriteStorageBuffer(uint32 descriptorIndex, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range) const
    {
        ASSERT(descriptorIndex < NumWrites);
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
        ASSERT(descriptorIndex < NumWrites);
        ASSERT(WriteDescriptors[descriptorIndex].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER);
        return DescriptorSet::CopyAndReturnNotEqual(WriteDescriptors[descriptorIndex].pTexelBufferView, view);
    }

    void SetDescriptorSet(VkDescriptorSet descriptorSet) const
    {
        for (uint32 i = 0; i < NumWrites; i++)
        {
            WriteDescriptors[i].dstSet = descriptorSet;
        }
    }
};

#endif
