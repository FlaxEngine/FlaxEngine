// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_PSO_PRECACHING

#include "GPUPipelineCache.h"
#include "GPUDevice.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Profiler/ProfilerMemory.h"
#include "Engine/Content/Loading/ContentLoadTask.h"

// Async handle values:
#define HANDLE_EMPTY 0
#define HANDLE_ACTIVE 1
#define HANDLE_FINISHED 2

// Async task that precaches PSO and runs on Content Pool threads.
class PSOTask : public ContentLoadTask
{
private:
    Function<void()> _callback;
    volatile int32* _handle;

public:
    PSOTask(const Function<void()>& callback, volatile int32* handle)
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
            Platform::AtomicStore(_handle, HANDLE_FINISHED);
        }
        return Result::Ok;
    }
};

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
    constexpr double HitchMinTime = 0.004; // 4ms
    volatile int64 MissCounter = 0;
    volatile int64 PrecacheAsyncJobsCounter = 0;
    Counter PrecachingCounter;
    Counter LookupCounter;
    Counter HitchCounter;
};

GPUPipelineCacheModes GPUPipelineCache::CacheModes = GPUPipelineCacheModes::None;
GPUPipelineMissModes GPUPipelineCache::MissModes = GPUPipelineMissModes::SkipDraw;

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
    LOG(Info, "GPU PSO lookup stats:");
    LookupCounter.Log();
    LOG(Info, "    > Misses: {}", MissCounter);
    LOG(Info, "GPU PSO hitch stats:");
    HitchCounter.Log();
}

#endif

void GPUPipelineCache::PrecacheAsync(const Function<void()>& callback, volatile int32* handle)
{
    Platform::InterlockedIncrement(&PrecacheAsyncJobsCounter);
    if (handle)
        *handle = HANDLE_ACTIVE;
    PROFILE_MEM(GraphicsShaders);
    New<PSOTask>(callback, handle)->Start();
}

bool GPUPipelineCache::IsPrecacheReady(volatile int32* handle, bool wait)
{
    if (Platform::AtomicRead(handle) != HANDLE_ACTIVE)
        return true;
    if (wait)
    {
        // Wait for the task
        PROFILE_CPU();
        ZoneColor(TracyWaitZoneColor);
        do
        {
            Platform::Sleep(1);
        } while (Platform::AtomicRead(handle) == HANDLE_ACTIVE);
    }
    return Platform::AtomicRead(handle) == HANDLE_FINISHED;
}

#endif
