// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_PROFILER

#include "ProfilerGPU.h"
#include "Engine/Core/Log.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPUTimerQuery.h"
#include "Engine/Graphics/GPUContext.h"

RenderStatsData RenderStatsData::Counter;

int32 ProfilerGPU::_depth = 0;
Array<GPUTimerQuery*> ProfilerGPU::_timerQueriesPool;
Array<GPUTimerQuery*> ProfilerGPU::_timerQueriesFree;
bool ProfilerGPU::Enabled = false;
bool ProfilerGPU::EventsEnabled = false;
int32 ProfilerGPU::CurrentBuffer = 0;
ProfilerGPU::EventBuffer ProfilerGPU::Buffers[PROFILER_GPU_EVENTS_FRAMES];

bool ProfilerGPU::EventBuffer::HasData() const
{
    return _isResolved && _data.HasItems();
}

void ProfilerGPU::EventBuffer::EndAll()
{
    for (int32 i = 0; i < _data.Count(); i++)
    {
        _data[i].Timer->End();
    }
}

void ProfilerGPU::EventBuffer::TryResolve()
{
    if (_isResolved || _data.IsEmpty())
        return;

    // Check all the queries from the back to the front (in some cases inner queries are not finished)
    for (int32 i = _data.Count() - 1; i >= 0; i--)
    {
        if (!_data[i].Timer->HasResult())
            return;
    }

    // Collect queries results and free them
    for (int32 i = 0; i < _data.Count(); i++)
    {
        auto& e = _data[i];
        e.Time = e.Timer->GetResult();
        _timerQueriesFree.Add(e.Timer);
        e.Timer = nullptr;
    }

    _isResolved = true;
}

int32 ProfilerGPU::EventBuffer::Add(const Event& e)
{
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

GPUTimerQuery* ProfilerGPU::GetTimerQuery()
{
    GPUTimerQuery* result;
    if (_timerQueriesFree.HasItems())
    {
        result = _timerQueriesFree.Last();
        _timerQueriesFree.RemoveLast();
    }
    else
    {
        result = GPUDevice::Instance->CreateTimerQuery();
        _timerQueriesPool.Add(result);
    }
    return result;
}

int32 ProfilerGPU::BeginEvent(const Char* name)
{
#if GPU_ALLOW_PROFILE_EVENTS
    if (EventsEnabled)
        GPUDevice::Instance->GetMainContext()->EventBegin(name);
#endif
    if (!Enabled)
        return -1;

    Event e;
    e.Name = name;
    e.Stats = RenderStatsData::Counter;
    e.Timer = GetTimerQuery();
    e.Timer->Begin();
    e.Depth = _depth++;

    auto& buffer = Buffers[CurrentBuffer];
    const auto index = buffer.Add(e);
    return index;
}

void ProfilerGPU::EndEvent(int32 index)
{
#if GPU_ALLOW_PROFILE_EVENTS
    if (EventsEnabled)
        GPUDevice::Instance->GetMainContext()->EventEnd();
#endif
    if (index == -1)
        return;
    _depth--;

    auto& buffer = Buffers[CurrentBuffer];
    auto e = buffer.Get(index);
    e->Stats.Mix(RenderStatsData::Counter);
    e->Timer->End();
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
    buffer.EndAll();
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
    _timerQueriesPool.ClearDelete();
    _timerQueriesFree.Clear();
}

#endif
