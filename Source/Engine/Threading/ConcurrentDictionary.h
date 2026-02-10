// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Platform/CriticalSection.h"

/// <summary>
/// Template for unordered dictionary with mapped key with value pairs that supports asynchronous data reading and writing.
/// Implemented via reader-writer lock pattern, so multiple threads can read data at the same time, but only one thread can write data and it blocks all other threads (including readers) until the write operation is finished.
/// Optimized for frequent reads (no lock operation).
/// </summary>
/// <typeparam name="KeyType">The type of the keys in the dictionary.</typeparam>
/// <typeparam name="ValueType">The type of the values in the dictionary.</typeparam>
/// <typeparam name="AllocationType">The type of memory allocator.</typeparam>
template<typename KeyType, typename ValueType, typename AllocationType = HeapAllocation>
class ConcurrentDictionary : Dictionary<KeyType, ValueType, AllocationType>
{
    friend ConcurrentDictionary;
public:
    typedef Dictionary<KeyType, ValueType, AllocationType> Base;
    typedef DictionaryBucket<KeyType, ValueType, AllocationType> Bucket;
    using AllocationData = typename AllocationType::template Data<Bucket>;
    using AllocationTag = typename AllocationType::Tag;

private:
    mutable volatile int64 _threadsReading = 0;
    volatile int64 _threadsWriting = 0;
    CriticalSection _locker;

public:
    /// <summary>
    /// Initializes an empty <see cref="ConcurrentDictionary"/> without reserving any space.
    /// </summary>
    ConcurrentDictionary()
    {
    }

    /// <summary>
    /// Initializes an empty <see cref="ConcurrentDictionary"/> without reserving any space.
    /// </summary>
    /// <param name="tag">The custom allocation tag.</param>
    ConcurrentDictionary(AllocationTag tag)
        : Base(tag)
    {
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="ConcurrentDictionary"/> class.
    /// </summary>
    ~ConcurrentDictionary()
    {
        Clear();
    }

public:
    /// <summary>
    /// Gets the amount of the elements in the collection.
    /// </summary>
    int32 Count() const
    {
        Reader reader(this);
        return Base::_elementsCount;
    }

    /// <summary>
    /// Gets the amount of the elements that can be contained by the collection.
    /// </summary>
    int32 Capacity() const
    {
        Reader reader(this);
        return Base::_size;
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
        Reader reader(this);
        typename Base::FindPositionResult pos;
        Base::FindPosition(key, pos);
        if (pos.ObjectIndex != -1)
            result = Base::_allocation.Get()[pos.ObjectIndex].Value;
        return pos.ObjectIndex != -1;
    }

public:
    /// <summary>
    /// Adds a pair of key and value to the collection.
    /// </summary>
    /// <param name="key">The key.</param>
    /// <param name="value">The value.</param>
    /// <returns>True if added element, otherwise false if it already exists (or other thread added it).</returns>
    template<typename KeyComparableType>
    bool Add(const KeyComparableType& key, const ValueType& value)
    {
        Writer writer(this);
        Bucket* bucket = Base::OnAdd(key, false, true);
        if (bucket)
            bucket->Occupy(key, value);
        return bucket != nullptr;
    }

    /// <summary>
    /// Removes element with a specified key.
    /// </summary>
    /// <param name="key">The element key to remove.</param>
    /// <returns>True if item was removed from collection, otherwise false.</returns>
    template<typename KeyComparableType>
    bool Remove(const KeyComparableType& key)
    {
        Writer writer(this);
        return Base::Remove(key);
    }

public:
    /// <summary>
    /// Removes all elements from the collection.
    /// </summary>
    void Clear()
    {
        Writer writer(this);
        Base::Clear();
    }

public:
    /// <summary>
    /// The read-only dictionary collection iterator.
    /// </summary>
    struct ConstIterator : Base::IteratorBase
    {
        friend ConcurrentDictionary;
    public:
        ConstIterator(const ConcurrentDictionary* collection, const int32 index)
            : Base::IteratorBase(collection, index)
        {
            if (collection)
                collection->BeginRead();
        }

        ConstIterator(const ConstIterator& i)
            : Base::IteratorBase(i._collection, i._index)
        {
            if (i.collection)
                i.collection->BeginRead();
        }

        ConstIterator(ConstIterator&& i) noexcept
            : Base::IteratorBase(i._collection, i._index)
        {
            i._collection = nullptr;
        }

        ~ConstIterator()
        {
            if (this->_collection)
                ((ConcurrentDictionary*)this->_collection)->EndRead();
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
            v._collection = nullptr;
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

private:
    void BeginWrite()
    {
        Platform::InterlockedIncrement(&_threadsWriting);

        // Wait for all reads to end
    RETRY:
        while (Platform::AtomicRead(&_threadsReading))
            Platform::Yield();

        // Thread-safe writing
        _locker.Lock();
        if (Platform::AtomicRead(&_threadsReading))
        {
            // Other reader entered during mutex locking so give them a chance to transition into active-waiting
            _locker.Unlock();
            goto RETRY;
        }
    }

    void EndWrite()
    {
        _locker.Unlock();
        Platform::InterlockedDecrement(&_threadsWriting);
    }

    void BeginRead() const
    {
    RETRY:
        Platform::InterlockedIncrement(&_threadsReading);

        // Check if any thread is writing (or is about to write)
        if (Platform::AtomicRead(&_threadsWriting) != 0)
        {
            // Wait for all writes to end
            Platform::InterlockedDecrement(&_threadsReading);
            while (Platform::AtomicRead(&_threadsWriting))
                Platform::Yield();

            // Try again
            goto RETRY;
        }
    }

    void EndRead() const
    {
        Platform::InterlockedDecrement(&_threadsReading);
    }

private:
    // Utility for methods that read-write state.
    struct Writer
    {
        ConcurrentDictionary* _collection;

        Writer(ConcurrentDictionary* collection)
            : _collection(collection)
        {
            _collection->BeginWrite();
        }

        ~Writer()
        {
            _collection->EndWrite();
        }
    };

    // Utility for methods that read-only state.
    struct Reader
    {
        const ConcurrentDictionary* _collection;

        Reader(const ConcurrentDictionary* collection)
            : _collection(collection)
        {
            _collection->BeginRead();
        }

        ~Reader()
        {
            _collection->EndRead();
        }
    };
};
