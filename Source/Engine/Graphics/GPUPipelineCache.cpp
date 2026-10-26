// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_PSO_PRECACHING

#include "GPUPipelineCache.h"
#include "GPUDevice.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Platform/Platform.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Profiler/ProfilerMemory.h"
#include "Engine/Content/Assets/Material.h"
#include "Engine/Content/Loading/ContentLoadTask.h"
#include "Engine/Graphics/GPUPipelineState.h"
#include "Engine/Graphics/Materials/IMaterial.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Threading/ConcurrentQueue.h"

// Maximum amount of jobs should be batches together
#define BATCH_SIZE_MAX 40

// Single PSO job to run in a batch (eg. single actor precaching).
struct PSOBatchJob
{
    GPUPipelineCache::BatchCallback Callback;
    volatile int32* Handle;
};

// Single PSO material job to run precaching of different shader permutations and vertex layouts at once (without locking asset).
struct PSOMaterialBatchJob
{
    GPUPipelineCacheBatch::MaterialPSO PSOs;
    Array<volatile int32*> Handles;
    bool IsRunning = false;
};

namespace
{
    ConcurrentQueue<PSOBatchJob> BatchJobs;
    volatile int64 PendingBatchTasks = 0; // Amount of PSO batch tasks that haven't started processing yet (meaning they can still pick up work from the queue)
#if GPU_ENABLE_DEVELOPMENT
    volatile int64 PrecacheAsyncMaterialJobsCounter = 0;
#endif
    GPUPipelineCacheBatch::DrawPassPSO* DrawPassList = nullptr;
    bool IsClosing = false;
    CriticalSection MaterialJobsLocker;
    Dictionary<Material*, PSOMaterialBatchJob> MaterialJobs;
};

// Async task that precaches PSO and runs on Content Pool threads.
class PSOTask : public ContentLoadTask
{
private:
    GPUPipelineCache::Callback _callback;
    volatile int32* _handle;

public:
    PSOTask(const GPUPipelineCache::Callback& callback, volatile int32* handle)
        : _callback(callback)
        , _handle(handle)
    {
    }

    Result run() override
    {
        PROFILE_CPU_NAMED(GPU_PSO_ZONE_NAME_PRECACHE);
        PROFILE_MEM(GraphicsShaders);
        ZoneColor(GPU_PSO_ZONE_COLOR_PRECACHE);
        _callback();
        if (_handle)
        {
            // Mark as done
            Platform::InterlockedDecrement(_handle);
        }
        return Result::Ok;
    }
};

// Async task that precaches batches of PSO and runs on Content Pool threads.
class PSOBatchTask : public ContentLoadTask
{
public:
    Result run() override
    {
        PROFILE_CPU_NAMED(GPU_PSO_ZONE_NAME_PRECACHE);
        PROFILE_MEM(GraphicsShaders);
        ZoneColor(GPU_PSO_ZONE_COLOR_PRECACHE);

        // Pickup jobs to run
        // Retry multiple times to fill up the buffer for good utilization
        constexpr double MaxPickupTime = 0.0001; // 0.1ms
        PSOBatchJob buffer[BATCH_SIZE_MAX];
        size_t bufferCount = 0;
        double startTime = Platform::GetTimeSeconds();
        do
        {
            Platform::Yield();
            size_t count = BatchJobs.try_dequeue_bulk(buffer + bufferCount, BATCH_SIZE_MAX - bufferCount);
            bufferCount += count;
        } while (bufferCount < BATCH_SIZE_MAX && Platform::GetTimeSeconds() - startTime < MaxPickupTime);
        Platform::InterlockedDecrement(&PendingBatchTasks);

        // Collect batch of PSOs to process (eg. from actors)
        GPUPipelineCacheBatch batch;
        for (int32 i = 0; i < bufferCount; i++)
            buffer[i].Callback(batch);

        // Combine job handles into one array
        Array<volatile int32*, FixedAllocation<BATCH_SIZE_MAX>> batchHandles;
        for (int32 i = 0; i < bufferCount; i++)
        {
            if (buffer[i].Handle)
                batchHandles.Add(buffer[i].Handle);
        }

        //LOG(Error, "PSO batch size: {} with {} materials", bufferCount, batch.Materials.Count());

        // Increment job handles by amount of materials so each material processing can decrement it and job will complete when all materials from batch are done
        {
            auto it = batch.Materials.Begin();
            while ((++it).IsNotEnd())
            {
                for (auto handle : batchHandles)
                    Platform::InterlockedIncrement(handle);
            }
        }

        // Go over all collected materials
        for (auto& e : batch.Materials)
        {
            ASSERT(e.Key);
            PSOMaterialBatchJob stolenJob;

            // Check if this material is already during processing (otherwise multiple task could cause multithreading issues inside GPUPipelineState or inefficient locking)
            MaterialJobsLocker.Lock();
            {
                auto& job = MaterialJobs[e.Key];
                if (job.IsRunning)
                {
                    // Put PSOs for this material in a shared buffer to process later at once
                    job.PSOs |= e.Value;

                    // Duplicate handles
                    job.Handles.Add(batchHandles);

                    // Move to the next material in a batch
                    MaterialJobsLocker.Unlock();
                    continue;
                }
                else
                {
                    // Steal accumulated jobs (from other tasks)
                    stolenJob = MoveTemp(job);
                    ASSERT(job.Handles.IsEmpty() && job.PSOs.VertexLayouts.IsEmpty());

                    // Mark material as active by this task
                    job.IsRunning = true;
                }
            }
            MaterialJobsLocker.Unlock();

            // Merge stolen jobs with current material
            auto eHandles = ToSpan(batchHandles);
            if (stolenJob.Handles.HasItems())
            {
                stolenJob.Handles.Add(batchHandles);
                e.Value |= stolenJob.PSOs;
                eHandles = ToSpan(stolenJob.Handles);
            }

            // Process this material now
            Run(e.Key, e.Value, eHandles);
        }

    RETRY_TASK:
        // Go over all skipped materials to resume precaching when noone else is running them
        Platform::Yield();
        bool gotMaterial = true;
        while (gotMaterial)
        {
            gotMaterial = false;
            PSOMaterialBatchJob job;
            Material* material = nullptr;
            MaterialJobsLocker.Lock();
            for (auto& e : MaterialJobs)
            {
                if (!e.Value.IsRunning && e.Value.Handles.HasItems())
                {
                    // Run this material job now and steal the work items
                    gotMaterial = true;
                    material = e.Key;
                    job = MoveTemp(e.Value);
                    e.Value.PSOs = GPUPipelineCacheBatch::MaterialPSO();
                    e.Value.IsRunning = true;
                    ASSERT(e.Value.Handles.IsEmpty());
                    break;
                }
            }
            MaterialJobsLocker.Unlock();

            if (gotMaterial)
            {
                // Process this material now
                Run(material, job.PSOs, ToSpan(job.Handles));
            }
        }

        // At this point all materials are processed or used by other tasks so free unused material jobs from the list
        Platform::Yield();
        MaterialJobsLocker.Lock();
        int32 idleJobsLeft = 0;
        for (auto it = MaterialJobs.Begin(); it.IsNotEnd(); ++it)
        {
            auto& pso = it->Value;
            if (!pso.IsRunning && pso.Handles.IsEmpty())
            {
                MaterialJobs.Remove(it);
            }
            else if (!pso.IsRunning)
            {
                idleJobsLeft++;
            }
        }
        MaterialJobsLocker.Unlock();

        // Go back to processing in case any material is still waiting in a queue
        if (idleJobsLeft > 0)
            goto RETRY_TASK;

        return Result::Ok;
    }

    void Run(Material* material, GPUPipelineCacheBatch::MaterialPSO& pso, Span<volatile int32*> handles)
    {
        PROFILE_CPU_ASSET(material);
        ZoneValue(handles.Length());

        // Precache PSOs for all draw passes (such as Forward, GBuffer, Motion Vectors, etc.)
        IMaterial::PrecacheParameters params;
        GPUPipelineCacheBatch::MaterialPSO materialBatches = MoveTemp(pso);
        DrawPass drawModes = materialBatches.DrawModes & material->GetDrawModes();
        for (auto drawPass = DrawPassList; drawPass; drawPass = drawPass->Next)
        {
            GPUPipelineState::PrecacheContext context;
            params.Pass = DrawPass::None;
            params.Domain = (uint32)GPUPipelineCacheBatch::PassDomains::DrawCalls;
            params.PrecacheContext = &context;
            params.MaterialFlags = materialBatches.MaterialFlags;
            drawPass->Callback(&params);
            params.Pass &= drawModes;
            if (params.Pass == DrawPass::None || params.Domain != (uint32)pso.Domain)
                continue; // Pass is inactive (or not matching material/actors usage flags)

            // For all used vertex layouts
            for (GPUVertexLayout* vertexLayout : materialBatches.VertexLayouts)
            {
                context.VertexLayout = vertexLayout;
                material->PrecachePSO(params);
            }
        }

        // Mark jobs as done
        for (auto handle : handles)
            Platform::InterlockedDecrement(handle);

        // Mark as done
        MaterialJobsLocker.Lock();
        ASSERT(MaterialJobs[material].IsRunning);
        MaterialJobs[material].IsRunning = false;
        MaterialJobsLocker.Unlock();
#if GPU_ENABLE_DEVELOPMENT
        Platform::InterlockedIncrement(&PrecacheAsyncMaterialJobsCounter);
#endif
    }
};

class GPUPipelineCacheService : public EngineService
{
public:
    GPUPipelineCacheService()
        : EngineService(TEXT("GPUPipelineCache"), -5)
    {
    }

    void BeforeExit() override
    {
        IsClosing = true;
    }
};

GPUPipelineCacheService GPUPipelineCacheServiceInstance;
GPUPipelineCacheModes GPUPipelineCache::CacheModes = GPUPipelineCacheModes::None;
GPUPipelineMissModes GPUPipelineCache::MissModes = GPUPipelineMissModes::SkipDraw;

#if GPU_ENABLE_DEVELOPMENT

// Utility for measuring time of the hitches with basic statistics.
struct Counter
{
    static constexpr double TimeFreq = 1000000; // 0.01ms
    volatile int64 Count = 0;
    volatile int64 TimeSum = 0;
    volatile int64 TimeMin = MAX_int64;
    volatile int64 TimeMax = 0;

    void Register(double time)
    {
        Platform::InterlockedIncrement(&Count);
        int64 t = (int64)(time * TimeFreq); // Convert to cycles count
        Platform::InterlockedAdd(&TimeSum, t);
        Platform::AtomicStore(&TimeMax, Math::Max(Platform::AtomicRead(&TimeMax), t));
    }

    void Log()
    {
        LOG(Info, "    > Count: {}", Count);
        if (Count == 0)
            return;
        double inv = (1000.0 / TimeFreq); // Convert cycles count to miliseconds
        LOG(Info, "    > Time: avg {:.2f} ms (max: {:.2f} ms, sum: {:.1f}s)", TimeSum * inv / Count, TimeMax * inv, TimeSum * inv * 0.001);
    }
};

namespace
{
    constexpr double HitchMinTime = 0.005; // 5ms
    volatile int64 MissCounter = 0;
    volatile int64 PrecacheAsyncJobsCounter = 0;
    volatile int64 PrecacheAsyncBatchJobsCounter = 0;
    Counter PrecachingCounter;
    Counter LookupCounter;
    Counter HitchCounter;
};

GPUPipelineLookup::GPUPipelineLookup()
{
    StartTime = Platform::GetTimeSeconds();
    Flags = Empty;
}

GPUPipelineLookup::~GPUPipelineLookup()
{
    double time = Platform::GetTimeSeconds() - StartTime;
    if (EnumHasAnyFlags(Flags, Precaching))
    {
        PrecachingCounter.Register(time);
    }
    else
    {
        LookupCounter.Register(time);
        if (time > HitchMinTime)
        {
            HitchCounter.Register(time);
        }
        if (EnumHasAnyFlags(Flags, Miss))
            Platform::InterlockedIncrement(&MissCounter);
    }
}

void GPUDevice::DumpPSOs()
{
    LOG(Info, "GPU PSO precache stats:");
    PrecachingCounter.Log();
    LOG(Info, "    > Async Jobs: {}", PrecacheAsyncJobsCounter);
    LOG(Info, "    > Async Batch Jobs: {}", PrecacheAsyncBatchJobsCounter);
    LOG(Info, "    > Async Material Jobs: {}", PrecacheAsyncMaterialJobsCounter);
    LOG(Info, "GPU PSO lookup stats:");
    LookupCounter.Log();
    LOG(Info, "    > Misses: {}", MissCounter);
    LOG(Info, "GPU PSO hitch stats:");
    HitchCounter.Log();
}

#endif

GPUPipelineCacheBatch::DrawPassPSO::DrawPassPSO(void(*callback)(void* paramsPtr))
    : Callback(callback)
    , Next(DrawPassList)
{
    DrawPassList = this;
}

GPUPipelineCacheBatch::MaterialPSO& operator|=(GPUPipelineCacheBatch::MaterialPSO& a, const GPUPipelineCacheBatch::MaterialPSO& b)
{
    a.DrawModes |= b.DrawModes;
    a.MaterialFlags |= b.MaterialFlags;
    a.VertexLayouts.AddUnique(b.VertexLayouts);
    return a;
}

void GPUPipelineCache::PrecacheAsync(const Callback& callback, volatile int32* handle)
{
#if GPU_ENABLE_DEVELOPMENT
    Platform::InterlockedIncrement(&PrecacheAsyncJobsCounter);
#endif
    if (handle)
        *handle = 1;
    PROFILE_MEM(GraphicsShaders);

    // Spawn a task
    New<PSOTask>(callback, handle)->Start();
}

void GPUPipelineCache::PrecacheAsync(const BatchCallback& callback, volatile int32* handle)
{
#if GPU_ENABLE_DEVELOPMENT
    Platform::InterlockedIncrement(&PrecacheAsyncBatchJobsCounter);
#endif
    if (handle)
        *handle = 1;
    PROFILE_MEM(GraphicsShaders);

    // Add to the batch
    BatchJobs.Add({ callback, handle });

    // Check if start a new batching task (avoid overspamming tasks but ensure sth can pick up a newly added job)
    constexpr uint32 headroom = 10;
    uint32 neededTasks = Math::DivideAndRoundUp<uint32>((uint32)BatchJobs.size_approx() + headroom, BATCH_SIZE_MAX);
    if (Platform::AtomicRead(&PendingBatchTasks) < neededTasks)
    {
        // Spawn a task
        Platform::InterlockedIncrement(&PendingBatchTasks);
        New<PSOBatchTask>()->Start();
    }
}

bool GPUPipelineCache::IsPrecacheReady(volatile int32* handle, bool wait)
{
    if (*handle <= 0 || Platform::AtomicRead(handle) <= 0)
        return true;
    if (IsClosing)
        return false;
    if (wait)
    {
        // Wait for the task
        PROFILE_CPU();
        ZoneColor(TracyWaitZoneColor);
        do
        {
            Platform::Sleep(1);
        } while (Platform::AtomicRead(handle) > 0 && !IsClosing);
    }
    return Platform::AtomicRead(handle) <= 0;
}

#endif
