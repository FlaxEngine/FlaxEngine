// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Span.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Platform/Platform.h"
#include "Engine/Serialization/WriteStream.h"
#include "Engine/Serialization/ReadStream.h"

/// <summary>
/// Universal utility class that can store the chunk of data or just reference to the memory.
/// Supports only value types that don't require constructor/destructor invocation.
/// </summary>
template<typename T>
class DataContainer : public Span<T>
{
public:

    typedef Span<T> Base;

private:

    bool _isAllocated;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="DataContainer"/> class.
    /// </summary>
    DataContainer()
        : Base()
        , _isAllocated(false)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="DataContainer"/> class.
    /// </summary>
    /// <param name="data">The data pointer to link.</param>
    /// <param name="length">The data length.</param>
    DataContainer(T* data, int32 length)
        : Base(data, length)
        , _isAllocated(false)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="DataContainer"/> class.
    /// </summary>
    /// <param name="data">The data array to link.</param>
    DataContainer(const Array<T>& data)
        : Base((T*)data.Get(), data.Count())
        , _isAllocated(false)
    {
    }

    /// <summary>
    /// Copy constructor
    /// </summary>
    /// <param name="other">Other container to copy</param>
    DataContainer(const DataContainer& other)
    {
        if (other.IsAllocated())
            Copy(other);
        else
            Link(other);
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="DataContainer"/> class.
    /// </summary>
    /// <param name="other">The other collection to move.</param>
    FORCE_INLINE DataContainer(DataContainer&& other)
    {
        Base::_data = other._data;
        Base::_length = other._length;
        _isAllocated = other._isAllocated;

        other._data = nullptr;
        other._length = 0;
        other._isAllocated = false;
    }

    /// <summary>
    /// The move assignment operator that deletes the current container of data and the moves items from the other container.
    /// </summary>
    /// <param name="other">The other collection to move.</param>
    /// <returns>The reference to this.</returns>
    DataContainer& operator=(DataContainer&& other)
    {
        if (this != &other)
        {
            Release();

            Base::_data = other._data;
            Base::_length = other._length;
            _isAllocated = other._isAllocated;

            other._data = nullptr;
            other._length = 0;
            other._isAllocated = false;
        }
        return *this;
    }

    /// <summary>
    /// Assignment operator
    /// </summary>
    /// <param name="other">The other container.</param>
    /// <returns>This</returns>
    DataContainer& operator=(const DataContainer& other)
    {
        if (this != &other)
        {
            if (other.IsAllocated())
                Copy(other);
            else
                Link(other);
        }
        return *this;
    }

    /// <summary>
    /// Destructor
    /// </summary>
    ~DataContainer()
    {
        if (_isAllocated)
            Allocator::Free(Base::_data);
    }

public:

    /// <summary>
    /// Returns true if data is allocated by container itself
    /// </summary>
    /// <returns>True if is allocated by container, otherwise false</returns>
    FORCE_INLINE bool IsAllocated() const
    {
        return _isAllocated;
    }

public:

    /// <summary>
    /// Link external data
    /// </summary>
    /// <param name="data">Data array to link</param>
    void Link(const Array<T>& data)
    {
        Link(data.Get(), data.Count());
    }

    /// <summary>
    /// Link external data
    /// </summary>
    /// <param name="data">Data to link</param>
    FORCE_INLINE void Link(const Span<T>& data)
    {
        Link(data.Get(), data.Length());
    }

    /// <summary>
    /// Link external data
    /// </summary>
    /// <param name="data">Data to link</param>
    FORCE_INLINE void Link(const DataContainer& data)
    {
        Link(data.Get(), data.Length());
    }

    /// <summary>
    /// Link external data
    /// </summary>
    /// <param name="data">Data to link</param>
    template<typename U>
    FORCE_INLINE void Link(const U* data)
    {
        Link(static_cast<T*>((void*)data), sizeof(U));
    }

    /// <summary>
    /// Link external data
    /// </summary>
    /// <param name="data">Data pointer</param>
    /// <param name="length">Data length</param>
    void Link(const T* data, int32 length)
    {
        Release();

        _isAllocated = false;
        Base::_length = length;
        Base::_data = (T*)data;
    }

    /// <summary>
    /// Allocate new memory chunk
    /// </summary>
    /// <param name="length">Length of the data (amount of T elements)</param>
    void Allocate(const int32 length)
    {
        if (_isAllocated && Base::_length == length)
            return;

        Release();

        if (length > 0)
        {
            _isAllocated = true;
            Base::_length = length;
            Base::_data = (T*)Allocator::Allocate(length * sizeof(T));
        }
    }

    /// <summary>
    /// Copies the data to a new allocated chunk
    /// </summary>
    /// <param name="data">Data array to copy</param>
    FORCE_INLINE void Copy(const Array<T>& data)
    {
        if (data.HasItems())
            Copy(data.Get(), data.Count());
        else
            Release();
    }

    /// <summary>
    /// Copies the data to a new allocated chunk
    /// </summary>
    /// <param name="data">Data to copy</param>
    FORCE_INLINE void Copy(const DataContainer& data)
    {
        if (data.IsValid())
            Copy(data.Get(), data.Length());
        else
            Release();
    }

    /// <summary>
    /// Copies the data to a new allocated chunk
    /// </summary>
    /// <param name="data">Data to copy</param>
    FORCE_INLINE void Copy(const Span<T>& data)
    {
        if (data.IsValid())
            Copy(data.Get(), data.Length());
        else
            Release();
    }

    /// <summary>
    /// Copies the data to a new allocated chunk
    /// </summary>
    /// <param name="data">Pointer to data to copy</param>
    template<typename U>
    FORCE_INLINE void Copy(const U* data)
    {
        Copy(static_cast<T*>((void*)data), sizeof(U));
    }

    /// <summary>
    /// Copies the data to a new allocated chunk
    /// </summary>
    /// <param name="data">Pointer to data to copy</param>
    /// <param name="length">Length of the data (amount of T elements)</param>
    void Copy(const T* data, int32 length)
    {
        ASSERT(data && length > 0);

        Allocate(length);
        ASSERT(Base::_data);
        Platform::MemoryCopy(Base::_data, data, length * sizeof(T));
    }

    /// <summary>
    /// Swaps data of two containers. Performs no data copy.
    /// </summary>
    /// <param name="data">Data to swap</param>
    void Swap(DataContainer& data)
    {
        ::Swap(_isAllocated, data._isAllocated);
        ::Swap(Base::_length, data._length);
        ::Swap(Base::_data, data._data);
    }

    /// <summary>
    /// Releases the data.
    /// </summary>
    void Release()
    {
        if (_isAllocated)
            Allocator::Free(Base::_data);
        _isAllocated = false;
        Base::_length = 0;
        Base::_data = nullptr;
    }

    /// <summary>
    /// Unlinks the buffer and clears the length to zero. Use with caution because it's not safe.
    /// </summary>
    void Unlink()
    {
        _isAllocated = false;
        Base::_length = 0;
        Base::_data = nullptr;
    }

    /// <summary>
    /// Sets the length of the internal allocated buffer. Can be used to `trim` length allocated data without performing reallocation. Use with caution because it's not safe.
    /// </summary>
    /// <param name="length">The length.</param>
    void SetLength(int32 length)
    {
        ASSERT(length >= 0);
        Base::_length = length;
    }

    /// <summary>
    /// Appends the specified data at the end of the container. Will perform the data allocation.
    /// </summary>
    /// <param name="data">The data.</param>
    /// <param name="length">The length.</param>
    void Append(T* data, int32 length)
    {
        if (length <= 0)
            return;
        if (Base::Length() == 0)
        {
            Copy(data, length);
            return;
        }

        auto prev = Base::_data;
        const auto prevLength = Base::_length;

        Base::_length = prevLength + length;
        Base::_data = (T*)Allocator::Allocate(Base::_length * sizeof(T));

        Platform::MemoryCopy(Base::_data, prev, prevLength * sizeof(T));
        Platform::MemoryCopy(Base::_data + prevLength * sizeof(T), data, length * sizeof(T));

        if (_isAllocated && prev)
            Allocator::Free(prev);
        _isAllocated = true;
    }

public:

    void Read(ReadStream* stream, int32 length)
    {
        // Note: this may not work for the objects, use with primitive types and structures

        ASSERT(stream != nullptr);
        Allocate(length);
        if (length > 0)
        {
            stream->ReadBytes((void*)Base::_data, length * sizeof(T));
        }
    }

    void Write(WriteStream* stream) const
    {
        // Note: this may not work for the objects, use with primitive types and structures

        ASSERT(stream != nullptr);
        if (Base::_length > 0)
        {
            stream->WriteBytes((void*)Base::_data, Base::_length * sizeof(T));
        }
    }
};

/// <summary>
/// General purpose raw bytes data container object.
/// </summary>
typedef DataContainer<byte> BytesContainer;
