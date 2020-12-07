// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#if COMPILE_WITH_PROFILER

#include "ProfilerCPU.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Threading/ThreadRegistry.h"

THREADLOCAL ProfilerCPU::Thread* ProfilerCPU::Thread::Current = nullptr;
Array<ProfilerCPU::Thread*> ProfilerCPU::Threads(64);
bool ProfilerCPU::Enabled = false;

void ProfilerCPU::EventBuffer::Extract(Array<Event>& data, bool withRemove)
{
    data.Clear();

    // Peek ring buffer state
    int32 count = _count;
    const int32 capacity = _capacity;

    // Skip if empty
    if (count == 0)
        return;

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
        if (i.Event().Depth == 0 && i.Event().End != 0)
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
        if (i.Event().End != 0 && i.Event().End <= lastRootEventEndTime)
        {
            lastEvent = i;
            break;
        }
    }

    if (withRemove)
    {
        // Remove all the events between [Begin(), lastEvent]
        _count -= (lastEvent.Index() - Begin().Index()) & _capacityMask;
    }

    // Extract all the events between [firstEvent, lastEvent]

    const int32 head = (lastEvent.Index() + 1) & _capacityMask;
    count = (lastEvent.Index() - firstEvent.Index() + 1) & _capacityMask;

    data.Resize(count, false);

    int32 tail = (head - count) & _capacityMask;
    int32 spaceLeft = capacity - tail;
    int32 spaceLeftCount = Math::Min(spaceLeft, count);
    int32 overflow = count - spaceLeft;

    Platform::MemoryCopy(data.Get(), &_data[tail], spaceLeftCount * sizeof(Event));
    if (overflow > 0)
        Platform::MemoryCopy(data.Get() + spaceLeftCount, &_data[0], overflow * sizeof(Event));
}

int32 ProfilerCPU::Thread::BeginEvent(const Char* name)
{
    const double time = Platform::GetTimeSeconds() * 1000.0;
    const auto index = Buffer.Add();

    Event& e = Buffer.Get(index);
    e.Start = time;
    e.End = 0;
    e.Depth = _depth++;
    e.NativeMemoryAllocation = 0;
    e.ManagedMemoryAllocation = 0;
    e.Name = name;

    return index;
}

void ProfilerCPU::Thread::EndEvent(int32 index)
{
    const double time = Platform::GetTimeSeconds() * 1000.0;

    _depth--;

    Event& e = Buffer.Get(index);
    e.End = time;
}

bool ProfilerCPU::IsProfilingCurrentThread()
{
    return Enabled && Thread::Current != nullptr;
}

ProfilerCPU::Thread* ProfilerCPU::GetCurrentThread()
{
    return Enabled ? Thread::Current : nullptr;
}

int32 ProfilerCPU::BeginEvent(const Char* name)
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

    return thread->BeginEvent(name);
}

void ProfilerCPU::EndEvent(int32 index)
{
    if (!Enabled)
        return;

    ASSERT(Thread::Current);
    Thread::Current->EndEvent(index);
}

void ProfilerCPU::Dispose()
{
    Enabled = false;
    Threads.ClearDelete();

    // Cleanup memory, note: calls to profiler after this point will end up with a crash (Thread::Current is invalid)
}

#endif
