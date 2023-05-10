// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#if COMPILE_WITH_PROFILER

#include "ProfilingTools.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Engine/Time.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Graphics/GPUDevice.h"

ProfilingTools::MainStats ProfilingTools::Stats;
Array<ProfilingTools::ThreadStats, InlinedAllocation<64>> ProfilingTools::EventsCPU;
Array<ProfilerGPU::Event> ProfilingTools::EventsGPU;

class ProfilingToolsService : public EngineService
{
public:
    ProfilingToolsService()
        : EngineService(TEXT("Profiling Tools"))
    {
        Platform::MemoryClear(&ProfilingTools::Stats, sizeof(ProfilingTools::MainStats));
    }

    void Update() override;
    void Dispose() override;
};

ProfilingToolsService ProfilingToolsServiceInstance;

void ProfilingToolsService::Update()
{
    ZoneScoped;

    // Capture stats
    {
        auto& stats = ProfilingTools::Stats;

        stats.ProcessMemory = Platform::GetProcessMemoryStats();
        stats.MemoryCPU = Platform::GetMemoryStats();
        stats.MemoryGPU.Total = GPUDevice::Instance->TotalGraphicsMemory;
        stats.MemoryGPU.Used = GPUDevice::Instance->GetMemoryUsage();
        stats.FPS = Engine::GetFramesPerSecond();

        stats.UpdateTimeMs = static_cast<float>(Time::Update.LastLength * 1000.0);
        stats.PhysicsTimeMs = static_cast<float>(Time::Physics.LastLength * 1000.0);
        stats.DrawCPUTimeMs = static_cast<float>(Time::Draw.LastLength * 1000.0);

        ProfilerGPU::GetLastFrameData(stats.DrawGPUTimeMs, stats.DrawStats);
    }

    // Extract CPU profiler events
    Platform::MemoryBarrier();
    const auto& threads = ProfilerCPU::Threads;
    Platform::MemoryBarrier();
    for (auto& pt : ProfilingTools::EventsCPU)
        pt.Events.Clear();
    ProfilingTools::EventsCPU.EnsureCapacity(threads.Count());
    for (int32 i = 0; i < threads.Count(); i++)
    {
        ProfilerCPU::Thread* thread = threads[i];
        if (thread == nullptr)
            continue;
        ProfilingTools::ThreadStats* pt = nullptr;
        for (auto& e : ProfilingTools::EventsCPU)
        {
            if (e.Name == thread->GetName())
            {
                pt = &e;
                break;
            }
        }
        if (!pt)
        {
            pt = &ProfilingTools::EventsCPU.AddOne();
            pt->Name = thread->GetName();
        }

        thread->Buffer.Extract(pt->Events, true);
    }

#if 0
    // Print CPU threads events to the log
    for (auto& pt : ProfilingTools::EventsCPU)
    {
        auto& events = pt.Events;
        if (events.HasItems())
        {
            LOG_FLOOR();
            LOG(Info, "Thread: {0}", pt.Name);
            for (int j = 0; j < events.Count(); j++)
            {
                auto e = events[j];
                String prev;
                for (int d = 0; d < e.Depth; d++)
                    prev += TEXT("\t");
                LOG(Warning, "{2}{0}, Time: {1} ms", e.Name, ((int)((e.End - e.Start) * 1000.0f) / 1000.0f), prev);
            }
            LOG(Info, "");
            LOG_FLOOR();
        }
    }
#endif

    // Get the last resolved GPU frame events
    ProfilingTools::EventsGPU.Clear();
    uint64 maxFrame = 0;
    int32 maxFrameIndex = -1;
    auto& frames = ProfilerGPU::Buffers;
    for (uint32 i = 0; i < ARRAY_COUNT(frames); i++)
    {
        if (frames[i].HasData() && frames[i].FrameIndex > maxFrame)
        {
            maxFrame = frames[i].FrameIndex;
            maxFrameIndex = i;
        }
    }
    if (maxFrameIndex != -1)
    {
        auto& frame = frames[maxFrameIndex];
        frame.Extract(ProfilingTools::EventsGPU);
    }

#if 0
    // Print CPU events to the log
    {
        if (ProfilingTools::EventsCPU.HasItems())
        {
            LOG_FLOOR();
            LOG(Info, "CPU");
            for (auto& pt : ProfilingTools::EventsCPU)
            {
                LOG(Info, "");
                LOG(Warning, "Thread {0}", pt.Name);
                for (auto& e : pt.Events)
                {
                    String prev;
                    for (int32 d = 0; d < e.Depth; d++)
                        prev += TEXT("\t");
                    const double time = e.End - e.Start;
                    LOG(Warning, "\t{2}{0}, Time: {1} ms", e.Name, ((int32)(time * 1000.0f) / 1000.0f), prev);
                }
            }
            LOG(Info, "");
            LOG_FLOOR();
        }
    }
#endif
#if 0
    // Print GPU events to the log
    {
        auto& events = ProfilingTools::EventsGPU;
        if (events.HasItems())
        {
            LOG_FLOOR();
            LOG(Info, "GPU");
            for (int j = 0; j < events.Count(); j++)
            {
                auto e = events[j];
                String prev;
                for (int d = 0; d < e.Depth; d++)
                    prev += TEXT("\t");
                LOG(Warning, "{2}{0}, Time: {1} ms", e.Name, ((int)(e.Time * 1000.0f) / 1000.0f), prev);
            }
            LOG(Info, "");
            LOG_FLOOR();
        }
    }
#endif
}

void ProfilingToolsService::Dispose()
{
    ProfilingTools::EventsCPU.Clear();
    ProfilingTools::EventsCPU.SetCapacity(0);
    ProfilingTools::EventsGPU.SetCapacity(0);
}

#endif
