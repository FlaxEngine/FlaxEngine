// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/Platform.h"
#include "Engine/Core/Memory/Allocation.h"
#include "Engine/Core/Math/Math.h"

/// <summary>
/// Template for dynamic array with variable capacity that stores the bit values.
/// </summary>
template<typename AllocationType = HeapAllocation>
API_CLASS(InBuild) class BitArray
{
    friend BitArray;
public:
    using ItemType = uint64;
    using AllocationData = typename AllocationType::template Data<ItemType>;

private:
    int32 _count;
    int32 _capacity;
    AllocationData _allocation;

    FORCE_INLINE static int32 ToItemCount(const int32 size)
    {
        return Math::DivideAndRoundUp<int32>(size, sizeof(ItemType));
    }

    FORCE_INLINE static int32 ToItemCapacity(const int32 size)
    {
        return Math::Max<int32>(Math::DivideAndRoundUp<int32>(size, sizeof(ItemType)), 1);
    }

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="BitArray"/> class.
    /// </summary>
    FORCE_INLINE BitArray()
        : _count(0)
        , _capacity(0)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="BitArray"/> class.
    /// </summary>
    /// <param name="capacity">The initial capacity.</param>
    explicit BitArray(const int32 capacity)
        : _count(0)
        , _capacity(capacity)
    {
        if (capacity > 0)
            _allocation.Allocate(ToItemCapacity(capacity));
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="BitArray"/> class.
    /// </summary>
    /// <param name="other">The other collection to copy.</param>
    BitArray(const BitArray& other) noexcept
    {
        _count = _capacity = other.Count();
        if (_capacity > 0)
        {
            const int32 itemsCapacity = ToItemCapacity(_capacity);
            _allocation.Allocate(itemsCapacity);
            Platform::MemoryCopy(Get(), other.Get(), itemsCapacity * sizeof(ItemType));
        }
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="BitArray"/> class.
    /// </summary>
    /// <param name="other">The other collection to copy.</param>
    template<typename OtherAllocationType = AllocationType>
    explicit BitArray(const BitArray<OtherAllocationType>& other) noexcept
    {
        _count = _capacity = other.Count();
        if (_capacity > 0)
        {
            const int32 itemsCapacity = ToItemCapacity(_capacity);
            _allocation.Allocate(itemsCapacity);
            Platform::MemoryCopy(Get(), other.Get(), itemsCapacity * sizeof(ItemType));
        }
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="BitArray"/> class.
    /// </summary>
    /// <param name="other">The other collection to move.</param>
    FORCE_INLINE BitArray(BitArray&& other) noexcept
        : _count(0)
        , _capacity(0)
    {
        Swap(other);
    }

    /// <summary>
    /// The assignment operator that deletes the current collection of items and the copies items from the other array.
    /// </summary>
    /// <param name="other">The other collection to copy.</param>
    /// <returns>The reference to this.</returns>
    BitArray& operator=(const BitArray& other) noexcept
    {
        if (this != &other)
        {
            if (_capacity < other._count)
            {
                _allocation.Free();
                _capacity = other._count;
                const int32 itemsCapacity = ToItemCapacity(_capacity);
                _allocation.Allocate(itemsCapacity);
                Platform::MemoryCopy(Get(), other.Get(), itemsCapacity * sizeof(ItemType));
            }
            _count = other._count;
        }
        return *this;
    }

    /// <summary>
    /// The move assignment operator that deletes the current collection of items and the moves items from the other array.
    /// </summary>
    /// <param name="other">The other collection to move.</param>
    /// <returns>The reference to this.</returns>
    BitArray& operator=(BitArray&& other) noexcept
    {
        if (this != &other)
        {
            _allocation.Free();
            _count = 0;
            _capacity = 0;
            Swap(other);
        }
        return *this;
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="BitArray"/> class.
    /// </summary>
    ~BitArray()
    {
    }

public:
    /// <summary>
    /// Gets the pointer to the bits storage data (linear allocation).
    /// </summary>
    FORCE_INLINE ItemType* Get()
    {
        return _allocation.Get();
    }

    /// <summary>
    /// Gets the pointer to the bits storage data (linear allocation).
    /// </summary>
    FORCE_INLINE const ItemType* Get() const
    {
        return _allocation.Get();
    }

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
    /// Gets the item at the given index.
    /// </summary>
    /// <param name="index">The index of the item.</param>
    /// <returns>The value of the item.</returns>
    FORCE_INLINE bool operator[](const int32 index) const
    {
        return Get(index);
    }

    /// <summary>
    /// Gets the item at the given index.
    /// </summary>
    /// <param name="index">The index of the item.</param>
    /// <returns>The value of the item.</returns>
    bool Get(const int32 index) const
    {
        ASSERT(index >= 0 && index < _count);
        const ItemType offset = index / sizeof(ItemType);
        const ItemType bitMask = (ItemType)(int32)(1 << (index & ((int32)sizeof(ItemType) - 1)));
        const ItemType item = ((ItemType*)_allocation.Get())[offset];
        return (item & bitMask) != 0;
    }

    /// <summary>
    /// Sets the item at the given index.
    /// </summary>
    /// <param name="index">The index of the item.</param>
    /// <param name="value">The value to set.</param>
    void Set(const int32 index, const bool value)
    {
        ASSERT(index >= 0 && index < _count);
        const ItemType offset = index / sizeof(ItemType);
        const ItemType bitMask = (ItemType)(int32)(1 << (index & ((int32)sizeof(ItemType) - 1)));
        ItemType& item = ((ItemType*)_allocation.Get())[offset];
        if (value)
            item |= bitMask;
        else
            item &= ~bitMask;  // Clear the bit
    }

public:
    /// <summary>
    /// Clear the collection without changing its capacity.
    /// </summary>
    FORCE_INLINE void Clear()
    {
        _count = 0;
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
        _allocation.Relocate(ToItemCapacity(capacity), ToItemCount(_count), ToItemCount(count));
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
        if (_count <= size)
            EnsureCapacity(size, preserveContents);
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
            const int32 capacity = _allocation.CalculateCapacityGrow(ToItemCapacity(_capacity), minCapacity);
            SetCapacity(capacity, preserveContents);
        }
    }

    /// <summary>
    /// Sets all items to the given value
    /// </summary>
    /// <param name="value">The value to assign to all the collection items.</param>
    void SetAll(const bool value)
    {
        if (_count != 0)
            Platform::MemorySet(_allocation.Get(), ToItemCount(_count) * sizeof(ItemType), value ? MAX_uint32 : 0);
    }

    /// <summary>
    /// Adds the specified item to the collection.
    /// </summary>
    /// <param name="item">The item to add.</param>
    void Add(const bool item)
    {
        EnsureCapacity(_count + 1);
        ++_count;
        Set(_count - 1, item);
    }

    /// <summary>
    /// Adds the specified item to the collection.
    /// </summary>
    /// <param name="items">The items to add.</param>
    /// <param name="count">The items count.</param>
    void Add(const bool* items, const int32 count)
    {
        EnsureCapacity(_count + count);
        for (int32 i = 0; i < count; ++i)
            Add(items[i]);
    }

    /// <summary>
    /// Adds the other collection to the collection.
    /// </summary>
    /// <param name="other">The other collection to add.</param>
    void Add(const BitArray& other)
    {
        EnsureCapacity(_count, other.Count());
        for (int32 i = 0; i < other.Count(); ++i)
            Add(other[i]);
    }

    /// <summary>
    /// Swaps the contents of collection with the other object without copy operation. Performs fast internal data exchange.
    /// </summary>
    /// <param name="other">The other collection.</param>
    void Swap(BitArray& other)
    {
        if IF_CONSTEXPR (AllocationType::HasSwap)
            _allocation.Swap(other._allocation);
        else
        {
            // Move to temp
            const int32 oldItemsCapacity = ToItemCount(_capacity);
            const int32 otherItemsCapacity = ToItemCount(other._capacity);
            AllocationData oldAllocation;
            if (oldItemsCapacity)
            {
                oldAllocation.Allocate(oldItemsCapacity);
                Memory::MoveItems(oldAllocation.Get(), _allocation.Get(), oldItemsCapacity);
                _allocation.Free();
            }

            // Move other to source
            if (otherItemsCapacity)
            {
                _allocation.Allocate(otherItemsCapacity);
                Memory::MoveItems(_allocation.Get(), other._allocation.Get(), otherItemsCapacity);
                other._allocation.Free();
            }

            // Move temp to other
            if (oldItemsCapacity)
            {
                other._allocation.Allocate(oldItemsCapacity);
                Memory::MoveItems(other._allocation.Get(), oldAllocation.Get(), oldItemsCapacity);
            }
        }
        ::Swap(_count, other._count);
        ::Swap(_capacity, other._capacity);
    }

public:
    template<typename OtherAllocationType = AllocationType>
    bool operator==(const BitArray<OtherAllocationType>& other) const
    {
        if (_count == other.Count())
        {
            for (int32 i = 0; i < _count; i++)
            {
                if (!(Get(i) == other.Get(i)))
                    return false;
            }
            return true;
        }
        return false;
    }

    template<typename OtherAllocationType = AllocationType>
    bool operator!=(const BitArray<OtherAllocationType>& other) const
    {
        return !operator==(other);
    }
};
