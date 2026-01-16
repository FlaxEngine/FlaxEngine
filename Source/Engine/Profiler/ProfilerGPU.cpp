// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_PROFILER

#include "ProfilerGPU.h"
#include "ProfilerMemory.h"
#include "Engine/Core/Log.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPUContext.h"

RenderStatsData RenderStatsData::Counter;

int32 ProfilerGPU::_depth = 0;
bool ProfilerGPU::Enabled = false;
bool ProfilerGPU::EventsEnabled = false;
int32 ProfilerGPU::CurrentBuffer = 0;
ProfilerGPU::EventBuffer ProfilerGPU::Buffers[PROFILER_GPU_EVENTS_FRAMES];

bool ProfilerGPU::EventBuffer::HasData() const
{
    return _isResolved && _data.HasItems();
}

void ProfilerGPU::EventBuffer::EndAllQueries()
{
    auto context = GPUDevice::Instance->GetMainContext();
    auto queries = _data.Get();
    for (int32 i = 0; i < _data.Count(); i++)
    {
        auto& e = queries[i];
        if (e.QueryActive)
        {
            e.QueryActive = false;
            context->EndQuery(e.Query);
        }
    }
}

void ProfilerGPU::EventBuffer::TryResolve()
{
    if (_isResolved || _data.IsEmpty())
        return;

    // Collect queries results
    PROFILE_MEM(Profiler);
    auto device = GPUDevice::Instance;
    auto queries = _data.Get();
    for (int32 i = 0; i < _data.Count(); i++)
    {
        auto& e = queries[i];
        ASSERT_LOW_LAYER(!e.QueryActive);
        uint64 time;
        if (device->GetQueryResult(e.Query, time, false))
        {
            e.Time = (float)time * 0.001f; // Convert to milliseconds
        }
        else
            return; // Skip if one of the queries is not yet ready (frame still in-flight)
    }

    _isResolved = true;
}

int32 ProfilerGPU::EventBuffer::Add(const Event& e)
{
    PROFILE_MEM(Profiler);
    const int32 index = _data.Count();
    _data.Add(e);
    return index;
}

void ProfilerGPU::EventBuffer::Extract(Array<Event>& data) const
{
    // Don't use unresolved data
    ASSERT(_isResolved);
    data = _data;
}

void ProfilerGPU::EventBuffer::Clear()
{
    _data.Clear();
    _isResolved = false;
    FrameIndex = 0;
    PresentTime = 0.0f;
}

int32 ProfilerGPU::BeginEvent(const Char* name)
{
    auto context = GPUDevice::Instance->GetMainContext();
#if GPU_ALLOW_PROFILE_EVENTS
    if (EventsEnabled)
        context->EventBegin(name);
#endif
    if (!Enabled)
        return -1;

    Event e;
    e.Name = name;
    e.Stats = RenderStatsData::Counter;
    e.Query = context->BeginQuery(GPUQueryType::Timer);
    e.Depth = _depth++;
    e.QueryActive = true;

    auto& buffer = Buffers[CurrentBuffer];
    const auto index = buffer.Add(e);
    return index;
}

void ProfilerGPU::EndEvent(int32 index)
{
    auto context = GPUDevice::Instance->GetMainContext();
#if GPU_ALLOW_PROFILE_EVENTS
    if (EventsEnabled)
        context->EventEnd();
#endif
    if (index == -1)
        return;
    _depth--;

    auto& buffer = Buffers[CurrentBuffer];
    auto e = buffer.Get(index);
    e->QueryActive = false;
    e->Stats.Mix(RenderStatsData::Counter);
    context->EndQuery(e->Query);
}

void ProfilerGPU::BeginFrame()
{
    // Clear stats
    RenderStatsData::Counter = RenderStatsData();
    _depth = 0;
    auto& buffer = Buffers[CurrentBuffer];
    buffer.FrameIndex = Engine::FrameCount;
    buffer.PresentTime = 0.0f;

    // Try to resolve previous frames
    for (int32 i = 0; i < PROFILER_GPU_EVENTS_FRAMES; i++)
    {
        Buffers[i].TryResolve();
    }
}

void ProfilerGPU::OnPresent()
{
    // End all current frame queries to prevent invalid event duration values
    auto& buffer = Buffers[CurrentBuffer];
    buffer.EndAllQueries();
}

void ProfilerGPU::OnPresentTime(float time)
{
    auto& buffer = Buffers[CurrentBuffer];
    buffer.PresentTime += time;
}

void ProfilerGPU::EndFrame()
{
    if (_depth)
    {
        LOG(Warning, "GPU Profiler events start/end mismatch");
    }

    // Move frame
    CurrentBuffer = (CurrentBuffer + 1) % PROFILER_GPU_EVENTS_FRAMES;

    // Prepare current frame buffer
    auto& buffer = Buffers[CurrentBuffer];
    buffer.Clear();
}

bool ProfilerGPU::GetLastFrameData(float& drawTimeMs, float& presentTimeMs, RenderStatsData& statsData)
{
    uint64 maxFrame = 0;
    int32 maxFrameIndex = -1;
    auto& frames = Buffers;
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
        const auto root = frame.Get(0);
        drawTimeMs = root->Time;
        presentTimeMs = frame.PresentTime;
        statsData = root->Stats;
        return true;
    }

    // No data
    drawTimeMs = 0.0f;
    presentTimeMs = 0.0f;
    Platform::MemoryClear(&statsData, sizeof(statsData));
    return false;
}

void ProfilerGPU::Dispose()
{
}

#endif
