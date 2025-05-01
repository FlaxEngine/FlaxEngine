// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "../Collections/Array.h"
#include "Engine/Platform/CriticalSection.h"

namespace CollectionPoolCacheUtils
{
    /// <summary>
    /// Clear callback used to initialize the given collection container type (clear array, etc.). Called when pool item is being reused or initialized.
    /// </summary>
    template<typename T>
    using ClearCallback = void(*)(T*);

    /// <summary>
    /// Create callback spawns a new entry of the pooled collection
    /// </summary>
    template<typename T>
    using CreateCallback = T * (*)();

    template<typename T>
    inline void DefaultClearCallback(T* obj)
    {
        obj->Clear();
    }

    template<typename T>
    inline T* DefaultCreateCallback()
    {
        return New<T>();
    }
}

/// <summary>
/// Cache container that holds a list of cached collections to allow reuse and reduced memory allocation amount. Helps with sharing data across code and usages. It's thread-safe.
/// </summary>
template<typename T, CollectionPoolCacheUtils::ClearCallback<T> ClearCallback = CollectionPoolCacheUtils::DefaultClearCallback<T>, CollectionPoolCacheUtils::CreateCallback<T> CreateCallback = CollectionPoolCacheUtils::DefaultCreateCallback<T>>
class CollectionPoolCache
{
public:
    /// <summary>
    /// Helper object used to access the pooled collection and return it to the pool after usage (on code scope execution end).
    /// </summary>
    struct ScopeCache
    {
        friend CollectionPoolCache;

    private:
        CollectionPoolCache* _pool;

        FORCE_INLINE ScopeCache(CollectionPoolCache* pool, T* value)
        {
            _pool = pool;
            Value = value;
        }

    public:
        T* Value;

        ScopeCache() = delete;
        ScopeCache(const ScopeCache& other) = delete;
        ScopeCache& operator=(const ScopeCache& other) = delete;
        ScopeCache& operator=(ScopeCache&& other) noexcept = delete;

        ScopeCache(ScopeCache&& other) noexcept
        {
            Value = other.Value;
            other.Value = nullptr;
        }

        ~ScopeCache()
        {
            _pool->Put(Value);
        }

        T* operator->()
        {
            return Value;
        }

        const T* operator->() const
        {
            return Value;
        }

        T& operator*()
        {
            return *Value;
        }

        const T& operator*() const
        {
            return *Value;
        }
    };

private:
    CriticalSection _locker;
    Array<T*, InlinedAllocation<64>> _pool;

public:
    /// <summary>
    /// Finalizes an instance of the <see cref="CollectionPoolCache"/> class.
    /// </summary>
    ~CollectionPoolCache()
    {
        _pool.ClearDelete();
    }

public:
    /// <summary>
    /// Gets the collection instance from the pool. Can reuse the object from the pool or create a new one. Returns collection is always cleared and ready to use.
    /// </summary>
    /// <returns>The collection (cleared).</returns>
    FORCE_INLINE ScopeCache Get()
    {
        return ScopeCache(this, GetUnscoped());
    }

    /// <summary>
    /// Gets the collection instance from the pool. Can reuse the object from the pool or create a new one. Returns collection is always cleared and ready to use.
    /// </summary>
    /// <returns>The collection (cleared).</returns>
    T* GetUnscoped()
    {
        T* result;
        _locker.Lock();
        if (_pool.HasItems())
            result = _pool.Pop();
        else
            result = CreateCallback();
        _locker.Unlock();
        ClearCallback(result);
        return result;
    }

    /// <summary>
    /// Puts the collection value back to the pool.
    /// </summary>
    void Put(T* value)
    {
        _locker.Lock();
        _pool.Add(value);
        _locker.Unlock();
    }

    /// <summary>
    /// Releases all the allocated resources (existing in the pool that are not during usage).
    /// </summary>
    void Release()
    {
        _locker.Lock();
        _pool.ClearDelete();
        _pool.Resize(0);
        _locker.Unlock();
    }
};
