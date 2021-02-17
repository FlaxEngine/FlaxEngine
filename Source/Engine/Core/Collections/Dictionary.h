// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Collections/HashFunctions.h"
#include "Engine/Core/Collections/Config.h"

/// <summary>
/// Template for unordered dictionary with mapped key with value pairs.
/// </summary>
template<typename KeyType, typename ValueType>
API_CLASS(InBuild) class Dictionary
{
    friend Dictionary;
public:

    /// <summary>
    /// Describes single portion of space for the key and value pair in a hash map.
    /// </summary>
    struct Bucket
    {
        friend Dictionary;

        enum State : byte
        {
            Empty = 0,
            Deleted = 1,
            Occupied = 2,
        };

    public:
        /// <summary>The key.</summary>
        KeyType Key;
        /// <summary>The value.</summary>
        ValueType Value;

    private:
        State _state;

        void Free()
        {
            if (_state == Occupied)
            {
                Memory::DestructItem(&Key);
                Memory::DestructItem(&Value);
            }
            _state = Empty;
        }

        void Delete()
        {
            _state = Deleted;
            Memory::DestructItem(&Key);
            Memory::DestructItem(&Value);
        }

        template<typename KeyComparableType>
        void Occupy(const KeyComparableType& key)
        {
            Memory::ConstructItems(&Key, &key, 1);
            Memory::ConstructItem(&Value);
            _state = Occupied;
        }

        template<typename KeyComparableType>
        void Occupy(const KeyComparableType& key, const ValueType& value)
        {
            Memory::ConstructItems(&Key, &key, 1);
            Memory::ConstructItems(&Value, &value, 1);
            _state = Occupied;
        }

        template<typename KeyComparableType>
        void Occupy(const KeyComparableType& key, ValueType&& value)
        {
            Memory::ConstructItems(&Key, &key, 1);
            Memory::MoveItems(&Value, &value, 1);
            _state = Occupied;
        }

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
    /// Initializes a new instance of the <see cref="Dictionary"/> class.
    /// </summary>
    Dictionary()
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Dictionary"/> class.
    /// </summary>
    /// <param name="capacity">The initial capacity.</param>
    Dictionary(int32 capacity)
    {
        SetCapacity(capacity);
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Dictionary"/> class.
    /// </summary>
    /// <param name="other">Other collection to copy</param>
    Dictionary(const Dictionary& other)
    {
        Clone(other);
    }

    /// <summary>
    /// Clones the data from the other collection.
    /// </summary>
    /// <param name="other">The other collection to copy.</param>
    /// <returns>The reference to this.</returns>
    Dictionary& operator=(const Dictionary& other)
    {
        // Ensure we're not trying to set to itself
        if (this != &other)
            Clone(other);
        return *this;
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="Dictionary"/> class.
    /// </summary>
    ~Dictionary()
    {
        Cleanup();
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
        return _tableSize;
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
    /// The Dictionary collection iterator.
    /// </summary>
    struct Iterator
    {
        friend Dictionary;
    private:

        Dictionary& _collection;
        int32 _index;

        Iterator(Dictionary& collection, const int32 index)
            : _collection(collection)
            , _index(index)
        {
        }

        Iterator(Dictionary const& collection, const int32 index)
            : _collection((Dictionary&)collection)
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
        /// Checks if iterator is in the end of the collection.
        /// </summary>
        FORCE_INLINE bool IsEnd() const
        {
            return _index == _collection._tableSize;
        }

        /// <summary>
        /// Checks if iterator is not in the end of the collection.
        /// </summary>
        FORCE_INLINE bool IsNotEnd() const
        {
            return _index != _collection._tableSize;
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

        FORCE_INLINE bool operator!() const
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

        Iterator operator++(int) const
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

        Iterator operator--(int) const
        {
            Iterator i = *this;
            --i;
            return i;
        }
    };

public:

    /// <summary>
    /// Gets element by the key (will add default ValueType element if key not found).
    /// </summary>
    /// <param name="key">The key of the element.</param>
    /// <returns>The value that is at given index.</returns>
    template<typename KeyComparableType>
    ValueType& At(const KeyComparableType& key)
    {
        // Ensure to have enough memory for the next item (in case of new element insertion)
        EnsureCapacity(_elementsCount + _deletedCount + 1);

        // Find location of the item or place to insert it
        FindPositionResult pos;
        FindPosition(key, pos);

        // Check if that key has been already added
        if (pos.ObjectIndex != -1)
        {
            return _table[pos.ObjectIndex].Value;
        }

        // Insert
        ASSERT(pos.FreeSlotIndex != -1);
        auto bucket = &_table[pos.FreeSlotIndex];
        bucket->Occupy(key);
        _elementsCount++;
        return bucket->Value;
    }

    /// <summary>
    /// Gets the element by the key.
    /// </summary>
    /// <param name="key">The ky of the element.</param>
    /// <returns>The value that is at given index.</returns>
    template<typename KeyComparableType>
    const ValueType& At(const KeyComparableType& key) const
    {
        FindPositionResult pos;
        FindPosition(key, pos);

        ASSERT(pos.ObjectIndex != -1);
        return _table[pos.ObjectIndex].Value;
    }

    /// <summary>
    /// Gets or sets the element by the key.
    /// </summary>
    /// <param name="key">The key of the element.</param>
    /// <returns>The value that is at given index.</returns>
    template<typename KeyComparableType>
    FORCE_INLINE ValueType& operator[](const KeyComparableType& key)
    {
        return At(key);
    }

    /// <summary>
    /// Gets or sets the element by the key.
    /// </summary>
    /// <param name="key">The ky of the element.</param>
    /// <returns>The value that is at given index.</returns>
    template<typename KeyComparableType>
    FORCE_INLINE const ValueType& operator[](const KeyComparableType& key) const
    {
        return At(key);
    }

    /// <summary>
    /// Tries to get element with given key.
    /// </summary>
    /// <param name="key">The key of the element.</param>
    /// <param name="result">The result value.</param>
    /// <returns>True if element of given key has been found, otherwise false.</returns>
    template<typename KeyComparableType>
    bool TryGet(const KeyComparableType& key, ValueType& result) const
    {
        if (IsEmpty())
            return false;

        FindPositionResult pos;
        FindPosition(key, pos);

        if (pos.ObjectIndex == -1)
            return false;
        result = _table[pos.ObjectIndex].Value;
        return true;
    }

    /// <summary>
    /// Tries to get pointer to the element with given key.
    /// </summary>
    /// <param name="key">The ky of the element.</param>
    /// <returns>Pointer to the element value or null if cannot find it.</returns>
    template<typename KeyComparableType>
    ValueType* TryGet(const KeyComparableType& key) const
    {
        if (IsEmpty())
            return nullptr;

        FindPositionResult pos;
        FindPosition(key, pos);

        if (pos.ObjectIndex == -1)
            return nullptr;
        return &_table[pos.ObjectIndex].Value;
    }

public:

    /// <summary>
    /// Clears the collection but without changing its capacity (all inserted elements: keys and values will be removed).
    /// </summary>
    void Clear()
    {
        if (_table)
        {
            // Free all buckets
            for (int32 i = 0; i < _tableSize; i++)
                _table[i].Free();
            _elementsCount = _deletedCount = 0;
        }
    }

    /// <summary>
    /// Clears the collection and delete value objects.
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
    /// Changes the capacity of the collection.
    /// </summary>
    /// <param name="capacity">The new capacity.</param>
    /// <param name="preserveContents">Enables preserving collection contents during resizing.</param>
    void SetCapacity(int32 capacity, bool preserveContents = true)
    {
        // Check if capacity won't change
        if (capacity == Capacity())
            return;

        // Cache previous state
        ASSERT(capacity >= 0);
        Bucket* oldTable = _table;
        const int32 oldTableSize = _tableSize;

        // Clear elements counters
        const int32 oldElementsCount = _elementsCount;
        _deletedCount = _elementsCount = 0;

        // Check if need to create a new table
        if (capacity > 0)
        {
            // Align capacity value to the next power of two (if it's not)
            if ((capacity & (capacity - 1)) != 0)
            {
                capacity++;
                capacity |= capacity >> 1;
                capacity |= capacity >> 2;
                capacity |= capacity >> 4;
                capacity |= capacity >> 8;
                capacity |= capacity >> 16;
                capacity = capacity + 1;
            }

            // Allocate new table
            _table = (Bucket*)Allocator::Allocate(capacity * sizeof(Bucket));
            _tableSize = capacity;
            for (int32 i = 0; i < capacity; i++)
                _table[i]._state = Bucket::Empty;
            if (oldElementsCount != 0 && preserveContents)
            {
                // Try to preserve all pairs in the collection
                for (int32 i = 0; i < oldTableSize; i++)
                {
                    if (oldTable[i].IsOccupied())
                        Add(oldTable[i].Key, oldTable[i].Value);
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
            for (int32 i = 0; i < oldTableSize; i++)
                oldTable[i].Free();
            Allocator::Free(oldTable);
        }
    }

    /// <summary>
    /// Increases collection capacity by given extra size (content will be preserved).
    /// </summary>
    /// <param name="extraSize">The extra size to enlarge collection.</param>
    FORCE_INLINE void IncreaseCapacity(int32 extraSize)
    {
        ASSERT(extraSize >= 0);
        SetCapacity(Capacity() + extraSize);
    }

    /// <summary>
    /// Ensures that collection has given capacity.
    /// </summary>
    /// <param name="minCapacity">The minimum required capacity.</param>
    void EnsureCapacity(int32 minCapacity)
    {
        if (Capacity() >= minCapacity)
            return;

        int32 capacity = Capacity() == 0 ? DICTIONARY_DEFAULT_CAPACITY : Capacity() * 2;
        if (capacity < minCapacity)
            capacity = minCapacity;
        SetCapacity(capacity);
    }

    /// <summary>
    /// Cleanup collection data (changes size to 0 without data preserving).
    /// </summary>
    FORCE_INLINE void Cleanup()
    {
        SetCapacity(0, false);
    }

public:

    /// <summary>
    /// Add pair element to the collection.
    /// </summary>
    /// <param name="key">The key.</param>
    /// <param name="value">The value.</param>
    /// <returns>Weak reference to the stored bucket.</returns>
    template<typename KeyComparableType>
    Bucket* Add(const KeyComparableType& key, const ValueType& value)
    {
        // Ensure to have enough memory for the next item (in case of new element insertion)
        EnsureCapacity(_elementsCount + _deletedCount + 1);

        // Find location of the item or place to insert it
        FindPositionResult pos;
        FindPosition(key, pos);

        // Ensure key is unknown
        ASSERT(pos.ObjectIndex == -1 && "That key has been already added to the dictionary.");

        // Insert
        ASSERT(pos.FreeSlotIndex != -1);
        Bucket* bucket = &_table[pos.FreeSlotIndex];
        bucket->Occupy(key, value);
        _elementsCount++;

        return bucket;
    }

    /// <summary>
    /// Add pair element to the collection.
    /// </summary>
    /// <param name="key">The key.</param>
    /// <param name="value">The value.</param>
    /// <returns>Weak reference to the stored bucket.</returns>
    template<typename KeyComparableType>
    Bucket* Add(const KeyComparableType& key, ValueType&& value)
    {
        // Ensure to have enough memory for the next item (in case of new element insertion)
        EnsureCapacity(_elementsCount + _deletedCount + 1);

        // Find location of the item or place to insert it
        FindPositionResult pos;
        FindPosition(key, pos);

        // Ensure key is unknown
        ASSERT(pos.ObjectIndex == -1 && "That key has been already added to the dictionary.");

        // Insert
        ASSERT(pos.FreeSlotIndex != -1);
        Bucket* bucket = &_table[pos.FreeSlotIndex];
        bucket->Occupy(key, MoveTemp(value));
        _elementsCount++;

        return bucket;
    }

    /// <summary>
    /// Add pair element to the collection.
    /// </summary>
    /// <param name="i">Iterator with key and value.</param>
    void Add(const Iterator& i)
    {
        ASSERT(&i._collection != this && i);
        Bucket& bucket = *i;
        Add(bucket.Key, bucket.Value);
    }

    /// <summary>
    /// Removes element with a specified key.
    /// </summary>
    /// <param name="key">The element key to remove.</param>
    /// <returns>True if cannot remove item from the collection because cannot find it, otherwise false.</returns>
    template<typename KeyComparableType>
    bool Remove(const KeyComparableType& key)
    {
        if (IsEmpty())
            return true;

        FindPositionResult pos;
        FindPosition(key, pos);

        if (pos.ObjectIndex != -1)
        {
            _table[pos.ObjectIndex].Delete();
            _elementsCount--;
            _deletedCount++;
            return true;
        }
        return false;
    }

    /// <summary>
    /// Removes element at specified iterator.
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

    /// <summary>
    /// Removes elements with a specified value
    /// </summary>
    /// <param name="value">Element value to remove</param>
    /// <returns>The amount of removed items. Zero if nothing changed.</returns>
    int32 RemoveValue(const ValueType& value)
    {
        int32 result = 0;
        for (auto i = Begin(); i.IsNotEnd(); ++i)
        {
            if (i->Value == value)
            {
                Remove(i);
                result++;
            }
        }
        return result;
    }

public:

    /// <summary>
    /// Finds the element with given key in the collection.
    /// </summary>
    /// <param name="key">The key to find.</param>
    /// <returns>The iterator for the found element or End if cannot find it.</returns>
    template<typename KeyComparableType>
    Iterator Find(const KeyComparableType& key) const
    {
        if (HasItems())
        {
            for (int32 i = 0; i < _tableSize; i++)
            {
                if (_table[i].IsOccupied() && _table[i].Key == key)
                    return Iterator(*this, i);
            }
        }
        return End();
    }

    /// <summary>
    /// Checks if given key is in a collection.
    /// </summary>
    /// <param name="key">The key to find.</param>
    /// <returns>True if key has been found in a collection, otherwise false.</returns>
    template<typename KeyComparableType>
    bool ContainsKey(const KeyComparableType& key) const
    {
        if (IsEmpty())
            return false;

        FindPositionResult pos;
        FindPosition(key, pos);
        return pos.ObjectIndex != -1;
    }

    /// <summary>
    /// Checks if given value is in a collection.
    /// </summary>
    /// <param name="value">The value to find.</param>
    /// <returns>True if value has been found in a collection, otherwise false.</returns>
    bool ContainsValue(const ValueType& value) const
    {
        if (HasItems())
        {
            for (int32 i = 0; i < _tableSize; i++)
            {
                if (_table[i].IsOccupied() && _table[i].Value == value)
                    return true;
            }
        }
        return false;
    }

    /// <summary>
    /// Searches for the specified object and returns the zero-based index of the first occurrence within the entire dictionary.
    /// </summary>
    /// <param name="value">The value of the key to find.</param>
    /// <param name="key">The output key.</param>
    /// <returns>True if value has been found, otherwise false.</returns>
    bool KeyOf(const ValueType& value, KeyType* key) const
    {
        if (HasItems())
        {
            for (int32 i = 0; i < _tableSize; i++)
            {
                if (_table[i].IsOccupied() && _table[i].Value == value)
                {
                    if (key)
                        *key = _table[i].Key;
                    return true;
                }
            }
        }

        return false;
    }

public:

    /// <summary>
    /// Clones other collection into this.
    /// </summary>
    /// <param name="other">The other collection to clone.</param>
    void Clone(const Dictionary& other)
    {
        Clear();
        SetCapacity(other.Capacity(), false);
        for (auto i = other.Begin(); i != other.End(); ++i)
            Add(i);
        ASSERT(Count() == other.Count());
        ASSERT(Capacity() == other.Capacity());
    }

    /// <summary>
    /// Gets the keys collection to the output array (will contain unique items).
    /// </summary>
    /// <param name="result">The result.</param>
    void GetKeys(Array<KeyType>& result) const
    {
        for (auto i = Begin(); i.IsNotEnd(); ++i)
            result.Add(i->Key);
    }

    /// <summary>
    /// Gets the values collection to the output array (may contain duplicates).
    /// </summary>
    /// <param name="result">The result.</param>
    void GetValues(Array<ValueType>& result) const
    {
        for (auto i = Begin(); i.IsNotEnd(); ++i)
            result.Add(i->Value);
    }

public:

    /// <summary>
    /// Gets iterator for beginning of the collection.
    /// </summary>
    /// <returns>Iterator for beginning of the collection.</returns>
    Iterator Begin() const
    {
        Iterator i(*this, -1);
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

    /// <summary>
    /// The result container of the dictionary item lookup searching.
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
    /// first deleted bucket we see, as long as we don't find the key later
    /// </summary>
    /// <param name="key">The ky to find.</param>
    /// <param name="result">The pair of values: where the object is and where it would go if you wanted to insert it.</param>
    template<typename KeyComparableType>
    void FindPosition(const KeyComparableType& key, FindPositionResult& result) const
    {
        ASSERT(_table);
        const int32 tableSizeMinusOne = _tableSize - 1;
        int32 bucketIndex = GetHash(key) & tableSizeMinusOne;
        int32 insertPos = -1;
        int32 checksCount = 0;
        result.FreeSlotIndex = -1;

        while (checksCount < _tableSize)
        {
            // Empty bucket
            if (_table[bucketIndex].IsEmpty())
            {
                // Found place to insert
                result.ObjectIndex = -1;
                result.FreeSlotIndex = insertPos == -1 ? bucketIndex : insertPos;
                return;
            }
            // Deleted bucket
            if (_table[bucketIndex].IsDeleted())
            {
                // Keep searching but mark to insert
                if (insertPos == -1)
                    insertPos = bucketIndex;
            }
                // Occupied bucket by target key
            else if (_table[bucketIndex].Key == key)
            {
                // Found key
                result.ObjectIndex = bucketIndex;
                return;
            }

            checksCount++;
            bucketIndex = (bucketIndex + DICTIONARY_PROB_FUNC(_tableSize, checksCount)) & tableSizeMinusOne;
        }

        result.ObjectIndex = -1;
        result.FreeSlotIndex = insertPos;
    }
};
