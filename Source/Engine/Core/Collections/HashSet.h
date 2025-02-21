// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Memory/Memory.h"
#include "Engine/Core/Memory/Allocation.h"
#include "Engine/Core/Collections/BucketState.h"
#include "Engine/Core/Collections/HashFunctions.h"
#include "Engine/Core/Collections/Config.h"

/// <summary>
/// Template for unordered set of values (without duplicates with O(1) lookup access).
/// </summary>
/// <typeparam name="T">The type of elements in the set.</typeparam>
/// <typeparam name="AllocationType">The type of memory allocator.</typeparam>
template<typename T, typename AllocationType = HeapAllocation>
API_CLASS(InBuild) class HashSet
{
    friend HashSet;
public:
    /// <summary>
    /// Describes single portion of space for the item in a hash map.
    /// </summary>
    struct Bucket
    {
        friend HashSet;

        /// <summary>The item.</summary>
        T Item;

    private:
        BucketState _state;

        FORCE_INLINE void Free()
        {
            if (_state == BucketState::Occupied)
                Memory::DestructItem(&Item);
            _state = BucketState::Empty;
        }

        FORCE_INLINE void Delete()
        {
            _state = BucketState::Deleted;
            Memory::DestructItem(&Item);
        }

        template<typename ItemType>
        FORCE_INLINE void Occupy(const ItemType& item)
        {
            Memory::ConstructItems(&Item, &item, 1);
            _state = BucketState::Occupied;
        }

        template<typename ItemType>
        FORCE_INLINE void Occupy(ItemType&& item)
        {
            Memory::MoveItems(&Item, &item, 1);
            _state = BucketState::Occupied;
        }

        FORCE_INLINE bool IsEmpty() const
        {
            return _state == BucketState::Empty;
        }

        FORCE_INLINE bool IsDeleted() const
        {
            return _state == BucketState::Deleted;
        }

        FORCE_INLINE bool IsOccupied() const
        {
            return _state == BucketState::Occupied;
        }

        FORCE_INLINE bool IsNotOccupied() const
        {
            return _state != BucketState::Occupied;
        }
    };

    using AllocationData = typename AllocationType::template Data<Bucket>;

private:
    int32 _elementsCount = 0;
    int32 _deletedCount = 0;
    int32 _size = 0;
    AllocationData _allocation;

    FORCE_INLINE static void MoveToEmpty(AllocationData& to, AllocationData& from, const int32 fromSize)
    {
        if IF_CONSTEXPR (AllocationType::HasSwap)
            to.Swap(from);
        else
        {
            to.Allocate(fromSize);
            Bucket* toData = to.Get();
            Bucket* fromData = from.Get();
            for (int32 i = 0; i < fromSize; ++i)
            {
                Bucket& fromBucket = fromData[i];
                if (fromBucket.IsOccupied())
                {
                    Bucket& toBucket = toData[i];
                    Memory::MoveItems(&toBucket.Item, &fromBucket.Item, 1);
                    toBucket._state = BucketState::Occupied;
                    Memory::DestructItem(&fromBucket.Item);
                    fromBucket._state = BucketState::Empty;
                }
            }
            from.Free();
        }
    }

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="HashSet"/> class.
    /// </summary>
    HashSet()
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="HashSet"/> class.
    /// </summary>
    /// <param name="capacity">The initial capacity.</param>
    explicit HashSet(const int32 capacity)
    {
        SetCapacity(capacity);
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="HashSet"/> class.
    /// </summary>
    /// <param name="other">The other collection to move.</param>
    HashSet(HashSet&& other) noexcept
    {
        _elementsCount = other._elementsCount;
        _deletedCount = other._deletedCount;
        _size = other._size;
        other._elementsCount = 0;
        other._deletedCount = 0;
        other._size = 0;
        MoveToEmpty(_allocation, other._allocation, _size);
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="HashSet"/> class.
    /// </summary>
    /// <param name="other">Other collection to copy</param>
    HashSet(const HashSet& other)
    {
        Clone(other);
    }

    /// <summary>
    /// Clones the data from the other collection.
    /// </summary>
    /// <param name="other">The other collection to copy.</param>
    /// <returns>The reference to this.</returns>
    HashSet& operator=(const HashSet& other)
    {
        if (this != &other)
            Clone(other);
        return *this;
    }

    /// <summary>
    /// Moves the data from the other collection.
    /// </summary>
    /// <param name="other">The other collection to move.</param>
    /// <returns>The reference to this.</returns>
    HashSet& operator=(HashSet&& other) noexcept
    {
        if (this != &other)
        {
            Clear();
            _allocation.Free();
            _elementsCount = other._elementsCount;
            _deletedCount = other._deletedCount;
            _size = other._size;
            other._elementsCount = 0;
            other._deletedCount = 0;
            other._size = 0;
            MoveToEmpty(_allocation, other._allocation, _size);
        }
        return *this;
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="HashSet"/> class.
    /// </summary>
    ~HashSet()
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
    /// The hash set collection iterator.
    /// </summary>
    struct Iterator
    {
        friend HashSet;
    private:
        HashSet* _collection;
        int32 _index;

    public:
        Iterator(HashSet* collection, const int32 index)
            : _collection(collection)
            , _index(index)
        {
        }

        Iterator(const HashSet* collection, const int32 index)
            : _collection(const_cast<HashSet*>(collection))
            , _index(index)
        {
        }

        Iterator()
            : _collection(nullptr)
            , _index(-1)
        {
        }

        Iterator(const Iterator& i)
            : _collection(i._collection)
            , _index(i._index)
        {
        }

        Iterator(Iterator&& i) noexcept
            : _collection(i._collection)
            , _index(i._index)
        {
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

        FORCE_INLINE Bucket& operator*() const
        {
            return _collection->_allocation.Get()[_index];
        }

        FORCE_INLINE Bucket* operator->() const
        {
            return &_collection->_allocation.Get()[_index];
        }

        FORCE_INLINE explicit operator bool() const
        {
            return _index >= 0 && _index < _collection->_size;
        }

        FORCE_INLINE bool operator!() const
        {
            return !(bool)*this;
        }

        FORCE_INLINE bool operator==(const Iterator& v) const
        {
            return _index == v._index && _collection == v._collection;
        }

        FORCE_INLINE bool operator!=(const Iterator& v) const
        {
            return _index != v._index || _collection != v._collection;
        }

        Iterator& operator=(const Iterator& v)
        {
            _collection = v._collection;
            _index = v._index;
            return *this;
        }

        Iterator& operator=(Iterator&& v) noexcept
        {
            _collection = v._collection;
            _index = v._index;
            return *this;
        }

        Iterator& operator++()
        {
            const int32 capacity = _collection->_size;
            if (_index != capacity)
            {
                const Bucket* data = _collection->_allocation.Get();
                do
                {
                    ++_index;
                }
                while (_index != capacity && data[_index].IsNotOccupied());
            }
            return *this;
        }

        Iterator operator++(int)
        {
            Iterator i = *this;
            ++i;
            return i;
        }

        Iterator& operator--()
        {
            if (_index > 0)
            {
                const Bucket* data = _collection->_allocation.Get();
                do
                {
                    --_index;
                }
                while (_index > 0 && data[_index].IsNotOccupied());
            }
            return *this;
        }

        Iterator operator--(int)
        {
            Iterator i = *this;
            --i;
            return i;
        }
    };

public:
    /// <summary>
    /// Removes all elements from the collection.
    /// </summary>
    void Clear()
    {
        if (_elementsCount + _deletedCount != 0)
        {
            Bucket* data = _allocation.Get();
            for (int32 i = 0; i < _size; i++)
                data[i].Free();
            _elementsCount = _deletedCount = 0;
        }
    }

    /// <summary>
    /// Clears the collection and delete value objects.
    /// Note: collection must contain pointers to the objects that have public destructor and be allocated using New method.
    /// </summary>
#if defined(_MSC_VER)
    template<typename = typename TEnableIf<TIsPointer<T>::Value>::Type>
#endif
    void ClearDelete()
    {
        for (Iterator i = Begin(); i.IsNotEnd(); ++i)
        {
            if (i->Item)
                ::Delete(i->Item);
        }
        Clear();
    }

    /// <summary>
    /// Changes capacity of the collection
    /// </summary>
    /// <param name="capacity">New capacity</param>
    /// <param name="preserveContents">Enable/disable preserving collection contents during resizing</param>
    void SetCapacity(int32 capacity, const bool preserveContents = true)
    {
        if (capacity == Capacity())
            return;
        ASSERT(capacity >= 0);
        AllocationData oldAllocation;
        MoveToEmpty(oldAllocation, _allocation, _size);
        const int32 oldSize = _size;
        const int32 oldElementsCount = _elementsCount;
        _deletedCount = _elementsCount = 0;
        if (capacity != 0 && (capacity & (capacity - 1)) != 0)
        {
            // Align capacity value to the next power of two (http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2)
            capacity--;
            capacity |= capacity >> 1;
            capacity |= capacity >> 2;
            capacity |= capacity >> 4;
            capacity |= capacity >> 8;
            capacity |= capacity >> 16;
            capacity++;
        }
        if (capacity)
        {
            _allocation.Allocate(capacity);
            Bucket* data = _allocation.Get();
            for (int32 i = 0; i < capacity; i++)
                data[i]._state = BucketState::Empty;
        }
        _size = capacity;
        Bucket* oldData = oldAllocation.Get();
        if (oldElementsCount != 0 && capacity != 0 && preserveContents)
        {
            FindPositionResult pos;
            for (int32 i = 0; i < oldSize; i++)
            {
                Bucket& oldBucket = oldData[i];
                if (oldBucket.IsOccupied())
                {
                    FindPosition(oldBucket.Item, pos);
                    ASSERT(pos.FreeSlotIndex != -1);
                    Bucket* bucket = &_allocation.Get()[pos.FreeSlotIndex];
                    Memory::MoveItems(&bucket->Item, &oldBucket.Item, 1);
                    bucket->_state = BucketState::Occupied;
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
    void EnsureCapacity(const int32 minCapacity, const bool preserveContents = true)
    {
        if (_size >= minCapacity)
            return;
        int32 capacity = _allocation.CalculateCapacityGrow(_size, minCapacity);
        if (capacity < DICTIONARY_DEFAULT_CAPACITY)
            capacity = DICTIONARY_DEFAULT_CAPACITY;
        SetCapacity(capacity, preserveContents);
    }

    /// <summary>
    /// Swaps the contents of collection with the other object without copy operation. Performs fast internal data exchange.
    /// </summary>
    /// <param name="other">The other collection.</param>
    void Swap(HashSet& other)
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
            HashSet tmp = MoveTemp(other);
            other = *this;
            *this = MoveTemp(tmp);
        }
    }

public:
    /// <summary>
    /// Add element to the collection.
    /// </summary>
    /// <param name="item">The element to add to the set.</param>
    /// <returns>True if element has been added to the collection, otherwise false if the element is already present.</returns>
    template<typename ItemType>
    bool Add(const ItemType& item)
    {
        Bucket* bucket = OnAdd(item);
        if (bucket)
            bucket->Occupy(item);
        return bucket != nullptr;
    }

    /// <summary>
    /// Add element to the collection.
    /// </summary>
    /// <param name="item">The element to add to the set.</param>
    /// <returns>True if element has been added to the collection, otherwise false if the element is already present.</returns>
    bool Add(T&& item)
    {
        Bucket* bucket = OnAdd(item);
        if (bucket)
            bucket->Occupy(MoveTemp(item));
        return bucket != nullptr;
    }

    /// <summary>
    /// Add element at iterator to the collection
    /// </summary>
    /// <param name="i">Iterator with item to add</param>
    void Add(const Iterator& i)
    {
        ASSERT(i._collection != this && i);
        const Bucket& bucket = *i;
        Add(bucket.Item);
    }

    /// <summary>
    /// Removes the specified element from the collection.
    /// </summary>
    /// <param name="item">The element to remove.</param>
    /// <returns>True if cannot remove item from the collection because cannot find it, otherwise false.</returns>
    template<typename ItemType>
    bool Remove(const ItemType& item)
    {
        if (IsEmpty())
            return false;
        FindPositionResult pos;
        FindPosition(item, pos);
        if (pos.ObjectIndex != -1)
        {
            _allocation.Get()[pos.ObjectIndex].Delete();
            --_elementsCount;
            ++_deletedCount;
            return true;
        }
        return false;
    }

    /// <summary>
    /// Removes an element at specified iterator position.
    /// </summary>
    /// <param name="i">The element iterator to remove.</param>
    /// <returns>True if cannot remove item from the collection because cannot find it, otherwise false.</returns>
    bool Remove(const Iterator& i)
    {
        ASSERT(i._collection == this);
        if (i)
        {
            ASSERT(_allocation.Get()[i._index].IsOccupied());
            _allocation.Get()[i._index].Delete();
            --_elementsCount;
            ++_deletedCount;
            return true;
        }
        return false;
    }

public:
    /// <summary>
    /// Find element with given item in the collection
    /// </summary>
    /// <param name="item">Item to find</param>
    /// <returns>Iterator for the found element or End if cannot find it</returns>
    template<typename ItemType>
    Iterator Find(const ItemType& item) const
    {
        if (IsEmpty())
            return End();
        FindPositionResult pos;
        FindPosition(item, pos);
        return pos.ObjectIndex != -1 ? Iterator(this, pos.ObjectIndex) : End();
    }

    /// <summary>
    /// Determines whether a collection contains the specified element.
    /// </summary>
    /// <param name="item">The item to locate.</param>
    /// <returns>True if value has been found in a collection, otherwise false</returns>
    template<typename ItemType>
    bool Contains(const ItemType& item) const
    {
        if (IsEmpty())
            return false;
        FindPositionResult pos;
        FindPosition(item, pos);
        return pos.ObjectIndex != -1;
    }

public:
    /// <summary>
    /// Clones other collection into this
    /// </summary>
    /// <param name="other">Other collection to clone</param>
    void Clone(const HashSet& other)
    {
        Clear();
        SetCapacity(other.Capacity(), false);
        for (Iterator i = other.Begin(); i != other.End(); ++i)
            Add(i);
        ASSERT(Count() == other.Count());
        ASSERT(Capacity() == other.Capacity());
    }

public:
    Iterator Begin() const
    {
        Iterator i(this, -1);
        ++i;
        return i;
    }

    Iterator End() const
    {
        return Iterator(this, _size);
    }

    Iterator begin()
    {
        Iterator i(this, -1);
        ++i;
        return i;
    }

    FORCE_INLINE Iterator end()
    {
        return Iterator(this, _size);
    }

    Iterator begin() const
    {
        Iterator i(this, -1);
        ++i;
        return i;
    }

    FORCE_INLINE Iterator end() const
    {
        return Iterator(this, _size);
    }

private:
    /// <summary>
    /// The result container of the set item lookup searching.
    /// </summary>
    struct FindPositionResult
    {
        int32 ObjectIndex;
        int32 FreeSlotIndex;
    };

    /// <summary>
    /// Returns a pair of positions: 1st where the object is, 2nd where
    /// it would go if you wanted to insert it. 1st is -1
    /// if object is not found; 2nd is -1 if it is.
    /// Note: because of deletions where-to-insert is not trivial: it's the
    /// first deleted bucket we see, as long as we don't find the item later
    /// </summary>
    /// <param name="item">The item to find</param>
    /// <param name="result">Pair of values: where the object is and where it would go if you wanted to insert it</param>
    template<typename ItemType>
    void FindPosition(const ItemType& item, FindPositionResult& result) const
    {
        ASSERT(_size);
        const int32 tableSizeMinusOne = _size - 1;
        int32 bucketIndex = GetHash(item) & tableSizeMinusOne;
        int32 insertPos = -1;
        int32 numChecks = 0;
        const Bucket* data = _allocation.Get();
        result.FreeSlotIndex = -1;
        while (numChecks < _size)
        {
            // Empty bucket
            const Bucket& bucket = data[bucketIndex];
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
            else if (bucket.Item == item)
            {
                // Found item
                result.ObjectIndex = bucketIndex;
                return;
            }

            numChecks++;
            bucketIndex = (bucketIndex + DICTIONARY_PROB_FUNC(_size, numChecks)) & tableSizeMinusOne;
        }
        result.ObjectIndex = -1;
        result.FreeSlotIndex = insertPos;
    }

    template<typename ItemType>
    Bucket* OnAdd(const ItemType& key)
    {
        // Check if need to rehash elements (prevent many deleted elements that use too much of capacity)
        if (_deletedCount * DICTIONARY_DEFAULT_SLACK_SCALE > _size)
            Compact();

        // Ensure to have enough memory for the next item (in case of new element insertion)
        EnsureCapacity((_elementsCount + 1) * DICTIONARY_DEFAULT_SLACK_SCALE + _deletedCount);

        // Find location of the item or place to insert it
        FindPositionResult pos;
        FindPosition(key, pos);

        // Check if object has been already added
        if (pos.ObjectIndex != -1)
            return nullptr;

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
            Bucket* data = _allocation.Get();
            for (int32 i = 0; i < _size; ++i)
                data[i]._state = BucketState::Empty;
        }
        else
        {
            // Rebuild entire table completely
            AllocationData oldAllocation;
            MoveToEmpty(oldAllocation, _allocation, _size);
            _allocation.Allocate(_size);
            Bucket* data = _allocation.Get();
            for (int32 i = 0; i < _size; ++i)
                data[i]._state = BucketState::Empty;
            Bucket* oldData = oldAllocation.Get();
            FindPositionResult pos;
            for (int32 i = 0; i < _size; ++i)
            {
                Bucket& oldBucket = oldData[i];
                if (oldBucket.IsOccupied())
                {
                    FindPosition(oldBucket.Item, pos);
                    ASSERT(pos.FreeSlotIndex != -1);
                    Bucket* bucket = &_allocation.Get()[pos.FreeSlotIndex];
                    Memory::MoveItems(&bucket->Item, &oldBucket.Item, 1);
                    bucket->_state = BucketState::Occupied;
                }
            }
            for (int32 i = 0; i < _size; ++i)
                oldData[i].Free();
        }
        _deletedCount = 0;
    }
};
