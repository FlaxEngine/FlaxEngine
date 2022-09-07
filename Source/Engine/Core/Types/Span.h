// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/Platform.h"

/// <summary>
/// Universal representation of a contiguous region of arbitrary memory.
/// </summary>
template<typename T>
class Span
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
    /// <returns>True if is valid, otherwise false.</returns>
    FORCE_INLINE bool IsValid() const
    {
        return _data != nullptr;
    }

    /// <summary>
    /// Returns true if data is invalid.
    /// </summary>
    /// <returns>True if is invalid, otherwise false.</returns>
    FORCE_INLINE bool IsInvalid() const
    {
        return _data == nullptr;
    }

    /// <summary>
    /// Gets length of the data.
    /// </summary>
    /// <returns>The amount of elements.</returns>
    FORCE_INLINE int32 Length() const
    {
        return _length;
    }

    /// <summary>
    /// Gets the pointer to the data.
    /// </summary>
    /// <returns>The data.</returns>
    FORCE_INLINE T* Get()
    {
        return _data;
    }

    /// <summary>
    /// Gets the pointer to the data.
    /// </summary>
    /// <returns>The data.</returns>
    FORCE_INLINE const T* Get() const
    {
        return _data;
    }

    /// <summary>
    /// Gets the pointer to the data.
    /// </summary>
    /// <returns>The data.</returns>
    template<typename U>
    FORCE_INLINE U* Get() const
    {
        return (U*)_data;
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
};

template<typename T>
inline Span<T> ToSpan(const T* ptr, int32 length)
{
    return Span<T>(ptr, length);
}
