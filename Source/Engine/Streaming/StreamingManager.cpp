// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "StreamingManager.h"
#include "StreamableResource.h"
#include "StreamingGroup.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Threading/Task.h"
#include "Engine/Graphics/GPUDevice.h"

namespace StreamingManagerImpl
{
    DateTime LastUpdateTime(0);
    CriticalSection UpdateLocker;
    int32 LastUpdateResourcesIndex = 0;
}

using namespace StreamingManagerImpl;

class StreamingManagerService : public EngineService
{
public:

    StreamingManagerService()
        : EngineService(TEXT("Streaming Manager"), 100)
    {
    }

    void Update() override;
};

StreamingManagerService StreamingManagerServiceInstance;

StreamableResourcesCollection StreamingManager::Resources;

void UpdateResource(StreamableResource* resource, DateTime now)
{
    ASSERT(resource && resource->CanBeUpdated());

    // Pick group and handler dedicated for that resource
    auto group = resource->GetGroup();
    auto handler = group->GetHandler();

    // Calculate target quality for that asset
    StreamingQuality targetQuality = MAX_STREAMING_QUALITY;
    if (resource->IsDynamic())
    {
        targetQuality = handler->CalculateTargetQuality(resource, now);
        // TODO: here we should apply resources group master scale (based on game settings quality level and memory level)
        targetQuality = Math::Clamp<StreamingQuality>(targetQuality, MIN_STREAMING_QUALITY, MAX_STREAMING_QUALITY);
    }

    // Update quality smoothing
    resource->Streaming.QualitySamples.Add(targetQuality);
    targetQuality = resource->Streaming.QualitySamples.Maximum();
    targetQuality = Math::Clamp<StreamingQuality>(targetQuality, MIN_STREAMING_QUALITY, MAX_STREAMING_QUALITY);

    // Calculate target residency level (discrete value)
    auto maxResidency = resource->GetMaxResidency();
    auto currentResidency = resource->GetCurrentResidency();
    auto allocatedResidency = resource->GetAllocatedResidency();
    auto targetResidency = handler->CalculateResidency(resource, targetQuality);
    ASSERT(allocatedResidency >= currentResidency && allocatedResidency >= 0);

    // Assign last update time
    auto updateTimeDelta = now - resource->Streaming.LastUpdate;
    resource->Streaming.LastUpdate = now;

    // Check if a target residency level has been changed
    if (targetResidency != resource->Streaming.TargetResidency)
    {
        // Register change
        resource->Streaming.TargetResidency = targetResidency;
        resource->Streaming.TargetResidencyChange = now;
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
        }

        // Calculate residency level to stream in (resources may want to incease/decrease it's quality in steps rather than at once)
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

void StreamingManagerService::Update()
{
    // Configuration
    // TODO: use game settings
    static TimeSpan ManagerUpdatesInterval = TimeSpan::FromMilliseconds(30);
    static TimeSpan ResourceUpdatesInterval = TimeSpan::FromMilliseconds(200);
    static int32 MaxResourcesPerUpdate = 50;

    // Check if skip update
    auto now = DateTime::NowUTC();
    auto delta = now - LastUpdateTime;
    int32 resourcesCount = StreamingManager::Resources.ResourcesCount();
    if (resourcesCount == 0 || delta < ManagerUpdatesInterval || GPUDevice::Instance->GetState() != GPUDevice::DeviceState::Ready)
        return;
    LastUpdateTime = now;

    PROFILE_CPU();

    // Start update
    ScopeLock lock(UpdateLocker);
    int32 resourcesUpdates = Math::Min(MaxResourcesPerUpdate, resourcesCount);

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
        const auto resource = StreamingManager::Resources[LastUpdateResourcesIndex];

        // Try to update it
        if (now - resource->Streaming.LastUpdate >= ResourceUpdatesInterval && resource->CanBeUpdated())
        {
            UpdateResource(resource, now);
            resourcesUpdates--;
        }
    }

    // TODO: add StreamingManager stats, update time per frame, updates per frame, etc.
}
