// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Memory/Memory.h"
#define MOODYCAMEL_EXCEPTIONS_ENABLED 0
#include <ThirdParty/concurrentqueue.h>

/// <summary>
/// The default engine configuration for concurrentqueue.
/// </summary>
struct ConcurrentQueueSettings : public moodycamel::ConcurrentQueueDefaultTraits
{
    // Use bigger blocks
    static const size_t BLOCK_SIZE = 256;

    // Use default engine memory allocator
    static inline void* malloc(size_t size)
    {
        return Allocator::Allocate((uint64)size);
    }

    // Use default engine memory allocator
    static inline void free(void* ptr)
    {
        return Allocator::Free(ptr);
    }
};

/// <summary>
/// Lock-free implementation of thread-safe queue.
/// Based on: https://github.com/cameron314/concurrentqueue
/// </summary>
template<typename T>
class ConcurrentQueue : public moodycamel::ConcurrentQueue<T, ConcurrentQueueSettings>
{
public:
    typedef moodycamel::ConcurrentQueue<T, ConcurrentQueueSettings> Base;

public:
    /// <summary>
    /// Gets an estimate of the total number of elements currently in the queue.
    /// </summary>
    FORCE_INLINE int32 Count() const
    {
        return static_cast<int32>(Base::size_approx());
    }

    /// <summary>
    /// Adds item to the collection.
    /// </summary>
    /// <param name="item">The item to add.</param>
    FORCE_INLINE void Add(const T& item)
    {
        enqueue(item);
    }

    /// <summary>
    /// Adds item to the collection.
    /// </summary>
    /// <param name="item">The item to add.</param>
    FORCE_INLINE void Add(T&& item)
    {
        enqueue(item);
    }
};
