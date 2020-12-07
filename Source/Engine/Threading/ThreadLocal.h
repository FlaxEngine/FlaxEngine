// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Collections/Array.h"
#include "Threading.h"

// Maximum amount of threads with an access to the thread local variable (we use static limit due to engine threading design)
#define THREAD_LOCAL_MAX_CAPACITY 16

/// <summary>
/// Per thread local variable
/// </summary>
template<typename T, int32 MaxThreads = THREAD_LOCAL_MAX_CAPACITY, bool ClearMemory = true>
class ThreadLocal
{
    // Note: this is kind of weak-implementation. We don't want to use locks/semaphores.
    // For better performance use 'THREADLOCAL' define before the variable

protected:

    struct Bucket
    {
        uint64 ThreadID;
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

public:

    int32 Count() const
    {
        int32 result = 0;
        for (int32 i = 0; i < MaxThreads; i++)
        {
            if (_buckets[i].ThreadID != 0)
                result++;
        }
        return result;
    }

    void GetValues(Array<T>& result) const
    {
        result.EnsureCapacity(MaxThreads);
        for (int32 i = 0; i < MaxThreads; i++)
        {
            result.Add(_buckets[i].Value);
        }
    }

protected:

    FORCE_INLINE static int32 Hash(const uint64 value)
    {
        return value & (MaxThreads - 1);
    }

    FORCE_INLINE int32 GetIndex()
    {
        // TODO: fix it because now we can use only (MaxThreads-1) buckets
        ASSERT(Count() < MaxThreads);

        auto key = Platform::GetCurrentThreadID();
        auto index = Hash(key);

        while (_buckets[index].ThreadID != key && _buckets[index].ThreadID != 0)
            index = Hash(index + 1);
        _buckets[index].ThreadID = key;

        return index;
    }
};

/// <summary>
/// Per thread local object
/// </summary>
template<typename T, int32 MaxThreads = THREAD_LOCAL_MAX_CAPACITY>
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

    void GetNotNullValues(Array<T*>& result) const
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
