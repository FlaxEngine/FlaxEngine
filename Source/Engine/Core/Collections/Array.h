// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include <initializer_list>
#include "Engine/Platform/Platform.h"
#include "Engine/Core/Memory/Memory.h"
#include "Engine/Core/Memory/Allocation.h"

/// <summary>
/// Template for dynamic array with variable capacity.
/// </summary>
/// <typeparam name="T">The type of elements in the array.</typeparam>
/// <typeparam name="AllocationType">The type of memory allocator.</typeparam>
template<typename T, typename AllocationType = HeapAllocation>
API_CLASS(InBuild) class Array
{
    friend Array;
public:
    using ItemType = T;
    using AllocationData = typename AllocationType::template Data<T>;

private:
    int32 _count;
    int32 _capacity;
    AllocationData _allocation;

    FORCE_INLINE static void MoveToEmpty(AllocationData& to, AllocationData& from, const int32 fromCount, const int32 fromCapacity)
    {
        if IF_CONSTEXPR (AllocationType::HasSwap)
            to.Swap(from);
        else
        {
            to.Allocate(fromCapacity);
            Memory::MoveItems(to.Get(), from.Get(), fromCount);
            Memory::DestructItems(from.Get(), fromCount);
            from.Free();
        }
    }

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="Array"/> class.
    /// </summary>
    FORCE_INLINE Array()
        : _count(0)
        , _capacity(0)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Array"/> class.
    /// </summary>
    /// <param name="capacity">The initial capacity.</param>
    explicit Array(const int32 capacity)
        : _count(0)
        , _capacity(capacity)
    {
        if (capacity > 0)
            _allocation.Allocate(capacity);
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Array"/> class.
    /// </summary>
    /// <param name="initList">The initial values defined in the array.</param>
    Array(std::initializer_list<T> initList)
    {
        _count = _capacity = static_cast<int32>(initList.size());
        if (_count > 0)
        {
            _allocation.Allocate(_count);
            Memory::ConstructItems(_allocation.Get(), initList.begin(), _count);
        }
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Array"/> class.
    /// </summary>
    /// <param name="data">The initial data.</param>
    /// <param name="length">The amount of items.</param>
    Array(const T* data, const int32 length)
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
    /// Initializes a new instance of the <see cref="Array"/> class.
    /// </summary>
    /// <param name="other">The other collection to copy.</param>
    Array(const Array& other)
    {
        _count = _capacity = other._count;
        if (_capacity > 0)
        {
            _allocation.Allocate(_capacity);
            Memory::ConstructItems(_allocation.Get(), other.Get(), other._count);
        }
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Array"/> class.
    /// </summary>
    /// <param name="other">The other collection to copy.</param>
    /// <param name="extraSize">The additionally amount of items to add to the add.</param>
    Array(const Array& other, int32 extraSize)
    {
        ASSERT(extraSize >= 0);
        _count = _capacity = other._count + extraSize;
        if (_capacity > 0)
        {
            _allocation.Allocate(_capacity);
            Memory::ConstructItems(_allocation.Get(), other.Get(), other._count);
            Memory::ConstructItems(_allocation.Get() + other._count, extraSize);
        }
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Array"/> class.
    /// </summary>
    /// <param name="other">The other collection to copy.</param>
    template<typename OtherT = T, typename OtherAllocationType = AllocationType>
    explicit Array(const Array<OtherT, OtherAllocationType>& other) noexcept
    {
        _capacity = other.Capacity();
        _count = other.Count();
        if (_capacity > 0)
        {
            _allocation.Allocate(_capacity);
            Memory::ConstructItems(_allocation.Get(), other.Get(), _count);
        }
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Array"/> class.
    /// </summary>
    /// <param name="other">The other collection to move.</param>
    Array(Array&& other) noexcept
    {
        _count = other._count;
        _capacity = other._capacity;
        other._count = 0;
        other._capacity = 0;
        MoveToEmpty(_allocation, other._allocation, _count, _capacity);
    }

    /// <summary>
    /// The assignment operator that deletes the current collection of items and the copies items from the initializer list.
    /// </summary>
    /// <param name="initList">The other collection to copy.</param>
    /// <returns>The reference to this.</returns>
    Array& operator=(std::initializer_list<T> initList) noexcept
    {
        Clear();
        if (initList.size() > 0)
        {
            EnsureCapacity(static_cast<int32>(initList.size()));
            _count = static_cast<int32>(initList.size());
            Memory::ConstructItems(_allocation.Get(), initList.begin(), _count);
        }
        return *this;
    }

    /// <summary>
    /// The assignment operator that deletes the current collection of items and the copies items from the other array.
    /// </summary>
    /// <param name="other">The other collection to copy.</param>
    /// <returns>The reference to this.</returns>
    Array& operator=(const Array& other) noexcept
    {
        if (this != &other)
        {
            Memory::DestructItems(_allocation.Get(), _count);
            if (_capacity < other.Count())
            {
                _allocation.Free();
                _capacity = other.Count();
                _allocation.Allocate(_capacity);
            }
            _count = other.Count();
            Memory::ConstructItems(_allocation.Get(), other.Get(), _count);
        }
        return *this;
    }

    /// <summary>
    /// The move assignment operator that deletes the current collection of items and the moves items from the other array.
    /// </summary>
    /// <param name="other">The other collection to move.</param>
    /// <returns>The reference to this.</returns>
    Array& operator=(Array&& other) noexcept
    {
        if (this != &other)
        {
            Memory::DestructItems(_allocation.Get(), _count);
            _allocation.Free();
            _count = other._count;
            _capacity = other._capacity;
            other._count = 0;
            other._capacity = 0;
            MoveToEmpty(_allocation, other._allocation, _count, _capacity);
        }
        return *this;
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="Array"/> class.
    /// </summary>
    ~Array()
    {
        Memory::DestructItems(_allocation.Get(), _count);
    }

public:
    /// <summary>
    /// Gets the amount of the items in the collection.
    /// </summary>
    FORCE_INLINE int32 Count() const
    {
        return _count;
    }

    /// <summary>
    /// Gets the amount of the items that can be contained by collection without resizing.
    /// </summary>
    FORCE_INLINE int32 Capacity() const
    {
        return _capacity;
    }

    /// <summary>
    /// Returns true if collection isn't empty.
    /// </summary>
    FORCE_INLINE bool HasItems() const
    {
        return _count != 0;
    }

    /// <summary>
    /// Returns true if collection is empty.
    /// </summary>
    FORCE_INLINE bool IsEmpty() const
    {
        return _count == 0;
    }

    /// <summary>
    /// Determines if given index is valid.
    /// </summary>
    /// <param name="index">The index.</param>
    /// <returns><c>true</c> if is valid a index; otherwise, <c>false</c>.</returns>
    bool IsValidIndex(const int32 index) const
    {
        return index < _count && index >= 0;
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
    /// Gets item at the given index.
    /// </summary>
    /// <returns>The reference to the item.</returns>
    FORCE_INLINE T& At(const int32 index)
    {
        ASSERT(index >= 0 && index < _count);
        return _allocation.Get()[index];
    }

    /// <summary>
    /// Gets item at the given index.
    /// </summary>
    /// <returns>The reference to the item.</returns>
    FORCE_INLINE const T& At(const int32 index) const
    {
        ASSERT(index >= 0 && index < _count);
        return _allocation.Get()[index];
    }

    /// <summary>
    /// Gets or sets the item at the given index.
    /// </summary>
    /// <returns>The reference to the item.</returns>
    FORCE_INLINE T& operator[](const int32 index)
    {
        ASSERT(index >= 0 && index < _count);
        return _allocation.Get()[index];
    }

    /// <summary>
    /// Gets the item at the given index.
    /// </summary>
    /// <returns>The reference to the item.</returns>
    FORCE_INLINE const T& operator[](const int32 index) const
    {
        ASSERT(index >= 0 && index < _count);
        return _allocation.Get()[index];
    }

    /// <summary>
    /// Gets the last item.
    /// </summary>
    FORCE_INLINE T& Last()
    {
        ASSERT(_count > 0);
        return _allocation.Get()[_count - 1];
    }

    /// <summary>
    /// Gets the last item.
    /// </summary>
    FORCE_INLINE const T& Last() const
    {
        ASSERT(_count > 0);
        return _allocation.Get()[_count - 1];
    }

    /// <summary>
    /// Gets the first item.
    /// </summary>
    FORCE_INLINE T& First()
    {
        ASSERT(_count > 0);
        return _allocation.Get()[0];
    }

    /// <summary>
    /// Gets the first item.
    /// </summary>
    FORCE_INLINE const T& First() const
    {
        ASSERT(_count > 0);
        return _allocation.Get()[0];
    }

public:
    FORCE_INLINE T* begin()
    {
        return &_allocation.Get()[0];
    }

    FORCE_INLINE T* end()
    {
        return &_allocation.Get()[_count];
    }

    FORCE_INLINE const T* begin() const
    {
        return &_allocation.Get()[0];
    }

    FORCE_INLINE const T* end() const
    {
        return &_allocation.Get()[_count];
    }

public:
    /// <summary>
    /// Clear the collection without changing its capacity.
    /// </summary>
    FORCE_INLINE void Clear()
    {
        Memory::DestructItems(_allocation.Get(), _count);
        _count = 0;
    }

    /// <summary>
    /// Clears the collection without changing its capacity. Deletes all not null items.
    /// Note: collection must contain pointers to the objects that have public destructor and be allocated using New method.
    /// </summary>
#if defined(_MSC_VER)
    template<typename = typename TEnableIf<TIsPointer<T>::Value>::Type>
#endif
    void ClearDelete()
    {
        T* data = Get();
        for (int32 i = 0; i < _count; i++)
        {
            if (data[i])
                Delete(data[i]);
        }
        Clear();
    }

    /// <summary>
    /// Changes the capacity of the collection.
    /// </summary>
    /// <param name="capacity">The new capacity.</param>
    /// <param name="preserveContents">True if preserve collection data when changing its size, otherwise collection after resize will be empty.</param>
    void SetCapacity(const int32 capacity, const bool preserveContents = true)
    {
        if (capacity == _capacity)
            return;
        ASSERT(capacity >= 0);
        const int32 count = preserveContents ? (_count < capacity ? _count : capacity) : 0;
        _allocation.Relocate(capacity, _count, count);
        _capacity = capacity;
        _count = count;
    }

    /// <summary>
    /// Resizes the collection to the specified size. If the size is equal or less to the current capacity no additional memory reallocation in performed.
    /// </summary>
    /// <param name="size">The new collection size.</param>
    /// <param name="preserveContents">True if preserve collection data when changing its size, otherwise collection after resize might not contain the previous data.</param>
    void Resize(const int32 size, const bool preserveContents = true)
    {
        if (_count > size)
        {
            Memory::DestructItems(_allocation.Get() + size, _count - size);
        }
        else
        {
            EnsureCapacity(size, preserveContents);
            Memory::ConstructItems(_allocation.Get() + _count, size - _count);
        }
        _count = size;
    }

    /// <summary>
    /// Ensures the collection has given capacity (or more).
    /// </summary>
    /// <param name="minCapacity">The minimum capacity.</param>
    /// <param name="preserveContents">True if preserve collection data when changing its size, otherwise collection after resize will be empty.</param>
    void EnsureCapacity(const int32 minCapacity, const bool preserveContents = true)
    {
        if (_capacity < minCapacity)
        {
            const int32 capacity = _allocation.CalculateCapacityGrow(_capacity, minCapacity);
            SetCapacity(capacity, preserveContents);
        }
    }

    /// <summary>
    /// Sets all items to the given value
    /// </summary>
    /// <param name="value">The value to assign to all the collection items.</param>
    void SetAll(const T& value)
    {
        T* data = _allocation.Get();
        for (int32 i = 0; i < _count; i++)
            data[i] = value;
    }

    /// <summary>
    /// Sets the collection data.
    /// </summary>
    /// <param name="data">The data.</param>
    /// <param name="count">The amount of items.</param>
    void Set(const T* data, const int32 count)
    {
        EnsureCapacity(count, false);
        Memory::DestructItems(_allocation.Get(), _count);
        _count = count;
        Memory::ConstructItems(_allocation.Get(), data, _count);
    }

    /// <summary>
    /// Adds the specified item to the collection.
    /// </summary>
    /// <param name="item">The item to add.</param>
    void Add(const T& item)
    {
        EnsureCapacity(_count + 1);
        Memory::ConstructItems(_allocation.Get() + _count, &item, 1);
        ++_count;
    }

    /// <summary>
    /// Adds the specified item to the collection.
    /// </summary>
    /// <param name="item">The item to add.</param>
    void Add(T&& item)
    {
        EnsureCapacity(_count + 1);
        Memory::MoveItems(_allocation.Get() + _count, &item, 1);
        ++_count;
    }

    /// <summary>
    /// Adds the specified item to the collection.
    /// </summary>
    /// <param name="items">The items to add.</param>
    /// <param name="count">The items count.</param>
    void Add(const T* items, const int32 count)
    {
        EnsureCapacity(_count + count);
        Memory::ConstructItems(_allocation.Get() + _count, items, count);
        _count += count;
    }

    /// <summary>
    /// Adds the other collection to the collection.
    /// </summary>
    /// <param name="other">The other collection to add.</param>
    template<typename OtherT, typename OtherAllocationType = AllocationType>
    FORCE_INLINE void Add(const Array<OtherT, OtherAllocationType>& other)
    {
        Add(other.Get(), other.Count());
    }

    /// <summary>
    /// Adds the unique item to the collection if it doesn't exist.
    /// </summary>
    /// <param name="item">The item to add.</param>
    FORCE_INLINE void AddUnique(const T& item)
    {
        if (!Contains(item))
            Add(item);
    }

    /// <summary>
    /// Adds the given amount of items to the collection.
    /// </summary>
    /// <param name="count">The items count.</param>
    FORCE_INLINE void AddDefault(const int32 count = 1)
    {
        EnsureCapacity(_count + count);
        Memory::ConstructItems(_allocation.Get() + _count, count);
        _count += count;
    }

    /// <summary>
    /// Adds the given amount of uninitialized items to the collection without calling the constructor.
    /// </summary>
    /// <param name="count">The items count.</param>
    FORCE_INLINE void AddUninitialized(const int32 count = 1)
    {
        EnsureCapacity(_count + count);
        _count += count;
    }

    /// <summary>
    /// Adds the one item to the collection and returns the reference to it.
    /// </summary>
    /// <returns>The reference to the added item.</returns>
    FORCE_INLINE T& AddOne()
    {
        EnsureCapacity(_count + 1);
        Memory::ConstructItems(_allocation.Get() + _count, 1);
        ++_count;
        return _allocation.Get()[_count - 1];
    }

    /// <summary>
    /// Adds the new items to the end of the collection, possibly reallocating the whole collection to fit. The new items will be zeroed.
    /// </summary>
    /// <remarks>
    /// Warning! AddZeroed() will create items without calling the constructor and this is not appropriate for item types that require a constructor to function properly.
    /// </remarks>
    /// <param name="count">The number of new items to add.</param>
    void AddZeroed(const int32 count = 1)
    {
        EnsureCapacity(_count + count);
        Platform::MemoryClear(_allocation.Get() + _count, count * sizeof(T));
        _count += count;
    }

    /// <summary>
    /// Insert the given item at specified index with keeping items order.
    /// </summary>
    /// <param name="index">The zero-based index at which item should be inserted.</param>
    /// <param name="item">The item to be inserted by copying.</param>
    void Insert(const int32 index, const T& item)
    {
        ASSERT(index >= 0 && index <= _count);
        EnsureCapacity(_count + 1);
        T* data = _allocation.Get();
        Memory::ConstructItems(data + _count, 1);
        for (int32 i = _count - 1; i >= index; i--)
            data[i + 1] = data[i];
        _count++;
        data[index] = item;
    }

    /// <summary>
    /// Insert the given item at specified index with keeping items order.
    /// </summary>
    /// <param name="index">The zero-based index at which item should be inserted.</param>
    /// <param name="item">The item to inserted by moving.</param>
    void Insert(const int32 index, T&& item)
    {
        ASSERT(index >= 0 && index <= _count);
        EnsureCapacity(_count + 1);
        T* data = _allocation.Get();
        Memory::ConstructItems(data + _count, 1);
        for (int32 i = _count - 1; i >= index; i--)
            data[i + 1] = MoveTemp(data[i]);
        ++_count;
        data[index] = MoveTemp(item);
    }

    /// <summary>
    /// Insert the given item at specified index with keeping items order.
    /// </summary>
    /// <param name="index">The zero-based index at which item should be inserted.</param>
    void Insert(const int32 index)
    {
        ASSERT(index >= 0 && index <= _count);
        EnsureCapacity(_count + 1);
        T* data = _allocation.Get();
        Memory::ConstructItems(data + _count, 1);
        for (int32 i = _count - 1; i >= index; i--)
            data[i + 1] = data[i];
        ++_count;
    }

    /// <summary>
    /// Determines whether the collection contains the specified item.
    /// </summary>
    /// <param name="item">The item to check.</param>
    /// <returns>True if item has been found in the collection, otherwise false.</returns>
    template<typename TComparableType>
    bool Contains(const TComparableType& item) const
    {
        const T* data = _allocation.Get();
        for (int32 i = 0; i < _count; i++)
        {
            if (data[i] == item)
                return true;
        }
        return false;
    }

    /// <summary>
    /// Removes the first occurrence of a specific object from the collection and keeps order of the items in the collection.
    /// </summary>
    /// <param name="item">The item to remove.</param>
    /// <returns>True if cannot remove item from the collection because cannot find it, otherwise false.</returns>
    bool RemoveKeepOrder(const T& item)
    {
        const int32 index = Find(item);
        if (index == -1)
            return true;
        RemoveAtKeepOrder(index);
        return false;
    }

    /// <summary>
    /// Removes all occurrence of a specific object from the collection and keeps order of the items in the collection.
    /// </summary>
    /// <param name="item">The item to remove.</param>
    void RemoveAllKeepOrder(const T& item)
    {
        for (int32 i = Count() - 1; i >= 0; --i)
        {
            if (_allocation.Get()[i] == item)
            {
                RemoveAtKeepOrder(i);
                if (IsEmpty())
                    break;
            }
        }
    }

    /// <summary>
    /// Removes the item at the specified index of the collection and keeps order of the items in the collection.
    /// </summary>
    /// <param name="index">The zero-based index of the item to remove.</param>
    void RemoveAtKeepOrder(const int32 index)
    {
        ASSERT(index < _count && index >= 0);
        --_count;
        T* data = _allocation.Get();
        if (index < _count)
        {
            T* dst = data + index;
            T* src = data + (index + 1);
            const int32 count = _count - index;
            for (int32 i = 0; i < count; ++i)
                dst[i] = MoveTemp(src[i]);
        }
        Memory::DestructItems(data + _count, 1);
    }

    /// <summary>
    /// Removes the first occurrence of a specific object from the collection.
    /// </summary>
    /// <param name="item">The item to remove.</param>
    /// <returns>True if cannot remove item from the collection because cannot find it, otherwise false.</returns>
    bool Remove(const T& item)
    {
        const int32 index = Find(item);
        if (index == -1)
            return true;
        RemoveAt(index);
        return false;
    }

    /// <summary>
    /// Removes all occurrence of a specific object from the collection.
    /// </summary>
    /// <param name="item">The item to remove.</param>
    void RemoveAll(const T& item)
    {
        for (int32 i = Count() - 1; i >= 0; --i)
        {
            if (_allocation.Get()[i] == item)
            {
                RemoveAt(i);
                if (IsEmpty())
                    break;
            }
        }
    }

    /// <summary>
    /// Removes the item at the specified index of the collection.
    /// </summary>
    /// <param name="index">The zero-based index of the item to remove.</param>
    void RemoveAt(const int32 index)
    {
        ASSERT(index < _count && index >= 0);
        --_count;
        T* data = _allocation.Get();
        if (_count)
            data[index] = data[_count];
        Memory::DestructItems(data + _count, 1);
    }

    /// <summary>
    /// Removes the last items from the collection.
    /// </summary>
    void RemoveLast()
    {
        ASSERT(_count > 0);
        --_count;
        Memory::DestructItems(_allocation.Get() + _count, 1);
    }

    /// <summary>
    /// Swaps the contents of collection with the other object without copy operation. Performs fast internal data exchange.
    /// </summary>
    /// <param name="other">The other collection.</param>
    void Swap(Array& other)
    {
        if IF_CONSTEXPR (AllocationType::HasSwap)
        {
            _allocation.Swap(other._allocation);
            ::Swap(_count, other._count);
            ::Swap(_capacity, other._capacity);
        }
        else
        {
            ::Swap(other, *this);
        }
    }

    /// <summary>
    /// Reverses the order of the added items in the collection.
    /// </summary>
    void Reverse()
    {
        T* data = _allocation.Get();
        const int32 count = _count / 2;
        for (int32 i = 0; i < count; ++i)
            ::Swap(data[i], data[_count - i - 1]);
    }

public:
    /// <summary>
    /// Performs push on stack operation (stack grows at the end of the collection).
    /// </summary>
    /// <param name="item">The item to append.</param>
    FORCE_INLINE void Push(const T& item)
    {
        Add(item);
    }

    /// <summary>
    /// Performs pop from stack operation (stack grows at the end of the collection).
    /// </summary>
    FORCE_INLINE T Pop()
    {
        T item = MoveTemp(Last());
        RemoveLast();
        return item;
    }

    /// <summary>
    /// Peeks items which is at the top of the stack (stack grows at the end of the collection).
    /// </summary>
    FORCE_INLINE T& Peek()
    {
        ASSERT(_count > 0);
        return _allocation.Get()[_count - 1];
    }

    /// <summary>
    /// Peeks items which is at the top of the stack (stack grows at the end of the collection).
    /// </summary>
    FORCE_INLINE const T& Peek() const
    {
        ASSERT(_count > 0);
        return _allocation.Get()[_count - 1];
    }

public:
    /// <summary>
    /// Performs enqueue to queue operation (queue head is in the beginning of queue).
    /// </summary>
    /// <param name="item">The item to append.</param>
    void Enqueue(const T& item)
    {
        Add(item);
    }

    /// <summary>
    /// Performs enqueue to queue operation (queue head is in the beginning of queue).
    /// </summary>
    /// <param name="item">The item to append.</param>
    void Enqueue(T&& item)
    {
        Add(MoveTemp(item));
    }

    /// <summary>
    /// Performs dequeue from queue operation (queue head is in the beginning of queue).
    /// </summary>
    /// <returns>The item.</returns>
    T Dequeue()
    {
        ASSERT(HasItems());
        T item = MoveTemp(_allocation.Get()[0]);
        RemoveAtKeepOrder(0);
        return item;
    }

public:
    /// <summary>
    /// Searches for the given item within the entire collection.
    /// </summary>
    /// <param name="item">The item to look for.</param>
    /// <param name="index">The found item index, -1 if missing.</param>
    /// <returns>True if found, otherwise false.</returns>
    template<typename ComparableType>
    FORCE_INLINE bool Find(const ComparableType& item, int32& index) const
    {
        index = Find(item);
        return index != -1;
    }

    /// <summary>
    /// Searches for the specified object and returns the zero-based index of the first occurrence within the entire collection.
    /// </summary>
    /// <param name="item">The item to find.</param>
    /// <returns>The zero-based index of the first occurrence of itm within the entire collection, if found; otherwise, -1.</returns>
    template<typename ComparableType>
    int32 Find(const ComparableType& item) const
    {
        if (_count > 0)
        {
            const T* RESTRICT start = _allocation.Get();
            for (const T * RESTRICT data = start, *RESTRICT dataEnd = data + _count; data != dataEnd; ++data)
            {
                if (*data == item)
                    return static_cast<int32>(data - start);
            }
        }
        return -1;
    }

    /// <summary>
    /// Searches for the given item within the entire collection starting from the end.
    /// </summary>
    /// <param name="item">The item to look for.</param>
    /// <param name="index">The found item index, -1 if missing.</param>
    /// <returns>True if found, otherwise false.</returns>
    template<typename ComparableType>
    FORCE_INLINE bool FindLast(const ComparableType& item, int& index) const
    {
        index = FindLast(item);
        return index != -1;
    }

    /// <summary>
    /// Searches for the specified object and returns the zero-based index of the first occurrence within the entire collection  starting from the end.
    /// </summary>
    /// <param name="item">The item to find.</param>
    /// <returns>The zero-based index of the first occurrence of itm within the entire collection, if found; otherwise, -1.</returns>
    template<typename ComparableType>
    int32 FindLast(const ComparableType& item) const
    {
        if (_count > 0)
        {
            const T* RESTRICT end = _allocation.Get() + _count;
            for (const T * RESTRICT data = end, *RESTRICT dataStart = data - _count; data != dataStart;)
            {
                --data;
                if (*data == item)
                    return static_cast<int32>(data - dataStart);
            }
        }
        return -1;
    }

public:
    template<typename OtherT = T, typename OtherAllocationType = AllocationType>
    bool operator==(const Array<OtherT, OtherAllocationType>& other) const
    {
        if (_count == other.Count())
        {
            const T* data = _allocation.Get();
            const T* otherData = other.Get();
            for (int32 i = 0; i < _count; i++)
            {
                if (!(data[i] == otherData[i]))
                    return false;
            }
            return true;
        }
        return false;
    }

    template<typename OtherT = T, typename OtherAllocationType = AllocationType>
    bool operator!=(const Array<OtherT, OtherAllocationType>& other) const
    {
        return !operator==(other);
    }

public:
    /// <summary>
    /// The collection iterator.
    /// </summary>
    struct Iterator
    {
        friend Array;
    private:
        Array* _array;
        int32 _index;

        Iterator(Array* array, const int32 index)
            : _array(array)
            , _index(index)
        {
        }

        Iterator(const Array* array, const int32 index)
            : _array(const_cast<Array*>(array))
            , _index(index)
        {
        }

    public:
        Iterator()
            : _array(nullptr)
            , _index(-1)
        {
        }

        Iterator(const Iterator& i)
            : _array(i._array)
            , _index(i._index)
        {
        }

        Iterator(Iterator&& i)
            : _array(i._array)
            , _index(i._index)
        {
        }

    public:
        FORCE_INLINE Array* GetArray() const
        {
            return _array;
        }

        FORCE_INLINE int32 GetIndex() const
        {
            return _index;
        }

        FORCE_INLINE bool IsEnd() const
        {
            return _index == _array->_count;
        }

        FORCE_INLINE bool IsNotEnd() const
        {
            return _index != _array->_count;
        }

        FORCE_INLINE T& operator*() const
        {
            return _array->Get()[_index];
        }

        FORCE_INLINE T* operator->() const
        {
            return &_array->Get()[_index];
        }

        FORCE_INLINE bool operator==(const Iterator& v) const
        {
            return _array == v._array && _index == v._index;
        }

        FORCE_INLINE bool operator!=(const Iterator& v) const
        {
            return _array != v._array || _index != v._index;
        }

        Iterator& operator=(const Iterator& v)
        {
            _array = v._array;
            _index = v._index;
            return *this;
        }

        Iterator& operator=(Iterator&& v)
        {
            _array = v._array;
            _index = v._index;
            return *this;
        }

        Iterator& operator++()
        {
            if (_index != _array->_count)
                _index++;
            return *this;
        }

        Iterator operator++(int)
        {
            Iterator temp = *this;
            if (_index != _array->_count)
                _index++;
            return temp;
        }

        Iterator& operator--()
        {
            if (_index > 0)
                --_index;
            return *this;
        }

        Iterator operator--(int)
        {
            Iterator temp = *this;
            if (_index > 0)
                --_index;
            return temp;
        }
    };

public:
    /// <summary>
    /// Gets iterator for beginning of the collection.
    /// </summary>
    FORCE_INLINE Iterator Begin() const
    {
        return Iterator(this, 0);
    }

    /// <summary>
    /// Gets iterator for ending of the collection.
    /// </summary>
    FORCE_INLINE Iterator End() const
    {
        return Iterator(this, _count);
    }
};

template<typename T, typename AllocationType>
void* operator new(const size_t size, Array<T, AllocationType>& array)
{
    ASSERT(size == sizeof(T));
    const int32 index = array.Count();
    array.AddUninitialized(1);
    return &array[index];
}

template<typename T, typename AllocationType>
void* operator new(const size_t size, Array<T, AllocationType>& array, const int32 index)
{
    ASSERT(size == sizeof(T));
    array.Insert(index);
    return &array[index];
}
