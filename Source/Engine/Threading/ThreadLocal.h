// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Platform/Platform.h"

/// <summary>
/// Per-thread local variable storage.
/// Implemented using atomic with per-thread storage indexed via thread id hashing.
/// ForConsider using 'THREADLOCAL' define before the variable instead.
/// </summary>
template<typename T, int32 MaxThreads = PLATFORM_THREADS_LIMIT, bool ClearMemory = true>
class ThreadLocal
{
protected:

    struct Bucket
    {
        volatile int64 ThreadID;
        T Value;
    };

    Bucket _buckets[MaxThreads];

public:

    ThreadLocal()
    {
        // Clear buckets
        if (ClearMemory)
        {
            Platform::MemoryClear(_buckets, sizeof(_buckets));
        }
        else
        {
            for (int32 i = 0; i < MaxThreads; i++)
                _buckets[i].ThreadID = 0;
        }
    }

public:

    T& Get()
    {
        return _buckets[GetIndex()].Value;
    }

    void Set(const T& value)
    {
        _buckets[GetIndex()].Value = value;
    }

    int32 Count() const
    {
        int32 result = 0;
        for (int32 i = 0; i < MaxThreads; i++)
        {
            if (Platform::AtomicRead((int64 volatile*)&_buckets[i].ThreadID) != 0)
                result++;
        }
        return result;
    }

    template<typename AllocationType = HeapAllocation>
    void GetValues(Array<T, AllocationType>& result) const
    {
        for (int32 i = 0; i < MaxThreads; i++)
        {
            if (Platform::AtomicRead((int64 volatile*)&_buckets[i].ThreadID) != 0)
                result.Add(_buckets[i].Value);
        }
    }

protected:

    FORCE_INLINE static int32 Hash(const int64 value)
    {
        return value & (MaxThreads - 1);
    }

    FORCE_INLINE int32 GetIndex()
    {
        int64 key = (int64)Platform::GetCurrentThreadID();
        auto index = Hash(key);
        while (true)
        {
            const int64 value = Platform::AtomicRead(&_buckets[index].ThreadID);
            if (value == key)
                break;
            if (value == 0 && Platform::InterlockedCompareExchange(&_buckets[index].ThreadID, key, 0) == 0)
                break;
            index = Hash(index + 1);
        }
        return index;
    }
};

/// <summary>
/// Per thread local object
/// </summary>
template<typename T, int32 MaxThreads = PLATFORM_THREADS_LIMIT>
class ThreadLocalObject : public ThreadLocal<T*, MaxThreads>
{
public:

    typedef ThreadLocal<T*, MaxThreads> Base;

public:

    void Delete()
    {
        auto value = Base::Get();
        Base::SetAll(nullptr);
        ::Delete(value);
    }

    void DeleteAll()
    {
        for (int32 i = 0; i < MaxThreads; i++)
        {
            auto& bucket = Base::_buckets[i];
            if (bucket.Value != nullptr)
            {
                ::Delete(bucket.Value);
                bucket.ThreadID = 0;
                bucket.Value = nullptr;
            }
        }
    }

    template<typename AllocationType = HeapAllocation>
    void GetNotNullValues(Array<T*, AllocationType>& result) const
    {
        result.EnsureCapacity(MaxThreads);
        for (int32 i = 0; i < MaxThreads; i++)
        {
            if (Base::_buckets[i].Value != nullptr)
            {
                result.Add(Base::_buckets[i].Value);
            }
        }
    }

    int32 CountNotNullValues() const
    {
        int32 result = 0;
        for (int32 i = 0; i < MaxThreads; i++)
        {
            if (Base::_buckets[i].Value != nullptr)
                result++;
        }
        return result;
    }
};
