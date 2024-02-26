// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/Platform.h"

/// <summary>
/// Represents a value type that can be assigned null. A nullable type can represent the correct range of values for its underlying value type, plus an additional null value.
/// </summary>
template<typename T>
struct NullableBase
{
protected:
    bool _hasValue;
    T _value;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="NullableBase{T}"/> struct.
    /// </summary>
    NullableBase()
    {
        _hasValue = false;
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="NullableBase{T}"/> struct.
    /// </summary>
    /// <NullableBase name="value">The initial value.</param>
    NullableBase<T>(const T& value)
    {
        _value = value;
        _hasValue = true;
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="NullableBase{T}"/> struct.
    /// </summary>
    /// <param name="other">The other.</param>
    NullableBase(const NullableBase& other)
    {
        _value = other._value;
        _hasValue = other._hasValue;
    }

public:
    /// <summary>
    /// Gets a value indicating whether the current NullableBase{T} object has a valid value of its underlying type.
    /// </summary>
    /// <returns><c>true</c> if this object has a valid value; otherwise, <c>false</c>.</returns>
    FORCE_INLINE bool HasValue() const
    {
        return _hasValue;
    }

    /// <summary>
    /// Gets the value of the current NullableBase{T} object if it has been assigned a valid underlying value.
    /// </summary>
    /// <returns>The value.</returns>
    FORCE_INLINE const T& GetValue() const
    {
        ASSERT(_hasValue);
        return _value;
    }

    /// <summary>
    /// Gets the value of the current NullableBase{T} object if it has been assigned a valid underlying value.
    /// </summary>
    /// <returns>The value.</returns>
    FORCE_INLINE T GetValue()
    {
        ASSERT(_hasValue);
        return _value;
    }

    /// <summary>
    /// Sets the value.
    /// </summary>
    /// <param name="value">The value.</param>
    void SetValue(const T& value)
    {
        _value = value;
        _hasValue = true;
    }

    /// <summary>
    /// Resets the value.
    /// </summary>
    void Reset()
    {
        _hasValue = false;
    }

public:
    /// <summary>
    /// Indicates whether the current NullableBase{T} object is equal to a specified object.
    /// </summary>
    /// <param name="other">The other object.</param>
    /// <returns>True if both values are equal.</returns>
    bool operator==(const NullableBase& other) const
    {
        if (_hasValue)
        {
            return other._hasValue && _value == other._value;
        }

        return !other._hasValue;
    }

    /// <summary>
    /// Indicates whether the current NullableBase{T} object is not equal to a specified object.
    /// </summary>
    /// <param name="other">The other object.</param>
    /// <returns>True if both values are not equal.</returns>
    FORCE_INLINE bool operator!=(const NullableBase& other) const
    {
        return !operator==(other);
    }
};

/// <summary>
/// Represents a value type that can be assigned null. A nullable type can represent the correct range of values for its underlying value type, plus an additional null value.
/// </summary>
template<typename T>
struct Nullable : NullableBase<T>
{
public:
    /// <summary>
    /// Initializes a new instance of the <see cref="Nullable{T}"/> struct.
    /// </summary>
    Nullable()
        : NullableBase<T>()
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Nullable{T}"/> struct.
    /// </summary>
    /// <NullableBase name="value">The initial value.</param>
    Nullable<T>(const T& value)
        : NullableBase<T>(value)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Nullable{T}"/> struct.
    /// </summary>
    /// <param name="other">The other.</param>
    Nullable(const Nullable& other)
        : NullableBase<T>(other)
    {
    }
};

/// <summary>
/// Nullable value container that contains a boolean value or null.
/// </summary>
template<>
struct Nullable<bool> : NullableBase<bool>
{
public:
    /// <summary>
    /// Initializes a new instance of the <see cref="Nullable{T}"/> struct.
    /// </summary>
    Nullable()
        : NullableBase<bool>()
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Nullable{T}"/> struct.
    /// </summary>
    /// <NullableBase name="value">The initial value.</param>
    Nullable(bool value)
        : NullableBase<bool>(value)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Nullable{T}"/> struct.
    /// </summary>
    /// <param name="other">The other.</param>
    Nullable(const Nullable& other)
        : NullableBase<bool>(other)
    {
    }

public:
    /// <summary>
    /// Gets a value indicating whether the current Nullable{T} object has a valid value and it's set to true.
    /// </summary>
    /// <returns><c>true</c> if this object has a valid value set to true; otherwise, <c>false</c>.</returns>
    FORCE_INLINE bool IsTrue() const
    {
        return _hasValue && _value;
    }

    /// <summary>
    /// Gets a value indicating whether the current Nullable{T} object has a valid value and it's set to false.
    /// </summary>
    /// <returns><c>true</c> if this object has a valid value set to false; otherwise, <c>false</c>.</returns>
    FORCE_INLINE bool IsFalse() const
    {
        return _hasValue && !_value;
    }

    /// <summary>
    /// Implicit conversion to boolean value.
    /// </summary>
    /// <returns>True if this object has a valid value set to true, otherwise false</returns>
    FORCE_INLINE operator bool() const
    {
        return _hasValue && _value;
    }
};
