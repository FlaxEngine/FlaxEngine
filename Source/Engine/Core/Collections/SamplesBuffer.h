// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "../Types/BaseTypes.h"
#include "Engine/Platform/Platform.h"

/// <summary>
/// Small buffer for samples used to calculate min/max/avg values.
/// </summary>
template<typename T, int32 Size>
class SamplesBuffer
{
protected:
    int32 _count = 0;
    T _data[Size];

public:
    /// <summary>
    /// Gets amount of elements in the collection.
    /// </summary>
    FORCE_INLINE int32 Count() const
    {
        return _count;
    }

    /// <summary>
    /// Gets amount of elements that can be added to the collection.
    /// </summary>
    FORCE_INLINE int32 Capacity() const
    {
        return Size;
    }

    /// <summary>
    /// Returns true if collection has any elements added.
    /// </summary>
    FORCE_INLINE bool HasItems() const
    {
        return _count > 0;
    }

    /// <summary>
    /// Returns true if collection is empty.
    /// </summary>
    FORCE_INLINE bool IsEmpty() const
    {
        return _count < 1;
    }

    /// <summary>
    /// Gets pointer to the first element in the collection.
    /// </summary>
    FORCE_INLINE T* Get() const
    {
        return _data;
    }

    /// <summary>
    /// Gets or sets element at the specified index.
    /// </summary>
    /// <param name="index">The index.</param>
    /// <returns>The element.</returns>
    FORCE_INLINE T& At(int32 index) const
    {
        ASSERT(index >= 0 && index < _count);
        return _data[index];
    }

    /// <summary>
    /// Gets or sets element at the specified index.
    /// </summary>
    /// <param name="index">The index.</param>
    /// <returns>The element.</returns>
    FORCE_INLINE T& operator[](int32 index)
    {
        ASSERT(index >= 0 && index < _count);
        return _data[index];
    }

    /// <summary>
    /// Gets or sets element at the specified index.
    /// </summary>
    /// <param name="index">The index.</param>
    /// <returns>The element.</returns>
    FORCE_INLINE const T& operator[](int32 index) const
    {
        ASSERT(index >= 0 && index < _count);
        return _data[index];
    }

    /// <summary>
    /// Gets the first element value.
    /// </summary>
    FORCE_INLINE T First() const
    {
        ASSERT(HasItems());
        return _data[0];
    }

    /// <summary>
    /// Gets last element value.
    /// </summary>
    FORCE_INLINE T Last() const
    {
        ASSERT(HasItems());
        return _data[_count - 1];
    }

public:
    /// <summary>
    /// Adds the specified value to the buffer.
    /// </summary>
    /// <param name="value">The value to add.</param>
    void Add(const T& value)
    {
        if (_count != Size)
            _count++;
        if (_count > 1)
        {
            for (int32 i = _count - 1; i > 0; i--)
                _data[i] = _data[i - 1];
        }
        _data[0] = value;
    }

    /// <summary>
    /// Sets all elements to the given value.
    /// </summary>
    /// <param name="value">The value.</param>
    void SetAll(const T value)
    {
        for (int32 i = 0; i < _count; i++)
            _data[i] = value;
    }

    /// <summary>
    /// Clears this collection.
    /// </summary>
    FORCE_INLINE void Clear()
    {
        _count = 0;
    }

public:
    /// <summary>
    /// Gets the minimum value in the buffer.
    /// </summary>
    T Minimum() const
    {
        ASSERT(HasItems());
        T result = _data[0];
        for (int32 i = 1; i < _count; i++)
        {
            if (_data[i] < result)
                result = _data[i];
        }
        return result;
    }

    /// <summary>
    /// Gets the maximum value in the buffer.
    /// </summary>
    T Maximum() const
    {
        ASSERT(HasItems());
        T result = _data[0];
        for (int32 i = 1; i < _count; i++)
        {
            if (_data[i] > result)
                result = _data[i];
        }
        return result;
    }

    /// <summary>
    /// Gets the average value in the buffer.
    /// </summary>
    T Average() const
    {
        ASSERT(HasItems());
        T result = _data[0];
        for (int32 i = 1; i < _count; i++)
        {
            result += _data[i];
        }
        return result / _count;
    }
};
