// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_VULKAN

#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPUResource.h"
#include "DescriptorSetVulkan.h"
#include "IncludeVulkanHeaders.h"
#include "Config.h"

enum class SpirvShaderResourceType;
enum class GPUTextureFlags;
class Engine;
class GPUContextVulkan;
class GPUAdapterVulkan;
class GPUSwapChainVulkan;
class CmdBufferVulkan;
class QueueVulkan;
class FenceVulkan;
class GPUTextureVulkan;
class GPUBufferVulkan;
class GPUTimerQueryVulkan;
class RenderPassVulkan;
class FenceManagerVulkan;
class GPUDeviceVulkan;
class UniformBufferUploaderVulkan;
class DescriptorPoolsManagerVulkan;

class SemaphoreVulkan
{
private:

    GPUDeviceVulkan* _device;
    VkSemaphore _semaphoreHandle;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="SemaphoreVulkan"/> class.
    /// </summary>
    /// <param name="device">The graphics device.</param>
    SemaphoreVulkan(GPUDeviceVulkan* device);

    /// <summary>
    /// Finalizes an instance of the <see cref="SemaphoreVulkan"/> class.
    /// </summary>
    ~SemaphoreVulkan();

    /// <summary>
    /// Gets the handle.
    /// </summary>
    /// <returns>The semaphore.</returns>
    inline VkSemaphore GetHandle() const
    {
        return _semaphoreHandle;
    }
};

class FenceVulkan
{
    friend FenceManagerVulkan;

private:

    VkFence _handle;
    bool _signaled;
    FenceManagerVulkan* _owner;

public:

    FenceVulkan(GPUDeviceVulkan* device, FenceManagerVulkan* owner, bool createSignaled);

    ~FenceVulkan();

public:

    inline VkFence GetHandle() const
    {
        return _handle;
    }

    inline bool IsSignaled() const
    {
        return _signaled;
    }

    FenceManagerVulkan* GetOwner() const
    {
        return _owner;
    }
};

class FenceManagerVulkan
{
private:

    GPUDeviceVulkan* _device;
    Array<FenceVulkan*> _freeFences;
    Array<FenceVulkan*> _usedFences;

public:

    FenceManagerVulkan()
        : _device(nullptr)
    {
    }

    ~FenceManagerVulkan();

public:

    /// <summary>
    /// Initializes the specified device.
    /// </summary>
    /// <param name="device">The graphics device.</param>
    void Init(GPUDeviceVulkan* device)
    {
        _device = device;
    }

    void Dispose();

    FenceVulkan* AllocateFence(bool createSignaled = false);

    inline bool IsFenceSignaled(FenceVulkan* fence)
    {
        if (fence->IsSignaled())
        {
            return true;
        }

        return CheckFenceState(fence);
    }

    // Returns true if waiting timed out or failed, false otherwise.
    bool WaitForFence(FenceVulkan* fence, uint64 timeInNanoseconds);

    void ResetFence(FenceVulkan* fence);

    // Sets the fence handle to null
    void ReleaseFence(FenceVulkan*& fence);

    // Sets the fence handle to null
    void WaitAndReleaseFence(FenceVulkan*& fence, uint64 timeInNanoseconds);

private:

    // Returns true if fence was signaled, otherwise false.
    bool CheckFenceState(FenceVulkan* fence);

    void DestroyFence(FenceVulkan* fence);
};

class DeferredDeletionQueueVulkan
{
public:

    enum Type
    {
        RenderPass,
        Buffer,
        BufferView,
        Image,
        ImageView,
        Pipeline,
        PipelineLayout,
        Framebuffer,
        DescriptorSetLayout,
        Sampler,
        Semaphore,
        ShaderModule,
        Event,
        QueryPool,
    };

private:

    struct Entry
    {
        uint64 FenceCounter;
        uint64 Handle;
        uint64 FrameNumber;
        VmaAllocation AllocationHandle;
        Type StructureType;
        CmdBufferVulkan* CmdBuffer;
    };

    GPUDeviceVulkan* _device;
    CriticalSection _locker;
    Array<Entry> _entries;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="DeferredDeletionQueueVulkan"/> class.
    /// </summary>
    /// <param name="device">The graphics device.</param>
    DeferredDeletionQueueVulkan(GPUDeviceVulkan* device);

    /// <summary>
    /// Finalizes an instance of the <see cref="DeferredDeletionQueueVulkan"/> class.
    /// </summary>
    ~DeferredDeletionQueueVulkan();

public:

    template<typename T>
    inline void EnqueueResource(Type type, T handle)
    {
        static_assert(sizeof(T) <= sizeof(uint64), "Invalid handle size.");
        EnqueueGenericResource(type, (uint64)handle, VK_NULL_HANDLE);
    }

    template<typename T>
    inline void EnqueueResource(Type type, T handle, VmaAllocation allocation)
    {
        static_assert(sizeof(T) <= sizeof(uint64), "Invalid handle size.");
        EnqueueGenericResource(type, (uint64)handle, allocation);
    }

    auto ReleaseResources(bool deleteImmediately = false) -> void;

private:

    void EnqueueGenericResource(Type type, uint64 handle, VmaAllocation allocation);
};

class RenderTargetLayoutVulkan
{
public:

    int32 RTsCount;
    MSAALevel MSAA;
    bool ReadDepth;
    bool WriteDepth;
    PixelFormat DepthFormat;
    PixelFormat RTVsFormats[GPU_MAX_RT_BINDED];
    VkExtent3D Extent;

public:

    bool operator==(const RenderTargetLayoutVulkan& other) const
    {
        return Platform::MemoryCompare((void*)this, &other, sizeof(RenderTargetLayoutVulkan)) == 0;
    }
};

uint32 GetHash(const RenderTargetLayoutVulkan& key);

class FramebufferVulkan
{
public:

    struct Key
    {
        RenderPassVulkan* RenderPass;
        int32 AttachmentCount;
        VkImageView Attachments[GPU_MAX_RT_BINDED + 1];

    public:

        bool operator==(const Key& other) const
        {
            return Platform::MemoryCompare((void*)this, &other, sizeof(Key)) == 0;
        }
    };

private:

    GPUDeviceVulkan* _device;
    VkFramebuffer _handle;

public:

    FramebufferVulkan(GPUDeviceVulkan* device, Key& key, VkExtent3D& extent, uint32 layers);
    ~FramebufferVulkan();

public:

    VkImageView Attachments[GPU_MAX_RT_BINDED + 1];
    VkExtent3D Extent;
    uint32 Layers;

public:

    inline VkFramebuffer GetHandle()
    {
        return _handle;
    }

    bool HasReference(VkImageView imageView) const;
};

uint32 GetHash(const FramebufferVulkan::Key& key);

class RenderPassVulkan
{
private:

    GPUDeviceVulkan* _device;
    VkRenderPass _handle;

public:

    RenderTargetLayoutVulkan Layout;

public:

    RenderPassVulkan(GPUDeviceVulkan* device, const RenderTargetLayoutVulkan& layout);
    ~RenderPassVulkan();

public:

    inline VkRenderPass GetHandle() const
    {
        return _handle;
    }
};

class QueryPoolVulkan
{
protected:

    struct Range
    {
        uint32 Start;
        uint32 Count;
    };

    GPUDeviceVulkan* _device;
    VkQueryPool _handle;

    volatile int32 _count;
    const uint32 _capacity;
    const VkQueryType _type;
#if VULKAN_RESET_QUERY_POOLS
    Array<Range> _resetRanges;
#endif

public:

    QueryPoolVulkan(GPUDeviceVulkan* device, int32 capacity, VkQueryType type);
    ~QueryPoolVulkan();

public:

    inline VkQueryPool GetHandle() const
    {
        return _handle;
    }

#if VULKAN_RESET_QUERY_POOLS
    void Reset(CmdBufferVulkan* cmdBuffer);
#endif
};

class BufferedQueryPoolVulkan : public QueryPoolVulkan
{
private:

    Array<uint64> _queryOutput;
    Array<uint64> _usedQueryBits;
    Array<uint64> _startedQueryBits;
    Array<uint64> _readResultsBits;

    // Last potentially free index in the pool
    int32 _lastBeginIndex;

public:

    BufferedQueryPoolVulkan(GPUDeviceVulkan* device, int32 capacity, VkQueryType type);
    bool AcquireQuery(uint32& resultIndex);
    void ReleaseQuery(uint32 queryIndex);
    void MarkQueryAsStarted(uint32 queryIndex);
    bool GetResults(GPUContextVulkan* context, uint32 index, uint64& result);
    bool HasRoom() const;
};

/// <summary>
/// The dummy Vulkan resources manager. Helps when user need to pass null texture handle to the shader.
/// </summary>
class HelperResourcesVulkan
{
public:

    enum class StaticSamplers
    {
        SamplerLinearClamp =0,
        SamplerPointClamp = 1,
        SamplerLinearWrap = 2,
        SamplerPointWrap = 3,
        ShadowSampler = 4,
        ShadowSamplerPCF = 5,

        MAX
    };

private:

    GPUDeviceVulkan* _device;
    GPUTextureVulkan* _dummyTextures[6];
    GPUBufferVulkan* _dummyBuffer;
    GPUBufferVulkan* _dummyVB;
    VkSampler _staticSamplers[static_cast<int32>(StaticSamplers::MAX)];

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="DummyResourcesVulkan"/> class.
    /// </summary>
    /// <param name="device">The graphics device.</param>
    HelperResourcesVulkan(GPUDeviceVulkan* device);

public:

    VkSampler GetStaticSampler(StaticSamplers type);

    GPUTextureVulkan* GetDummyTexture(SpirvShaderResourceType type);

    GPUBufferVulkan* GetDummyBuffer();

    GPUBufferVulkan* GetDummyVertexBuffer();

    void Dispose();
};

/// <summary>
/// Vulkan staging buffers manager.
/// </summary>
class StagingManagerVulkan
{
private:

    struct PendingItems
    {
        uint64 FenceCounter;
        Array<GPUBuffer*> Resources;
    };

    struct PendingItemsPerCmdBuffer
    {
        CmdBufferVulkan* CmdBuffer;
        Array<PendingItems> Items;

        inline PendingItems* FindOrAdd(uint64 fence);
    };

    struct FreeEntry
    {
        GPUBuffer* Buffer;
        uint64 FrameNumber;
    };

    GPUDeviceVulkan* _device;
    CriticalSection _locker;
    Array<GPUBuffer*> _allBuffers;
    Array<FreeEntry> _freeBuffers;
    Array<PendingItemsPerCmdBuffer> _pendingBuffers;
#if !BUILD_RELEASE
    uint64 _allBuffersTotalSize = 0;
    uint64 _allBuffersPeekSize = 0;
    uint64 _allBuffersAllocSize = 0;
    uint64 _allBuffersFreeSize = 0;
#endif

public:

    StagingManagerVulkan(GPUDeviceVulkan* device);
    GPUBuffer* AcquireBuffer(uint32 size, GPUResourceUsage usage);
    void ReleaseBuffer(CmdBufferVulkan* cmdBuffer, GPUBuffer*& buffer);
    void ProcessPendingFree();
    void Dispose();

private:

    PendingItemsPerCmdBuffer* FindOrAdd(CmdBufferVulkan* cmdBuffer);
};

/// <summary>
/// Implementation of Graphics Device for Vulkan backend.
/// </summary>
class GPUDeviceVulkan : public GPUDevice
{
    friend GPUContextVulkan;
    friend GPUSwapChainVulkan;
    friend FenceManagerVulkan;

private:

    CriticalSection _fenceLock;

    Dictionary<RenderTargetLayoutVulkan, RenderPassVulkan*> _renderPasses;
    Dictionary<FramebufferVulkan::Key, FramebufferVulkan*> _framebuffers;
    Dictionary<DescriptorSetLayoutInfoVulkan, PipelineLayoutVulkan*> _layouts;
    // TODO: use mutex to protect those collections BUT use 2 pools per cache: one lock-free with lookup only and second protected with mutex synced on frame end!

public:

    // Create new graphics device (returns Vulkan if failed)
    // @returns Created device or Vulkan
    static GPUDevice* Create();

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUDeviceVulkan"/> class.
    /// </summary>
    /// <param name="shaderProfile">The shader profile.</param>
    /// <param name="adapter">The GPU device adapter.</param>
    GPUDeviceVulkan(ShaderProfile shaderProfile, GPUAdapterVulkan* adapter);

    /// <summary>
    /// Finalizes an instance of the <see cref="GPUDeviceVulkan"/> class.
    /// </summary>
    ~GPUDeviceVulkan();

public:

    struct OptionalVulkanDeviceExtensions
    {
        uint32 HasKHRMaintenance1 : 1;
        uint32 HasKHRMaintenance2 : 1;
        uint32 HasMirrorClampToEdge : 1;
        uint32 HasKHRExternalMemoryCapabilities : 1;
        uint32 HasKHRGetPhysicalDeviceProperties2 : 1;
        uint32 HasEXTValidationCache : 1;
    };

    static void GetInstanceLayersAndExtensions(Array<const char*>& outInstanceExtensions, Array<const char*>& outInstanceLayers, bool& outDebugUtils);
    static void GetDeviceExtensionsAndLayers(VkPhysicalDevice gpu, Array<const char*>& outDeviceExtensions, Array<const char*>& outDeviceLayers);

    void ParseOptionalDeviceExtensions(const Array<const char*>& deviceExtensions);
    static OptionalVulkanDeviceExtensions OptionalDeviceExtensions;

public:

    /// <summary>
    /// The Vulkan instance.
    /// </summary>
    static VkInstance Instance;

    /// <summary>
    /// The Vulkan instance extensions.
    /// </summary>
    static Array<const char*> InstanceExtensions;

    /// <summary>
    /// The Vulkan instance layers.
    /// </summary>
    static Array<const char*> InstanceLayers;

public:

    /// <summary>
    /// The main Vulkan commands context.
    /// </summary>
    GPUContextVulkan* MainContext = nullptr;

    /// <summary>
    /// The Vulkan adapter.
    /// </summary>
    GPUAdapterVulkan* Adapter = nullptr;

    /// <summary>
    /// The Vulkan device.
    /// </summary>
    VkDevice Device = VK_NULL_HANDLE;

    /// <summary>
    /// The Vulkan device queues family properties.
    /// </summary>
    Array<VkQueueFamilyProperties> QueueFamilyProps;

    /// <summary>
    /// The Vulkan fence manager.
    /// </summary>
    FenceManagerVulkan FenceManager;

    /// <summary>
    /// The Vulkan resources deferred deletion queue.
    /// </summary>
    DeferredDeletionQueueVulkan DeferredDeletionQueue;

    /// <summary>
    /// The staging buffers manager.
    /// </summary>
    StagingManagerVulkan StagingManager;

    /// <summary>
    /// The helper device resources manager.
    /// </summary>
    HelperResourcesVulkan HelperResources;

    /// <summary>
    /// The graphics queue.
    /// </summary>
    QueueVulkan* GraphicsQueue = nullptr;

    /// <summary>
    /// The compute queue.
    /// </summary>
    QueueVulkan* ComputeQueue = nullptr;

    /// <summary>
    /// The transfer queue.
    /// </summary>
    QueueVulkan* TransferQueue = nullptr;

    /// <summary>
    /// The present queue.
    /// </summary>
    QueueVulkan* PresentQueue = nullptr;

    /// <summary>
    /// The Vulkan memory allocator.
    /// </summary>
    VmaAllocator Allocator = VK_NULL_HANDLE;

    /// <summary>
    /// The pipeline cache.
    /// </summary>
    VkPipelineCache PipelineCache = VK_NULL_HANDLE;

#if VK_EXT_validation_cache

    /// <summary>
    /// The optional validation cache.
    /// </summary>
    VkValidationCacheEXT ValidationCache = VK_NULL_HANDLE;

#endif

    /// <summary>
    /// The uniform buffers uploader.
    /// </summary>
    UniformBufferUploaderVulkan* UniformBufferUploader = nullptr;

    /// <summary>
    /// The descriptor pools manager.
    /// </summary>
    DescriptorPoolsManagerVulkan* DescriptorPoolsManager = nullptr;

    /// <summary>
    /// The physical device limits.
    /// </summary>
    VkPhysicalDeviceLimits PhysicalDeviceLimits;

    /// <summary>
    /// The physical device enabled features.
    /// </summary>
    VkPhysicalDeviceFeatures PhysicalDeviceFeatures;

    Array<BufferedQueryPoolVulkan*> TimestampQueryPools;

#if VULKAN_RESET_QUERY_POOLS
    Array<QueryPoolVulkan*> QueriesToReset;
#endif

    inline BufferedQueryPoolVulkan* FindAvailableQueryPool(Array<BufferedQueryPoolVulkan*>& pools, VkQueryType queryType)
    {
        // Try to use pool with available space inside
        for (int32 i = 0; i < pools.Count(); i++)
        {
            auto pool = pools[i];
            if (pool->HasRoom())
            {
                return pool;
            }
        }

        // Create new pool
        enum
        {
            NUM_OCCLUSION_QUERIES_PER_POOL = 4096,
            NUM_TIMESTAMP_QUERIES_PER_POOL = 1024,
        };
        const auto pool = New<BufferedQueryPoolVulkan>(this, queryType == VK_QUERY_TYPE_OCCLUSION ? NUM_OCCLUSION_QUERIES_PER_POOL : NUM_TIMESTAMP_QUERIES_PER_POOL, queryType);
        pools.Add(pool);
        return pool;
    }

    inline BufferedQueryPoolVulkan* FindAvailableTimestampQueryPool()
    {
        return FindAvailableQueryPool(TimestampQueryPools, VK_QUERY_TYPE_TIMESTAMP);
    }

    RenderPassVulkan* GetOrCreateRenderPass(RenderTargetLayoutVulkan& layout);
    FramebufferVulkan* GetOrCreateFramebuffer(FramebufferVulkan::Key& key, VkExtent3D& extent, uint32 layers);
    PipelineLayoutVulkan* GetOrCreateLayout(DescriptorSetLayoutInfoVulkan& key);
    void OnImageViewDestroy(VkImageView imageView);

public:

    /// <summary>
    /// Setups the present queue to be ready for the given window surface.
    /// </summary>
    /// <param name="surface">The surface.</param>
    void SetupPresentQueue(VkSurfaceKHR surface);

    /// <summary>
    /// Finds the closest pixel format that a specific Vulkan device supports.
    /// </summary>
    /// <param name="format">The input format.</param>
    /// <param name="flags">The texture usage flags.</param>
    /// <param name="optimalTiling">If set to <c>true</c> the optimal tiling should be used, otherwise use linear tiling.</param>
    /// <returns>The output format.</returns>
    PixelFormat GetClosestSupportedPixelFormat(PixelFormat format, GPUTextureFlags flags, bool optimalTiling);

#if VK_EXT_validation_cache

    /// <summary>
    /// Loads the validation cache.
    /// </summary>
    void LoadValidationCache();

    /// <summary>
    /// Saves the validation cache.
    /// </summary>
    bool SaveValidationCache();

#endif

public:

    // [GPUDevice]
    GPUContext* GetMainContext() override;
    GPUAdapter* GetAdapter() const override;
    void* GetNativePtr() const override;
    bool Init() override;
    void DrawBegin() override;
    void Dispose() override;
    void WaitForGPU() override;
    GPUTexture* CreateTexture(const StringView& name) override;
    GPUShader* CreateShader(const StringView& name) override;
    GPUPipelineState* CreatePipelineState() override;
    GPUTimerQuery* CreateTimerQuery() override;
    GPUBuffer* CreateBuffer(const StringView& name) override;
    GPUSwapChain* CreateSwapChain(Window* window) override;
};

/// <summary>
/// GPU resource implementation for Vulkan backend.
/// </summary>
template<class BaseType>
class GPUResourceVulkan : public GPUResourceBase<GPUDeviceVulkan, BaseType>
{
public:

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUResourceVulkan"/> class.
    /// </summary>
    /// <param name="device">The graphics device.</param>
    /// <param name="name">The resource name.</param>
    GPUResourceVulkan(GPUDeviceVulkan* device, const StringView& name)
        : GPUResourceBase<GPUDeviceVulkan, BaseType>(device, name)
    {
    }
};

/// <summary>
/// Represents a GPU resource that contain descriptor resource for binding to the pipeline (shader resource, sampler, buffer, etc.).
/// </summary>
class DescriptorOwnerResourceVulkan
{
public:

    /// <summary>
    /// Finalizes an instance of the <see cref="DescriptorOwnerResourceVulkan"/> class.
    /// </summary>
    virtual ~DescriptorOwnerResourceVulkan()
    {
    }

public:

    /// <summary>
    /// Gets the sampler descriptor.
    /// </summary>
    /// <param name="context">The GPU context. Can be sued to add memory barriers to the pipeline before binding the descriptor to the pipeline.</param>
    /// <param name="sampler">The sampler.</param>
    virtual void DescriptorAsSampler(GPUContextVulkan* context, VkSampler& sampler)
    {
        CRASH;
    }

    /// <summary>
    /// Gets the image descriptor.
    /// </summary>
    /// <param name="context">The GPU context. Can be sued to add memory barriers to the pipeline before binding the descriptor to the pipeline.</param>
    /// <param name="imageView">The image view.</param>
    /// <param name="layout">The image layout.</param>
    virtual void DescriptorAsImage(GPUContextVulkan* context, VkImageView& imageView, VkImageLayout& layout)
    {
        CRASH;
    }

    /// <summary>
    /// Gets the storage image descriptor.
    /// </summary>
    /// <param name="context">The GPU context. Can be sued to add memory barriers to the pipeline before binding the descriptor to the pipeline.</param>
    /// <param name="imageView">The image view.</param>
    /// <param name="layout">The image layout.</param>
    virtual void DescriptorAsStorageImage(GPUContextVulkan* context, VkImageView& imageView, VkImageLayout& layout)
    {
        CRASH;
    }

    /// <summary>
    /// Gets the uniform texel buffer descriptor.
    /// </summary>
    /// <param name="context">The GPU context. Can be sued to add memory barriers to the pipeline before binding the descriptor to the pipeline.</param>
    /// <param name="bufferView">The buffer view.</param>
    virtual void DescriptorAsUniformTexelBuffer(GPUContextVulkan* context, const VkBufferView*& bufferView)
    {
        CRASH;
    }

    /// <summary>
    /// Gets the storage buffer descriptor.
    /// </summary>
    /// <param name="context">The GPU context. Can be sued to add memory barriers to the pipeline before binding the descriptor to the pipeline.</param>
    /// <param name="buffer">The buffer.</param>
    /// <param name="offset">The range offset (in bytes).</param>
    /// <param name="range">The range size (in bytes).</param>
    virtual void DescriptorAsStorageBuffer(GPUContextVulkan* context, VkBuffer& buffer, VkDeviceSize& offset, VkDeviceSize& range)
    {
        CRASH;
    }

    /// <summary>
    /// Gets the dynamic uniform buffer descriptor.
    /// </summary>
    /// <param name="context">The GPU context. Can be sued to add memory barriers to the pipeline before binding the descriptor to the pipeline.</param>
    /// <param name="buffer">The buffer.</param>
    /// <param name="offset">The range offset (in bytes).</param>
    /// <param name="range">The range size (in bytes).</param>
    /// <param name="dynamicOffset">The dynamic offset (in bytes).</param>
    virtual void DescriptorAsDynamicUniformBuffer(GPUContextVulkan* context, VkBuffer& buffer, VkDeviceSize& offset, VkDeviceSize& range, uint32& dynamicOffset)
    {
        CRASH;
    }
};

extern GPUDevice* CreateGPUDeviceVulkan();

#endif
