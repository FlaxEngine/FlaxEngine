// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Platform/Platform.h"

#define THREAD_LOCAL_USE_DYNAMIC_BUCKETS (PLATFORM_DESKTOP)

/// <summary>
/// Per-thread local variable storage for basic types (POD). Implemented using atomic with per-thread storage indexed via thread id hashing. Consider using 'THREADLOCAL' define before the variable instead.
/// </summary>
template<typename T, int32 MaxThreads = PLATFORM_THREADS_LIMIT>
class ThreadLocal
{
protected:
    static_assert(TIsPODType<T>::Value, "Only POD types are supported");

    struct Bucket
    {
        volatile int64 ThreadID;
        T Value;
    };

    Bucket _staticBuckets[MaxThreads];
#if THREAD_LOCAL_USE_DYNAMIC_BUCKETS
    Bucket* _dynamicBuckets = nullptr;
    constexpr static int32 DynamicMaxThreads = 1024;
#endif

public:
    ThreadLocal()
    {
        Platform::MemoryClear(_staticBuckets, sizeof(_staticBuckets));
    }

#if THREAD_LOCAL_USE_DYNAMIC_BUCKETS
    ~ThreadLocal()
    {
        Platform::Free(_dynamicBuckets);
    }
#endif

public:
    FORCE_INLINE T& Get()
    {
        return GetBucket().Value;
    }

    FORCE_INLINE void Set(const T& value)
    {
        GetBucket().Value = value;
    }

    int32 Count() const
    {
        int32 result = 0;
        for (int32 i = 0; i < MaxThreads; i++)
        {
            if (Platform::AtomicRead((int64 volatile*)&_staticBuckets[i].ThreadID) != 0)
                result++;
        }
#if THREAD_LOCAL_USE_DYNAMIC_BUCKETS
        if (auto dynamicBuckets = (Bucket*)Platform::AtomicRead((intptr volatile*)&_dynamicBuckets))
        {
            for (int32 i = 0; i < MaxThreads; i++)
            {
                if (Platform::AtomicRead((int64 volatile*)&dynamicBuckets[i].ThreadID) != 0)
                    result++;
            }
        }
#endif
        return result;
    }

    template<typename AllocationType = HeapAllocation>
    void GetValues(Array<T, AllocationType>& result) const
    {
        for (int32 i = 0; i < MaxThreads; i++)
        {
            if (Platform::AtomicRead((int64 volatile*)&_staticBuckets[i].ThreadID) != 0)
                result.Add(_staticBuckets[i].Value);
        }
#if THREAD_LOCAL_USE_DYNAMIC_BUCKETS
        if (auto dynamicBuckets = (Bucket*)Platform::AtomicRead((intptr volatile*)&_dynamicBuckets))
        {
            for (int32 i = 0; i < MaxThreads; i++)
            {
                if (Platform::AtomicRead((int64 volatile*)&dynamicBuckets[i].ThreadID) != 0)
                    result.Add(dynamicBuckets[i].Value);
            }
        }
#endif
    }

    void Clear()
    {
        Platform::MemoryClear(_staticBuckets, sizeof(_staticBuckets));
#if THREAD_LOCAL_USE_DYNAMIC_BUCKETS
        Platform::Free(_dynamicBuckets);
        _dynamicBuckets = nullptr;
#endif
    }

protected:
    Bucket& GetBucket()
    {
        const int64 key = (int64)Platform::GetCurrentThreadID();

        // Search statically allocated buckets
        int32 index = (int32)(key & (MaxThreads - 1));
        int32 spaceLeft = MaxThreads;
        while (spaceLeft)
        {
            const int64 value = Platform::AtomicRead(&_staticBuckets[index].ThreadID);
            if (value == key)
                return _staticBuckets[index];
            if (value == 0 && Platform::InterlockedCompareExchange(&_staticBuckets[index].ThreadID, key, 0) == 0)
                return _staticBuckets[index];
            index = (index + 1) & (MaxThreads - 1);
            spaceLeft--;
        }

#if THREAD_LOCAL_USE_DYNAMIC_BUCKETS
        // Allocate dynamic buckets if missing
    DYNAMIC:
        auto dynamicBuckets = (Bucket*)Platform::AtomicRead((intptr volatile*)&_dynamicBuckets);
        if (!dynamicBuckets)
        {
            dynamicBuckets = (Bucket*)Platform::Allocate(DynamicMaxThreads * sizeof(Bucket), 16);
            Platform::MemoryClear(dynamicBuckets, DynamicMaxThreads * sizeof(Bucket));
            if (Platform::InterlockedCompareExchange((intptr volatile*)&_dynamicBuckets, (intptr)dynamicBuckets, 0) != 0)
            {
                Platform::Free(dynamicBuckets);
                goto DYNAMIC;
            }
        }

        // Search dynamically allocated buckets
        index = (int32)(key & (DynamicMaxThreads - 1));
        spaceLeft = DynamicMaxThreads;
        while (spaceLeft)
        {
            const int64 value = Platform::AtomicRead(&dynamicBuckets[index].ThreadID);
            if (value == key)
                return dynamicBuckets[index];
            if (value == 0 && Platform::InterlockedCompareExchange(&dynamicBuckets[index].ThreadID, key, 0) == 0)
                return dynamicBuckets[index];
            index = (index + 1) & (DynamicMaxThreads - 1);
            spaceLeft--;
        }
#endif

        return *(Bucket*)nullptr;
    }
};
