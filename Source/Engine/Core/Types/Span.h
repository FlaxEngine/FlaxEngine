// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/Platform.h"

/// <summary>
/// Universal representation of a contiguous region of arbitrary memory.
/// </summary>
template<typename T>
API_CLASS(InBuild) class Span
{
protected:
    T* _data;
    int32 _length;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="Span"/> class.
    /// </summary>
    Span()
        : _data(nullptr)
        , _length(0)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Span"/> class.
    /// </summary>
    /// <param name="data">The data pointer to link.</param>
    /// <param name="length">The data length.</param>
    Span(const T* data, int32 length)
        : _data((T*)data)
        , _length(length)
    {
    }

public:
    /// <summary>
    /// Returns true if data is valid.
    /// </summary>
    FORCE_INLINE bool IsValid() const
    {
        return _data != nullptr;
    }

    /// <summary>
    /// Returns true if data is invalid.
    /// </summary>
    FORCE_INLINE bool IsInvalid() const
    {
        return _data == nullptr;
    }

    /// <summary>
    /// Gets length of the data.
    /// </summary>
    FORCE_INLINE int32 Length() const
    {
        return _length;
    }

    /// <summary>
    /// Gets the pointer to the data.
    /// </summary>
    FORCE_INLINE T* Get()
    {
        return _data;
    }

    /// <summary>
    /// Gets the pointer to the data.
    /// </summary>
    FORCE_INLINE const T* Get() const
    {
        return _data;
    }

    /// <summary>
    /// Gets the pointer to the data.
    /// </summary>
    template<typename U>
    FORCE_INLINE U* Get() const
    {
        return (U*)_data;
    }

public:
    /// <summary>
    /// Constructs a slice out of the current span that begins at a specified index.
    /// </summary>
    /// <param name="start">The zero-based index at which to begin the slice.</param>
    /// <returns>A span that consists of all elements of the current span from <paramref name="start" /> to the end of the span.</returns>
    Span<T> Slice(int32 start)
    {
        ASSERT(start <= _length);
        return Span(_data + start, _length - start);
    }

    /// <summary>
    /// Constructs a slice out of the current span starting at a specified index for a specified length.
    /// </summary>
    /// <param name="start">The zero-based index at which to begin this slice.</param>
    /// <param name="length">The length for the result slice.</param>
    /// <returns>A span that consists of <paramref name="length" /> elements from the current span starting at <paramref name="start" />.</returns>
    Span<T> Slice(int32 start, int32 length)
    {
        ASSERT(start + length <= _length);
        return Span(_data + start, length);
    }

public:
    /// <summary>
    /// Gets or sets the element by index
    /// </summary>
    /// <param name="index">The index.</param>
    /// <returns>The item reference.</returns>
    FORCE_INLINE T& operator[](int32 index)
    {
        ASSERT(index >= 0 && index < _length);
        return _data[index];
    }

    /// <summary>
    /// Gets or sets the element by index
    /// </summary>
    /// <param name="index">The index.</param>
    /// <returns>The item constant reference.</returns>
    FORCE_INLINE const T& operator[](int32 index) const
    {
        ASSERT(index >= 0 && index < _length);
        return _data[index];
    }

    FORCE_INLINE T* begin()
    {
        return _data;
    }

    FORCE_INLINE T* end()
    {
        return _data + _length;
    }

    FORCE_INLINE const T* begin() const
    {
        return _data;
    }

    FORCE_INLINE const T* end() const
    {
        return _data + _length;
    }
};

template<typename T>
inline Span<T> ToSpan(const T* ptr, int32 length)
{
    return Span<T>(ptr, length);
}

template<typename T, typename U = T, typename AllocationType = HeapAllocation>
inline Span<U> ToSpan(const Array<T, AllocationType>& data)
{
    return Span<U>((U*)data.Get(), data.Count());
}

template<typename T>
inline bool SpanContains(const Span<T> span, const T& value)
{
    for (int32 i = 0; i < span.Length(); i++)
    {
        if (span.Get()[i] == value)
            return true;
    }
    return false;
}
