// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Memory/Memory.h"
#include "Engine/Core/Memory/Allocation.h"
#include "Engine/Core/Collections/HashFunctions.h"
#include "Engine/Core/Collections/Config.h"

/// <summary>
/// Template for unordered dictionary with mapped key with value pairs.
/// </summary>
template<typename KeyType, typename ValueType, typename AllocationType = HeapAllocation>
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

    typedef typename AllocationType::template Data<Bucket> AllocationData;

private:

    int32 _elementsCount = 0;
    int32 _deletedCount = 0;
    int32 _size = 0;
    AllocationData _allocation;

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
    /// <param name="other">The other collection to move.</param>
    Dictionary(Dictionary&& other) noexcept
        : _elementsCount(other._elementsCount)
        , _deletedCount(other._deletedCount)
        , _size(other._size)
    {
        _elementsCount = other._elementsCount;
        _deletedCount = other._deletedCount;
        _size = other._size;
        other._elementsCount = 0;
        other._deletedCount = 0;
        other._size = 0;
        _allocation.Swap(other._allocation);
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
        if (this != &other)
            Clone(other);
        return *this;
    }

    /// <summary>
    /// Moves the data from the other collection.
    /// </summary>
    /// <param name="other">The other collection to move.</param>
    /// <returns>The reference to this.</returns>
    Dictionary& operator=(Dictionary&& other) noexcept
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
            _allocation.Swap(other._allocation);
        }
        return *this;
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="Dictionary"/> class.
    /// </summary>
    ~Dictionary()
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

        Iterator(Iterator&& i)
            : _collection(i._collection)
            , _index(i._index)
        {
        }

    public:

        FORCE_INLINE bool IsEnd() const
        {
            return _index == _collection._size;
        }

        FORCE_INLINE bool IsNotEnd() const
        {
            return _index != _collection._size;
        }

        FORCE_INLINE Bucket& operator*() const
        {
            return _collection._allocation.Get()[_index];
        }

        FORCE_INLINE Bucket* operator->() const
        {
            return &_collection._allocation.Get()[_index];
        }

        FORCE_INLINE explicit operator bool() const
        {
            return _index >= 0 && _index < _collection._size;
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

        Iterator& operator++()
        {
            const int32 capacity = _collection.Capacity();
            if (_index != capacity)
            {
                const Bucket* data = _collection._allocation.Get();
                do
                {
                    _index++;
                } while (_index != capacity && data[_index].IsNotOccupied());
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
                const Bucket* data = _collection._allocation.Get();
                do
                {
                    _index--;
                } while (_index > 0 && data[_index].IsNotOccupied());
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
            return _allocation.Get()[pos.ObjectIndex].Value;

        // Insert
        ASSERT(pos.FreeSlotIndex != -1);
        auto bucket = &_allocation.Get()[pos.FreeSlotIndex];
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
        return _allocation.Get()[pos.ObjectIndex].Value;
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
        result = _allocation.Get()[pos.ObjectIndex].Value;
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
        return (ValueType*)&_allocation.Get()[pos.ObjectIndex].Value;
    }

public:

    /// <summary>
    /// Clears the collection but without changing its capacity (all inserted elements: keys and values will be removed).
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
    template<typename = typename TEnableIf<TIsPointer<ValueType>::Value>::Type>
#endif
    void ClearDelete()
    {
        for (auto i = Begin(); i.IsNotEnd(); ++i)
        {
            if (i->Value)
                Delete(i->Value);
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
        if (capacity == Capacity())
            return;
        ASSERT(capacity >= 0);
        AllocationData oldAllocation;
        oldAllocation.Swap(_allocation);
        const int32 oldSize = _size;
        const int32 oldElementsCount = _elementsCount;
        _deletedCount = _elementsCount = 0;
        if (capacity != 0 && (capacity & (capacity - 1)) != 0)
        {
            // Align capacity value to the next power of two (if it's not)
            capacity++;
            capacity |= capacity >> 1;
            capacity |= capacity >> 2;
            capacity |= capacity >> 4;
            capacity |= capacity >> 8;
            capacity |= capacity >> 16;
            capacity = capacity + 1;
        }
        if (capacity)
        {
            _allocation.Allocate(capacity);
            Bucket* data = _allocation.Get();
            for (int32 i = 0; i < capacity; i++)
                data[i]._state = Bucket::Empty;
        }
        _size = capacity;
        Bucket* oldData = oldAllocation.Get();
        if (oldElementsCount != 0 && preserveContents)
        {
            // TODO; move keys and values on realloc
            for (int32 i = 0; i < oldSize; i++)
            {
                if (oldData[i].IsOccupied())
                    Add(oldData[i].Key, oldData[i].Value);
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
    void EnsureCapacity(int32 minCapacity)
    {
        if (_size >= minCapacity)
            return;
        if (minCapacity < DICTIONARY_DEFAULT_CAPACITY)
            minCapacity = DICTIONARY_DEFAULT_CAPACITY;
        const int32 capacity = _allocation.CalculateCapacityGrow(_size, minCapacity);
        SetCapacity(capacity);
    }

    /// <summary>
    /// Cleanup collection data (changes size to 0 without data preserving).
    /// </summary>
    FORCE_INLINE void Cleanup()
    {
        SetCapacity(0, false);
    }

    /// <summary>
    /// Swaps the contents of collection with the other object without copy operation. Performs fast internal data exchange.
    /// </summary>
    /// <param name="other">The other collection.</param>
    void Swap(Dictionary& other)
    {
        ::Swap(_elementsCount, other._elementsCount);
        ::Swap(_deletedCount, other._deletedCount);
        ::Swap(_size, other._size);
        _allocation.Swap(other._allocation);
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
        Bucket* bucket = &_allocation.Get()[pos.FreeSlotIndex];
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
        Bucket* bucket = &_allocation.Get()[pos.FreeSlotIndex];
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
            _allocation.Get()[pos.ObjectIndex].Delete();
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
            ASSERT(_allocation.Get()[i._index].IsOccupied());
            _allocation.Get()[i._index].Delete();
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
            const Bucket* data = _allocation.Get();
            for (int32 i = 0; i < _size; i++)
            {
                if (data[i].IsOccupied() && data[i].Key == key)
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
            const Bucket* data = _allocation.Get();
            for (int32 i = 0; i < _size; i++)
            {
                if (data[i].IsOccupied() && data[i].Value == value)
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
            const Bucket* data = _allocation.Get();
            for (int32 i = 0; i < _size; i++)
            {
                if (data[i].IsOccupied() && data[i].Value == value)
                {
                    if (key)
                        *key = data[i].Key;
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
    template<typename ArrayAllocation>
    void GetKeys(Array<KeyType, ArrayAllocation>& result) const
    {
        for (auto i = Begin(); i.IsNotEnd(); ++i)
            result.Add(i->Key);
    }

    /// <summary>
    /// Gets the values collection to the output array (may contain duplicates).
    /// </summary>
    /// <param name="result">The result.</param>
    template<typename ArrayAllocation>
    void GetValues(Array<ValueType, ArrayAllocation>& result) const
    {
        for (auto i = Begin(); i.IsNotEnd(); ++i)
            result.Add(i->Value);
    }

public:

    Iterator Begin() const
    {
        Iterator i(*this, -1);
        ++i;
        return i;
    }

    Iterator End() const
    {
        return Iterator(*this, _size);
    }

    Iterator begin()
    {
        Iterator i(*this, -1);
        ++i;
        return i;
    }

    FORCE_INLINE Iterator end()
    {
        return Iterator(*this, _size);
    }

    const Iterator begin() const
    {
        Iterator i(*this, -1);
        ++i;
        return i;
    }

    FORCE_INLINE const Iterator end() const
    {
        return Iterator(*this, _size);
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
        ASSERT(_size);
        const int32 tableSizeMinusOne = _size - 1;
        int32 bucketIndex = GetHash(key) & tableSizeMinusOne;
        int32 insertPos = -1;
        int32 checksCount = 0;
        const Bucket* data = _allocation.Get();
        result.FreeSlotIndex = -1;
        while (checksCount < _size)
        {
            // Empty bucket
            auto& bucket = data[bucketIndex];
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
                // Occupied bucket by target key
            else if (bucket.Key == key)
            {
                // Found key
                result.ObjectIndex = bucketIndex;
                return;
            }
            checksCount++;
            bucketIndex = (bucketIndex + DICTIONARY_PROB_FUNC(_size, checksCount)) & tableSizeMinusOne;
        }
        result.ObjectIndex = -1;
        result.FreeSlotIndex = insertPos;
    }
};
