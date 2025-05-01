// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_PROFILER

#include "ProfilingTools.h"
#include "Engine/Core/Types/Pair.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Engine/Time.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Networking/NetworkInternal.h"

ProfilingTools::MainStats ProfilingTools::Stats;
Array<ProfilingTools::ThreadStats, InlinedAllocation<64>> ProfilingTools::EventsCPU;
Array<ProfilerGPU::Event> ProfilingTools::EventsGPU;
Array<ProfilingTools::NetworkEventStat> ProfilingTools::EventsNetwork;

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

        float presentTime;
        ProfilerGPU::GetLastFrameData(stats.DrawGPUTimeMs, presentTime, stats.DrawStats);
        stats.DrawCPUTimeMs = Math::Max(stats.DrawCPUTimeMs - presentTime, 0.0f); // Remove swapchain present wait time to exclude from drawing on CPU
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

    // Get the last events from networking runtime
    {
        auto& networkEvents = ProfilingTools::EventsNetwork;
        networkEvents.Resize(NetworkInternal::ProfilerEvents.Count());
        int32 i = 0;
        for (const auto& e : NetworkInternal::ProfilerEvents)
        {
            const auto& src = e.Value;
            auto& dst = networkEvents[i++];
            dst.Count = src.Count;
            dst.DataSize = src.DataSize;
            dst.MessageSize = src.MessageSize;
            dst.Receivers = src.Receivers;
            const StringAnsiView& typeName = e.Key.First.GetType().Fullname;
            uint64 len = Math::Min<uint64>(typeName.Length(), ARRAY_COUNT(dst.Name) - 10);
            Platform::MemoryCopy(dst.Name, typeName.Get(), len);
            const StringAnsiView& name = e.Key.Second;
            if (name.HasChars())
            {
                uint64 pos = len;
                dst.Name[pos++] = ':';
                dst.Name[pos++] = ':';
                len = Math::Min<uint64>(name.Length(), ARRAY_COUNT(dst.Name) - pos - 1);
                Platform::MemoryCopy(dst.Name + pos, name.Get(), len);
                dst.Name[pos + len] = 0;
            }
            else
            {
                dst.Name[len] = 0;
            }
        }
        NetworkInternal::ProfilerEvents.Clear();
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
    ProfilingTools::EventsNetwork.SetCapacity(0);
}

bool ProfilingTools::GetEnabled()
{
    return ProfilerCPU::Enabled && ProfilerGPU::Enabled;
}

void ProfilingTools::SetEnabled(bool enabled)
{
    ProfilerCPU::Enabled = enabled;
    ProfilerGPU::Enabled = enabled;
    ProfilerGPU::EventsEnabled = enabled;
    NetworkInternal::EnableProfiling = enabled;
}

#endif
