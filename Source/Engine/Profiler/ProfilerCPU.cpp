// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_PROFILER

#include "ProfilerCPU.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Threading/ThreadRegistry.h"

THREADLOCAL ProfilerCPU::Thread* ProfilerCPU::Thread::Current = nullptr;
Array<ProfilerCPU::Thread*, InlinedAllocation<64>> ProfilerCPU::Threads;
bool ProfilerCPU::Enabled = false;

ProfilerCPU::EventBuffer::EventBuffer()
{
    _capacity = 8192;
    _capacityMask = _capacity - 1;
    _data = NewArray<Event>(_capacity);
    _head = 0;
    _count = 0;
}

ProfilerCPU::EventBuffer::~EventBuffer()
{
    DeleteArray(_data, _capacity);
}

void ProfilerCPU::EventBuffer::Extract(Array<Event>& data, bool withRemoval)
{
    data.Clear();

    // Peek ring buffer state
    int32 count = _count;
    const int32 capacity = _capacity;

    // Skip if empty
    if (count == 0)
        return;

    // Fix iterators when buffer is full (begin == end)
    if (count == capacity)
    {
        _count--;
        count--;
    }

    // Find the first item (skip non-root events)
    Iterator firstEvent = Begin();
    for (auto i = firstEvent; i.IsNotEnd(); ++i)
    {
        if (i.Event().Depth == 0)
        {
            firstEvent = i;
            break;
        }
    }

    // Skip if no root event found inside the buffer
    if (firstEvent.IsEnd())
        return;

    // Find the last item (last event in ended root event)
    Iterator lastEndedRoot = End();
    for (auto i = Last(); i != firstEvent; --i)
    {
        if (i.Event().Depth == 0 && i.Event().End > 0)
        {
            lastEndedRoot = i;
            break;
        }
    }

    // Skip if no finished root event found inside the buffer
    if (lastEndedRoot.IsEnd())
        return;

    // Find the last non-root event in last root event
    Iterator lastEvent = lastEndedRoot;
    const double lastRootEventEndTime = lastEndedRoot.Event().End;
    for (auto i = --End(); i != lastEndedRoot; --i)
    {
        if (i.Event().End > 0 && i.Event().End <= lastRootEventEndTime)
        {
            lastEvent = i;
            break;
        }
    }

    if (withRemoval)
    {
        // Remove all the events between [Begin(), lastEvent]
        _count -= (lastEvent.Index() - Begin().Index()) & _capacityMask;
    }

    // Extract all the events between [firstEvent, lastEvent]
    const int32 head = (lastEvent.Index() + 1) & _capacityMask;
    count = (lastEvent.Index() - firstEvent.Index() + 1) & _capacityMask;
    data.Resize(count, false);
    const int32 tail = (head - count) & _capacityMask;
    const int32 spaceLeft = capacity - tail;
    const int32 spaceLeftCount = Math::Min(spaceLeft, count);
    const int32 overflow = count - spaceLeft;
    Platform::MemoryCopy(data.Get(), &_data[tail], spaceLeftCount * sizeof(Event));
    if (overflow > 0)
        Platform::MemoryCopy(data.Get() + spaceLeftCount, &_data[0], overflow * sizeof(Event));
}

int32 ProfilerCPU::Thread::BeginEvent()
{
    const double time = Platform::GetTimeSeconds() * 1000.0;
    const auto index = Buffer.Add();
    Event& e = Buffer.Get(index);
    e.Start = time;
    e.End = 0;
    e.Depth = _depth++;
    e.NativeMemoryAllocation = 0;
    e.ManagedMemoryAllocation = 0;
    return index;
}

void ProfilerCPU::Thread::EndEvent(int32 index)
{
    const double time = Platform::GetTimeSeconds() * 1000.0;
    _depth--;
    Event& e = Buffer.Get(index);
    e.End = time;
}

void ProfilerCPU::Thread::EndEvent()
{
    const double time = Platform::GetTimeSeconds() * 1000.0;
    _depth--;
    for (auto i = Buffer.Last(); i != Buffer.Begin(); --i)
    {
        Event& e = i.Event();
        if (e.End <= 0)
        {
            e.End = time;
            break;
        }
    }
}

bool ProfilerCPU::IsProfilingCurrentThread()
{
    return Enabled && Thread::Current != nullptr;
}

ProfilerCPU::Thread* ProfilerCPU::GetCurrentThread()
{
    return Enabled ? Thread::Current : nullptr;
}

int32 ProfilerCPU::BeginEvent()
{
    if (!Enabled)
        return -1;
    auto thread = Thread::Current;
    if (thread == nullptr)
    {
        const auto id = Platform::GetCurrentThreadID();
        const auto t = ThreadRegistry::GetThread(id);
        if (t)
            thread = New<Thread>(t->GetName());
        else if (id == Globals::MainThreadID)
            thread = New<Thread>(TEXT("Main"));
        else
            thread = New<Thread>(TEXT("Thread"));

        Thread::Current = thread;
        Threads.Add(thread);
    }
    return thread->BeginEvent();
}

int32 ProfilerCPU::BeginEvent(const Char* name)
{
    if (!Enabled)
        return -1;
    const auto index = BeginEvent();
    const auto thread = Thread::Current;
    auto& e = thread->Buffer.Get(index);
    auto dst = e.Name;
    auto src = name;
    if (src)
    {
        const auto end = dst + ARRAY_COUNT(e.Name) - 1;
        while (*src && dst != end)
            *dst++ = *src++;
    }
    *dst = 0;
    return index;
}

int32 ProfilerCPU::BeginEvent(const char* name)
{
    if (!Enabled)
        return -1;
    const auto index = BeginEvent();
    const auto thread = Thread::Current;
    auto& e = thread->Buffer.Get(index);
    auto dst = e.Name;
    auto src = name;
    if (src)
    {
        const auto end = dst + ARRAY_COUNT(e.Name) - 1;
        while (*src && dst != end)
            *dst++ = *src++;
    }
    *dst = 0;
    return index;
}

void ProfilerCPU::EndEvent(int32 index)
{
    if (index != -1 && Thread::Current)
        Thread::Current->EndEvent(index);
}

void ProfilerCPU::EndEvent()
{
    if (Enabled && Thread::Current)
        Thread::Current->EndEvent();
}

void ProfilerCPU::Dispose()
{
    Enabled = false;
    Threads.ClearDelete();

    // Cleanup memory, note: calls to profiler after this point will end up with a crash (Thread::Current is invalid)
}

#endif
