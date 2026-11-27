// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_PROFILER

#include "ProfilerCPU.h"
#include "ProfilerMemory.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Threading/ThreadRegistry.h"
#include "Engine/Threading/Threading.h"

THREADLOCAL ProfilerCPU::Thread* ProfilerCPU::Thread::Current = nullptr;
Array<ProfilerCPU::Thread*, InlinedAllocation<64>> ProfilerCPU::Threads;
bool ProfilerCPU::Enabled = false;
CriticalSection ProfilerCPU::ThreadsLocker;

ProfilerCPU::EventBuffer::EventBuffer()
{
    _capacity = 8192;
    _data = NewArray<Event>(_capacity);
    _head = 0;
    _read = 0;
}

ProfilerCPU::EventBuffer::~EventBuffer()
{
    DeleteArray(_data, _capacity);
}

int32 ProfilerCPU::EventBuffer::Add()
{
    // Keep indices stable while a scope is open. Only reuse storage after extraction drains it.
    if (_head == Platform::AtomicRead(&_read) || _head == _capacity)
    {
        ScopeLock lock(_storageLock);
        if (_head == _read)
        {
            Platform::AtomicStore(&_read, 0);
            Platform::AtomicStore(&_head, 0);
        }
        if (_head == _capacity)
        {
            const int32 capacity = _capacity * 2;
            auto* data = NewArray<Event>(capacity);
            Platform::MemoryCopy(data, _data, _head * sizeof(Event));
            auto* previous = _data;
            const int32 previousCapacity = _capacity;
            _data = data;
            _capacity = capacity;
            DeleteArray(previous, previousCapacity);
        }
    }
    const int32 index = _head;
    auto& event = _data[index];
    event.Start = event.End = 0;
    event.Depth = 0;
    event.Name[0] = 0;
    event.NativeMemoryAllocation = event.ManagedMemoryAllocation = 0;
    Platform::AtomicStore(&_head, index + 1);
    return index;
}

void ProfilerCPU::EventBuffer::EndEvent(int32 index, double time)
{
    union { double Time; int64 Bits; } value;
    value.Time = time;
    Platform::AtomicStore(reinterpret_cast<volatile int64*>(&_data[index].End), value.Bits);
}

double ProfilerCPU::EventBuffer::ReadEnd(const Event& event)
{
    union { double Time; int64 Bits; } value;
    value.Bits = Platform::AtomicRead(reinterpret_cast<const volatile int64*>(&event.End));
    return value.Time;
}

void ProfilerCPU::EventBuffer::Extract(Array<Event>& data, bool withRemoval)
{
    ScopeLock lock(_storageLock);
    data.Clear();
    const int32 head = Platform::AtomicRead(&_head);
    int32 first = Platform::AtomicRead(&_read);
    while (first < head && _data[first].Depth != 0) first++;
    if (first == head) return;
    int32 lastRoot = head - 1;
    while (lastRoot >= first)
    {
        // Completion is the publication point for the record and its finished children.
        if (ReadEnd(_data[lastRoot]) > 0 && _data[lastRoot].Depth == 0) break;
        lastRoot--;
    }
    if (lastRoot < first) return;
    // The root can finish after the first head snapshot. Include every child it published.
    const int32 completedHead = Platform::AtomicRead(&_head);
    int32 end = lastRoot + 1;
    while (end < completedHead && _data[end].Depth != 0) end++;
    data.Resize(end - first, false);
    Platform::MemoryCopy(data.Get(), _data + first, (end - first) * sizeof(Event));
    if (withRemoval) Platform::AtomicStore(&_read, end);
}

int32 ProfilerCPU::Thread::BeginEvent()
{
    const auto index = Buffer.Add();
    auto& event = Buffer.Get(index);
    event.Start = Platform::GetTimeSeconds() * 1000.0;
    event.Depth = _activeEvents.Count();
    _activeEvents.Add(index);
    return index;
}

void ProfilerCPU::Thread::EndEvent(int32 index)
{
    if (_activeEvents.IsEmpty()) return;
    ASSERT(_activeEvents.Last() == index);
    _activeEvents.RemoveLast();
    Buffer.EndEvent(index, Platform::GetTimeSeconds() * 1000.0);
}

void ProfilerCPU::Thread::EndEvent()
{
    if (_activeEvents.HasItems()) EndEvent(_activeEvents.Last());
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
        PROFILE_MEM(Profiler);
        const auto id = Platform::GetCurrentThreadID();
        const auto t = ThreadRegistry::GetThread(id);
        if (t)
            thread = New<Thread>(t->GetName());
        else if (id == Globals::MainThreadID)
            thread = New<Thread>(TEXT("Main"));
        else
            thread = New<Thread>(String::Format(TEXT("Thread {0}"), id));

        Thread::Current = thread;
        ScopeLock lock(ThreadsLocker);
        thread->ID = Threads.Count() + 1;
        Threads.Add(thread);
    }
    return thread->BeginEvent();
}

int32 ProfilerCPU::BeginEvent(const Char* name)
{
    if (!Enabled)
        return -1;
    const auto index = BeginEvent();
    if (index < 0) return index;
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
    if (index < 0) return index;
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
