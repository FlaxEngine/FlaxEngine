// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Allocation.h"
#include "Engine/Platform/CriticalSection.h"

/// <summary>
/// Allocator that uses pages for stack-based allocs without freeing memory during it's lifetime.
/// </summary>
class ArenaAllocator
{
private:
    struct Page
    {
        Page* Next;
        uint32 Offset, Size;
    };

    int32 _pageSize;
    Page* _first = nullptr;

public:
    ArenaAllocator(int32 pageSizeBytes = 1024 * 1024) // 1 MB by default
        : _pageSize(pageSizeBytes)
    {
    }

    ~ArenaAllocator()
    {
        Free();
    }

    // Allocates a chunk of unitialized memory.
    void* Allocate(uint64 size, uint64 alignment = 1);

    // Frees all memory allocations within allocator.
    void Free();

    // Creates a new object within the arena allocator.
    template<class T, class... Args>
    inline T* New(Args&&...args)
    {
        T* ptr = (T*)Allocate(sizeof(T));
        new(ptr) T(Forward<Args>(args)...);
        return ptr;
    }

    // Invokes destructor on values in an array and clears it.
    template<typename Value, typename Allocator>
    static void ClearDelete(Array<Value, Allocator>& collection)
    {
        Value* ptr = collection.Get();
        for (int32 i = 0; i < collection.Count(); i++)
            Memory::DestructItem(ptr[i]);
        collection.Clear();
    }

    // Invokes destructor on values in a dictionary and clears it.
    template<typename Key, typename Value, typename Allocator>
    static void ClearDelete(Dictionary<Key, Value, Allocator>& collection)
    {
        for (auto it = collection.Begin(); it.IsNotEnd(); ++it)
            Memory::DestructItem(it->Value);
        collection.Clear();
    }
};

/// <summary>
/// Allocator that uses pages for stack-based allocs without freeing memory during it's lifetime. Thread-safe to allocate memory from multiple threads at once.
/// </summary>
class ConcurrentArenaAllocator
{
private:
    struct Page
    {
        Page* Next;
        volatile int64 Offset;
        int64 Size;
    };

    int32 _pageSize;
    volatile int64 _first = 0;
#if !BUILD_RELEASE
    volatile int64 _totalBytes = 0;
#endif
    void*(*_allocate1)(uint64 size, uint64 alignment) = nullptr;
    void(*_free1)(void* ptr) = nullptr;
    void*(*_allocate2)(uint64 size) = nullptr;
    void(*_free2)(void* ptr, uint64 size) = nullptr;
    CriticalSection _locker;

public:
    ConcurrentArenaAllocator(int32 pageSizeBytes, void* (*customAllocate)(uint64 size, uint64 alignment), void(*customFree)(void* ptr))
        : _pageSize(pageSizeBytes)
        , _allocate1(customAllocate)
        , _free1(customFree)
    {
    }

    ConcurrentArenaAllocator(int32 pageSizeBytes, void* (*customAllocate)(uint64 size), void(*customFree)(void* ptr, uint64 size))
        : _pageSize(pageSizeBytes)
        , _allocate2(customAllocate)
        , _free2(customFree)
    {
    }

    ConcurrentArenaAllocator(int32 pageSizeBytes = 1024 * 1024) // 1 MB by default
        : ConcurrentArenaAllocator(pageSizeBytes, Allocator::Allocate, Allocator::Free)
    {
    }

    ~ConcurrentArenaAllocator()
    {
        Free();
    }

    // Gets the total amount of bytes allocated in arena (excluding alignment).
    int64 GetTotalBytes() const
    {
        return Platform::AtomicRead(&_totalBytes);
    }

    // Allocates a chunk of unitialized memory.
    void* Allocate(uint64 size, uint64 alignment = 1);

    // Frees all memory allocations within allocator.
    void Free();

    // Creates a new object within the arena allocator.
    template<class T, class... Args>
    inline T* New(Args&&...args)
    {
        T* ptr = (T*)Allocate(sizeof(T));
        new(ptr) T(Forward<Args>(args)...);
        return ptr;
    }
};

/// <summary>
/// The memory allocation policy that uses a part of shared page allocator. Allocations are performed in stack-manner, and free is no-op.
/// </summary>
template<typename ArenaType>
class ArenaAllocationBase
{
public:
    enum { HasSwap = true };
    typedef ArenaType* Tag;

    template<typename T>
    class Data
    {
    private:
        T* _data = nullptr;
        ArenaType* _arena = nullptr;

    public:
        FORCE_INLINE Data()
        {
        }

        FORCE_INLINE Data(Tag tag)
        {
            _arena = tag;
        }

        FORCE_INLINE ~Data()
        {
        }

        FORCE_INLINE T* Get()
        {
            return _data;
        }

        FORCE_INLINE const T* Get() const
        {
            return _data;
        }

        FORCE_INLINE int32 CalculateCapacityGrow(int32 capacity, const int32 minCapacity) const
        {
            return AllocationUtils::CalculateCapacityGrow(capacity, minCapacity);
        }

        FORCE_INLINE void Allocate(const int32 capacity)
        {
            ASSERT_LOW_LAYER(!_data && _arena);
            _data = (T*)_arena->Allocate(capacity * sizeof(T), alignof(T));
        }

        FORCE_INLINE void Relocate(const int32 capacity, int32 oldCount, int32 newCount)
        {
            ASSERT_LOW_LAYER(_arena);
            T* newData = capacity != 0 ? (T*)_arena->Allocate(capacity * sizeof(T), alignof(T)) : nullptr;
            if (oldCount)
            {
                if (newCount > 0)
                    Memory::MoveItems(newData, _data, newCount);
                Memory::DestructItems(_data, oldCount);
            }
            _data = newData;
        }

        FORCE_INLINE void Free()
        {
            _data = nullptr;
        }

        FORCE_INLINE void Swap(Data& other)
        {
            ::Swap(_data, other._data);
            _arena = other._arena; // TODO: find a better way to move allocation with AllocationUtils::MoveToEmpty to preserve/maintain allocation tag ownership
        }
    };
};

/// <summary>
/// The memory allocation policy that uses a part of shared page allocator. Allocations are performed in stack-manner, and free is no-op.
/// </summary>
typedef ArenaAllocationBase<ArenaAllocator> ArenaAllocation;

/// <summary>
/// The memory allocation policy that uses a part of shared page allocator. Allocations are performed in stack-manner, and free is no-op.
/// </summary>
typedef ArenaAllocationBase<ConcurrentArenaAllocator> ConcurrentArenaAllocation;
