// Copyright (c) Wojciech Figat. All rights reserved.

#include "ThreadRegistry.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Platform/CriticalSection.h"

namespace ThreadRegistryImpl
{
    Dictionary<uint64, Thread*> Registry(64);
    CriticalSection Locker;
}

using namespace ThreadRegistryImpl;

Thread* ThreadRegistry::GetThread(uint64 id)
{
    Thread* result = nullptr;

    Locker.Lock();
    Registry.TryGet(id, result);
    Locker.Unlock();

    return result;
}

int32 ThreadRegistry::Count()
{
    Locker.Lock();
    int32 count = Registry.Count();
    Locker.Unlock();

    return count;
}

void ThreadRegistry::KillEmAll()
{
    Locker.Lock();
    for (auto i = Registry.Begin(); i.IsNotEnd(); ++i)
    {
        i->Value->Kill(false);
    }
    Locker.Unlock();

    // Now album Kill'Em All from Metallica...
}

void ThreadRegistry::Add(Thread* thread)
{
    ASSERT(thread && thread->GetID() != 0);
    Locker.Lock();
    ASSERT(!Registry.ContainsKey(thread->GetID()) && !Registry.ContainsValue(thread));
    Registry.Add(thread->GetID(), thread);
    Locker.Unlock();
}

void ThreadRegistry::Remove(Thread* thread)
{
    ASSERT(thread && thread->GetID() != 0);
    Locker.Lock();
#if ENABLE_ASSERTION_LOW_LAYERS
    Thread** currentValue = Registry.TryGet(thread->GetID());
    ASSERT_LOW_LAYER(!currentValue || *currentValue == thread);
#endif
    Registry.Remove(thread->GetID());
    Locker.Unlock();
}
