// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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

struct FenceVulkan
{
    VkFence Handle;
    bool IsSignaled;
};

class FenceManagerVulkan
{
private:
    GPUDeviceVulkan* _device = nullptr;
    Array<FenceVulkan*> _freeFences;
    Array<FenceVulkan*> _usedFences;

public:
    ~FenceManagerVulkan();

public:
    void Init(GPUDeviceVulkan* device)
    {
        _device = device;
    }

    void Dispose();

    FenceVulkan* AllocateFence(bool createSignaled = false);

    FORCE_INLINE bool IsFenceSignaled(FenceVulkan* fence) const
    {
        return fence->IsSignaled || CheckFenceState(fence);
    }

    // Returns true if waiting timed out or failed, false otherwise.
    bool WaitForFence(FenceVulkan* fence, uint64 timeInNanoseconds) const;

    void ResetFence(FenceVulkan* fence) const;

    // Sets the fence handle to null
    void ReleaseFence(FenceVulkan*& fence);

    // Sets the fence handle to null
    void WaitAndReleaseFence(FenceVulkan*& fence, uint64 timeInNanoseconds);

private:
    // Returns true if fence was signaled, otherwise false.
    bool CheckFenceState(FenceVulkan* fence) const;

    void DestroyFence(FenceVulkan* fence) const;
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

    void ReleaseResources(bool immediately = false);

private:
    void EnqueueGenericResource(Type type, uint64 handle, VmaAllocation allocation);
};

struct RenderTargetLayoutVulkan
{
    union
    {
        struct
        {
            uint32 Layers : 10; // Limited by GPU_MAX_TEXTURE_ARRAY_SIZE
            uint32 RTsCount : 3; // Limited by GPU_MAX_RT_BINDED
            uint32 ReadDepth : 1;
            uint32 WriteDepth : 1;
            uint32 ReadStencil : 1;
            uint32 WriteStencil : 1;
            uint32 BlendEnable : 1;
        };

        uint32 Flags;
    };

    MSAALevel MSAA;
    PixelFormat DepthFormat;
    PixelFormat RTVsFormats[GPU_MAX_RT_BINDED];
    VkExtent2D Extent;

    FORCE_INLINE bool operator==(const RenderTargetLayoutVulkan& other) const
    {
        return Platform::MemoryCompare(this, &other, sizeof(RenderTargetLayoutVulkan)) == 0;
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

        FORCE_INLINE bool operator==(const Key& other) const
        {
            return Platform::MemoryCompare(this, &other, sizeof(Key)) == 0;
        }
    };

    FramebufferVulkan(GPUDeviceVulkan* device, const Key& key, const VkExtent2D& extent, uint32 layers);
    ~FramebufferVulkan();

    GPUDeviceVulkan* Device;
    VkFramebuffer Handle;
    VkImageView Attachments[GPU_MAX_RT_BINDED + 1];
    VkExtent2D Extent;
    uint32 Layers;

    bool HasReference(VkImageView imageView) const;
};

uint32 GetHash(const FramebufferVulkan::Key& key);

class RenderPassVulkan
{
public:
    GPUDeviceVulkan* Device;
    VkRenderPass Handle;
    RenderTargetLayoutVulkan Layout;
#if VULKAN_USE_DEBUG_DATA
    VkRenderPassCreateInfo DebugCreateInfo;
#endif

    RenderPassVulkan(GPUDeviceVulkan* device, const RenderTargetLayoutVulkan& layout);
    ~RenderPassVulkan();
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
    bool ResetBeforeUse;
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
    bool AcquireQuery(CmdBufferVulkan* cmdBuffer, uint32& resultIndex);
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
private:
    GPUDeviceVulkan* _device;
    GPUTextureVulkan* _dummyTextures[6];
    GPUBufferVulkan* _dummyBuffer;
    GPUBufferVulkan* _dummyVB;
    VkSampler _staticSamplers[GPU_STATIC_SAMPLERS_COUNT];

public:
    HelperResourcesVulkan(GPUDeviceVulkan* device);

public:
    VkSampler* GetStaticSamplers();
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
    struct PendingEntry
    {
        GPUBuffer* Buffer;
        CmdBufferVulkan* CmdBuffer;
        uint64 FenceCounter;
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
    Array<PendingEntry> _pendingBuffers;
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
    mutable void* _nativePtr[2];

    Dictionary<RenderTargetLayoutVulkan, RenderPassVulkan*> _renderPasses;
    Dictionary<FramebufferVulkan::Key, FramebufferVulkan*> _framebuffers;
    Dictionary<DescriptorSetLayoutInfoVulkan, PipelineLayoutVulkan*> _layouts;
    // TODO: use mutex to protect those collections BUT use 2 pools per cache: one lock-free with lookup only and second protected with mutex synced on frame end!

public:
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
#if VULKAN_USE_VALIDATION_CACHE
        uint32 HasEXTValidationCache : 1;
#endif
    };

    static OptionalVulkanDeviceExtensions OptionalDeviceExtensions;

private:
    static void GetInstanceLayersAndExtensions(Array<const char*>& outInstanceExtensions, Array<const char*>& outInstanceLayers, bool& outDebugUtils);
    void GetDeviceExtensionsAndLayers(VkPhysicalDevice gpu, Array<const char*>& outDeviceExtensions, Array<const char*>& outDeviceLayers);
    static void ParseOptionalDeviceExtensions(const Array<const char*>& deviceExtensions);

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

#if VULKAN_USE_VALIDATION_CACHE
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
            auto pool = pools.Get()[i];
            if (pool->HasRoom())
                return pool;
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
    FramebufferVulkan* GetOrCreateFramebuffer(FramebufferVulkan::Key& key, VkExtent2D& extent, uint32 layers);
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

    /// <summary>
    /// Saves the pipeline cache.
    /// </summary>
    bool SavePipelineCache();

#if VK_EXT_validation_cache
    /// <summary>
    /// Saves the validation cache.
    /// </summary>
    bool SaveValidationCache();
#endif

private:
    bool IsVkFormatSupported(VkFormat vkFormat, VkFormatFeatureFlags wantedFeatureFlags, bool optimalTiling) const;

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
    GPUSampler* CreateSampler() override;
    GPUSwapChain* CreateSwapChain(Window* window) override;
    GPUConstantBuffer* CreateConstantBuffer(uint32 size, const StringView& name) override;
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
    /// <param name="context">The GPU context. Can be used to add memory barriers to the pipeline before binding the descriptor to the pipeline.</param>
    /// <param name="sampler">The sampler.</param>
    virtual void DescriptorAsSampler(GPUContextVulkan* context, VkSampler& sampler)
    {
        CRASH;
    }

    /// <summary>
    /// Gets the image descriptor.
    /// </summary>
    /// <param name="context">The GPU context. Can be used to add memory barriers to the pipeline before binding the descriptor to the pipeline.</param>
    /// <param name="imageView">The image view.</param>
    /// <param name="layout">The image layout.</param>
    virtual void DescriptorAsImage(GPUContextVulkan* context, VkImageView& imageView, VkImageLayout& layout)
    {
        CRASH;
    }

    /// <summary>
    /// Gets the storage image descriptor (VK_DESCRIPTOR_TYPE_STORAGE_IMAGE).
    /// </summary>
    /// <param name="context">The GPU context. Can be used to add memory barriers to the pipeline before binding the descriptor to the pipeline.</param>
    /// <param name="imageView">The image view.</param>
    /// <param name="layout">The image layout.</param>
    virtual void DescriptorAsStorageImage(GPUContextVulkan* context, VkImageView& imageView, VkImageLayout& layout)
    {
        CRASH;
    }

    /// <summary>
    /// Gets the uniform texel buffer descriptor (VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER).
    /// </summary>
    /// <param name="context">The GPU context. Can be used to add memory barriers to the pipeline before binding the descriptor to the pipeline.</param>
    /// <param name="bufferView">The buffer view.</param>
    virtual void DescriptorAsUniformTexelBuffer(GPUContextVulkan* context, VkBufferView& bufferView)
    {
        CRASH;
    }

    /// <summary>
    /// Gets the storage buffer descriptor (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER).
    /// </summary>
    /// <param name="context">The GPU context. Can be used to add memory barriers to the pipeline before binding the descriptor to the pipeline.</param>
    /// <param name="buffer">The buffer.</param>
    /// <param name="offset">The range offset (in bytes).</param>
    /// <param name="range">The range size (in bytes).</param>
    virtual void DescriptorAsStorageBuffer(GPUContextVulkan* context, VkBuffer& buffer, VkDeviceSize& offset, VkDeviceSize& range)
    {
        CRASH;
    }

    /// <summary>
    /// Gets the storage texel buffer descriptor (VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER).
    /// </summary>
    /// <param name="context">The GPU context. Can be used to add memory barriers to the pipeline before binding the descriptor to the pipeline.</param>
    /// <param name="bufferView">The buffer view.</param>
    virtual void DescriptorAsStorageTexelBuffer(GPUContextVulkan* context, VkBufferView& bufferView)
    {
        CRASH;
    }

    /// <summary>
    /// Gets the dynamic uniform buffer descriptor (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC).
    /// </summary>
    /// <param name="context">The GPU context. Can be used to add memory barriers to the pipeline before binding the descriptor to the pipeline.</param>
    /// <param name="buffer">The buffer.</param>
    /// <param name="offset">The range offset (in bytes).</param>
    /// <param name="range">The range size (in bytes).</param>
    /// <param name="dynamicOffset">The dynamic offset (in bytes).</param>
    virtual void DescriptorAsDynamicUniformBuffer(GPUContextVulkan* context, VkBuffer& buffer, VkDeviceSize& offset, VkDeviceSize& range, uint32& dynamicOffset)
    {
        CRASH;
    }

#if !BUILD_RELEASE
    // Utilities for incorrect resource usage.
    virtual bool HasSRV() const { return false; }
    virtual bool HasUAV() const { return false; }
#endif
};

extern GPUDevice* CreateGPUDeviceVulkan();

#endif
