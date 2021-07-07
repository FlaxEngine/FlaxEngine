// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Core.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Platform/Platform.h"
#include "HashFunctions.h"
#include "Config.h"

/// <summary>
/// Template for unordered set of values (without duplicates with O(1) lookup access).
/// </summary>
template<typename T>
API_CLASS(InBuild) class HashSet
{
    friend HashSet;

public:

    /// <summary>
    /// Describes single portion of space for the item in a hash map
    /// </summary>
    struct Bucket
    {
        friend HashSet;

    public:

        enum State : byte
        {
            Empty,
            Deleted,
            Occupied,
        };

    public:

        T Item;

    private:

        State _state;

    public:

        Bucket()
            : _state(Empty)
        {
        }

        ~Bucket()
        {
        }

    public:

        void Free()
        {
            _state = Empty;
        }

        void Delete()
        {
            _state = Deleted;
        }

        void Occupy(const T& item)
        {
            Item = item;
            _state = Occupied;
        }

    public:

        FORCE_INLINE bool IsEmpty() const
        {
            return _state == Empty;
        }

        FORCE_INLINE bool IsDeleted() const
        {
            return _state == Deleted;
        }

        FORCE_INLINE bool IsOccupied() const
        {
            return _state == Occupied;
        }

        FORCE_INLINE bool IsNotOccupied() const
        {
            return _state != Occupied;
        }
    };

private:

    int32 _elementsCount = 0;
    int32 _deletedCount = 0;
    int32 _tableSize = 0;
    Bucket* _table = nullptr;

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
    HashSet(int32 capacity)
    {
        ASSERT(capacity >= 0);
        SetCapacity(capacity);
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
        // Ensure we're not trying to set to itself
        if (this != &other)
            Clone(other);
        return *this;
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="HashSet"/> class.
    /// </summary>
    ~HashSet()
    {
        Cleanup();
    }

public:

    /// <summary>
    /// Gets the amount of the elements in the collection.
    /// </summary>
    /// <returns>The amount of elements in the collection.</returns>
    FORCE_INLINE int32 Count() const
    {
        return _elementsCount;
    }

    /// <summary>
    /// Gets the amount of the elements that can be contained by the collection.
    /// </summary>
    /// <returns>The capacity of the collection.</returns>
    FORCE_INLINE int32 Capacity() const
    {
        return _tableSize;
    }

    /// <summary>
    /// Returns true if collection is empty.
    /// </summary>
    /// <returns>True if is empty, otherwise false.</returns>
    FORCE_INLINE bool IsEmpty() const
    {
        return _elementsCount == 0;
    }

    /// <summary>
    /// Returns true if collection has one or more elements.
    /// </summary>
    /// <returns>True if isn't empty, otherwise false.</returns>
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

        HashSet& _collection;
        int32 _index;

        Iterator(HashSet& collection, const int32 index)
            : _collection(collection)
            , _index(index)
        {
        }

        Iterator(HashSet const& collection, const int32 index)
            : _collection((HashSet&)collection)
            , _index(index)
        {
        }

    public:

        Iterator(const Iterator& i)
            : _collection(i._collection)
            , _index(i._index)
        {
        }

    public:

        /// <summary>
        /// Checks if iterator is in the end of the collection
        /// </summary>
        /// <returns>True if is in the end, otherwise false</returns>
        FORCE_INLINE bool IsEnd() const
        {
            return _index == _collection.Capacity();
        }

        /// <summary>
        /// Checks if iterator is not in the end of the collection
        /// </summary>
        /// <returns>True if is not in the end, otherwise false</returns>
        FORCE_INLINE bool IsNotEnd() const
        {
            return _index != _collection.Capacity();
        }

    public:

        FORCE_INLINE Bucket& operator*() const
        {
            return _collection._table[_index];
        }

        FORCE_INLINE Bucket* operator->() const
        {
            return &_collection._table[_index];
        }

        FORCE_INLINE explicit operator bool() const
        {
            return _index >= 0 && _index < _collection._tableSize;
        }

        FORCE_INLINE bool operator !() const
        {
            return !(bool)*this;
        }

        FORCE_INLINE bool operator==(const Iterator& v) const
        {
            return _index == v._index && &_collection == &v._collection;
        }

        FORCE_INLINE bool operator!=(const Iterator& v) const
        {
            return _index != v._index || &_collection != &v._collection;
        }

    public:

        Iterator& operator++()
        {
            const int32 capacity = _collection.Capacity();
            if (_index != capacity)
            {
                do
                {
                    _index++;
                } while (_index != capacity && _collection._table[_index].IsNotOccupied());
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
                do
                {
                    _index--;
                } while (_index > 0 && _collection._table[_index].IsNotOccupied());
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
        if (_table)
        {
            // Free all buckets
            // Note: this will not clear allocated objects space!
            for (int32 i = 0; i < _tableSize; i++)
                _table[i].Free();
            _elementsCount = _deletedCount = 0;
        }
    }

    /// <summary>
    /// Clear the collection and delete value objects.
    /// </summary>
    void ClearDelete()
    {
        for (auto i = Begin(); i.IsNotEnd(); ++i)
        {
            if (i->Value)
                ::Delete(i->Value);
        }
        Clear();
    }

    /// <summary>
    /// Changes capacity of the collection
    /// </summary>
    /// <param name="capacity">New capacity</param>
    /// <param name="preserveContents">Enable/disable preserving collection contents during resizing</param>
    void SetCapacity(int32 capacity, bool preserveContents = true)
    {
        // Validate input
        ASSERT(capacity >= 0);

        // Check if capacity won't change
        if (capacity == Capacity())
            return;

        // Cache previous state
        auto oldTable = _table;
        auto oldTableSize = _tableSize;

        // Clear elements counters
        const auto oldElementsCount = _elementsCount;
        _deletedCount = _elementsCount = 0;

        // Check if need to create a new table
        if (capacity > 0)
        {
            // Align capacity value
            if (Math::IsPowerOfTwo(capacity) == false)
                capacity = Math::RoundUpToPowerOf2(capacity);

            // Allocate new table
            _table = NewArray<Bucket>(capacity);
            _tableSize = capacity;

            // Check if preserve content
            if (oldElementsCount != 0 && preserveContents)
            {
                // Try to preserve all values in the collection
                for (int32 i = 0; i < oldTableSize; i++)
                {
                    if (oldTable[i].IsOccupied())
                        Add(oldTable[i].Item);
                }
            }
        }
        else
        {
            // Clear data
            _table = nullptr;
            _tableSize = 0;
        }
        ASSERT(preserveContents == false || _elementsCount == oldElementsCount);

        // Delete old table
        if (oldTable)
        {
            DeleteArray(oldTable, oldTableSize);
        }
    }

    /// <summary>
    /// Increases collection capacity by given extra size (content will be preserved)
    /// </summary>
    /// <param name="extraSize">Extra size to enlarge collection</param>
    FORCE_INLINE void IncreaseCapacity(int32 extraSize)
    {
        ASSERT(extraSize >= 0);
        SetCapacity(Capacity() + extraSize);
    }

    /// <summary>
    /// Ensures that collection has given capacity
    /// </summary>
    /// <param name="minCapacity">Minimum required capacity</param>
    void EnsureCapacity(int32 minCapacity)
    {
        if (Capacity() >= minCapacity)
            return;
        int32 num = Capacity() == 0 ? DICTIONARY_DEFAULT_CAPACITY : Capacity() * 2;
        SetCapacity(Math::Clamp<int32>(num, minCapacity, MAX_int32 - 1410));
    }

    /// <summary>
    /// Cleanup collection data (changes size to 0 without data preserving)
    /// </summary>
    FORCE_INLINE void Cleanup()
    {
        SetCapacity(0, false);
    }

public:

    /// <summary>
    /// Add element to the collection.
    /// </summary>
    /// <param name="item">The element to add to the set.</param>
    /// <returns>True if element has been added to the collection, otherwise false if the element is already present.</returns>
    bool Add(const T& item)
    {
        // Ensure to have enough memory for the next item (in case of new element insertion)
        EnsureCapacity(_elementsCount + _deletedCount + 1);

        // Find location of the item or place to insert it
        FindPositionResult pos;
        FindPosition(item, pos);

        // Check if object has been already added
        if (pos.ObjectIndex != INVALID_INDEX)
            return false;

        // Insert
        ASSERT(pos.FreeSlotIndex != INVALID_INDEX);
        auto bucket = &_table[pos.FreeSlotIndex];
        bucket->Occupy(item);
        _elementsCount++;

        return true;
    }

    /// <summary>
    /// Add element at iterator to the collection
    /// </summary>
    /// <param name="i">Iterator with item to add</param>
    void Add(const Iterator& i)
    {
        ASSERT(&i._collection != this && i);
        Bucket& bucket = *i;
        Add(bucket.Item);
    }

    /// <summary>
    /// Removes the specified element from the collection.
    /// </summary>
    /// <param name="item">The element to remove.</param>
    /// <returns>True if cannot remove item from the collection because cannot find it, otherwise false.</returns>
    bool Remove(const T& item)
    {
        if (IsEmpty())
            return true;

        FindPositionResult pos;
        FindPosition(item, pos);

        if (pos.ObjectIndex != INVALID_INDEX)
        {
            _table[pos.ObjectIndex].Delete();
            _elementsCount--;
            _deletedCount++;
            return true;
        }

        return false;
    }

    /// <summary>
    /// Removes an element at specified iterator position.
    /// </summary>
    /// <param name="i">The element iterator to remove.</param>
    /// <returns>True if cannot remove item from the collection because cannot find it, otherwise false.</returns>
    bool Remove(Iterator& i)
    {
        ASSERT(&i._collection == this);
        if (i)
        {
            ASSERT(_table[i._index].IsOccupied());
            _table[i._index].Delete();
            _elementsCount--;
            _deletedCount++;
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
    Iterator Find(const T& item) const
    {
        if (IsEmpty())
            return End();

        FindPositionResult pos;
        FindPosition(item, pos);

        return pos.ObjectIndex != INVALID_INDEX ? Iterator(*this, pos.ObjectIndex) : End();
    }

    /// <summary>
    /// Determines whether a collection contains the specified element.
    /// </summary>
    /// <param name="item">The item to locate.</param>
    /// <returns>True if value has been found in a collection, otherwise false</returns>
    bool Contains(const T& item) const
    {
        if (IsEmpty())
            return false;

        FindPositionResult pos;
        FindPosition(item, pos);

        return pos.ObjectIndex != INVALID_INDEX;
    }

public:

    /// <summary>
    /// Clones other collection into this
    /// </summary>
    /// <param name="other">Other collection to clone</param>
    void Clone(const HashSet& other)
    {
        // Clear previous data
        Clear();

        // Update capacity
        SetCapacity(other.Capacity(), false);

        // Clone items
        for (auto i = other.Begin(); i != other.End(); ++i)
            Add(i);

        // Check
        ASSERT(Count() == other.Count());
        ASSERT(Capacity() == other.Capacity());
    }

public:

    /// <summary>
    /// Gets iterator for beginning of the collection.
    /// </summary>
    /// <returns>Iterator for beginning of the collection.</returns>
    Iterator Begin() const
    {
        Iterator i(*this, INVALID_INDEX);
        ++i;
        return i;
    }

    /// <summary>
    /// Gets iterator for ending of the collection.
    /// </summary>
    /// <returns>Iterator for ending of the collection.</returns>
    Iterator End() const
    {
        return Iterator(*this, _tableSize);
    }

    Iterator begin()
    {
        Iterator i(*this, -1);
        ++i;
        return i;
    }

    FORCE_INLINE Iterator end()
    {
        return Iterator(*this, _tableSize);
    }

    const Iterator begin() const
    {
        Iterator i(*this, -1);
        ++i;
        return i;
    }

    FORCE_INLINE const Iterator end() const
    {
        return Iterator(*this, _tableSize);
    }

protected:

    struct FindPositionResult
    {
        int32 ObjectIndex;
        int32 FreeSlotIndex;
    };

    /// <summary>
    /// Returns a pair of positions: 1st where the object is, 2nd where
    /// it would go if you wanted to insert it. 1st is INVALID_INDEX
    /// if object is not found; 2nd is INVALID_INDEX if it is.
    /// Note: because of deletions where-to-insert is not trivial: it's the
    /// first deleted bucket we see, as long as we don't find the item later
    /// </summary>
    /// <param name="item">The item to find</param>
    /// <param name="result">Pair of values: where the object is and where it would go if you wanted to insert it</param>
    void FindPosition(const T& item, FindPositionResult& result) const
    {
        ASSERT(_table);

        const int32 tableSizeMinusOne = _tableSize - 1;
        int32 bucketIndex = GetHash(item) & tableSizeMinusOne;
        int32 insertPos = INVALID_INDEX;
        int32 numChecks = 0;
        result.FreeSlotIndex = INVALID_INDEX;

        while (numChecks < _tableSize)
        {
            // Empty bucket
            if (_table[bucketIndex].IsEmpty())
            {
                // Found place to insert
                result.ObjectIndex = INVALID_INDEX;
                result.FreeSlotIndex = insertPos == INVALID_INDEX ? bucketIndex : insertPos;
                return;
            }
            // Deleted bucket
            if (_table[bucketIndex].IsDeleted())
            {
                // Keep searching but mark to insert
                if (insertPos == INVALID_INDEX)
                    insertPos = bucketIndex;
            }
                // Occupied bucket by target item
            else if (_table[bucketIndex].Item == item)
            {
                // Found item
                result.ObjectIndex = bucketIndex;
                return;
            }

            numChecks++;
            bucketIndex = (bucketIndex + DICTIONARY_PROB_FUNC(_tableSize, numChecks)) & tableSizeMinusOne;
        }

        result.ObjectIndex = INVALID_INDEX;
        result.FreeSlotIndex = insertPos;
    }
};
