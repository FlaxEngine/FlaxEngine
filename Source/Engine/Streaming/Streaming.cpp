// Copyright (c) Wojciech Figat. All rights reserved.

#include "Streaming.h"
#include "StreamableResource.h"
#include "StreamingGroup.h"
#include "StreamingSettings.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Threading/TaskGraph.h"
#include "Engine/Threading/Task.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/Textures/GPUSampler.h"
#include "Engine/Serialization/Serialization.h"

namespace StreamingManagerImpl
{
    int32 LastUpdateResourcesIndex = 0;
    CriticalSection ResourcesLock;
    Array<StreamableResource*> Resources;
    Array<GPUSampler*, InlinedAllocation<32>> TextureGroupSamplers;
    GPUSampler* FallbackSampler = nullptr;
}

using namespace StreamingManagerImpl;

class StreamingService : public EngineService
{
public:
    StreamingService()
        : EngineService(TEXT("Streaming"), 100)
    {
    }

    bool Init() override;
    void BeforeExit() override;
};

class StreamingSystem : public TaskGraphSystem
{
public:
    void Job(int32 index);
    void Execute(TaskGraph* graph) override;
};

namespace
{
    TaskGraphSystem* System = nullptr;
}

StreamingService StreamingServiceInstance;

Array<TextureGroup, InlinedAllocation<32>> Streaming::TextureGroups;

void StreamingSettings::Apply()
{
    Streaming::TextureGroups = TextureGroups;
    SAFE_DELETE_GPU_RESOURCES(TextureGroupSamplers);
    TextureGroupSamplers.Resize(TextureGroups.Count(), false);
}

StreamableResource::StreamableResource(StreamingGroup* group)
    : _group(group)
    , _isDynamic(true)
    , _isStreaming(false)
    , _streamingQuality(1.0f)
{
    ASSERT(_group != nullptr);
}

StreamableResource::~StreamableResource()
{
    StopStreaming();
}

void StreamableResource::RequestStreamingUpdate()
{
    Streaming.LastUpdateTime = 0.0;
}

void StreamableResource::ResetStreaming(bool error)
{
    Streaming.Error = error;
    Streaming.TargetResidency = 0;
    Streaming.LastUpdateTime = 3e+30f; // Very large number to skip any updates
}

void StreamableResource::StartStreaming(bool isDynamic)
{
    _isDynamic = isDynamic;
    if (!_isStreaming)
    {
        _isStreaming = true;
        ResourcesLock.Lock();
        Resources.Add(this);
        ResourcesLock.Unlock();
    }
}

void StreamableResource::StopStreaming()
{
    if (_isStreaming)
    {
        ResourcesLock.Lock();
        Resources.Remove(this);
        ResourcesLock.Unlock();
        Streaming = StreamingCache();
        _isStreaming = false;
    }
}

void UpdateResource(StreamableResource* resource, double currentTime)
{
    ASSERT(resource && resource->CanBeUpdated());

    // Pick group and handler dedicated for that resource
    auto group = resource->GetGroup();
    auto handler = group->GetHandler();

    // Calculate target quality for that asset
    float targetQuality = 1.0f;
    if (resource->IsDynamic())
    {
        targetQuality = handler->CalculateTargetQuality(resource, currentTime);
        targetQuality = Math::Saturate(targetQuality);
    }

    // Update quality smoothing
    resource->Streaming.QualitySamples.Add(targetQuality);
    targetQuality = resource->Streaming.QualitySamples.Maximum();
    targetQuality = Math::Saturate(targetQuality);

    // Calculate target residency level (discrete value)
    auto maxResidency = resource->GetMaxResidency();
    auto currentResidency = resource->GetCurrentResidency();
    auto allocatedResidency = resource->GetAllocatedResidency();
    auto targetResidency = handler->CalculateResidency(resource, targetQuality);
    ASSERT(allocatedResidency >= currentResidency && allocatedResidency >= 0);
    resource->Streaming.LastUpdateTime = currentTime;

    // Check if a target residency level has been changed
    if (targetResidency != resource->Streaming.TargetResidency)
    {
        // Register change
        resource->Streaming.TargetResidency = targetResidency;
        resource->Streaming.TargetResidencyChangeTime = currentTime;
    }

    // Check if need to change resource current residency
    if (handler->RequiresStreaming(resource, currentResidency, targetResidency))
    {
        // Check if need to change allocation for that resource
        if (allocatedResidency != targetResidency)
        {
            // Update resource allocation
            Task* allocateTask = resource->UpdateAllocation(targetResidency);
            if (allocateTask)
            {
                // When resource wants to perform reallocation on a task then skip further updating until it's done
                allocateTask->Start();
                resource->RequestStreamingUpdate();
                return;
            }
            else if (resource->GetAllocatedResidency() < targetResidency)
            {
                // Allocation failed (eg. texture format is not supported or run out of memory)
                resource->ResetStreaming();
                return;
            }
        }

        // Calculate residency level to stream in (resources may want to increase/decrease it's quality in steps rather than at once)
        int32 requestedResidency = handler->CalculateRequestedResidency(resource, targetResidency);

        // Create streaming task (resource type specific)
        Task* streamingTask = resource->CreateStreamingTask(requestedResidency);
        if (streamingTask != nullptr)
        {
            streamingTask->Start();
        }
    }
    else
    {
        // TODO: Check if target residency is stable (no changes for a while)

        // TODO: deallocate or decrease memory usage after timeout? (timeout should be smaller on low mem)
    }

    // low memory case:
    // if we are on budget and cannot load everything we have to:
    // decrease global resources quality scale (per resources group)
    // decrease asset deallocate timeout

    // low mem detecting:
    // for low mem we have to get whole memory budget for a group and then
    // subtract immutable resources mem usage (like render targets or non dynamic resources)
    // so we get amount of memory we can distribute and we can detect if we are out of the limit
    // low mem should be updated once per a few frames
}

bool StreamingService::Init()
{
    System = New<StreamingSystem>();
    Engine::UpdateGraph->AddSystem(System);
    return false;
}

void StreamingService::BeforeExit()
{
    SAFE_DELETE_GPU_RESOURCE(FallbackSampler);
    SAFE_DELETE_GPU_RESOURCES(TextureGroupSamplers);
    TextureGroupSamplers.Resize(0);
    SAFE_DELETE(System);
}

void StreamingSystem::Job(int32 index)
{
    PROFILE_CPU_NAMED("Streaming.Job");

    // TODO: use streaming settings
    const double ResourceUpdatesInterval = 0.1;
    int32 MaxResourcesPerUpdate = 50;

    // Start update
    ScopeLock lock(ResourcesLock);
    const int32 resourcesCount = Resources.Count();
    int32 resourcesUpdates = Math::Min(MaxResourcesPerUpdate, resourcesCount);
    const double currentTime = Platform::GetTimeSeconds();

    // Update high priority queue and then rest of the resources
    // Note: resources in the update queue are updated always, while others only between specified intervals
    int32 resourcesChecks = resourcesCount;
    while (resourcesUpdates > 0 && resourcesChecks-- > 0)
    {
        // Move forward
        LastUpdateResourcesIndex++;
        if (LastUpdateResourcesIndex >= resourcesCount)
            LastUpdateResourcesIndex = 0;

        // Peek resource
        const auto resource = Resources[LastUpdateResourcesIndex];

        // Try to update it
        if (currentTime - resource->Streaming.LastUpdateTime >= ResourceUpdatesInterval && resource->CanBeUpdated())
        {
            UpdateResource(resource, currentTime);
            resourcesUpdates--;
        }
    }

    // TODO: add StreamingManager stats, update time per frame, updates per frame, etc.
}

void StreamingSystem::Execute(TaskGraph* graph)
{
    if (Resources.Count() == 0 || GPUDevice::Instance->GetState() != GPUDevice::DeviceState::Ready)
        return;

    // Schedule work to update all storage containers in async
    Function<void(int32)> job;
    job.Bind<StreamingSystem, &StreamingSystem::Job>(this);
    graph->DispatchJob(job, 1);
}

StreamingStats Streaming::GetStats()
{
    StreamingStats stats;
    ResourcesLock.Lock();
    stats.ResourcesCount = Resources.Count();
    for (auto e : Resources)
    {
        if (e->Streaming.TargetResidency > e->GetCurrentResidency())
            stats.StreamingResourcesCount++;
    }
    ResourcesLock.Unlock();
    return stats;
}

void Streaming::RequestStreamingUpdate()
{
    PROFILE_CPU();
    ResourcesLock.Lock();
    for (auto e : Resources)
        e->RequestStreamingUpdate();
    ResourcesLock.Unlock();
}

GPUSampler* Streaming::GetTextureGroupSampler(int32 index)
{
    GPUSampler* sampler = nullptr;
    if (index >= 0 && index < TextureGroupSamplers.Count())
    {
        // Sampler from texture group options
        auto& group = TextureGroups[index];
        auto desc = GPUSamplerDescription::New(group.SamplerFilter);
        desc.MaxAnisotropy = group.MaxAnisotropy;
        sampler = TextureGroupSamplers[index];
        if (!sampler)
        {
            sampler = GPUSampler::New();
#if GPU_ENABLE_RESOURCE_NAMING
            sampler->SetName(group.Name);
#endif
            sampler->Init(desc);
            TextureGroupSamplers[index] = sampler;
        }
        if (sampler->GetDescription().Filter != desc.Filter || sampler->GetDescription().MaxAnisotropy != desc.MaxAnisotropy)
            sampler->Init(desc);
    }
    if (!sampler)
    {
        // Default sampler to prevent issue
        if (!FallbackSampler)
        {
            FallbackSampler = GPUSampler::New();
#if GPU_ENABLE_RESOURCE_NAMING
            FallbackSampler->SetName(TEXT("FallbackSampler"));
#endif
            FallbackSampler->Init(GPUSamplerDescription::New(GPUSamplerFilter::Trilinear));
        }
        sampler = FallbackSampler;
    }
    return sampler;
}
