// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if GRAPHICS_API_VULKAN

// Declaration for Vulkan Memory Allocator
#define VMA_IMPLEMENTATION

#include "GPUDeviceVulkan.h"
#include "GPUAdapterVulkan.h"
#include "GPUShaderVulkan.h"
#include "GPUContextVulkan.h"
#include "GPUPipelineStateVulkan.h"
#include "GPUTextureVulkan.h"
#include "GPUTimerQueryVulkan.h"
#include "GPUBufferVulkan.h"
#include "GPUSwapChainVulkan.h"
#include "RenderToolsVulkan.h"
#include "QueueVulkan.h"
#include "Config.h"
#include "CmdBufferVulkan.h"
#include "FlaxEngine.Gen.h"
#include "Engine/Core/Collections/ArrayExtensions.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/File.h"
#include "Engine/Graphics/Textures/GPUSamplerDescription.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#include "Engine/Core/Utilities.h"
#include "Engine/Core/Math/Color32.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Engine/CommandLine.h"
#include "Engine/Utilities/StringConverter.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Threading/Threading.h"

#if !USE_EDITOR && (PLATFORM_WINDOWS || PLATFORM_LINUX)
#include "Engine/Core/Config/PlatformSettings.h"
#endif

GPUDeviceVulkan::OptionalVulkanDeviceExtensions GPUDeviceVulkan::OptionalDeviceExtensions;
VkInstance GPUDeviceVulkan::Instance;
Array<const char*> GPUDeviceVulkan::InstanceExtensions;
Array<const char*> GPUDeviceVulkan::InstanceLayers;

bool SupportsDebugUtilsExt = false;
#if VULKAN_USE_DEBUG_LAYER
#if VK_EXT_debug_utils
VkDebugUtilsMessengerEXT Messenger = VK_NULL_HANDLE;
#endif

bool SupportsDebugCallbackExt = false;
VkDebugReportCallbackEXT MsgCallback = VK_NULL_HANDLE;

extern VulkanValidationLevel ValidationLevel;

static VKAPI_ATTR VkBool32 VKAPI_PTR DebugReportFunction(VkDebugReportFlagsEXT msgFlags, VkDebugReportObjectTypeEXT objType, uint64_t srcObject, size_t location, int32 msgCode, const char* layerPrefix, const char* msg, void* userData)
{
    const Char* msgPrefix = TEXT("UNKNOWN");
    if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
    {
        msgPrefix = TEXT("ERROR");

        if (!StringUtils::Compare(layerPrefix, "SC"))
        {
            if (msgCode == 3)
            {
                // Attachment N not written by fragment shader
                return VK_FALSE;
            }
            else if (msgCode == 5)
            {
                // SPIR-V module not valid: MemoryBarrier: Vulkan specification requires Memory Semantics to have one of the following bits set: Acquire, Release, AcquireRelease or SequentiallyConsistent
                return VK_FALSE;
            }
        }
    }
    else if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
    {
        msgPrefix = TEXT("WARN");

        if (!StringUtils::Compare(layerPrefix, "SC"))
        {
            if (msgCode == 2)
            {
                // Fragment shader writes to output location 0 with no matching attachment
                return VK_FALSE;
            }
        }
    }
    else if (msgFlags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
    {
        msgPrefix = TEXT("PERF");

        if (!StringUtils::Compare(layerPrefix, "SC"))
        {
            if (msgCode == 2)
            {
                // Vertex shader outputs unused interpolator
                return VK_FALSE;
            }
        }
        else if (!StringUtils::Compare(layerPrefix, "DS"))
        {
            if (msgCode == 15)
            {
                // DescriptorSet previously bound is incompatible with set newly bound as set #0 so set #1 and any subsequent sets were disturbed by newly bound pipelineLayout
                return VK_FALSE;
            }
        }
    }
    else if (msgFlags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
    {
        msgPrefix = TEXT("INFO");
    }
    else if (msgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
    {
        msgPrefix = TEXT("DEBUG");
    }
    else
    {
        CRASH;
    }

    // Send info
    LOG(Info, "[Vulkan] {0}:{1}:{2} {3}", msgPrefix, String(layerPrefix), msgCode, String(msg));

    return VK_FALSE;
}

#if VK_EXT_debug_utils

static VKAPI_ATTR VkBool32 VKAPI_PTR DebugUtilsCallback(VkDebugUtilsMessageSeverityFlagBitsEXT msgSeverity, VkDebugUtilsMessageTypeFlagsEXT msgType, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData)
{
    // Ignore some errors
    switch (msgType)
    {
    case 2:
        switch (callbackData->messageIdNumber)
        {
        case 0: // Attachment 4 not written by fragment shader; undefined values will be written to attachment
        case 2: // Fragment shader writes to output location 0 with no matching attachment
        case 3: // Attachment 2 not written by fragment shader
        case 5: // SPIR-V module not valid: MemoryBarrier: Vulkan specification requires Memory Semantics to have one of the following bits set: Acquire, Release, AcquireRelease or SequentiallyConsistent
#if PLATFORM_ANDROID
        case -1666394502: // After query pool creation, each query must be reset before it is used. Queries must also be reset between uses.
        case 602160055: // Attachment 4 not written by fragment shader; undefined values will be written to attachment. TODO: investigate it for PS_GBuffer shader from Deferred material with USE_LIGHTMAP=1
#endif
            return VK_FALSE;
        }
        break;
    case 4:
        switch (callbackData->messageIdNumber)
        {
        case 0: // Vertex shader writes to output location 0.0 which is not consumed by fragment shader
#if PLATFORM_ANDROID
            // TODO: implement preTransform for Android to improve swapchain presentation performance
        case 558591440: // preTransform doesn't match the currentTransform returned by vkGetPhysicalDeviceSurfaceCapabilitiesKHR, the presentation engine will transform the image content as part of the presentation operation.
        case 101294395: // Vertex shader writes to output location 0.0 which is not consumed by fragment shader
#endif
            return VK_FALSE;
        }
        break;
    case 6:
        switch (callbackData->messageIdNumber)
        {
        case 2: // Vertex shader writes to output location 0.0 which is not consumed by fragment shader
            return VK_FALSE;
        }
        break;
    }

    const Char* severity = TEXT("");
    if (msgSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        severity = TEXT("Error");
    }
    else if (msgSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        severity = TEXT("Warning");
    }
    else if (msgSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        severity = TEXT("Info");
    }
    else if (msgSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
    {
        severity = TEXT("Verbose");
    }

    const Char* type = TEXT("");
    if (msgType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
    {
        if (msgType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
        {
            type = TEXT("General/Validation");
        }
        else
        {
            type = TEXT("General");
        }
    }
    else if (msgType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
    {
        if (msgType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
        {
            type = TEXT("Perf/Validation");
        }
        else
        {
            type = TEXT("Validation");
        }
    }
    else if (msgType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
    {
        type = TEXT("Perf");
    }

    // Send info
    if (callbackData->pMessageIdName)
    {
        LOG(Info, "[Vulkan] {0} {1}:{2}({3}) {4}", type, severity, callbackData->messageIdNumber, String(callbackData->pMessageIdName), String(callbackData->pMessage));
    }
    else
    {
        LOG(Info, "[Vulkan] {0} {1}:{2} {3}", type, severity, callbackData->messageIdNumber, String(callbackData->pMessage));
    }

    return VK_FALSE;
}
#endif

void SetupDebugLayerCallback()
{
#if VK_EXT_debug_utils
    if (SupportsDebugUtilsExt)
    {
        if (vkCreateDebugUtilsMessengerEXT)
        {
            VkDebugUtilsMessengerCreateInfoEXT createInfo;
            RenderToolsVulkan::ZeroStruct(createInfo, VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT);
            createInfo.pfnUserCallback = DebugUtilsCallback;
            switch ((int32)ValidationLevel)
            {
            default:
                createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
                // Fall-through...
            case 4:
                createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
                // Fall-through...
            case 3:
                createInfo.messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
                // Fall-through...
            case 2:
                createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
                createInfo.messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
                // Fall-through...
            case 1:
                createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
                createInfo.messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
                break;
            case 0:
                // Nothing to do
                break;
            }
            const VkResult result = vkCreateDebugUtilsMessengerEXT(GPUDeviceVulkan::Instance, &createInfo, nullptr, &Messenger);
            LOG_VULKAN_RESULT(result);
        }
    }
    else if (SupportsDebugCallbackExt)
#else
	if (SupportsDebugCallbackExt)
#endif
    {
        if (vkCreateDebugReportCallbackEXT)
        {
            VkDebugReportCallbackCreateInfoEXT createInfo;
            RenderToolsVulkan::ZeroStruct(createInfo, VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT);
            createInfo.pfnCallback = DebugReportFunction;
            switch ((int32)ValidationLevel)
            {
            default:
                createInfo.flags |= VK_DEBUG_REPORT_DEBUG_BIT_EXT;
                // Fall-through...
            case 4:
                createInfo.flags |= VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
                // Fall-through...
            case 3:
                createInfo.flags |= VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
                // Fall-through...
            case 2:
                createInfo.flags |= VK_DEBUG_REPORT_WARNING_BIT_EXT;
                // Fall-through...
            case 1:
                createInfo.flags |= VK_DEBUG_REPORT_ERROR_BIT_EXT;
                break;
            case 0:
                // Nothing to do
                break;
            }
            const VkResult result = vkCreateDebugReportCallbackEXT(GPUDeviceVulkan::Instance, &createInfo, nullptr, &MsgCallback);
            LOG_VULKAN_RESULT(result);
        }
        else
        {
            LOG(Warning, "GetProcAddr: Unable to find vkDbgCreateMsgCallback; debug reporting skipped!");
        }
    }
    else
    {
        LOG(Warning, "Instance does not support 'VK_EXT_debug_report' extension; debug reporting skipped!");
    }
}

void RemoveDebugLayerCallback()
{
#if VK_EXT_debug_utils
    if (Messenger != VK_NULL_HANDLE)
    {
        if (vkDestroyDebugUtilsMessengerEXT)
            vkDestroyDebugUtilsMessengerEXT(GPUDeviceVulkan::Instance, Messenger, nullptr);
        Messenger = VK_NULL_HANDLE;
    }
    else if (MsgCallback != VK_NULL_HANDLE)
#else
	if (MsgCallback != VK_NULL_HANDLE)
#endif
    {
        if (vkDestroyDebugReportCallbackEXT)
            vkDestroyDebugReportCallbackEXT(GPUDeviceVulkan::Instance, MsgCallback, nullptr);
        MsgCallback = VK_NULL_HANDLE;
    }
}

#endif

DeferredDeletionQueueVulkan::DeferredDeletionQueueVulkan(GPUDeviceVulkan* device)
    : _device(device)
{
}

DeferredDeletionQueueVulkan::~DeferredDeletionQueueVulkan()
{
    ASSERT(_entries.IsEmpty());
}

void DeferredDeletionQueueVulkan::ReleaseResources(bool deleteImmediately)
{
    ScopeLock lock(&_locker);
    const uint64 checkFrame = Engine::FrameCount - VULKAN_RESOURCE_DELETE_SAFE_FRAMES_COUNT;
    for (int32 i = 0; i < _entries.Count(); i++)
    {
        Entry* e = &_entries[i];

        if (deleteImmediately || (checkFrame > e->FrameNumber && (e->CmdBuffer == nullptr || e->FenceCounter < e->CmdBuffer->GetFenceSignaledCounter()))
        )
        {
            if (e->AllocationHandle == VK_NULL_HANDLE)
            {
                switch (e->StructureType)
                {
#define SWITCH_CASE(type) case Type::type: vkDestroy##type(_device->Device, (Vk##type)e->Handle, nullptr); break
                SWITCH_CASE(RenderPass);
                SWITCH_CASE(Buffer);
                SWITCH_CASE(BufferView);
                SWITCH_CASE(Image);
                SWITCH_CASE(ImageView);
                SWITCH_CASE(Pipeline);
                SWITCH_CASE(PipelineLayout);
                SWITCH_CASE(Framebuffer);
                SWITCH_CASE(DescriptorSetLayout);
                SWITCH_CASE(Sampler);
                SWITCH_CASE(Semaphore);
                SWITCH_CASE(ShaderModule);
                SWITCH_CASE(Event);
                SWITCH_CASE(QueryPool);
#undef SWITCH_CASE
                default:
#if !BUILD_RELEASE
                CRASH;
#endif
                    break;
                }
            }
            else
            {
                if (e->StructureType == Image)
                {
                    vmaDestroyImage(_device->Allocator, (VkImage)e->Handle, e->AllocationHandle);
                }
                else if (e->StructureType == Buffer)
                {
                    vmaDestroyBuffer(_device->Allocator, (VkBuffer)e->Handle, e->AllocationHandle);
                }
                else
                {
                    CRASH;
                }
            }

            _entries.RemoveAt(i--);

            if (_entries.IsEmpty())
                break;
        }
    }
}

void DeferredDeletionQueueVulkan::EnqueueGenericResource(Type type, uint64 handle, VmaAllocation allocation)
{
    ASSERT(handle != 0);
    const auto queue = _device->GraphicsQueue;

    Entry entry;
    queue->GetLastSubmittedInfo(entry.CmdBuffer, entry.FenceCounter);
    entry.Handle = handle;
    entry.AllocationHandle = allocation;
    entry.StructureType = type;
    entry.FrameNumber = Engine::FrameCount;

    ScopeLock lock(_locker);

#if BUILD_DEBUG
    const std::function<bool(const Entry&)> ContainsHandle = [handle](const Entry& e)
    {
        return e.Handle == handle;
    };
    ASSERT(!ArrayExtensions::Any(_entries, ContainsHandle));
#endif
    _entries.Add(entry);
}

uint32 GetHash(const RenderTargetLayoutVulkan& key)
{
    uint32 hash = (int32)key.MSAA * 11;
    CombineHash(hash, (uint32)key.ReadDepth);
    CombineHash(hash, (uint32)key.WriteDepth);
    CombineHash(hash, (uint32)key.DepthFormat * 93473262);
    CombineHash(hash, key.RTsCount * 136);
    for (int32 i = 0; i < ARRAY_COUNT(key.RTVsFormats); i++)
    {
        CombineHash(hash, (uint32)key.RTVsFormats[i]);
    }
    return hash;
}

uint32 GetHash(const FramebufferVulkan::Key& key)
{
    uint32 hash = (int32)(intptr)key.RenderPass;
    CombineHash(hash, (uint32)key.AttachmentCount * 136);
    for (int32 i = 0; i < ARRAY_COUNT(key.Attachments); i++)
    {
        CombineHash(hash, (uint32)(intptr)key.Attachments[i]);
    }
    return hash;
}

FramebufferVulkan::FramebufferVulkan(GPUDeviceVulkan* device, Key& key, VkExtent3D& extent, uint32 layers)
    : _device(device)
    , _handle(VK_NULL_HANDLE)
    , Extent(extent)
    , Layers(layers)
{
    Platform::MemoryCopy(Attachments, key.Attachments, sizeof(Attachments));

    VkFramebufferCreateInfo createInfo;
    RenderToolsVulkan::ZeroStruct(createInfo, VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO);
    createInfo.renderPass = key.RenderPass->GetHandle();
    createInfo.attachmentCount = key.AttachmentCount;
    createInfo.pAttachments = key.Attachments;
    createInfo.width = extent.width;
    createInfo.height = extent.height;
    createInfo.layers = layers;
    VALIDATE_VULKAN_RESULT(vkCreateFramebuffer(device->Device, &createInfo, nullptr, &_handle));
}

FramebufferVulkan::~FramebufferVulkan()
{
    _device->DeferredDeletionQueue.EnqueueResource(DeferredDeletionQueueVulkan::Type::Framebuffer, _handle);
}

bool FramebufferVulkan::HasReference(VkImageView imageView) const
{
    for (int32 i = 0; i < ARRAY_COUNT(Attachments); i++)
    {
        if (Attachments[i] == imageView)
            return true;
    }
    return false;
}

RenderPassVulkan::RenderPassVulkan(GPUDeviceVulkan* device, const RenderTargetLayoutVulkan& layout)
    : _device(device)
    , _handle(VK_NULL_HANDLE)
    , Layout(layout)
{
    const int32 colorAttachmentsCount = layout.RTsCount;
    const bool hasDepthStencilAttachment = layout.DepthFormat != PixelFormat::Unknown;
    const int32 attachmentsCount = colorAttachmentsCount + (hasDepthStencilAttachment ? 1 : 0);

    VkAttachmentReference colorReferences[GPU_MAX_RT_BINDED];
    VkAttachmentReference depthStencilReference;
    VkAttachmentDescription attachments[GPU_MAX_RT_BINDED + 1];

    VkSubpassDescription subpassDesc;
    Platform::MemoryClear(&subpassDesc, sizeof(subpassDesc));
    subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDesc.colorAttachmentCount = colorAttachmentsCount;
    subpassDesc.pColorAttachments = colorReferences;
    subpassDesc.pResolveAttachments = nullptr;
    for (int32 i = 0; i < colorAttachmentsCount; i++)
    {
        VkAttachmentDescription& attachment = attachments[i];
        attachment.flags = 0;
        attachment.format = RenderToolsVulkan::ToVulkanFormat(layout.RTVsFormats[i]);
        attachment.samples = (VkSampleCountFlagBits)layout.MSAA;
        //attachment.loadOp = currentBlendDesc->BlendEnable ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_DONT_CARE; // TODO: Only if any destination blend?
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // TODO: only load when using blend mode
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        VkAttachmentReference& reference = colorReferences[i];
        reference.attachment = i;
        reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    if (hasDepthStencilAttachment)
    {
        VkImageLayout depthStencilLayout;
        if (layout.ReadDepth && !layout.WriteDepth)
            depthStencilLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        else
            depthStencilLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // Use last slot for depth stencil attachment
        VkAttachmentDescription& depthAttachment = attachments[colorAttachmentsCount];
        depthAttachment.flags = 0;
        depthAttachment.format = RenderToolsVulkan::ToVulkanFormat(layout.DepthFormat);
        depthAttachment.samples = (VkSampleCountFlagBits)layout.MSAA;
        // TODO: fix those operations for load and store
        depthAttachment.loadOp = layout.ReadDepth || true ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.storeOp = layout.WriteDepth || true ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // TODO: Handle stencil
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = depthStencilLayout;
        depthAttachment.finalLayout = depthStencilLayout;
        depthStencilReference.attachment = colorAttachmentsCount;
        depthStencilReference.layout = depthStencilLayout;
        subpassDesc.pDepthStencilAttachment = &depthStencilReference;
    }

    VkRenderPassCreateInfo createInfo;
    RenderToolsVulkan::ZeroStruct(createInfo, VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO);
    createInfo.attachmentCount = attachmentsCount;
    createInfo.pAttachments = attachments;
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpassDesc;
    VALIDATE_VULKAN_RESULT(vkCreateRenderPass(device->Device, &createInfo, nullptr, &_handle));
}

RenderPassVulkan::~RenderPassVulkan()
{
    _device->DeferredDeletionQueue.EnqueueResource(DeferredDeletionQueueVulkan::Type::RenderPass, _handle);
}

QueryPoolVulkan::QueryPoolVulkan(GPUDeviceVulkan* device, int32 capacity, VkQueryType type)
    : _device(device)
    , _handle(VK_NULL_HANDLE)
    , _count(0)
    , _capacity(capacity)
    , _type(type)
{
    VkQueryPoolCreateInfo createInfo;
    RenderToolsVulkan::ZeroStruct(createInfo, VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO);
    createInfo.queryType = type;
    createInfo.queryCount = capacity;
    VALIDATE_VULKAN_RESULT(vkCreateQueryPool(device->Device, &createInfo, nullptr, &_handle));
#if VULKAN_RESET_QUERY_POOLS
    _resetRanges.Add(Range{ 0, static_cast<uint32>(capacity) });
    device->QueriesToReset.Add(this);
#endif
}

QueryPoolVulkan::~QueryPoolVulkan()
{
#if VULKAN_RESET_QUERY_POOLS
    _device->QueriesToReset.Remove(this);
#endif
    _device->DeferredDeletionQueue.EnqueueResource(DeferredDeletionQueueVulkan::Type::QueryPool, _handle);
}

#if VULKAN_RESET_QUERY_POOLS

void QueryPoolVulkan::Reset(CmdBufferVulkan* cmdBuffer)
{
    for (auto& range : _resetRanges)
    {
        vkCmdResetQueryPool(cmdBuffer->GetHandle(), _handle, range.Start, range.Count);
    }
    _resetRanges.Clear();
}

#endif

BufferedQueryPoolVulkan::BufferedQueryPoolVulkan(GPUDeviceVulkan* device, int32 capacity, VkQueryType type)
    : QueryPoolVulkan(device, capacity, type)
    , _lastBeginIndex(0)
{
    _queryOutput.Resize(capacity);
    _usedQueryBits.AddZeroed((capacity + 63) / 64);
    _startedQueryBits.AddZeroed((capacity + 63) / 64);
    _readResultsBits.AddZeroed((capacity + 63) / 64);
}

bool BufferedQueryPoolVulkan::AcquireQuery(uint32& resultIndex)
{
    const uint64 allUsedMask = (uint64)-1;
    for (int32 wordIndex = _lastBeginIndex / 64; wordIndex < _usedQueryBits.Count(); wordIndex++)
    {
        uint64 beginQueryWord = _usedQueryBits[wordIndex];
        if (beginQueryWord != allUsedMask)
        {
            resultIndex = 0;
            while ((beginQueryWord & 1) == 1)
            {
                resultIndex++;
                beginQueryWord >>= 1;
            }
            resultIndex += wordIndex * 64;
            const uint64 bit = (uint64)1 << (uint64)(resultIndex % 64);
            _usedQueryBits[wordIndex] = _usedQueryBits[wordIndex] | bit;
            _readResultsBits[wordIndex] &= ~bit;
            _lastBeginIndex = resultIndex + 1;
            return true;
        }
    }

    return false;
}

void BufferedQueryPoolVulkan::ReleaseQuery(uint32 queryIndex)
{
    const uint32 word = queryIndex / 64;
    const uint64 bit = (uint64)1 << (queryIndex % 64);
    _usedQueryBits[word] = _usedQueryBits[word] & ~bit;
    _readResultsBits[word] = _readResultsBits[word] & ~bit;
    if (queryIndex < (uint32)_lastBeginIndex)
    {
        // Use the lowest word available
        const uint64 allUsedMask = (uint64)-1;
        const int32 lastQueryWord = _lastBeginIndex / 64;
        if (lastQueryWord < _usedQueryBits.Count() && _usedQueryBits[lastQueryWord] == allUsedMask)
        {
            _lastBeginIndex = (uint32)queryIndex;
        }
    }
}

void BufferedQueryPoolVulkan::MarkQueryAsStarted(uint32 queryIndex)
{
    const uint32 word = queryIndex / 64;
    const uint64 bit = (uint64)1 << (queryIndex % 64);
    _startedQueryBits[word] = _startedQueryBits[word] | bit;
}

bool BufferedQueryPoolVulkan::GetResults(GPUContextVulkan* context, uint32 index, uint64& result)
{
    const uint64 bit = (uint64)(index % 64);
    const uint64 bitMask = (uint64)1 << bit;
    const uint32 word = index / 64;

    if ((_startedQueryBits[word] & bitMask) == 0)
    {
        // Query never started/ended
        result = 0;
        return true;
    }

    if ((_readResultsBits[word] & bitMask) == 0)
    {
        const VkResult vkResult = vkGetQueryPoolResults(_device->Device, _handle, index, 1, sizeof(uint64), &_queryOutput[index], sizeof(uint64), VK_QUERY_RESULT_64_BIT);
        if (vkResult == VK_SUCCESS)
        {
            _readResultsBits[word] = _readResultsBits[word] | bitMask;

#if VULKAN_RESET_QUERY_POOLS
            // Add to reset
            if (!_device->QueriesToReset.Contains(this))
                _device->QueriesToReset.Add(this);
            _resetRanges.Add(Range{ index, 1 });
#endif
        }
        else if (vkResult == VK_NOT_READY)
        {
            // No results yet
            result = 0;
            return false;
        }
        else
        {
            LOG_VULKAN_RESULT(vkResult);
        }
    }

    result = _queryOutput[index];
    return true;
}

bool BufferedQueryPoolVulkan::HasRoom() const
{
    const uint64 allUsedMask = (uint64)-1;
    if (_lastBeginIndex < _usedQueryBits.Count() * 64)
    {
        ASSERT((_usedQueryBits[_lastBeginIndex / 64] & allUsedMask) != allUsedMask);
        return true;
    }
    return false;
}

HelperResourcesVulkan::HelperResourcesVulkan(GPUDeviceVulkan* device)
    : _device(device)
    , _dummyBuffer(nullptr)
    , _dummyVB(nullptr)
{
    Platform::MemoryClear(_dummyTextures, sizeof(_dummyTextures));
    Platform::MemoryClear(_staticSamplers, sizeof(_staticSamplers));
}

void InitSampler(VkSamplerCreateInfo& createInfo, bool supportsMirrorClampToEdge, GPUSamplerFilter filter, GPUSamplerAddressMode addressU, GPUSamplerAddressMode addressV, GPUSamplerAddressMode addressW, GPUSamplerCompareFunction compareFunction)
{
    createInfo.magFilter = RenderToolsVulkan::ToVulkanMagFilterMode(filter);
    createInfo.minFilter = RenderToolsVulkan::ToVulkanMinFilterMode(filter);
    createInfo.mipmapMode = RenderToolsVulkan::ToVulkanMipFilterMode(filter);
    createInfo.addressModeU = RenderToolsVulkan::ToVulkanWrapMode(addressU, supportsMirrorClampToEdge);
    createInfo.addressModeV = RenderToolsVulkan::ToVulkanWrapMode(addressV, supportsMirrorClampToEdge);
    createInfo.addressModeW = RenderToolsVulkan::ToVulkanWrapMode(addressW, supportsMirrorClampToEdge);
    createInfo.compareEnable = compareFunction != GPUSamplerCompareFunction::Never ? VK_TRUE : VK_FALSE;
    createInfo.compareOp = RenderToolsVulkan::ToVulkanSamplerCompareFunction(compareFunction);
}

VkSampler HelperResourcesVulkan::GetStaticSampler(StaticSamplers type)
{
    if (!_staticSamplers[0])
    {
        const bool supportsMirrorClampToEdge = _device->OptionalDeviceExtensions.HasMirrorClampToEdge;

        VkSamplerCreateInfo createInfo;
        RenderToolsVulkan::ZeroStruct(createInfo, VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO);
        createInfo.mipLodBias = 0.0f;
        createInfo.minLod = 0.0f;
        createInfo.maxLod = MAX_float;
        createInfo.maxAnisotropy = 1.0f;
        createInfo.anisotropyEnable = VK_FALSE;
        createInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;

        // Linear Clamp
        InitSampler(createInfo, supportsMirrorClampToEdge, GPUSamplerFilter::Trilinear, GPUSamplerAddressMode::Clamp, GPUSamplerAddressMode::Clamp, GPUSamplerAddressMode::Clamp, GPUSamplerCompareFunction::Never);
        VALIDATE_VULKAN_RESULT(vkCreateSampler(_device->Device, &createInfo, nullptr, &_staticSamplers[0]));

        // Point Clamp
        InitSampler(createInfo, supportsMirrorClampToEdge, GPUSamplerFilter::Point, GPUSamplerAddressMode::Clamp, GPUSamplerAddressMode::Clamp, GPUSamplerAddressMode::Clamp, GPUSamplerCompareFunction::Never);
        VALIDATE_VULKAN_RESULT(vkCreateSampler(_device->Device, &createInfo, nullptr, &_staticSamplers[1]));

        // Linear Wrap
        InitSampler(createInfo, supportsMirrorClampToEdge, GPUSamplerFilter::Trilinear, GPUSamplerAddressMode::Wrap, GPUSamplerAddressMode::Wrap, GPUSamplerAddressMode::Wrap, GPUSamplerCompareFunction::Never);
        VALIDATE_VULKAN_RESULT(vkCreateSampler(_device->Device, &createInfo, nullptr, &_staticSamplers[2]));

        // Point Wrap
        InitSampler(createInfo, supportsMirrorClampToEdge, GPUSamplerFilter::Point, GPUSamplerAddressMode::Wrap, GPUSamplerAddressMode::Wrap, GPUSamplerAddressMode::Wrap, GPUSamplerCompareFunction::Never);
        VALIDATE_VULKAN_RESULT(vkCreateSampler(_device->Device, &createInfo, nullptr, &_staticSamplers[3]));

        // Shadow
        InitSampler(createInfo, supportsMirrorClampToEdge, GPUSamplerFilter::Point, GPUSamplerAddressMode::Clamp, GPUSamplerAddressMode::Clamp, GPUSamplerAddressMode::Clamp, GPUSamplerCompareFunction::Less);
        VALIDATE_VULKAN_RESULT(vkCreateSampler(_device->Device, &createInfo, nullptr, &_staticSamplers[4]));

        // Shadow PCF
        InitSampler(createInfo, supportsMirrorClampToEdge, GPUSamplerFilter::Trilinear, GPUSamplerAddressMode::Clamp, GPUSamplerAddressMode::Clamp, GPUSamplerAddressMode::Clamp, GPUSamplerCompareFunction::Less);
        VALIDATE_VULKAN_RESULT(vkCreateSampler(_device->Device, &createInfo, nullptr, &_staticSamplers[5]));
    }

    return _staticSamplers[static_cast<int32>(type)];
}

GPUTextureVulkan* HelperResourcesVulkan::GetDummyTexture(SpirvShaderResourceType type)
{
    int32 index;
    switch (type)
    {
    case SpirvShaderResourceType::Texture1D:
        index = 0;
        break;
    case SpirvShaderResourceType::Texture2D:
        index = 1;
        break;
    case SpirvShaderResourceType::Texture3D:
        index = 2;
        break;
    case SpirvShaderResourceType::TextureCube:
        index = 3;
        break;
    case SpirvShaderResourceType::Texture1DArray:
        index = 4;
        break;
    case SpirvShaderResourceType::Texture2DArray:
        index = 5;
        break;
    default:
    CRASH;
        return nullptr;
    }

    auto texture = _dummyTextures[index];
    if (!texture)
    {
        texture = (GPUTextureVulkan*)_device->CreateTexture(TEXT("DummyTexture"));
        GPUTextureDescription desc;
        const PixelFormat format = PixelFormat::R8G8B8A8_UNorm;
        const GPUTextureFlags flags = GPUTextureFlags::ShaderResource | GPUTextureFlags::UnorderedAccess;
        switch (type)
        {
        case SpirvShaderResourceType::Texture1D:
            desc = GPUTextureDescription::New1D(1, 1, format, flags, 1);
            break;
        case SpirvShaderResourceType::Texture2D:
            desc = GPUTextureDescription::New2D(1, 1, format, flags);
            break;
        case SpirvShaderResourceType::Texture3D:
            desc = GPUTextureDescription::New3D(1, 1, 1, format, flags);
            break;
        case SpirvShaderResourceType::TextureCube:
            desc = GPUTextureDescription::NewCube(1, format, flags);
            break;
        case SpirvShaderResourceType::Texture1DArray:
            desc = GPUTextureDescription::New1D(1, 1, format, flags, 4);
            break;
        case SpirvShaderResourceType::Texture2DArray:
            desc = GPUTextureDescription::New2D(1, 1, format, flags, 4);
            break;
        default: ;
        }
        texture->Init(desc);
        ASSERT(texture->View(0));
        _dummyTextures[index] = texture;
    }

    return texture;
}

GPUBufferVulkan* HelperResourcesVulkan::GetDummyBuffer()
{
    if (!_dummyBuffer)
    {
        _dummyBuffer = (GPUBufferVulkan*)_device->CreateBuffer(TEXT("DummyBuffer"));
        _dummyBuffer->Init(GPUBufferDescription::Buffer(sizeof(int32), GPUBufferFlags::ShaderResource | GPUBufferFlags::UnorderedAccess, PixelFormat::R32_SInt));
    }

    return _dummyBuffer;
}

GPUBufferVulkan* HelperResourcesVulkan::GetDummyVertexBuffer()
{
    if (!_dummyVB)
    {
        _dummyVB = (GPUBufferVulkan*)_device->CreateBuffer(TEXT("DummyVertexBuffer"));
        _dummyVB->Init(GPUBufferDescription::Vertex(sizeof(Color32), 1, &Color32::Transparent));
    }

    return _dummyVB;
}

void HelperResourcesVulkan::Dispose()
{
    for (int32 i = 0; i < ARRAY_COUNT(_dummyTextures); i++)
    {
        SAFE_DELETE_GPU_RESOURCE(_dummyTextures[i]);
    }

    SAFE_DELETE_GPU_RESOURCE(_dummyBuffer);
    SAFE_DELETE_GPU_RESOURCE(_dummyVB);

    for (int32 i = 0; i < ARRAY_COUNT(_staticSamplers); i++)
    {
        auto& sampler = _staticSamplers[i];
        if (sampler != VK_NULL_HANDLE)
        {
            _device->DeferredDeletionQueue.EnqueueResource(DeferredDeletionQueueVulkan::Type::Sampler, sampler);
            sampler = VK_NULL_HANDLE;
        }
    }
}

StagingManagerVulkan::PendingItems* StagingManagerVulkan::PendingItemsPerCmdBuffer::FindOrAdd(uint64 fence)
{
    for (int32 i = 0; i < Items.Count(); i++)
    {
        if (Items[i].FenceCounter == fence)
        {
            return &Items[i];
        }
    }

    const auto item = &Items.AddOne();
    item->FenceCounter = fence;
    item->Resources.Clear();
    return item;
}

StagingManagerVulkan::StagingManagerVulkan(GPUDeviceVulkan* device)
    : _device(device)
{
}

GPUBuffer* StagingManagerVulkan::AcquireBuffer(uint32 size, GPUResourceUsage usage)
{
    // Try reuse free buffer
    {
        ScopeLock lock(_locker);

        for (int32 i = 0; i < _freeBuffers.Count(); i++)
        {
            auto& freeBuffer = _freeBuffers[i];
            if (freeBuffer.Buffer->GetSize() == size && freeBuffer.Buffer->GetDescription().Usage == usage)
            {
                const auto buffer = freeBuffer.Buffer;
                _freeBuffers.RemoveAt(i);
                return buffer;
            }
        }
    }

    // Allocate new buffer
    auto buffer = _device->CreateBuffer(TEXT("Pooled Staging"));
    if (buffer->Init(GPUBufferDescription::Buffer(size, GPUBufferFlags::None, PixelFormat::Unknown, nullptr, 0, usage)))
    {
        LOG(Warning, "Failed to create pooled staging buffer.");
        return nullptr;
    }

    // Cache buffer
    {
        ScopeLock lock(_locker);

        _allBuffers.Add(buffer);
#if !BUILD_RELEASE
        _allBuffersAllocSize += size;
        _allBuffersTotalSize += size;
        _allBuffersPeekSize = Math::Max(_allBuffersTotalSize, _allBuffersPeekSize);
#endif
    }

    return buffer;
}

void StagingManagerVulkan::ReleaseBuffer(CmdBufferVulkan* cmdBuffer, GPUBuffer*& buffer)
{
    ScopeLock lock(_locker);

    if (cmdBuffer)
    {
        // Return to pending pool (need to wait until command buffer will be executed and buffer will be reusable)
        auto items = FindOrAdd(cmdBuffer);
        auto item = items->FindOrAdd(cmdBuffer->GetFenceSignaledCounter());
        item->Resources.Add(buffer);
    }
    else
    {
        // Return to pool
        _freeBuffers.Add({ buffer, Engine::FrameCount });
    }

    // Clear reference
    buffer = nullptr;
}

void StagingManagerVulkan::ProcessPendingFree()
{
    ScopeLock lock(_locker);

    // Find staging buffers that has been processed by the GPU and can be reused
    for (int32 i = _pendingBuffers.Count() - 1; i >= 0; i--)
    {
        auto& e = _pendingBuffers[i];

        for (int32 fenceIndex = e.Items.Count() - 1; fenceIndex >= 0; fenceIndex--)
        {
            auto& items = e.Items[fenceIndex];
            if (items.FenceCounter < e.CmdBuffer->GetFenceSignaledCounter())
            {
                for (auto buffer : items.Resources)
                {
                    // Return to pool
                    _freeBuffers.Add({ buffer, Engine::FrameCount });
                }

                e.Items.RemoveAt(fenceIndex);
            }
        }

        if (e.Items.IsEmpty())
        {
            _pendingBuffers.RemoveAt(i);
        }
    }

    // Free staging buffers that has not been used for a few frames
    const uint64 SafeFramesCount = 30;
    for (int32 i = _freeBuffers.Count() - 1; i >= 0; i--)
    {
        auto& e = _freeBuffers[i];
        if (e.FrameNumber + SafeFramesCount < Engine::FrameCount)
        {
            auto buffer = e.Buffer;

            // Remove buffer from lists
            _allBuffers.Remove(buffer);
            _freeBuffers.RemoveAt(i);

#if !BUILD_RELEASE
            // Update stats
            _allBuffersFreeSize += buffer->GetSize();
            _allBuffersTotalSize -= buffer->GetSize();
#endif

            // Release memory
            buffer->ReleaseGPU();
            Delete(buffer);
        }
    }
}

void StagingManagerVulkan::Dispose()
{
    ScopeLock lock(_locker);

#if !BUILD_RELEASE
    LOG(Info, "Vulakn staging buffers peek memory usage: {0}, allocs: {1}, frees: {2}", Utilities::BytesToText(_allBuffersPeekSize), Utilities::BytesToText(_allBuffersAllocSize), Utilities::BytesToText(_allBuffersFreeSize));
#endif

    // Release buffers and clear memory
    for (auto buffer : _allBuffers)
    {
        buffer->ReleaseGPU();
        Delete(buffer);
    }
    _allBuffers.Resize(0);
    _pendingBuffers.Resize(0);
}

StagingManagerVulkan::PendingItemsPerCmdBuffer* StagingManagerVulkan::FindOrAdd(CmdBufferVulkan* cmdBuffer)
{
    for (int32 i = 0; i < _pendingBuffers.Count(); i++)
    {
        if (_pendingBuffers[i].CmdBuffer == cmdBuffer)
        {
            return &_pendingBuffers[i];
        }
    }

    const auto item = &_pendingBuffers.AddOne();
    item->CmdBuffer = cmdBuffer;
    item->Items.Clear();
    return item;
}

GPUDeviceVulkan::GPUDeviceVulkan(ShaderProfile shaderProfile, GPUAdapterVulkan* adapter)
    : GPUDevice(RendererType::Vulkan, shaderProfile)
    , _renderPasses(512)
    , _framebuffers(512)
    , _layouts(4096)
    , Adapter(adapter)
    , DeferredDeletionQueue(this)
    , StagingManager(this)
    , HelperResources(this)
{
}

GPUDevice* GPUDeviceVulkan::Create()
{
#if !USE_EDITOR && (PLATFORM_WINDOWS || PLATFORM_LINUX)
	auto settings = PlatformSettings::Get();
	if (!settings->SupportVulkan)
	{
		// Skip if there is no support
		LOG(Warning, "Cannot use Vulkan (support disabled).");
		return nullptr;
	}
#endif

    // Initialize bindings
    VkResult result = volkInitialize();
    if (result != VK_SUCCESS)
    {
        LOG(Warning, "Graphics Device init failed with error {0}", RenderToolsVulkan::GetVkErrorString(result));
        return nullptr;
    }

    // Engine registration
    const StringAsANSI<256> appName(*Globals::ProductName);
    VkApplicationInfo appInfo;
    RenderToolsVulkan::ZeroStruct(appInfo, VK_STRUCTURE_TYPE_APPLICATION_INFO);
    appInfo.pApplicationName = appName.Get();
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Flax";
    appInfo.engineVersion = VK_MAKE_VERSION(FLAXENGINE_VERSION_MAJOR, FLAXENGINE_VERSION_MINOR, FLAXENGINE_VERSION_BUILD);
    appInfo.apiVersion = VULKAN_API_VERSION;

    VkInstanceCreateInfo instInfo;
    RenderToolsVulkan::ZeroStruct(instInfo, VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO);
    instInfo.pApplicationInfo = &appInfo;

    GetInstanceLayersAndExtensions(InstanceExtensions, InstanceLayers, SupportsDebugUtilsExt);

    const auto hasExtension = [](const Array<const char*>& extensions, const char* name) -> bool
    {
        const std::function<bool(const char* const&)> callback = [&name](const char* const& extension) -> bool
        {
            return extension && StringUtils::Compare(extension, name) == 0;
        };
        return ArrayExtensions::Any(extensions, callback);
    };

    instInfo.enabledExtensionCount = InstanceExtensions.Count();
    instInfo.ppEnabledExtensionNames = instInfo.enabledExtensionCount > 0 ? static_cast<const char* const*>(InstanceExtensions.Get()) : nullptr;
    instInfo.enabledLayerCount = InstanceLayers.Count();
    instInfo.ppEnabledLayerNames = instInfo.enabledLayerCount > 0 ? InstanceLayers.Get() : nullptr;
#if VULKAN_USE_DEBUG_LAYER
    SupportsDebugCallbackExt = !SupportsDebugUtilsExt && hasExtension(InstanceExtensions, VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif

    // Create Vulkan instance
    result = vkCreateInstance(&instInfo, nullptr, &Instance);
    if (result == VK_ERROR_INCOMPATIBLE_DRIVER)
    {
        // Missing driver
        Platform::Fatal(TEXT("Cannot find a compatible Vulkan driver.\nPlease look at the Getting Started guide for additional information."));
        return nullptr;
    }
    if (result == VK_ERROR_EXTENSION_NOT_PRESENT)
    {
        // Extensions error
        uint32_t propertyCount;
        vkEnumerateInstanceExtensionProperties(nullptr, &propertyCount, nullptr);
        Array<VkExtensionProperties> properties;
        properties.Resize(propertyCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &propertyCount, properties.Get());
        for (const char* extension : InstanceExtensions)
        {
            bool found = false;
            for (uint32_t propertyIndex = 0; propertyIndex < propertyCount; propertyIndex++)
            {
                if (!StringUtils::Compare(properties[propertyIndex].extensionName, extension))
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                LOG(Warning, "Missing required Vulkan extension: {0}", String(extension));
            }
        }
        auto error = String::Format(TEXT("Vulkan driver doesn't contain specified extensions:\n{0}\nPlease make sure your layers path is set appropriately."));
        Platform::Error(*error);
        return nullptr;
    }
    if (result != VK_SUCCESS)
    {
        // Driver error
        LOG(Warning, "Vulkan create instance failed with error code: {0}", RenderToolsVulkan::GetVkErrorString(result));
        Platform::Fatal(TEXT("Vulkan failed to create instance\n\nDo you have a compatible Vulkan driver installed?"));
        return nullptr;
    }

    // Setup bindings
    volkLoadInstance(Instance);

    // Setup debug layer
#if VULKAN_USE_DEBUG_LAYER
    SetupDebugLayerCallback();
#endif

    // Enumerate all GPU devices and pick one
    uint32 gpuCount = 0;
    VALIDATE_VULKAN_RESULT(vkEnumeratePhysicalDevices(Instance, &gpuCount, nullptr));
    ASSERT(gpuCount >= 1);
    Array<VkPhysicalDevice> gpus;
    gpus.AddZeroed(gpuCount);
    VALIDATE_VULKAN_RESULT(vkEnumeratePhysicalDevices(Instance, &gpuCount, gpus.Get()));
    Array<GPUAdapterVulkan> adapters;
    adapters.EnsureCapacity(gpuCount);
    for (uint32 gpuIndex = 0; gpuIndex < gpuCount; gpuIndex++)
    {
        GPUAdapterVulkan adapter(gpus[gpuIndex]);
        adapters.Add(adapter);

        const Char* type;
        switch (adapter.GpuProps.deviceType)
        {
        case VK_PHYSICAL_DEVICE_TYPE_OTHER:
            type = TEXT("Other");
            break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            type = TEXT("Integrated GPU");
            break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            type = TEXT("Discrete GPU");
            break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            type = TEXT("Virtual GPU");
            break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            type = TEXT("CPU");
            break;
        default:
            type = TEXT("Unknown");
        }

        LOG(Info, "Adapter {}: '{}', API {}.{}.{}, Driver {}.{}.{}", gpuIndex, adapter.Description, VK_VERSION_MAJOR(adapter.GpuProps.apiVersion), VK_VERSION_MINOR(adapter.GpuProps.apiVersion), VK_VERSION_PATCH(adapter.GpuProps.apiVersion), VK_VERSION_MAJOR(adapter.GpuProps.driverVersion), VK_VERSION_MINOR(adapter.GpuProps.driverVersion), VK_VERSION_PATCH(adapter.GpuProps.driverVersion));
        LOG(Info, "	VendorId: 0x{:x}, Type: {}, Max Descriptor Sets Bound: {}, Timestamps: {}", adapter.GpuProps.vendorID, type, adapter.GpuProps.limits.maxBoundDescriptorSets, !!adapter.GpuProps.limits.timestampComputeAndGraphics);
    }

    // Select the adapter to use
    int32 selectedAdapter = 0;
    uint32 vendorId = 0;
    if (CommandLine::Options.NVIDIA)
        vendorId = GPU_VENDOR_ID_NVIDIA;
    else if (CommandLine::Options.AMD)
        vendorId = GPU_VENDOR_ID_AMD;
    else if (CommandLine::Options.Intel)
        vendorId = GPU_VENDOR_ID_INTEL;
    if (vendorId != 0)
    {
        for (int32 i = 0; i < adapters.Count(); i++)
        {
            if (adapters[i].GetVendorId() == vendorId)
            {
                selectedAdapter = i;
                break;
            }
        }
    }
    ASSERT(selectedAdapter != -1 && adapters[selectedAdapter].IsValid());

    // Create device
    auto device = New<GPUDeviceVulkan>(ShaderProfile::Vulkan_SM5, New<GPUAdapterVulkan>(adapters[selectedAdapter]));
    if (device->Init())
    {
        LOG(Warning, "Graphics Device init failed");
        Delete(device);
        return nullptr;
    }

    return device;
}

GPUDeviceVulkan::~GPUDeviceVulkan()
{
    // Ensure to be disposed
    GPUDeviceVulkan::Dispose();
}

RenderPassVulkan* GPUDeviceVulkan::GetOrCreateRenderPass(RenderTargetLayoutVulkan& layout)
{
    // Try reuse cached version
    RenderPassVulkan* renderPass;
    if (_renderPasses.TryGet(layout, renderPass))
    {
        return renderPass;
    }

    PROFILE_CPU_NAMED("Create Render Pass");

    // Create object and cache it
    renderPass = New<RenderPassVulkan>(this, layout);
    _renderPasses.Add(layout, renderPass);
    return renderPass;
}

FramebufferVulkan* GPUDeviceVulkan::GetOrCreateFramebuffer(FramebufferVulkan::Key& key, VkExtent3D& extent, uint32 layers)
{
    // Try reuse cached version
    FramebufferVulkan* framebuffer;
    if (_framebuffers.TryGet(key, framebuffer))
    {
        return framebuffer;
    }

    PROFILE_CPU_NAMED("Create Framebuffer");

    // Create object and cache it
    framebuffer = New<FramebufferVulkan>(this, key, extent, layers);
    _framebuffers.Add(key, framebuffer);
    return framebuffer;
}

PipelineLayoutVulkan* GPUDeviceVulkan::GetOrCreateLayout(DescriptorSetLayoutInfoVulkan& key)
{
    // Try reuse cached version
    PipelineLayoutVulkan* layout;
    if (_layouts.TryGet(key, layout))
    {
        return layout;
    }

    PROFILE_CPU_NAMED("Create Pipeline Layout");

    // Create object and cache it
    layout = New<PipelineLayoutVulkan>(this, key);
    _layouts.Add(key, layout);
    return layout;
}

void GPUDeviceVulkan::OnImageViewDestroy(VkImageView imageView)
{
    for (auto i = _framebuffers.Begin(); i.IsNotEnd(); ++i)
    {
        if (i->Value->HasReference(imageView))
        {
            Delete(i->Value);
            _framebuffers.Remove(i);
            --i;
        }
    }
}

void GPUDeviceVulkan::SetupPresentQueue(VkSurfaceKHR surface)
{
    if (PresentQueue)
        return;

    const auto supportsPresent = [surface](VkPhysicalDevice physicalDevice, QueueVulkan* queue)
    {
        VkBool32 supportsPresent = VK_FALSE;
        const uint32 queueFamilyIndex = queue->GetFamilyIndex();
        VALIDATE_VULKAN_RESULT(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, surface, &supportsPresent));
        if (supportsPresent)
        {
            LOG(Info, "Vulkan Queue Family {0}: supports present", queueFamilyIndex);
        }
        return supportsPresent == VK_TRUE;
    };

    const auto gpu = Adapter->Gpu;
    bool graphics = supportsPresent(gpu, GraphicsQueue);
    if (!graphics)
    {
        LOG(Error, "Vulkan Graphics Queue doesn't support present");
    }
    // TODO: test using Compute queue for present
    //bool compute = SupportsPresent(gpu, ComputeQueue);
    PresentQueue = GraphicsQueue;
}

PixelFormat GPUDeviceVulkan::GetClosestSupportedPixelFormat(PixelFormat format, GPUTextureFlags flags, bool optimalTiling)
{
    // Collect features to use
    VkFormatFeatureFlags wantedFeatureFlags = 0;
    if (flags & GPUTextureFlags::ShaderResource)
        wantedFeatureFlags |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
    if (flags & GPUTextureFlags::RenderTarget)
        wantedFeatureFlags |= VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
    if (flags & GPUTextureFlags::DepthStencil)
        wantedFeatureFlags |= VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    if (flags & GPUTextureFlags::UnorderedAccess)
        wantedFeatureFlags |= VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;

    if (!IsVkFormatSupported(RenderToolsVulkan::ToVulkanFormat(format), wantedFeatureFlags, optimalTiling))
    {
        // Special case for depth-stencil formats
        if (flags & GPUTextureFlags::DepthStencil)
        {
            const bool hasStencil = PixelFormatExtensions::HasStencil(format);

            // Spec guarantees at least one depth-only, and one depth-stencil format to be supported
            if (hasStencil)
            {
                if (IsVkFormatSupported(VK_FORMAT_D32_SFLOAT_S8_UINT, wantedFeatureFlags, optimalTiling))
                    format = PixelFormat::D32_Float;
                else
                    format = PixelFormat::D24_UNorm_S8_UInt;
            }
            else
            {
                // The only format that could have failed is 32-bit depth, so we must use the alternative 16-bit. Spec guarantees it is always supported.
                format = PixelFormat::D16_UNorm;
            }
        }
        else
        {
            // TODO: implement it?
            LOG(Warning, "Unsupported Vulkan format {0}", (int32)format);
        }
    }

    return format;
}

#if VK_EXT_validation_cache

void GetValidationCachePath(String& path)
{
#if USE_EDITOR
    path = Globals::ProjectCacheFolder / TEXT("VulkanValidation.cache");
#else
    path = Globals::ProductLocalFolder / TEXT("VulkanValidation.cache");
#endif
}

void GPUDeviceVulkan::LoadValidationCache()
{
    Array<uint8> data;

    String path;
    GetValidationCachePath(path);

    if (FileSystem::FileExists(path))
    {
        LOG(Info, "Trying to load Vulkan validation cache file {0}", path);
        File::ReadAllBytes(path, data);

        if (data.HasItems())
        {
            int32* dataPtr = (int32*)data.Get();
            if (*dataPtr > 0)
            {
                dataPtr++;
                const int32 version = *dataPtr++;
                if (version == VK_PIPELINE_CACHE_HEADER_VERSION_ONE)
                {
                    dataPtr += VK_UUID_SIZE / sizeof(int32);
                }
                else
                {
                    LOG(Warning, "Bad validation cache file, version: {0}, expected: {1}", version, VK_PIPELINE_CACHE_HEADER_VERSION_ONE);
                    data.Clear();
                }
            }
            else
            {
                LOG(Warning, "Bad validation cache file, header size: {0}", *dataPtr);
                data.Clear();
            }
        }
    }

    VkValidationCacheCreateInfoEXT validationCreateInfo;
    RenderToolsVulkan::ZeroStruct(validationCreateInfo, VK_STRUCTURE_TYPE_VALIDATION_CACHE_CREATE_INFO_EXT);
    validationCreateInfo.initialDataSize = data.Count();
    validationCreateInfo.pInitialData = data.Count() > 0 ? data.Get() : nullptr;
    const VkResult result = vkCreateValidationCacheEXT(Device, &validationCreateInfo, nullptr, &ValidationCache);
    LOG_VULKAN_RESULT(result);
}

bool GPUDeviceVulkan::SaveValidationCache()
{
    // Skip if missing
    if (ValidationCache == VK_NULL_HANDLE || !vkGetValidationCacheDataEXT)
        return false;

    // Query data size
    size_t dataSize = 0;
    VkResult result = vkGetValidationCacheDataEXT(Device, ValidationCache, &dataSize, nullptr);
    LOG_VULKAN_RESULT_WITH_RETURN(result);
    if (dataSize <= 0)
        return false;

    // Query data
    Array<byte> data;
    data.Resize((int32)dataSize);
    result = vkGetValidationCacheDataEXT(Device, ValidationCache, &dataSize, data.Get());
    LOG_VULKAN_RESULT_WITH_RETURN(result);

    // Save data
    String path;
    GetValidationCachePath(path);
    return File::WriteAllBytes(path, data);
}

#endif

bool GPUDeviceVulkan::IsVkFormatSupported(VkFormat vkFormat, VkFormatFeatureFlags wantedFeatureFlags, bool optimalTiling) const
{
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(Adapter->Gpu, vkFormat, &props);
    const VkFormatFeatureFlags featureFlags = optimalTiling ? props.optimalTilingFeatures : props.linearTilingFeatures;
    if ((featureFlags & wantedFeatureFlags) != wantedFeatureFlags)
        return false;

    //VkImageFormatProperties imageProps;
    //vkGetPhysicalDeviceImageFormatProperties(Adapter->Gpu, vkFormat, , &imageProps);

    return true;
}

GPUContext* GPUDeviceVulkan::GetMainContext()
{
    return reinterpret_cast<GPUContext*>(MainContext);
}

GPUAdapter* GPUDeviceVulkan::GetAdapter() const
{
    return static_cast<GPUAdapter*>(Adapter);
}

void* GPUDeviceVulkan::GetNativePtr() const
{
    return static_cast<void*>(Device);
}

static int32 GetMaxSampleCount(VkSampleCountFlags counts)
{
    if (counts & VK_SAMPLE_COUNT_64_BIT)
    {
        return VK_SAMPLE_COUNT_64_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_32_BIT)
    {
        return VK_SAMPLE_COUNT_32_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_16_BIT)
    {
        return VK_SAMPLE_COUNT_16_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_8_BIT)
    {
        return VK_SAMPLE_COUNT_8_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_4_BIT)
    {
        return VK_SAMPLE_COUNT_4_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_2_BIT)
    {
        return VK_SAMPLE_COUNT_2_BIT;
    }
    return VK_SAMPLE_COUNT_1_BIT;
}

bool GPUDeviceVulkan::Init()
{
    TotalGraphicsMemory = 0;

    _state = DeviceState::Created;
    const auto gpu = Adapter->Gpu;

    // Get queues properties
    uint32 queueCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueCount, nullptr);
    ASSERT(queueCount >= 1);
    QueueFamilyProps.AddDefault(queueCount);
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueCount, QueueFamilyProps.Get());

    // Query device features
    vkGetPhysicalDeviceFeatures(Adapter->Gpu, &PhysicalDeviceFeatures);

    // Get extensions and layers
    Array<const char*> deviceExtensions;
    Array<const char*> validationLayers;
    GetDeviceExtensionsAndLayers(gpu, deviceExtensions, validationLayers);
    ParseOptionalDeviceExtensions(deviceExtensions);

    // Setup device info
    VkDeviceCreateInfo deviceInfo;
    RenderToolsVulkan::ZeroStruct(deviceInfo, VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO);
    deviceInfo.enabledExtensionCount = deviceExtensions.Count();
    deviceInfo.ppEnabledExtensionNames = deviceExtensions.Get();
    deviceInfo.enabledLayerCount = validationLayers.Count();
    deviceInfo.ppEnabledLayerNames = deviceInfo.enabledLayerCount > 0 ? validationLayers.Get() : nullptr;

    // Setup queues info
    Array<VkDeviceQueueCreateInfo> queueFamilyInfos;
    int32 graphicsQueueFamilyIndex = -1;
    int32 computeQueueFamilyIndex = -1;
    int32 transferQueueFamilyIndex = -1;
    LOG(Info, "Found {0} queue families:", QueueFamilyProps.Count());
    uint32 numPriorities = 0;
    for (int32 familyIndex = 0; familyIndex < QueueFamilyProps.Count(); familyIndex++)
    {
        const VkQueueFamilyProperties& curProps = QueueFamilyProps[familyIndex];

        bool isValidQueue = false;
        if ((curProps.queueFlags & VK_QUEUE_GRAPHICS_BIT) == VK_QUEUE_GRAPHICS_BIT)
        {
            if (graphicsQueueFamilyIndex == -1)
            {
                graphicsQueueFamilyIndex = familyIndex;
                isValidQueue = true;
            }
            else
            {
                // TODO: Support for multi-queue and choose the best queue
            }
        }

        if ((curProps.queueFlags & VK_QUEUE_COMPUTE_BIT) == VK_QUEUE_COMPUTE_BIT)
        {
            if (computeQueueFamilyIndex == -1 && graphicsQueueFamilyIndex != familyIndex)
            {
                computeQueueFamilyIndex = familyIndex;
                isValidQueue = true;
            }
        }

        if ((curProps.queueFlags & VK_QUEUE_TRANSFER_BIT) == VK_QUEUE_TRANSFER_BIT)
        {
            // Favor a non-gfx transfer queue
            if (transferQueueFamilyIndex == -1 && (curProps.queueFlags & VK_QUEUE_GRAPHICS_BIT) != VK_QUEUE_GRAPHICS_BIT && (curProps.queueFlags & VK_QUEUE_COMPUTE_BIT) != VK_QUEUE_COMPUTE_BIT)
            {
                transferQueueFamilyIndex = familyIndex;
                isValidQueue = true;
            }
        }

        String queueTypeInfo;
        if ((curProps.queueFlags & VK_QUEUE_GRAPHICS_BIT) == VK_QUEUE_GRAPHICS_BIT)
            queueTypeInfo += TEXT(" graphics");
        if ((curProps.queueFlags & VK_QUEUE_COMPUTE_BIT) == VK_QUEUE_COMPUTE_BIT)
            queueTypeInfo += TEXT(" compute");
        if ((curProps.queueFlags & VK_QUEUE_TRANSFER_BIT) == VK_QUEUE_TRANSFER_BIT)
            queueTypeInfo += TEXT(" transfer");
        if ((curProps.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) == VK_QUEUE_SPARSE_BINDING_BIT)
            queueTypeInfo += TEXT(" sparse");

        if (!isValidQueue)
        {
            LOG(Info, "Skipping unnecessary queue family {0}: {1} queues{2}", familyIndex, curProps.queueCount, queueTypeInfo);
            continue;
        }

        const int32 queueIndex = queueFamilyInfos.Count();
        queueFamilyInfos.AddZeroed(1);
        VkDeviceQueueCreateInfo& curQueue = queueFamilyInfos[queueIndex];
        curQueue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        curQueue.queueFamilyIndex = familyIndex;
        curQueue.queueCount = curProps.queueCount;
        numPriorities += curProps.queueCount;
        LOG(Info, "- queue family {0}: {1} queues{2}", familyIndex, curProps.queueCount, queueTypeInfo);
    }
    Array<float> queuePriorities;
    queuePriorities.AddDefault(numPriorities);
    float* currentPriority = queuePriorities.Get();
    for (int32 index = 0; index < queueFamilyInfos.Count(); index++)
    {
        VkDeviceQueueCreateInfo& queue = queueFamilyInfos[index];
        queue.pQueuePriorities = currentPriority;
        const VkQueueFamilyProperties& properties = QueueFamilyProps[queue.queueFamilyIndex];
        for (int32 queueIndex = 0; queueIndex < (int32)properties.queueCount; queueIndex++)
        {
            *currentPriority++ = 1.0f;
        }
    }
    deviceInfo.queueCreateInfoCount = queueFamilyInfos.Count();
    deviceInfo.pQueueCreateInfos = queueFamilyInfos.Get();

    VkPhysicalDeviceFeatures enabledFeatures;
    VulkanPlatform::RestrictEnabledPhysicalDeviceFeatures(PhysicalDeviceFeatures, enabledFeatures);
    deviceInfo.pEnabledFeatures = &enabledFeatures;

    // Create the device
    VALIDATE_VULKAN_RESULT(vkCreateDevice(gpu, &deviceInfo, nullptr, &Device));

    // Optimize bindings
    volkLoadDevice(Device);

    // Create queues
    if (graphicsQueueFamilyIndex == -1)
    {
        LOG(Error, "Missing Vulkan graphics queue.");
        return true;
    }
    GraphicsQueue = New<QueueVulkan>(this, graphicsQueueFamilyIndex);
    ComputeQueue = computeQueueFamilyIndex != -1 ? New<QueueVulkan>(this, computeQueueFamilyIndex) : GraphicsQueue;
    TransferQueue = transferQueueFamilyIndex != -1 ? New<QueueVulkan>(this, transferQueueFamilyIndex) : GraphicsQueue;

    // Init device limits
    {
        PhysicalDeviceLimits = Adapter->GpuProps.limits;
        MSAALevel maxMsaa = MSAALevel::None;
        if (PhysicalDeviceFeatures.sampleRateShading)
        {
            const int32 framebufferColorSampleCounts = GetMaxSampleCount(PhysicalDeviceLimits.framebufferColorSampleCounts);
            const int32 framebufferDepthSampleCounts = GetMaxSampleCount(PhysicalDeviceLimits.framebufferDepthSampleCounts);
            maxMsaa = (MSAALevel)Math::Clamp(Math::Min<int32>(framebufferColorSampleCounts, framebufferDepthSampleCounts), 1, 8);
        }

        auto& limits = Limits;
        limits.HasCompute = GetShaderProfile() == ShaderProfile::Vulkan_SM5 && PhysicalDeviceLimits.maxComputeWorkGroupCount[0] >= GPU_MAX_CS_DISPATCH_THREAD_GROUPS && PhysicalDeviceLimits.maxComputeWorkGroupCount[1] >= GPU_MAX_CS_DISPATCH_THREAD_GROUPS;
        limits.HasTessellation = !!PhysicalDeviceFeatures.tessellationShader && PhysicalDeviceLimits.maxBoundDescriptorSets > (uint32_t)DescriptorSet::Domain;
        limits.HasGeometryShaders = false; // TODO: add geometry shaders support for Vulkan
        limits.HasInstancing = true;
        limits.HasVolumeTextureRendering = true;
        limits.HasDrawIndirect = false; // TODO: add Draw Indirect support for Vulkan
        limits.HasAppendConsumeBuffers = false; // TODO: add Append Consume buffers support for Vulkan
        limits.HasSeparateRenderTargetBlendState = true;
        limits.HasDepthAsSRV = true;
        limits.HasReadOnlyDepth = true;
        limits.HasMultisampleDepthAsSRV = !!PhysicalDeviceFeatures.sampleRateShading;
        limits.MaximumMipLevelsCount = Math::Min(static_cast<int32>(log2(PhysicalDeviceLimits.maxImageDimension2D)), GPU_MAX_TEXTURE_MIP_LEVELS);
        limits.MaximumTexture1DSize = PhysicalDeviceLimits.maxImageDimension1D;
        limits.MaximumTexture1DArraySize = PhysicalDeviceLimits.maxImageArrayLayers;
        limits.MaximumTexture2DSize = PhysicalDeviceLimits.maxImageDimension2D;
        limits.MaximumTexture2DArraySize = PhysicalDeviceLimits.maxImageArrayLayers;
        limits.MaximumTexture3DSize = PhysicalDeviceLimits.maxImageDimension3D;
        limits.MaximumTextureCubeSize = PhysicalDeviceLimits.maxImageDimensionCube;

        for (int32 i = 0; i < static_cast<int32>(PixelFormat::MAX); i++)
        {
            const auto format = static_cast<PixelFormat>(i);
            const auto vkFormat = RenderToolsVulkan::ToVulkanFormat(format);

            MSAALevel msaa = MSAALevel::None;
            FormatSupport support = FormatSupport::None;

            if (vkFormat != VK_FORMAT_UNDEFINED)
            {
                VkFormatProperties properties;
                Platform::MemoryClear(&properties, sizeof(properties));
                vkGetPhysicalDeviceFormatProperties(gpu, vkFormat, &properties);

                // Query image format features support flags
#define CHECK_IMAGE_FORMAT(bit, feature) if (((properties.linearTilingFeatures & bit) == bit) || ((properties.optimalTilingFeatures & bit) == bit)) support |= feature
                if (properties.linearTilingFeatures != 0 || properties.optimalTilingFeatures != 0)
                    support |= FormatSupport::Texture1D | FormatSupport::Texture2D | FormatSupport::Texture3D | FormatSupport::TextureCube;
                CHECK_IMAGE_FORMAT(VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT, FormatSupport::ShaderLoad);
                //VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT,
                //VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT
                CHECK_IMAGE_FORMAT(VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT, FormatSupport::RenderTarget);
                CHECK_IMAGE_FORMAT(VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT, FormatSupport::Blendable);
                CHECK_IMAGE_FORMAT(VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, FormatSupport::DepthStencil);
                //VK_FORMAT_FEATURE_BLIT_SRC_BIT
                //VK_FORMAT_FEATURE_BLIT_DST_BIT
                CHECK_IMAGE_FORMAT(VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT, FormatSupport::ShaderSample | FormatSupport::ShaderSampleComparison);
#undef CHECK_IMAGE_FORMAT

                // Query buffer format features support flags
#define CHECK_BUFFER_FORMAT(bit, feature) if ((properties.bufferFeatures & bit) == bit) support |= feature
                if (properties.bufferFeatures != 0)
                    support |= FormatSupport::Buffer;
                //VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT
                //VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT
                //VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT
                CHECK_BUFFER_FORMAT(VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT, FormatSupport::InputAssemblyVertexBuffer);
#undef CHECK_BUFFER_FORMAT

                // Unused bits
                //VK_FORMAT_FEATURE_TRANSFER_SRC_BIT
                //VK_FORMAT_FEATURE_TRANSFER_DST_BIT
                //VK_FORMAT_FEATURE_MIDPOINT_CHROMA_SAMPLES_BIT
                //VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_LINEAR_FILTER_BIT
                //VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_SEPARATE_RECONSTRUCTION_FILTER_BIT
                //VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_CHROMA_RECONSTRUCTION_EXPLICIT_BIT
                //VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_CHROMA_RECONSTRUCTION_EXPLICIT_FORCEABLE_BIT
                //VK_FORMAT_FEATURE_DISJOINT_BIT
                //VK_FORMAT_FEATURE_COSITED_CHROMA_SAMPLES_BIT
                //VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_CUBIC_BIT_IMG
                //VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_MINMAX_BIT_EXT

                // Multi-sampling support
                if (support & FormatSupport::Texture2D)
                    msaa = maxMsaa;
            }

            FeaturesPerFormat[i] = FormatFeatures(format, msaa, support);
        }
    }

    // Setup memory limit and print memory info
    {
        VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceMemoryProperties(gpu, &memoryProperties);
        LOG(Info, "Max memory allocations: {0}", Adapter->GpuProps.limits.maxMemoryAllocationCount);
        LOG(Info, "Found {0} device memory heaps:", memoryProperties.memoryHeapCount);
        for (uint32 i = 0; i < memoryProperties.memoryHeapCount; i++)
        {
            const VkMemoryHeap& heap = memoryProperties.memoryHeaps[i];
            bool isGPUHeap = (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) == VK_MEMORY_HEAP_DEVICE_LOCAL_BIT;
            LOG(Info, "-  memory heap {0}: flags 0x{1:x}, size {2} MB (GPU: {3})", i, heap.flags, (uint32)(heap.size / 1024 / 1024), isGPUHeap);
            if (isGPUHeap)
                TotalGraphicsMemory += heap.size;
        }
        LOG(Info, "Found {0} device memory types:", memoryProperties.memoryTypeCount);
        for (uint32 i = 0; i < memoryProperties.memoryTypeCount; i++)
        {
            const VkMemoryType& type = memoryProperties.memoryTypes[i];
            String flagsInfo;
            if ((type.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
                flagsInfo += TEXT("local, ");
            if ((type.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
                flagsInfo += TEXT("host visible, ");
            if ((type.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
                flagsInfo += TEXT("host coherent, ");
            if ((type.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) == VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
                flagsInfo += TEXT("host cached, ");
            if ((type.propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) == VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
                flagsInfo += TEXT("lazy, ");
            if (flagsInfo.HasChars())
                flagsInfo = TEXT(", properties: ") + flagsInfo.Left(flagsInfo.Length() - 2);
            LOG(Info, "-  memory type {0}: flags 0x{1:x}, heap {2}{3}", i, type.propertyFlags, type.heapIndex, flagsInfo);
        }
    }

    // Initialize memory allocator
    {
        VmaVulkanFunctions vulkanFunctions;
#define INIT_FUNC(name) vulkanFunctions.name = name
        INIT_FUNC(vkGetPhysicalDeviceProperties);
        INIT_FUNC(vkGetPhysicalDeviceMemoryProperties);
        INIT_FUNC(vkAllocateMemory);
        INIT_FUNC(vkFreeMemory);
        INIT_FUNC(vkMapMemory);
        INIT_FUNC(vkUnmapMemory);
        INIT_FUNC(vkFlushMappedMemoryRanges);
        INIT_FUNC(vkInvalidateMappedMemoryRanges);
        INIT_FUNC(vkBindBufferMemory);
        INIT_FUNC(vkBindImageMemory);
        INIT_FUNC(vkGetBufferMemoryRequirements);
        INIT_FUNC(vkGetImageMemoryRequirements);
        INIT_FUNC(vkCreateBuffer);
        INIT_FUNC(vkDestroyBuffer);
        INIT_FUNC(vkCreateImage);
        INIT_FUNC(vkDestroyImage);
        INIT_FUNC(vkCmdCopyBuffer);
#if VMA_DEDICATED_ALLOCATION
        INIT_FUNC(vkGetBufferMemoryRequirements2KHR);
        INIT_FUNC(vkGetImageMemoryRequirements2KHR);
#endif
#undef INIT_FUNC
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_0;
        allocatorInfo.physicalDevice = gpu;
        allocatorInfo.instance = Instance;
        allocatorInfo.device = Device;
        allocatorInfo.pVulkanFunctions = &vulkanFunctions;
        VALIDATE_VULKAN_RESULT(vmaCreateAllocator(&allocatorInfo, &Allocator));
    }

    // Prepare stuff
    FenceManager.Init(this);
    UniformBufferUploader = New<UniformBufferUploaderVulkan>(this);
    DescriptorPoolsManager = New<DescriptorPoolsManagerVulkan>(this);
    MainContext = New<GPUContextVulkan>(this, GraphicsQueue);
    // TODO: create and load PipelineCache
#if VK_EXT_validation_cache
    if (OptionalDeviceExtensions.HasEXTValidationCache && vkCreateValidationCacheEXT && vkDestroyValidationCacheEXT)
    {
        LoadValidationCache();
    }
#endif

    _state = DeviceState::Ready;
    return GPUDevice::Init();
}

void GPUDeviceVulkan::DrawBegin()
{
    // Base
    GPUDevice::DrawBegin();

    // Flush resources
    DeferredDeletionQueue.ReleaseResources();
    StagingManager.ProcessPendingFree();
    DescriptorPoolsManager->GC();
}

void GPUDeviceVulkan::Dispose()
{
    GPUDeviceLock lock(this);

    // Check if has been disposed already
    if (_state == DeviceState::Disposed)
        return;

    // Set current state
    _state = DeviceState::Disposing;

    // Wait for rendering end
    WaitForGPU();

    // Pre dispose
    preDispose();

    // Clear stuff
    _framebuffers.ClearDelete();
    _renderPasses.ClearDelete();
    _layouts.ClearDelete();
    HelperResources.Dispose();
    StagingManager.Dispose();
    TimestampQueryPools.ClearDelete();
    SAFE_DELETE_GPU_RESOURCE(UniformBufferUploader);
    Delete(DescriptorPoolsManager);
    SAFE_DELETE(MainContext);
    if (TransferQueue != GraphicsQueue && ComputeQueue != TransferQueue)
        SAFE_DELETE(TransferQueue);
    if (ComputeQueue != GraphicsQueue)
        SAFE_DELETE(ComputeQueue);
    SAFE_DELETE(GraphicsQueue);
    PresentQueue = nullptr;
    FenceManager.Dispose();
    DeferredDeletionQueue.ReleaseResources(true);
    vmaDestroyAllocator(Allocator);
    Allocator = VK_NULL_HANDLE;
    if (PipelineCache != VK_NULL_HANDLE)
    {
        vkDestroyPipelineCache(Device, PipelineCache, nullptr);
        PipelineCache = VK_NULL_HANDLE;
    }
#if VK_EXT_validation_cache
    if (ValidationCache != VK_NULL_HANDLE)
    {
        if (SaveValidationCache())
        {
            LOG(Warning, "Failed to save Vulkan validation cache");
        }
        vkDestroyValidationCacheEXT(Device, ValidationCache, nullptr);
        ValidationCache = VK_NULL_HANDLE;
    }
#endif

    // Destroy device
    vkDestroyDevice(Device, nullptr);
    Device = VK_NULL_HANDLE;
    SAFE_DELETE(Adapter);

    // Shutdown Vulkan
#if VULKAN_USE_DEBUG_LAYER
    RemoveDebugLayerCallback();
#endif
    vkDestroyInstance(Instance, nullptr);

    // Base
    GPUDevice::Dispose();

    // Set current state
    _state = DeviceState::Disposed;
}

void GPUDeviceVulkan::WaitForGPU()
{
    if (Device != VK_NULL_HANDLE)
    {
        VALIDATE_VULKAN_RESULT(vkDeviceWaitIdle(Device));
    }
}

GPUTexture* GPUDeviceVulkan::CreateTexture(const StringView& name)
{
    return New<GPUTextureVulkan>(this, name);
}

GPUShader* GPUDeviceVulkan::CreateShader(const StringView& name)
{
    return New<GPUShaderVulkan>(this, name);
}

GPUPipelineState* GPUDeviceVulkan::CreatePipelineState()
{
    return New<GPUPipelineStateVulkan>(this);
}

GPUTimerQuery* GPUDeviceVulkan::CreateTimerQuery()
{
    return New<GPUTimerQueryVulkan>(this);
}

GPUBuffer* GPUDeviceVulkan::CreateBuffer(const StringView& name)
{
    return New<GPUBufferVulkan>(this, name);
}

GPUSwapChain* GPUDeviceVulkan::CreateSwapChain(Window* window)
{
    return New<GPUSwapChainVulkan>(this, window);
}

SemaphoreVulkan::SemaphoreVulkan(GPUDeviceVulkan* device)
    : _device(device)
{
    // Create semaphore
    VkSemaphoreCreateInfo info;
    RenderToolsVulkan::ZeroStruct(info, VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
    VALIDATE_VULKAN_RESULT(vkCreateSemaphore(device->Device, &info, nullptr, &_semaphoreHandle));
}

SemaphoreVulkan::~SemaphoreVulkan()
{
    ASSERT(_semaphoreHandle != VK_NULL_HANDLE);
    _device->DeferredDeletionQueue.EnqueueResource(DeferredDeletionQueueVulkan::Semaphore, _semaphoreHandle);
    _semaphoreHandle = VK_NULL_HANDLE;
}

FenceVulkan::~FenceVulkan()
{
    ASSERT(_handle == VK_NULL_HANDLE);
}

FenceVulkan::FenceVulkan(GPUDeviceVulkan* device, FenceManagerVulkan* owner, bool createSignaled)
    : _signaled(createSignaled)
    , _owner(owner)
{
    VkFenceCreateInfo info;
    RenderToolsVulkan::ZeroStruct(info, VK_STRUCTURE_TYPE_FENCE_CREATE_INFO);
    info.flags = createSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
    VALIDATE_VULKAN_RESULT(vkCreateFence(device->Device, &info, nullptr, &_handle));
}

FenceManagerVulkan::~FenceManagerVulkan()
{
    ASSERT(_usedFences.IsEmpty());
}

void FenceManagerVulkan::Dispose()
{
    ScopeLock lock(_device->_fenceLock);

    ASSERT(_usedFences.IsEmpty());
    for (FenceVulkan* fence : _freeFences)
    {
        DestroyFence(fence);
    }
    _freeFences.Clear();
}

FenceVulkan* FenceManagerVulkan::AllocateFence(bool createSignaled)
{
    ScopeLock lock(_device->_fenceLock);

    FenceVulkan* fence;
    if (_freeFences.HasItems())
    {
        fence = _freeFences.Last();
        _freeFences.RemoveLast();
        _usedFences.Add(fence);

        if (createSignaled)
        {
            fence->_signaled = true;
        }

        return fence;
    }

    fence = New<FenceVulkan>(_device, this, createSignaled);
    _usedFences.Add(fence);
    return fence;
}

bool FenceManagerVulkan::WaitForFence(FenceVulkan* fence, uint64 timeInNanoseconds)
{
    ASSERT(_usedFences.Contains(fence));
    ASSERT(!fence->_signaled);

    const VkResult result = vkWaitForFences(_device->Device, 1, &fence->_handle, true, timeInNanoseconds);
    LOG_VULKAN_RESULT(result);
    if (result == VK_SUCCESS)
    {
        fence->_signaled = true;
        return false;
    }

    return true;
}

void FenceManagerVulkan::ResetFence(FenceVulkan* fence)
{
    if (fence->_signaled)
    {
        VALIDATE_VULKAN_RESULT(vkResetFences(_device->Device, 1, &fence->_handle));
        fence->_signaled = false;
    }
}

void FenceManagerVulkan::ReleaseFence(FenceVulkan*& fence)
{
    ScopeLock lock(_device->_fenceLock);

    ResetFence(fence);
    _usedFences.Remove(fence);
    _freeFences.Add(fence);
    fence = nullptr;
}

void FenceManagerVulkan::WaitAndReleaseFence(FenceVulkan*& fence, uint64 timeInNanoseconds)
{
    ScopeLock lock(_device->_fenceLock);

    if (!fence->IsSignaled())
    {
        WaitForFence(fence, timeInNanoseconds);
    }

    ResetFence(fence);
    _usedFences.Remove(fence);
    _freeFences.Add(fence);
    fence = nullptr;
}

bool FenceManagerVulkan::CheckFenceState(FenceVulkan* fence)
{
    ASSERT(_usedFences.Contains(fence));
    ASSERT(!fence->_signaled);

    const VkResult result = vkGetFenceStatus(_device->Device, fence->GetHandle());
    if (result == VK_SUCCESS)
    {
        fence->_signaled = true;
        return true;
    }

    return false;
}

void FenceManagerVulkan::DestroyFence(FenceVulkan* fence)
{
    vkDestroyFence(_device->Device, fence->GetHandle(), nullptr);
    fence->_handle = VK_NULL_HANDLE;
    Delete(fence);
}

GPUDevice* CreateGPUDeviceVulkan()
{
    return GPUDeviceVulkan::Create();
}

#endif
