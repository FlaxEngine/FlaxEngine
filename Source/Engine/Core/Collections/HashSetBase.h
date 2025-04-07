// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Memory/Memory.h"
#include "Engine/Core/Memory/Allocation.h"
#include "Engine/Core/Memory/AllocationUtils.h"
#include "Engine/Core/Collections/HashFunctions.h"

/// <summary>
/// Default capacity for the hash set collections (minimum initial amount of space for the elements).
/// </summary>
#ifndef HASH_SET_DEFAULT_CAPACITY
#if PLATFORM_DESKTOP
#define HASH_SET_DEFAULT_CAPACITY 256
#else
#define HASH_SET_DEFAULT_CAPACITY 64
#endif
#endif

/// <summary>
/// Default slack space divider for the hash sets.
/// </summary>
#define HASH_SET_DEFAULT_SLACK_SCALE 3

/// <summary>
/// Function for hash set that tells how change hash index during iteration (size param is a buckets table size).
/// </summary>
#define HASH_SET_PROB_FUNC(size, numChecks) (numChecks)
//#define HASH_SET_PROB_FUNC(size, numChecks) (1)

// [Deprecated in v1.10] Use HASH_SET_DEFAULT_CAPACITY
#define DICTIONARY_DEFAULT_CAPACITY HASH_SET_DEFAULT_CAPACITY
// [Deprecated in v1.10] Use HASH_SET_DEFAULT_SLACK_SCALE
#define DICTIONARY_DEFAULT_SLACK_SCALE HASH_SET_DEFAULT_SLACK_SCALE
// [Deprecated in v1.10] Use HASH_SET_PROB_FUNC
#define DICTIONARY_PROB_FUNC(size, numChecks) (numChecks) HASH_SET_PROB_FUNC(size, numChecks)

/// <summary>
/// Tells if the object is occupied, and if not, if the bucket is a subject of compaction.
/// </summary>
enum class HashSetBucketState : byte
{
    Empty = 0,
    Deleted = 1,
    Occupied = 2,
};

/// <summary>
/// Base class for unordered set of values (without duplicates with O(1) lookup access).
/// </summary>
/// <typeparam name="BucketType">The type of bucket structure that stores element data and state.</typeparam>
/// <typeparam name="AllocationType">The type of memory allocator.</typeparam>
template<typename AllocationType, typename BucketType>
class HashSetBase
{
    friend HashSetBase;

public:
    // Type of allocation data used to store hash set buckets.
    using AllocationData = typename AllocationType::template Data<BucketType>;

protected:
    int32 _elementsCount = 0;
    int32 _deletedCount = 0;
    int32 _size = 0;
    AllocationData _allocation;

    HashSetBase()
    {
    }

    void MoveToEmpty(HashSetBase&& other)
    {
        _elementsCount = other._elementsCount;
        _deletedCount = other._deletedCount;
        _size = other._size;
        other._elementsCount = 0;
        other._deletedCount = 0;
        other._size = 0;
        AllocationUtils::MoveToEmpty<BucketType, AllocationType>(_allocation, other._allocation, _size, _size);
    }

    ~HashSetBase()
    {
        Clear();
    }

public:
    /// <summary>
    /// Gets the amount of the elements in the collection.
    /// </summary>
    FORCE_INLINE int32 Count() const
    {
        return _elementsCount;
    }

    /// <summary>
    /// Gets the amount of the elements that can be contained by the collection.
    /// </summary>
    FORCE_INLINE int32 Capacity() const
    {
        return _size;
    }

    /// <summary>
    /// Returns true if collection is empty.
    /// </summary>
    FORCE_INLINE bool IsEmpty() const
    {
        return _elementsCount == 0;
    }

    /// <summary>
    /// Returns true if collection has one or more elements.
    /// </summary>
    FORCE_INLINE bool HasItems() const
    {
        return _elementsCount != 0;
    }

public:
    /// <summary>
    /// Removes all elements from the collection.
    /// </summary>
    void Clear()
    {
        if (_elementsCount + _deletedCount != 0)
        {
            BucketType* data = _allocation.Get();
            for (int32 i = 0; i < _size; i++)
                data[i].Free();
            _elementsCount = _deletedCount = 0;
        }
    }

    /// <summary>
    /// Changes the capacity of the collection.
    /// </summary>
    /// <param name="capacity">The new capacity.</param>
    /// <param name="preserveContents">True if preserve collection data when changing its size, otherwise collection after resize will be empty.</param>
    void SetCapacity(int32 capacity, const bool preserveContents = true)
    {
        if (capacity == _size)
            return;
        ASSERT(capacity >= 0);
        AllocationData oldAllocation;
        AllocationUtils::MoveToEmpty<BucketType, AllocationType>(oldAllocation, _allocation, _size, _size);
        const int32 oldSize = _size;
        const int32 oldElementsCount = _elementsCount;
        _deletedCount = _elementsCount = 0;
        if (capacity != 0 && (capacity & (capacity - 1)) != 0)
            capacity = AllocationUtils::AlignToPowerOf2(capacity);
        if (capacity)
        {
            _allocation.Allocate(capacity);
            BucketType* data = _allocation.Get();
            for (int32 i = 0; i < capacity; i++)
                data[i]._state = HashSetBucketState::Empty;
        }
        _size = capacity;
        BucketType* oldData = oldAllocation.Get();
        if (oldElementsCount != 0 && capacity != 0 && preserveContents)
        {
            FindPositionResult pos;
            for (int32 i = 0; i < oldSize; i++)
            {
                BucketType& oldBucket = oldData[i];
                if (oldBucket.IsOccupied())
                {
                    FindPosition(oldBucket.GetKey(), pos);
                    ASSERT(pos.FreeSlotIndex != -1);
                    BucketType& bucket = _allocation.Get()[pos.FreeSlotIndex];
                    bucket = MoveTemp(oldBucket);
                    _elementsCount++;
                }
            }
        }
        if (oldElementsCount != 0)
        {
            for (int32 i = 0; i < oldSize; i++)
                oldData[i].Free();
        }
    }

    /// <summary>
    /// Ensures that collection has given capacity.
    /// </summary>
    /// <param name="minCapacity">The minimum required capacity.</param>
    /// <param name="preserveContents">True if preserve collection data when changing its size, otherwise collection after resize will be empty.</param>
    void EnsureCapacity(int32 minCapacity, const bool preserveContents = true)
    {
        minCapacity *= HASH_SET_DEFAULT_SLACK_SCALE;
        if (_size >= minCapacity)
            return;
        int32 capacity = _allocation.CalculateCapacityGrow(_size, minCapacity);
        if (capacity < HASH_SET_DEFAULT_CAPACITY)
            capacity = HASH_SET_DEFAULT_CAPACITY;
        SetCapacity(capacity, preserveContents);
    }

    /// <summary>
    /// Swaps the contents of collection with the other object without copy operation. Performs fast internal data exchange.
    /// </summary>
    /// <param name="other">The other collection.</param>
    void Swap(HashSetBase& other)
    {
        if IF_CONSTEXPR (AllocationType::HasSwap)
        {
            ::Swap(_elementsCount, other._elementsCount);
            ::Swap(_deletedCount, other._deletedCount);
            ::Swap(_size, other._size);
            _allocation.Swap(other._allocation);
        }
        else
        {
            ::Swap(other, *this);
        }
    }

public:
    /// <summary>
    /// The collection iterator base implementation.
    /// </summary>
    struct IteratorBase
    {
    protected:
        const HashSetBase* _collection;
        int32 _index;

        IteratorBase(const HashSetBase* collection, const int32 index)
            : _collection(collection)
            , _index(index)
        {
        }

        void Next()
        {
            const int32 capacity = _collection->_size;
            if (_index != capacity)
            {
                const BucketType* data = _collection->_allocation.Get();
                do
                {
                    ++_index;
                }
                while (_index != capacity && data[_index].IsNotOccupied());
            }
        }

        void Prev()
        {
            if (_index > 0)
            {
                const BucketType* data = _collection->_allocation.Get();
                do
                {
                    --_index;
                }
                while (_index > 0 && data[_index].IsNotOccupied());
            }
        }

    public:
        FORCE_INLINE int32 Index() const
        {
            return _index;
        }

        FORCE_INLINE bool IsEnd() const
        {
            return _index == _collection->_size;
        }

        FORCE_INLINE bool IsNotEnd() const
        {
            return _index != _collection->_size;
        }

        FORCE_INLINE const BucketType& operator*() const
        {
            return _collection->_allocation.Get()[_index];
        }

        FORCE_INLINE const BucketType* operator->() const
        {
            return &_collection->_allocation.Get()[_index];
        }

        FORCE_INLINE explicit operator bool() const
        {
            return _index >= 0 && _index < _collection->_size;
        }
    };

protected:
    /// <summary>
    /// The result container of the set item lookup searching.
    /// </summary>
    struct FindPositionResult
    {
        int32 ObjectIndex;
        int32 FreeSlotIndex;
    };

    /// <summary>
    /// Returns a pair of positions: 1st where the object is, 2nd where it would go if you wanted to insert it.
    /// 1st is -1 if object is not found; 2nd is -1 if it is.
    /// Because of deletions where-to-insert is not trivial: it's the first deleted bucket we see, as long as we don't find the item later.
    /// </summary>
    /// <param name="key">The key to find</param>
    /// <param name="result">A pair of values: where the object is and where it would go if you wanted to insert it</param>
    template<typename KeyComparableType>
    void FindPosition(const KeyComparableType& key, FindPositionResult& result) const
    {
        result.FreeSlotIndex = -1;
        if (_size == 0)
        {
            result.ObjectIndex = -1;
            return;
        }
        const int32 tableSizeMinusOne = _size - 1;
        int32 bucketIndex = GetHash(key) & tableSizeMinusOne;
        int32 insertPos = -1;
        int32 checksCount = 0;
        const BucketType* data = _allocation.Get();
        while (checksCount < _size)
        {
            // Empty bucket
            const BucketType& bucket = data[bucketIndex];
            if (bucket.IsEmpty())
            {
                // Found place to insert
                result.ObjectIndex = -1;
                result.FreeSlotIndex = insertPos == -1 ? bucketIndex : insertPos;
                return;
            }
            // Deleted bucket
            if (bucket.IsDeleted())
            {
                // Keep searching but mark to insert
                if (insertPos == -1)
                    insertPos = bucketIndex;
            }
            // Occupied bucket by target item
            else if (bucket.GetKey() == key)
            {
                // Found item
                result.ObjectIndex = bucketIndex;
                return;
            }

            // Move to the next bucket
            checksCount++;
            bucketIndex = (bucketIndex + HASH_SET_PROB_FUNC(_size, checksCount)) & tableSizeMinusOne;
        }
        result.ObjectIndex = -1;
        result.FreeSlotIndex = insertPos;
    }

    template<typename KeyComparableType>
    BucketType* OnAdd(const KeyComparableType& key, bool checkUnique = true)
    {
        // Check if need to rehash elements (prevent many deleted elements that use too much of capacity)
        if (_deletedCount * HASH_SET_DEFAULT_SLACK_SCALE > _size)
            Compact();

        // Ensure to have enough memory for the next item (in case of new element insertion)
        EnsureCapacity(((_elementsCount + 1) * HASH_SET_DEFAULT_SLACK_SCALE + _deletedCount) / HASH_SET_DEFAULT_SLACK_SCALE);

        // Find location of the item or place to insert it
        FindPositionResult pos;
        FindPosition(key, pos);

        // Check if object has been already added
        if (pos.ObjectIndex != -1)
        {
            if (checkUnique)
            {
                Platform::CheckFailed("That key has been already added to the collection.", __FILE__, __LINE__);
                return nullptr;
            }
            return &_allocation.Get()[pos.ObjectIndex];
        }

        // Insert
        ASSERT(pos.FreeSlotIndex != -1);
        ++_elementsCount;
        return &_allocation.Get()[pos.FreeSlotIndex];
    }

    void Compact()
    {
        if (_elementsCount == 0)
        {
            // Fast path if it's empty
            BucketType* data = _allocation.Get();
            for (int32 i = 0; i < _size; ++i)
                data[i]._state = HashSetBucketState::Empty;
        }
        else
        {
            // Rebuild entire table completely
            AllocationData oldAllocation;
            AllocationUtils::MoveToEmpty<BucketType, AllocationType>(oldAllocation, _allocation, _size, _size);
            _allocation.Allocate(_size);
            BucketType* data = _allocation.Get();
            for (int32 i = 0; i < _size; ++i)
                data[i]._state = HashSetBucketState::Empty;
            BucketType* oldData = oldAllocation.Get();
            FindPositionResult pos;
            for (int32 i = 0; i < _size; ++i)
            {
                BucketType& oldBucket = oldData[i];
                if (oldBucket.IsOccupied())
                {
                    FindPosition(oldBucket.GetKey(), pos);
                    ASSERT(pos.FreeSlotIndex != -1);
                    BucketType& bucket = _allocation.Get()[pos.FreeSlotIndex];
                    bucket = MoveTemp(oldBucket);
                }
            }
            for (int32 i = 0; i < _size; ++i)
                oldData[i].Free();
        }
        _deletedCount = 0;
    }
};
