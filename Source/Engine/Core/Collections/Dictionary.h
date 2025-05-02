// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "HashSetBase.h"

/// <summary>
/// Describes single portion of space for the key and value pair in a hash map.
/// </summary>
template<typename KeyType, typename ValueType, typename AllocationType>
struct DictionaryBucket
{
    friend Memory;
    friend HashSetBase<AllocationType, DictionaryBucket>;
    friend Dictionary<KeyType, ValueType, AllocationType>;

    /// <summary>The key.</summary>
    KeyType Key;
    /// <summary>The value.</summary>
    ValueType Value;

private:
    HashSetBucketState _state;

    DictionaryBucket()
        : _state(HashSetBucketState::Empty)
    {
    }

    DictionaryBucket(DictionaryBucket&& other) noexcept
    {
        _state = other._state;
        if (other._state == HashSetBucketState::Occupied)
        {
            Memory::MoveItems(&Key, &other.Key, 1);
            Memory::MoveItems(&Value, &other.Value, 1);
            other._state = HashSetBucketState::Empty;
        }
    }

    DictionaryBucket& operator=(DictionaryBucket&& other) noexcept
    {
        if (this != &other)
        {
            if (_state == HashSetBucketState::Occupied)
            {
                Memory::DestructItem(&Key);
                Memory::DestructItem(&Value);
            }
            _state = other._state;
            if (other._state == HashSetBucketState::Occupied)
            {
                Memory::MoveItems(&Key, &other.Key, 1);
                Memory::MoveItems(&Value, &other.Value, 1);
                other._state = HashSetBucketState::Empty;
            }
        }
        return *this;
    }

    /// <summary>Copying a bucket is useless, because a key must be unique in the dictionary.</summary>
    DictionaryBucket(const DictionaryBucket&) = delete;

    /// <summary>Copying a bucket is useless, because a key must be unique in the dictionary.</summary>
    DictionaryBucket& operator=(const DictionaryBucket&) = delete;

    ~DictionaryBucket()
    {
        if (_state == HashSetBucketState::Occupied)
        {
            Memory::DestructItem(&Key);
            Memory::DestructItem(&Value);
        }
    }

    FORCE_INLINE void Free()
    {
        if (_state == HashSetBucketState::Occupied)
        {
            Memory::DestructItem(&Key);
            Memory::DestructItem(&Value);
        }
        _state = HashSetBucketState::Empty;
    }

    FORCE_INLINE void Delete()
    {
        ASSERT(IsOccupied());
        _state = HashSetBucketState::Deleted;
        Memory::DestructItem(&Key);
        Memory::DestructItem(&Value);
    }

    template<typename KeyComparableType>
    FORCE_INLINE void Occupy(const KeyComparableType& key)
    {
        Memory::ConstructItems(&Key, &key, 1);
        Memory::ConstructItem(&Value);
        _state = HashSetBucketState::Occupied;
    }

    template<typename KeyComparableType>
    FORCE_INLINE void Occupy(const KeyComparableType& key, const ValueType& value)
    {
        Memory::ConstructItems(&Key, &key, 1);
        Memory::ConstructItems(&Value, &value, 1);
        _state = HashSetBucketState::Occupied;
    }

    template<typename KeyComparableType>
    FORCE_INLINE void Occupy(const KeyComparableType& key, ValueType&& value)
    {
        Memory::ConstructItems(&Key, &key, 1);
        Memory::MoveItems(&Value, &value, 1);
        _state = HashSetBucketState::Occupied;
    }

    FORCE_INLINE bool IsEmpty() const
    {
        return _state == HashSetBucketState::Empty;
    }

    FORCE_INLINE bool IsDeleted() const
    {
        return _state == HashSetBucketState::Deleted;
    }

    FORCE_INLINE bool IsOccupied() const
    {
        return _state == HashSetBucketState::Occupied;
    }

    FORCE_INLINE bool IsNotOccupied() const
    {
        return _state != HashSetBucketState::Occupied;
    }

    FORCE_INLINE const KeyType& GetKey() const
    {
        return Key;
    }
};

/// <summary>
/// Template for unordered dictionary with mapped key with value pairs.
/// </summary>
/// <typeparam name="KeyType">The type of the keys in the dictionary.</typeparam>
/// <typeparam name="ValueType">The type of the values in the dictionary.</typeparam>
/// <typeparam name="AllocationType">The type of memory allocator.</typeparam>
template<typename KeyType, typename ValueType, typename AllocationType = HeapAllocation>
API_CLASS(InBuild) class Dictionary : public HashSetBase<AllocationType, DictionaryBucket<KeyType, ValueType, AllocationType>>
{
    friend Dictionary;
public:
    typedef DictionaryBucket<KeyType, ValueType, AllocationType> Bucket;
    typedef HashSetBase<AllocationType, Bucket> Base;

public:
    /// <summary>
    /// Initializes an empty <see cref="Dictionary"/> without reserving any space.
    /// </summary>
    Dictionary()
    {
    }

    /// <summary>
    /// Initializes <see cref="Dictionary"/> by reserving space.
    /// </summary>
    /// <param name="capacity">The number of elements that can be added without a need to allocate more memory.</param>
    FORCE_INLINE explicit Dictionary(const int32 capacity)
    {
        Base::SetCapacity(capacity);
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Dictionary"/> class.
    /// </summary>
    /// <param name="other">The other collection to move.</param>
    Dictionary(Dictionary&& other) noexcept
    {
        Base::MoveToEmpty(MoveTemp(other));
    }

    /// <summary>
    /// Initializes <see cref="Dictionary"/> by copying the elements from the other collection.
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
            Base::Clear();
            Base::_allocation.Free();
            Base::MoveToEmpty(MoveTemp(other));
        }
        return *this;
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="Dictionary"/> class.
    /// </summary>
    ~Dictionary()
    {
    }

public:
    /// <summary>
    /// The read-only dictionary collection iterator.
    /// </summary>
    struct ConstIterator : Base::IteratorBase
    {
        friend Dictionary;
    public:
        ConstIterator(const Dictionary* collection, const int32 index)
            : Base::IteratorBase(collection, index)
        {
        }

        ConstIterator()
            : Base::IteratorBase(nullptr, -1)
        {
        }

        ConstIterator(const ConstIterator& i)
            : Base::IteratorBase(i._collection, i._index)
        {
        }

        ConstIterator(ConstIterator&& i) noexcept
            : Base::IteratorBase(i._collection, i._index)
        {
        }

    public:
        FORCE_INLINE bool operator!() const
        {
            return !(bool)*this;
        }

        FORCE_INLINE bool operator==(const ConstIterator& v) const
        {
            return this->_index == v._index && this->_collection == v._collection;
        }

        FORCE_INLINE bool operator!=(const ConstIterator& v) const
        {
            return this->_index != v._index || this->_collection != v._collection;
        }

        ConstIterator& operator=(const ConstIterator& v)
        {
            this->_collection = v._collection;
            this->_index = v._index;
            return *this;
        }

        ConstIterator& operator=(ConstIterator&& v) noexcept
        {
            this->_collection = v._collection;
            this->_index = v._index;
            return *this;
        }

        ConstIterator& operator++()
        {
            this->Next();
            return *this;
        }

        ConstIterator operator++(int) const
        {
            ConstIterator i = *this;
            i.Next();
            return i;
        }

        ConstIterator& operator--()
        {
            this->Prev();
            return *this;
        }

        ConstIterator operator--(int) const
        {
            ConstIterator i = *this;
            i.Prev();
            return i;
        }
    };

    /// <summary>
    /// The dictionary collection iterator.
    /// </summary>
    struct Iterator : Base::IteratorBase
    {
        friend Dictionary;
    public:
        Iterator(Dictionary* collection, const int32 index)
            : Base::IteratorBase(collection, index)
        {
        }

        Iterator()
            : Base::IteratorBase(nullptr, -1)
        {
        }

        Iterator(const Iterator& i)
            : Base::IteratorBase(i._collection, i._index)
        {
        }

        Iterator(Iterator&& i) noexcept
            : Base::IteratorBase(i._collection, i._index)
        {
        }

    public:
        FORCE_INLINE Bucket& operator*() const
        {
            return ((Dictionary*)this->_collection)->_allocation.Get()[this->_index];
        }

        FORCE_INLINE Bucket* operator->() const
        {
            return &((Dictionary*)this->_collection)->_allocation.Get()[this->_index];
        }

        FORCE_INLINE bool operator!() const
        {
            return !(bool)*this;
        }

        FORCE_INLINE bool operator==(const Iterator& v) const
        {
            return this->_index == v._index && this->_collection == v._collection;
        }

        FORCE_INLINE bool operator!=(const Iterator& v) const
        {
            return this->_index != v._index || this->_collection != v._collection;
        }

        Iterator& operator=(const Iterator& v)
        {
            this->_collection = v._collection;
            this->_index = v._index;
            return *this;
        }

        Iterator& operator=(Iterator&& v) noexcept
        {
            this->_collection = v._collection;
            this->_index = v._index;
            return *this;
        }

        Iterator& operator++()
        {
            this->Next();
            return *this;
        }

        Iterator operator++(int) const
        {
            Iterator i = *this;
            i.Next();
            return i;
        }

        Iterator& operator--()
        {
            this->Prev();
            return *this;
        }

        Iterator operator--(int) const
        {
            Iterator i = *this;
            i.Prev();
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
        Bucket* bucket = Base::OnAdd(key, false);
        if (bucket->_state != HashSetBucketState::Occupied)
            bucket->Occupy(key);
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
        typename Base::FindPositionResult pos;
        Base::FindPosition(key, pos);
        ASSERT(pos.ObjectIndex != -1);
        return Base::_allocation.Get()[pos.ObjectIndex].Value;
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
        typename Base::FindPositionResult pos;
        Base::FindPosition(key, pos);
        if (pos.ObjectIndex == -1)
            return false;
        result = Base::_allocation.Get()[pos.ObjectIndex].Value;
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
        typename Base::FindPositionResult pos;
        Base::FindPosition(key, pos);
        if (pos.ObjectIndex == -1)
            return nullptr;
        return const_cast<ValueType*>(&Base::_allocation.Get()[pos.ObjectIndex].Value);
    }

public:
    /// <summary>
    /// Clears the collection and delete value objects.
    /// Note: collection must contain pointers to the objects that have public destructor and be allocated using New method.
    /// </summary>
#if defined(_MSC_VER)
    template<typename = typename TEnableIf<TIsPointer<ValueType>::Value>::Type>
#endif
    void ClearDelete()
    {
        for (Iterator i = Begin(); i.IsNotEnd(); ++i)
        {
            if (i->Value)
                ::Delete(i->Value);
        }
        Base::Clear();
    }

public:
    /// <summary>
    /// Add pair element to the collection.
    /// </summary>
    /// <param name="key">The key.</param>
    /// <param name="value">The value.</param>
    /// <returns>Weak reference to the stored bucket.</returns>
    template<typename KeyComparableType>
    FORCE_INLINE Bucket* Add(const KeyComparableType& key, const ValueType& value)
    {
        Bucket* bucket = Base::OnAdd(key);
        bucket->Occupy(key, value);
        return bucket;
    }

    /// <summary>
    /// Add pair element to the collection.
    /// </summary>
    /// <param name="key">The key.</param>
    /// <param name="value">The value.</param>
    /// <returns>Weak reference to the stored bucket.</returns>
    template<typename KeyComparableType>
    FORCE_INLINE Bucket* Add(const KeyComparableType& key, ValueType&& value)
    {
        Bucket* bucket = Base::OnAdd(key);
        bucket->Occupy(key, MoveTemp(value));
        return bucket;
    }

    /// <summary>
    /// Add pair element to the collection.
    /// </summary>
    /// <param name="i">Iterator with key and value.</param>
    DEPRECATED("Use Add with separate Key and Value from iterator.") void Add(const Iterator& i)
    {
        ASSERT(i._collection != this && i);
        const Bucket& bucket = *i;
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
        typename Base::FindPositionResult pos;
        Base::FindPosition(key, pos);
        if (pos.ObjectIndex != -1)
        {
            Base::_allocation.Get()[pos.ObjectIndex].Delete();
            --Base::_elementsCount;
            ++Base::_deletedCount;
            return true;
        }
        return false;
    }

    /// <summary>
    /// Removes element at specified iterator.
    /// </summary>
    /// <param name="i">The element iterator to remove.</param>
    /// <returns>True if cannot remove item from the collection because cannot find it, otherwise false.</returns>
    bool Remove(const Iterator& i)
    {
        ASSERT(i._collection == this);
        if (i)
        {
            Base::_allocation.Get()[i._index].Delete();
            --Base::_elementsCount;
            ++Base::_deletedCount;
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
        for (Iterator i = Begin(); i.IsNotEnd(); ++i)
        {
            if (i->Value == value)
            {
                Remove(i);
                ++result;
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
    Iterator Find(const KeyComparableType& key)
    {
        if (Base::IsEmpty())
            return End();
        typename Base::FindPositionResult pos;
        Base::FindPosition(key, pos);
        return pos.ObjectIndex != -1 ? Iterator(this, pos.ObjectIndex) : End();
    }

    /// <summary>
    /// Finds the element with given key in the collection.
    /// </summary>
    /// <param name="key">The key to find.</param>
    /// <returns>The iterator for the found element or End if cannot find it.</returns>
    template<typename KeyComparableType>
    ConstIterator Find(const KeyComparableType& key) const
    {
        typename Base::FindPositionResult pos;
        Base::FindPosition(key, pos);
        return pos.ObjectIndex != -1 ? ConstIterator(this, pos.ObjectIndex) : End();
    }

    /// <summary>
    /// Checks if given key is in a collection.
    /// </summary>
    /// <param name="key">The key to find.</param>
    /// <returns>True if key has been found in a collection, otherwise false.</returns>
    template<typename KeyComparableType>
    bool ContainsKey(const KeyComparableType& key) const
    {
        typename Base::FindPositionResult pos;
        Base::FindPosition(key, pos);
        return pos.ObjectIndex != -1;
    }

    /// <summary>
    /// Checks if given value is in a collection.
    /// </summary>
    /// <param name="value">The value to find.</param>
    /// <returns>True if value has been found in a collection, otherwise false.</returns>
    bool ContainsValue(const ValueType& value) const
    {
        if (Base::HasItems())
        {
            const Bucket* data = Base::_allocation.Get();
            for (int32 i = 0; i < Base::_size; ++i)
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
        if (Base::HasItems())
        {
            const Bucket* data = Base::_allocation.Get();
            for (int32 i = 0; i < Base::_size; ++i)
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
        // TODO: if both key and value are POD types then use raw memory copy for buckets
        Base::Clear();
        Base::SetCapacity(other.Capacity(), false);
        for (ConstIterator i = other.Begin(); i != other.End(); ++i)
        {
            const Bucket& bucket = *i;
            Add(bucket.Key, bucket.Value);
        }
        ASSERT(Base::Count() == other.Count());
        ASSERT(Base::Capacity() == other.Capacity());
    }

    /// <summary>
    /// Gets the keys collection to the output array (will contain unique items).
    /// </summary>
    /// <param name="result">The result.</param>
    template<typename ArrayAllocation>
    void GetKeys(Array<KeyType, ArrayAllocation>& result) const
    {
        for (ConstIterator i = Begin(); i.IsNotEnd(); ++i)
            result.Add(i->Key);
    }

    /// <summary>
    /// Gets the values collection to the output array (may contain duplicates).
    /// </summary>
    /// <param name="result">The result.</param>
    template<typename ArrayAllocation>
    void GetValues(Array<ValueType, ArrayAllocation>& result) const
    {
        for (ConstIterator i = Begin(); i.IsNotEnd(); ++i)
            result.Add(i->Value);
    }

public:
    Iterator Begin()
    {
        Iterator i(this, -1);
        ++i;
        return i;
    }

    Iterator End()
    {
        return Iterator(this, Base::_size);
    }

    ConstIterator Begin() const
    {
        ConstIterator i(this, -1);
        ++i;
        return i;
    }

    ConstIterator End() const
    {
        return ConstIterator(this, Base::_size);
    }

    Iterator begin()
    {
        Iterator i(this, -1);
        ++i;
        return i;
    }

    FORCE_INLINE Iterator end()
    {
        return Iterator(this, Base::_size);
    }

    ConstIterator begin() const
    {
        ConstIterator i(this, -1);
        ++i;
        return i;
    }

    FORCE_INLINE ConstIterator end() const
    {
        return ConstIterator(this, Base::_size);
    }
};
