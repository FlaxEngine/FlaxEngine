// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_PSO_PRECACHING

#include "GPUPipelineStateCache.h"
#include "GPUDevice.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"

#if !BUILD_RELEASE

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
        Platform::AtomicStore(&TimeMin, Math::Min(Platform::AtomicRead(&TimeMin), t));
        Platform::AtomicStore(&TimeMax, Math::Max(Platform::AtomicRead(&TimeMax), t));
    }

    void Log()
    {
        LOG(Info, "    > Count: {}", Count);
        if (Count == 0)
            return;
        double inv = (1000.0 / TimeFreq); // Convert cycles count to miliseconds
        LOG(Info, "    > Lookup time: avg {:.2f} ms (min: {:.2f}, max: {:.2f})", TimeSum * inv / Count, TimeMin * inv, TimeMax * inv);
    }
};

namespace
{
    constexpr double HitchMinTime = 0.004; // 4ms
    Counter LookupCounter;
    Counter HitchCounter;
};

GPUPipelineStateLookup::~GPUPipelineStateLookup()
{
    double time = Platform::GetTimeSeconds() - StartTime;
    LookupCounter.Register(time);
    if (time > HitchMinTime)
    {
        HitchCounter.Register(time);
    }
}

void GPUDevice::DumpPSOs()
{
    LOG(Info, "GPU PSO cache stats:");
    LookupCounter.Log();
    LOG(Info, "GPU PSO hitch stats:");
    HitchCounter.Log();
}

#endif

#endif
