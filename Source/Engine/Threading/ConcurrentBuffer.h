// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/Platform.h"
#include "Engine/Platform/CriticalSection.h"
#include "Engine/Core/Memory/Allocation.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Core.h"

/// <summary>
/// The concurrent data buffer allows to implement asynchronous data writing to the linear buffer by more than one worker thread at once.
/// Supports only value types that don't require constructor/destructor invocation.
/// </summary>
template<typename T>
class ConcurrentBuffer
{
    friend ConcurrentBuffer;

private:

    int64 _count;
    int64 _capacity;
    T* _data;
    CriticalSection _resizeLocker;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="ConcurrentBuffer"/> class.
    /// </summary>
    ConcurrentBuffer()
        : _count(0)
        , _capacity(0)
        , _data(nullptr)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="ConcurrentBuffer"/> class.
    /// </summary>
    /// <param name="capacity">The capacity.</param>
    ConcurrentBuffer(int32 capacity)
        : _count(0)
        , _capacity(capacity)
    {
        if (_capacity > 0)
            _data = (T*)Allocator::Allocate(_capacity * sizeof(T));
        else
            _data = nullptr;
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="ConcurrentBuffer"/> class.
    /// </summary>
    ~ConcurrentBuffer()
    {
        Allocator::Free(_data);
    }

public:

    /// <summary>
    /// Gets the amount of the elements in the collection.
    /// </summary>
    /// <returns>The items count.</returns>
    FORCE_INLINE int64 Count()
    {
        return Platform::AtomicRead(&_count);
    }

    /// <summary>
    /// Get amount of the elements that can be holed by collection without resizing.
    /// </summary>
    /// <returns>the items capacity.</returns>
    FORCE_INLINE int64 Capacity() const
    {
        return _capacity;
    }

    /// <summary>
    /// Determines whether this collection isn't empty.
    /// </summary>
    /// <returns><c>true</c> if this collection has elements; otherwise, <c>false</c>.</returns>
    FORCE_INLINE bool HasItems() const
    {
        return _count != 0;
    }

    /// <summary>
    /// Determines whether this collection is empty.
    /// </summary>
    /// <returns><c>true</c> if this collection is empty; otherwise, <c>false</c>.</returns>
    FORCE_INLINE bool IsEmpty() const
    {
        return _count == 0;
    }

    /// <summary>
    /// Gets the pointer to the first element in the collection.
    /// </summary>
    /// <returns>The allocation start.</returns>
    FORCE_INLINE T* Get()
    {
        return _data;
    }

    /// <summary>
    /// Gets the pointer to the first element in the collection.
    /// </summary>
    /// <returns>The allocation start.</returns>
    FORCE_INLINE const T* Get() const
    {
        return _data;
    }

    /// <summary>
    /// Gets the last element.
    /// </summary>
    /// <returns>The last element reference.</returns>
    FORCE_INLINE T& Last()
    {
        ASSERT(_count > 0);
        return _data[_count - 1];
    }

    /// <summary>
    /// Gets the last element.
    /// </summary>
    /// <returns>The last element reference.</returns>
    FORCE_INLINE const T& Last() const
    {
        ASSERT(_count > 0);
        return _data[_count - 1];
    }

    /// <summary>
    /// Gets the first element.
    /// </summary>
    /// <returns>The first element reference.</returns>
    FORCE_INLINE T& First()
    {
        ASSERT(_count > 0);
        return _data[0];
    }

    /// <summary>
    /// Gets the first element.
    /// </summary>
    /// <returns>The first element reference.</returns>
    FORCE_INLINE const T& First() const
    {
        ASSERT(_count > 0);
        return _data[0];
    }

    /// <summary>
    /// Get or sets element by the index.
    /// </summary>
    /// <param name="index">The index.</param>
    /// <returns>The item reference.</returns>
    FORCE_INLINE T& operator[](int64 index)
    {
        ASSERT(index >= 0 && index < _count);
        return _data[index];
    }

    /// <summary>
    /// Get or sets element by the index.
    /// </summary>
    /// <param name="index">The index.</param>
    /// <returns>The item reference (constant).</returns>
    FORCE_INLINE const T& operator[](int64 index) const
    {
        ASSERT(index >= 0 && index < _count);
        return _data[index];
    }

public:

    /// <summary>
    /// Clear the collection but without changing its capacity.
    /// </summary>
    FORCE_INLINE void Clear()
    {
        Platform::InterlockedExchange(&_count, 0);
    }

    /// <summary>
    /// Releases this buffer data.
    /// </summary>
    void Release()
    {
        _resizeLocker.Lock();
        Allocator::Free(_data);
        _data = nullptr;
        _capacity = 0;
        _count = 0;
        _resizeLocker.Unlock();
    }

    /// <summary>
    /// Sets the custom size of the collection. Only for the custom usage with dedicated data.
    /// </summary>
    /// <param name="size">The size.</param>
    void SetSize(int32 size)
    {
        ASSERT(size >= 0 && size <= _capacity);
        _count = size;
    }

    /// <summary>
    /// Adds the single item to the collection. Handles automatic buffer resizing. Thread-safe function that can be called from many threads at once.
    /// </summary>
    /// <param name="item">The item to add.</param>
    /// <returns>The index of the added item.</returns>
    FORCE_INLINE int64 Add(const T& item)
    {
        return Add(&item, 1);
    }

    /// <summary>
    /// Adds the array of items to the collection. Handles automatic buffer resizing. Thread-safe function that can be called from many threads at once.
    /// </summary>
    /// <param name="items">The collection of items to add.</param>
    /// <param name="count">The items count.</param>
    /// <returns>The index of the added first item.</returns>
    int64 Add(const T* items, int32 count)
    {
        const int64 index = Platform::InterlockedAdd(&_count, (int64)count);
        const int64 newCount = index + (int64)count;
        EnsureCapacity(newCount);
        Memory::CopyItems(_data + index, items, count);
        return index;
    }

    /// <summary>
    /// Adds a collection of items to the collection.
    /// </summary>
    /// <param name="collection">The collection of items to add.</param>
    FORCE_INLINE void Add(ConcurrentBuffer<T>& collection)
    {
        Add(collection.Get(), collection.Count());
    }

    /// <summary>
    /// Adds the given amount of items to the collection.
    /// </summary>
    /// <param name="count">The items count.</param>
    /// <returns>The index of the added first item.</returns>
    int64 AddDefault(int32 count = 1)
    {
        const int64 index = Platform::InterlockedAdd(&_count, (int64)count);
        const int64 newCount = index + (int64)count;
        EnsureCapacity(newCount);
        Memory::ConstructItems(_data + newCount, count);
        return index;
    }

    /// <summary>
    /// Adds the one item to the collection and returns the reference to it.
    /// </summary>
    /// <returns>The reference to the added item.</returns>
    FORCE_INLINE T& AddOne()
    {
        const int64 index = Platform::InterlockedAdd(&_count, 1);
        const int64 newCount = index + 1;
        EnsureCapacity(newCount);
        Memory::ConstructItems(_data + index, 1);
        return _data[index - 1];
    }

    /// <summary>
    /// Adds the new items to the end of the collection, possibly reallocating the whole collection to fit. The new items will be zeroed.
    /// </summary>
    /// Warning! AddZeroed() will create items without calling the constructor and this is not appropriate for item types that require a constructor to function properly.
    /// <remarks>
    /// </remarks>
    /// <param name="count">The number of new items to add.</param>
    /// <returns>The index of the added first item.</returns>
    int64 AddZeroed(int32 count = 1)
    {
        const int64 index = Platform::InterlockedAdd(&_count, 1);
        const int64 newCount = index + 1;
        EnsureCapacity(newCount);
        Platform::MemoryClear(Get() + index, count * sizeof(T));
        return _data[index - 1];
    }

    /// <summary>
    /// Ensures that the buffer has the given the capacity (equal or more). Preserves the existing items by copy operation.
    /// </summary>
    /// <param name="minCapacity">The minimum capacity.</param>
    void EnsureCapacity(int64 minCapacity)
    {
        // Early out
        if (_capacity >= minCapacity)
            return;

        _resizeLocker.Lock();

        // Skip if the other thread performed resizing
        if (_capacity >= minCapacity)
        {
            _resizeLocker.Unlock();
            return;
        }

        // Compute the new capacity
        int64 newCapacity = _capacity == 0 ? 8 : Math::RoundUpToPowerOf2(_capacity) * 2;
        if (newCapacity < minCapacity)
        {
            newCapacity = minCapacity;
        }
        ASSERT(newCapacity > _capacity);

        // Allocate new data
        T* newData = nullptr;
        if (newCapacity > 0)
        {
            newData = (T*)Allocator::Allocate(newCapacity * sizeof(T));
        }

        // Check if has data
        if (_data)
        {
            // Check if preserve contents
            if (newData && _count > 0)
            {
                Platform::MemoryCopy(newData, _data, _capacity * sizeof(T));
            }

            // Delete old data
            Allocator::Free(_data);
        }

        // Set state
        _data = newData;
        _capacity = newCapacity;

        _resizeLocker.Unlock();
    }

    /// <summary>
    /// Swaps the contents of buffer with the other object without copy operation. Performs fast internal data exchange.
    /// </summary>
    /// <param name="other">The other buffer.</param>
    void Swap(ConcurrentBuffer& other)
    {
        const auto count = _count;
        const auto capacity = _capacity;
        const auto data = _data;

        _count = other._count;
        _capacity = other._capacity;
        _data = other._data;

        other._count = count;
        other._capacity = capacity;
        other._data = data;
    }

public:

    /// <summary>
    /// Checks if the given element is in the collection.
    /// </summary>
    /// <param name="item">The item.</param>
    /// <returns><c>true</c> if collection contains the specified item; otherwise, <c>false</c>.</returns>
    bool Contains(const T& item) const
    {
        for (int32 i = 0; i < _count; i++)
        {
            if (_data[i] == item)
            {
                return true;
            }
        }

        return false;
    }

    /// <summary>
    /// Searches for the specified object and returns the zero-based index of the first occurrence within the entire collection.
    /// </summary>
    /// <param name="item">The item.</param>
    /// <returns>The zero-based index of the first occurrence of item within the entire collection, if found; otherwise, INVALID_INDEX.</returns>
    int32 IndexOf(const T& item) const
    {
        for (int32 i = 0; i < _count; i++)
        {
            if (_data[i] == item)
            {
                return i;
            }
        }

        return INVALID_INDEX;
    }
};
