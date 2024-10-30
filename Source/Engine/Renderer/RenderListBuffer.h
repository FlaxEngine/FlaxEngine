// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/Platform.h"
#include "Engine/Platform/CriticalSection.h"
#include "Engine/Core/Memory/Memory.h"
#include "Engine/Core/Memory/Allocation.h"

/// <summary>
/// Template for dynamic array with variable capacity that support concurrent elements appending (atomic add).
/// </summary>
/// <typeparam name="T">The type of elements in the array.</typeparam>
/// <typeparam name="AllocationType">The type of memory allocator.</typeparam>
template<typename T, typename AllocationType = HeapAllocation>
class RenderListBuffer
{
    friend RenderListBuffer;
public:
    using ItemType = T;
    using AllocationData = typename AllocationType::template Data<T>;

private:
    volatile int64 _count;
    volatile int64 _capacity;
    volatile int64 _threadsAdding = 0;
    volatile int64 _threadsResizing = 0;
    AllocationData _allocation;
    CriticalSection _locker;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="RenderListBuffer"/> class.
    /// </summary>
    FORCE_INLINE RenderListBuffer()
        : _count(0)
        , _capacity(0)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="RenderListBuffer"/> class.
    /// </summary>
    /// <param name="capacity">The initial capacity.</param>
    explicit RenderListBuffer(const int32 capacity)
        : _count(0)
        , _capacity(capacity)
    {
        if (capacity > 0)
            _allocation.Allocate(capacity);
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="RenderListBuffer"/> class.
    /// </summary>
    /// <param name="data">The initial data.</param>
    /// <param name="length">The amount of items.</param>
    RenderListBuffer(const T* data, const int32 length)
    {
        ASSERT(length >= 0);
        _count = _capacity = length;
        if (length > 0)
        {
            _allocation.Allocate(length);
            Memory::ConstructItems(_allocation.Get(), data, length);
        }
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="RenderListBuffer"/> class.
    /// </summary>
    /// <param name="other">The other collection to copy.</param>
    RenderListBuffer(const RenderListBuffer& other)
    {
        _count = _capacity = other._count;
        if (_capacity > 0)
        {
            _allocation.Allocate(_capacity);
            Memory::ConstructItems(_allocation.Get(), other.Get(), static_cast<int32>(other._count));
        }
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="RenderListBuffer"/> class.
    /// </summary>
    /// <param name="other">The other collection to move.</param>
    RenderListBuffer(RenderListBuffer&& other) noexcept
    {
        _count = other._count;
        _capacity = other._capacity;
        other._count = 0;
        other._capacity = 0;
        _allocation.Swap(other._allocation);
    }

    /// <summary>
    /// The assignment operator that deletes the current collection of items and the copies items from the other array.
    /// </summary>
    /// <param name="other">The other collection to copy.</param>
    /// <returns>The reference to this.</returns>
    RenderListBuffer& operator=(const RenderListBuffer& other) noexcept
    {
        if (this != &other)
        {
            Memory::DestructItems(_allocation.Get(), static_cast<int32>(_count));
            if (_capacity < other.Count())
            {
                _allocation.Free();
                _capacity = other.Count();
                _allocation.Allocate(_capacity);
            }
            _count = other.Count();
            Memory::ConstructItems(_allocation.Get(), other.Get(), static_cast<int32>(_count));
        }
        return *this;
    }

    /// <summary>
    /// The move assignment operator that deletes the current collection of items and the moves items from the other array.
    /// </summary>
    /// <param name="other">The other collection to move.</param>
    /// <returns>The reference to this.</returns>
    RenderListBuffer& operator=(RenderListBuffer&& other) noexcept
    {
        if (this != &other)
        {
            Memory::DestructItems(_allocation.Get(), static_cast<int32>(_count));
            _allocation.Free();
            _count = other._count;
            _capacity = other._capacity;
            other._count = 0;
            other._capacity = 0;
            _allocation.Swap(other._allocation);
        }
        return *this;
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="RenderListBuffer"/> class.
    /// </summary>
    ~RenderListBuffer()
    {
        Memory::DestructItems(_allocation.Get(), static_cast<int32>(_count));
    }

public:
    /// <summary>
    /// Gets the amount of the items in the collection.
    /// </summary>
    FORCE_INLINE int32 Count() const
    {
        return static_cast<int32>(Platform::AtomicRead((volatile int64*)&_count));
    }

    /// <summary>
    /// Gets the amount of the items that can be contained by collection without resizing.
    /// </summary>
    FORCE_INLINE int32 Capacity() const
    {
        return static_cast<int32>(Platform::AtomicRead((volatile int64*)&_capacity));
    }

    /// <summary>
    /// Gets the critical section locking the collection during resizing.
    /// </summary>
    FORCE_INLINE const CriticalSection& Locker() const
    {
        return _locker;
    }

    /// <summary>
    /// Gets the pointer to the first item in the collection (linear allocation).
    /// </summary>
    FORCE_INLINE T* Get()
    {
        return _allocation.Get();
    }

    /// <summary>
    /// Gets the pointer to the first item in the collection (linear allocation).
    /// </summary>
    FORCE_INLINE const T* Get() const
    {
        return _allocation.Get();
    }

    /// <summary>
    /// Gets or sets the item at the given index.
    /// </summary>
    /// <returns>The reference to the item.</returns>
    FORCE_INLINE T& operator[](const int32 index)
    {
        ASSERT(index >= 0 && index < Count());
        return _allocation.Get()[index];
    }

    /// <summary>
    /// Gets the item at the given index.
    /// </summary>
    /// <returns>The reference to the item.</returns>
    FORCE_INLINE const T& operator[](const int32 index) const
    {
        ASSERT(index >= 0 && index < Count());
        return _allocation.Get()[index];
    }

public:
    FORCE_INLINE T* begin()
    {
        return &_allocation.Get()[0];
    }

    FORCE_INLINE T* end()
    {
        return &_allocation.Get()[Count()];
    }

    FORCE_INLINE const T* begin() const
    {
        return &_allocation.Get()[0];
    }

    FORCE_INLINE const T* end() const
    {
        return &_allocation.Get()[Count()];
    }

public:
    /// <summary>
    /// Clear the collection without changing its capacity.
    /// </summary>
    void Clear()
    {
        _locker.Lock();
        Memory::DestructItems(_allocation.Get(), static_cast<int32>(_count));
        _count = 0;
        _locker.Unlock();
    }

    /// <summary>
    /// Changes the capacity of the collection.
    /// </summary>
    /// <param name="capacity">The new capacity.</param>
    /// <param name="preserveContents">True if preserve collection data when changing its size, otherwise collection after resize will be empty.</param>
    void SetCapacity(const int32 capacity, const bool preserveContents = true)
    {
        if (capacity == Capacity())
            return;
        _locker.Lock();
        ASSERT(capacity >= 0);
        const int32 count = preserveContents ? (static_cast<int32>(_count) < capacity ? static_cast<int32>(_count) : capacity) : 0;
        _allocation.Relocate(capacity, static_cast<int32>(_count), count);
        Platform::AtomicStore(&_capacity, capacity);
        Platform::AtomicStore(&_count, count);
        _locker.Unlock();
    }

    /// <summary>
    /// Resizes the collection to the specified size. If the size is equal or less to the current capacity no additional memory reallocation in performed.
    /// </summary>
    /// <param name="size">The new collection size.</param>
    /// <param name="preserveContents">True if preserve collection data when changing its size, otherwise collection after resize might not contain the previous data.</param>
    void Resize(const int32 size, const bool preserveContents = true)
    {
        _locker.Lock();
        if (_count > size)
        {
            Memory::DestructItems(_allocation.Get() + size, static_cast<int32>(_count) - size);
        }
        else
        {
            EnsureCapacity(size, preserveContents);
            Memory::ConstructItems(_allocation.Get() + _count, size - static_cast<int32>(_count));
        }
        _count = size;
        _locker.Unlock();
    }

    /// <summary>
    /// Ensures the collection has given capacity (or more).
    /// </summary>
    /// <param name="minCapacity">The minimum capacity.</param>
    void EnsureCapacity(const int32 minCapacity)
    {
        _locker.Lock();
        int32 capacity = static_cast<int32>(Platform::AtomicRead(&_capacity));
        if (capacity < minCapacity)
        {
            capacity = _allocation.CalculateCapacityGrow(capacity, minCapacity);
            const int32 count = static_cast<int32>(_count);
            _allocation.Relocate(capacity, count, count);
            Platform::AtomicStore(&_capacity, capacity);
        }
        _locker.Unlock();
    }

    /// <summary>
    /// Adds the specified item to the collection.
    /// </summary>
    /// <param name="item">The item to add.</param>
    /// <returns>Index of the added element.</returns>
    FORCE_INLINE int32 Add(const T& item)
    {
        const int32 index = AddOne();
        Memory::ConstructItems(_allocation.Get() + index, &item, 1);
        Platform::InterlockedDecrement(&_threadsAdding);
        return index;
    }

    /// <summary>
    /// Adds the specified item to the collection.
    /// </summary>
    /// <param name="item">The item to add.</param>
    /// <returns>Index of the added element.</returns>
    FORCE_INLINE int32 Add(T&& item)
    {
        const int32 index = AddOne();
        Memory::MoveItems(_allocation.Get() + index, &item, 1);
        Platform::InterlockedDecrement(&_threadsAdding);
        return index;
    }

private:
    int32 AddOne()
    {
        Platform::InterlockedIncrement(&_threadsAdding);
        int32 count = static_cast<int32>(Platform::AtomicRead(&_count));
        int32 capacity = static_cast<int32>(Platform::AtomicRead(&_capacity));
        const int32 minCapacity = GetMinCapacity(count);
        if (minCapacity > capacity || Platform::AtomicRead(&_threadsResizing)) // Resize if not enough space or someone else is already doing it (don't add mid-resizing)
        {
            // Move from adding to resizing
            Platform::InterlockedIncrement(&_threadsResizing);
            Platform::InterlockedDecrement(&_threadsAdding);

            // Wait for all threads to stop adding items before resizing can happen
        RETRY:
            while (Platform::AtomicRead(&_threadsAdding))
                Platform::Sleep(0);

            // Thread-safe resizing
            _locker.Lock();
            capacity = static_cast<int32>(Platform::AtomicRead(&_capacity));
            if (capacity < minCapacity)
            {
                if (Platform::AtomicRead(&_threadsAdding))
                {
                    // Other thread entered during mutex locking so give them a chance to do safe resizing
                    _locker.Unlock();
                    goto RETRY;
                }
                capacity = _allocation.CalculateCapacityGrow(capacity, minCapacity);
                count = static_cast<int32>(Platform::AtomicRead(&_count));
                _allocation.Relocate(capacity, count, count);
                Platform::AtomicStore(&_capacity, capacity);
            }

            // Move from resizing to adding
            Platform::InterlockedIncrement(&_threadsAdding);
            Platform::InterlockedDecrement(&_threadsResizing);

            // Let other thread enter resizing-area
            _locker.Unlock();
        }
        return static_cast<int32>(Platform::InterlockedIncrement(&_count)) - 1;
    }

    FORCE_INLINE static int32 GetMinCapacity(const int32 count)
    {
        // Ensure there is a slack for others threads to reduce resize counts in highly multi-threaded environment
        constexpr int32 slack = PLATFORM_THREADS_LIMIT * 8;
        int32 capacity = count + slack;
        {
            // Round up to the next power of 2 and multiply by 2
            capacity--;
            capacity |= capacity >> 1;
            capacity |= capacity >> 2;
            capacity |= capacity >> 4;
            capacity |= capacity >> 8;
            capacity |= capacity >> 16;
            capacity = (capacity + 1) * 2;
        }
        return capacity;
    }
};
