// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "../Collections/Array.h"
#include "Engine/Core/Math/Math.h"

/// <summary>
/// Template for dynamic array with variable capacity that uses fixed size memory chunks for data storage rather than linear allocation.
/// </summary>
/// <remarks>
/// Array with variable capacity that does not moves elements when it grows so you can add item and use pointer to it while still keep adding new items.
/// </remarks>
template<typename T, int32 ChunkSize>
class ChunkedArray
{
    friend ChunkedArray;

private:
    // TODO: don't use Array but small struct and don't InlinedArray or Chunk* but Chunk (less dynamic allocations)
    typedef Array<T> Chunk;

    int32 _count = 0;
    Array<Chunk*, InlinedAllocation<32>> _chunks;

public:
    ChunkedArray()
    {
    }

    ~ChunkedArray()
    {
        _chunks.ClearDelete();
    }

public:
    /// <summary>
    /// Gets the amount of the elements in the collection.
    /// </summary>
    FORCE_INLINE int32 Count() const
    {
        return _count;
    }

    /// <summary>
    /// Gets the amount of the elements that can be hold by collection without resizing.
    /// </summary>
    FORCE_INLINE int32 Capacity() const
    {
        return _chunks.Count() * ChunkSize;
    }

    /// <summary>
    /// Returns true if array isn't empty.
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

public:
    // Gets element by index
    FORCE_INLINE T& At(int32 index) const
    {
        ASSERT(index >= 0 && index < _count);
        return _chunks[index / ChunkSize]->At(index % ChunkSize);
    }

    // Gets/Sets element by index
    FORCE_INLINE T& operator[](int32 index)
    {
        ASSERT(index >= 0 && index < _count);
        return _chunks[index / ChunkSize]->At(index % ChunkSize);
    }

    // Gets/Sets element by index
    FORCE_INLINE const T& operator[](int32 index) const
    {
        ASSERT(index >= 0 && index < _count);
        return _chunks[index / ChunkSize]->At(index % ChunkSize);
    }

public:
    /// <summary>
    /// Chunked array iterator.
    /// </summary>
    struct Iterator
    {
        friend ChunkedArray;
    private:
        ChunkedArray* _collection;
        int32 _chunkIndex;
        int32 _index;

        Iterator(const ChunkedArray* collection, const int32 index)
            : _collection(const_cast<ChunkedArray*>(collection))
            , _chunkIndex(index / ChunkSize)
            , _index(index % ChunkSize)
        {
        }

    public:
        Iterator()
            : _collection(nullptr)
            , _chunkIndex(INVALID_INDEX)
            , _index(INVALID_INDEX)
        {
        }

        Iterator(const Iterator& i)
            : _collection(i._collection)
            , _chunkIndex(i._chunkIndex)
            , _index(i._index)
        {
        }

        Iterator(Iterator&& i)
            : _collection(i._collection)
            , _chunkIndex(i._chunkIndex)
            , _index(i._index)
        {
        }

    public:
        FORCE_INLINE int32 Index() const
        {
            return _chunkIndex * ChunkSize + _index;
        }

        FORCE_INLINE bool IsEnd() const
        {
            return (_chunkIndex * ChunkSize + _index) == _collection->_count;
        }

        FORCE_INLINE bool IsNotEnd() const
        {
            return (_chunkIndex * ChunkSize + _index) != _collection->_count;
        }

        FORCE_INLINE T& operator*() const
        {
            return _collection->_chunks[_chunkIndex]->At(_index);
        }

        FORCE_INLINE T* operator->() const
        {
            return &_collection->_chunks[_chunkIndex]->At(_index);
        }

        FORCE_INLINE bool operator==(const Iterator& v) const
        {
            return _collection == v._collection && _chunkIndex == v._chunkIndex && _index == v._index;
        }

        FORCE_INLINE bool operator!=(const Iterator& v) const
        {
            return _collection != v._collection || _chunkIndex != v._chunkIndex || _index != v._index;
        }

        Iterator& operator=(const Iterator& v)
        {
            _collection = v._collection;
            _chunkIndex = v._chunkIndex;
            _index = v._index;
            return *this;
        }

        Iterator& operator=(Iterator&& v)
        {
            _collection = v._collection;
            _chunkIndex = v._chunkIndex;
            _index = v._index;
            return *this;
        }

        Iterator& operator++()
        {
            // Check if it is not at end
            if ((_chunkIndex * ChunkSize + _index) != _collection->_count)
            {
                // Move forward within chunk
                _index++;

                if (_index == ChunkSize && _chunkIndex < _collection->_chunks.Count() - 1)
                {
                    // Move to next chunk
                    _chunkIndex++;
                    _index = 0;
                }
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
            // Check if it's not at beginning
            if (_index != 0 || _chunkIndex != 0)
            {
                if (_index == 0)
                {
                    // Move to previous chunk
                    _chunkIndex--;
                    _index = ChunkSize - 1;
                }
                else
                {
                    // Move backward within chunk
                    _index--;
                }
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
    /// Adds the specified item to the collection.
    /// </summary>
    /// <param name="item">The item.</param>
    /// <returns>The pointer to the allocated item in the storage.</returns>
    T* Add(const T& item)
    {
        // Find first chunk with some space
        Chunk* chunk = nullptr;
        for (int32 i = 0; i < _chunks.Count(); i++)
        {
            if (_chunks[i]->Count() < ChunkSize)
            {
                chunk = _chunks[i];
                break;
            }
        }

        // Allocate chunk if missing
        if (chunk == nullptr)
        {
            chunk = New<Chunk>();
            chunk->SetCapacity(ChunkSize);
            _chunks.Add(chunk);
        }

        // Add item
        _count++;
        chunk->Add(item);
        return &chunk->At(chunk->Count() - 1);
    }

    /// <summary>
    /// Adds the one item to the collection and returns the reference to it.
    /// </summary>
    /// <returns>The reference to the added item.</returns>
    T& AddOne()
    {
        // Find first chunk with some space
        Chunk* chunk = nullptr;
        for (int32 i = 0; i < _chunks.Count(); i++)
        {
            if (_chunks[i]->Count() < ChunkSize)
            {
                chunk = _chunks[i];
                break;
            }
        }

        // Allocate chunk if missing
        if (chunk == nullptr)
        {
            chunk = New<Chunk>();
            chunk->SetCapacity(ChunkSize);
            _chunks.Add(chunk);
        }

        // Add item
        _count++;
        return chunk->AddOne();
    }

    /// <summary>
    /// Removes the element at specified iterator position.
    /// </summary>
    /// <param name="i">The element iterator to remove.</param>
    void Remove(const Iterator& i)
    {
        if (IsEmpty())
            return;
        ASSERT(i._collection == this);
        ASSERT(i._chunkIndex < _chunks.Count() && i._index < ChunkSize);
        ASSERT(i.Index() < Count());

        auto lastChunkIndex = (_count - 1) / ChunkSize;
        auto lastIndex = (_count - 1) % ChunkSize;
        auto& lastChunk = *_chunks[lastChunkIndex];

        // Check if remove element from the last chunk
        if (i._chunkIndex == lastChunkIndex)
        {
            // Remove that item
            lastChunk.RemoveAt(i._index);
        }
        else
        {
            // Swap that item with the last item from the last chunk
            (*_chunks[i._chunkIndex])[i._index] = lastChunk[lastIndex];
            lastChunk.RemoveLast();
        }

        _count--;
    }

    /// <summary>
    /// Clears the collection but without changing its capacity.
    /// </summary>
    void Clear()
    {
        _count = 0;
        for (int32 i = 0; i < _chunks.Count(); i++)
        {
            _chunks[i]->Clear();
        }
    }

    /// <summary>
    /// Clears the collection and releases the dynamic memory allocated within the container.
    /// </summary>
    void Release()
    {
        Clear();
        _chunks.ClearDelete();
        _chunks.Resize(0);
    }

    /// <summary>
    /// Ensures that collection has a given capacity. It does not preserve collection contents.
    /// </summary>
    /// <param name="minCapacity">The minimum required capacity.</param>
    void EnsureCapacity(int32 minCapacity)
    {
        int32 minChunks = minCapacity / ChunkSize;
        if (minCapacity % ChunkSize != 0)
            ++minChunks;
        while (_chunks.Count() < minChunks)
        {
            auto chunk = New<Chunk>();
            chunk->SetCapacity(ChunkSize);
            _chunks.Add(chunk);
        }
    }

    /// <summary>
    /// Resizes that collection to the specified new size. It may not preserve collection contents in case of shrinking.
    /// </summary>
    /// <param name="newSize">The new size.</param>
    void Resize(int32 newSize)
    {
        while (newSize < Count())
        {
            auto& chunk = _chunks.Last();
            int32 itemsLeft = Count() - newSize;
            int32 itemsToRemove = Math::Min(chunk->Count(), itemsLeft);
            chunk->Resize(chunk->Count() - itemsToRemove);
            _count -= itemsToRemove;
            if (chunk->Count() == 0)
            {
                Delete(chunk);
                _chunks.RemoveLast();
            }
        }
        if (newSize > Count())
        {
            EnsureCapacity(newSize);

            // Add more items until reach the new size
            int32 chunkIndex = 0;
            int32 itemsReaming = newSize - Count();
            while (itemsReaming != 0)
            {
                ASSERT(chunkIndex != _chunks.Count());
                auto& chunk = _chunks[chunkIndex];

                // Insert some items to this chunk if can
                const int32 spaceLeft = chunk->Capacity() - chunk->Count();
                int32 itemsToAdd = Math::Min(itemsReaming, spaceLeft);
                chunk->Resize(chunk->Count() + itemsToAdd);
                _count += itemsToAdd;

                // Update counter
                itemsReaming = newSize - Count();

                // Move to the next chunk to fill it
                chunkIndex++;
            }
        }
        ASSERT(newSize == Count());
    }

    /// <summary>
    /// Searches for the specified object and returns the zero-based index of the first occurrence within the entire collection.
    /// </summary>
    /// <param name="item">The item to find.</param>
    /// <returns>The zero-based index of the first occurrence of itm within the entire collection, if found; otherwise, INVALID_INDEX.</returns>
    int32 Find(const T& item) const
    {
        int32 startIndex = 0;
        for (int32 chunkIndex = 0; chunkIndex < _chunks.Count(); chunkIndex++)
        {
            const int32 index = _chunks[chunkIndex]->Find(item);
            if (index != INVALID_INDEX)
                return startIndex + index;
            startIndex += ChunkSize;
            if (startIndex > _count)
                break;
        }
        return INVALID_INDEX;
    }

public:
    Iterator Begin() const
    {
        return Iterator(this, 0);
    }

    Iterator End() const
    {
        return Iterator(this, _count);
    }

    Iterator IteratorAt(int32 index) const
    {
        return Iterator(this, index);
    }

    FORCE_INLINE Iterator begin()
    {
        return Iterator(this, 0);
    }

    FORCE_INLINE Iterator end()
    {
        return Iterator(this, _count);
    }

    FORCE_INLINE const Iterator begin() const
    {
        return Iterator(this, 0);
    }

    FORCE_INLINE const Iterator end() const
    {
        return Iterator(this, _count);
    }
};
