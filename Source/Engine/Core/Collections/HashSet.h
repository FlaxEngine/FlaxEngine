// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "HashSetBase.h"

/// <summary>
/// Describes single portion of space for the item in a hash set.
/// </summary>
template<typename T, typename AllocationType>
struct HashSetBucket
{
    friend Memory;
    friend HashSetBase<AllocationType, HashSetBucket>;
    friend HashSet<T, AllocationType>;

    /// <summary>The item.</summary>
    T Item;

private:
    HashSetBucketState _state;

    HashSetBucket()
        : _state(HashSetBucketState::Empty)
    {
    }

    HashSetBucket(HashSetBucket&& other) noexcept
    {
        _state = other._state;
        if (other._state == HashSetBucketState::Occupied)
        {
            Memory::MoveItems(&Item, &other.Item, 1);
            other._state = HashSetBucketState::Empty;
        }
    }

    HashSetBucket& operator=(HashSetBucket&& other) noexcept
    {
        if (this != &other)
        {
            if (_state == HashSetBucketState::Occupied)
            {
                Memory::DestructItem(&Item);
            }
            _state = other._state;
            if (other._state == HashSetBucketState::Occupied)
            {
                Memory::MoveItems(&Item, &other.Item, 1);
                other._state = HashSetBucketState::Empty;
            }
        }
        return *this;
    }

    /// <summary>Copying a bucket is useless, because a key must be unique in the dictionary.</summary>
    HashSetBucket(const HashSetBucket&) = delete;

    /// <summary>Copying a bucket is useless, because a key must be unique in the dictionary.</summary>
    HashSetBucket& operator=(const HashSetBucket&) = delete;

    ~HashSetBucket()
    {
        if (_state == HashSetBucketState::Occupied)
            Memory::DestructItem(&Item);
    }

    FORCE_INLINE void Free()
    {
        if (_state == HashSetBucketState::Occupied)
            Memory::DestructItem(&Item);
        _state = HashSetBucketState::Empty;
    }

    FORCE_INLINE void Delete()
    {
        ASSERT(IsOccupied());
        _state = HashSetBucketState::Deleted;
        Memory::DestructItem(&Item);
    }

    template<typename ItemType>
    FORCE_INLINE void Occupy(const ItemType& item)
    {
        Memory::ConstructItems(&Item, &item, 1);
        _state = HashSetBucketState::Occupied;
    }

    template<typename ItemType>
    FORCE_INLINE void Occupy(ItemType&& item)
    {
        Memory::MoveItems(&Item, &item, 1);
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

    FORCE_INLINE const T& GetKey() const
    {
        return Item;
    }
};

/// <summary>
/// Template for unordered set of values (without duplicates with O(1) lookup access).
/// </summary>
/// <typeparam name="T">The type of elements in the set.</typeparam>
/// <typeparam name="AllocationType">The type of memory allocator.</typeparam>
template<typename T, typename AllocationType = HeapAllocation>
API_CLASS(InBuild) class HashSet : public HashSetBase<AllocationType, HashSetBucket<T, AllocationType>>
{
    friend HashSet;
public:
    typedef HashSetBucket<T, AllocationType> Bucket;
    typedef HashSetBase<AllocationType, Bucket> Base;

public:
    /// <summary>
    /// Initializes an empty <see cref="HashSet"/> without reserving any space.
    /// </summary>
    HashSet()
    {
    }

    /// <summary>
    /// Initializes <see cref="HashSet"/> by reserving space.
    /// </summary>
    /// <param name="capacity">The number of elements that can be added without a need to allocate more memory.</param>
    FORCE_INLINE explicit HashSet(const int32 capacity)
    {
        Base::SetCapacity(capacity);
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="HashSet"/> class.
    /// </summary>
    /// <param name="other">The other collection to move.</param>
    HashSet(HashSet&& other) noexcept
    {
        Base::MoveToEmpty(MoveTemp(other));
    }

    /// <summary>
    /// Initializes <see cref="HashSet"/> by copying the elements from the other collection.
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
            Base::Clear();
            Base::_allocation.Free();
            Base::MoveToEmpty(MoveTemp(other));
        }
        return *this;
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="HashSet"/> class.
    /// </summary>
    ~HashSet()
    {
    }

public:
    /// <summary>
    /// The read-only hash set collection iterator.
    /// </summary>
    struct ConstIterator : Base::IteratorBase
    {
        friend HashSet;
    public:
        ConstIterator(const HashSet* collection, const int32 index)
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
    /// The hash set collection iterator.
    /// </summary>
    struct Iterator : Base::IteratorBase
    {
        friend HashSet;
    public:
        Iterator(HashSet* collection, const int32 index)
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
            return ((HashSet*)this->_collection)->_allocation.Get()[this->_index];
        }

        FORCE_INLINE Bucket* operator->() const
        {
            return &((HashSet*)this->_collection)->_allocation.Get()[this->_index];
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

        Iterator operator--(int)
        {
            Iterator i = *this;
            i.Prev();
            return i;
        }
    };
    
public:
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
        Base::Clear();
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
        Bucket* bucket = Base::OnAdd(item, false);
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
        Bucket* bucket = Base::OnAdd(item, false);
        if (bucket)
            bucket->Occupy(MoveTemp(item));
        return bucket != nullptr;
    }

    /// <summary>
    /// Add element at iterator to the collection
    /// </summary>
    /// <param name="i">Iterator with item to add</param>
    DEPRECATED("Use Add with separate Key and Value from iterator.") void Add(const Iterator& i)
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
        typename Base::FindPositionResult pos;
        Base::FindPosition(item, pos);
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
    /// Removes an element at specified iterator position.
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

public:
    /// <summary>
    /// Find element with given item in the collection
    /// </summary>
    /// <param name="item">Item to find</param>
    /// <returns>Iterator for the found element or End if cannot find it</returns>
    template<typename ItemType>
    Iterator Find(const ItemType& item)
    {
        typename Base::FindPositionResult pos;
        Base::FindPosition(item, pos);
        return pos.ObjectIndex != -1 ? Iterator(this, pos.ObjectIndex) : End();
    }

    /// <summary>
    /// Find element with given item in the collection
    /// </summary>
    /// <param name="item">Item to find</param>
    /// <returns>Iterator for the found element or End if cannot find it</returns>
    template<typename ItemType>
    ConstIterator Find(const ItemType& item) const
    {
        typename Base::FindPositionResult pos;
        Base::FindPosition(item, pos);
        return pos.ObjectIndex != -1 ? ConstIterator(this, pos.ObjectIndex) : End();
    }

    /// <summary>
    /// Determines whether a collection contains the specified element.
    /// </summary>
    /// <param name="item">The item to locate.</param>
    /// <returns>True if value has been found in a collection, otherwise false</returns>
    template<typename ItemType>
    bool Contains(const ItemType& item) const
    {
        typename Base::FindPositionResult pos;
        Base::FindPosition(item, pos);
        return pos.ObjectIndex != -1;
    }

public:
    /// <summary>
    /// Clones other collection into this.
    /// </summary>
    /// <param name="other">The other collection to clone.</param>
    void Clone(const HashSet& other)
    {
        Base::Clear();
        Base::SetCapacity(other.Capacity(), false);
        for (ConstIterator i = other.Begin(); i != other.End(); ++i)
            Add(i->Item);
        ASSERT(Base::Count() == other.Count());
        ASSERT(Base::Capacity() == other.Capacity());
    }

    /// <summary>
    /// Gets the items collection to the output array (will contain unique items).
    /// </summary>
    /// <param name="result">The result.</param>
    template<typename ArrayAllocation>
    void GetItems(Array<T, ArrayAllocation>& result) const
    {
        for (ConstIterator i = Begin(); i.IsNotEnd(); ++i)
            result.Add(i->Item);
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
