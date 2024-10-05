// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/Platform.h"

/// <summary>
/// Represents a value type that can be assigned null. A nullable type can represent the correct range of values for its underlying value type, plus an additional null value.
/// </summary>
template<typename T>
struct Nullable
{
private:
    union // Prevents default construction of T
    {
        T _value;
    };
    bool _hasValue;

    /// <summary>
    /// Ensures that the lifetime of the wrapped value ends correctly. This method is called when the state of the wrapper is no more needed.
    /// </summary>
    FORCE_INLINE void KillOld()
    {
        if (_hasValue)
        {
            _value.~T();
        }
    }

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="NullableBase{T}"/> struct with a null value.
    /// </summary>
    Nullable()
        : _hasValue(false)
    {
        // Value is not initialized.
    }

    ~Nullable()
    {
        KillOld();
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="NullableBase{T}"/> struct by copying the value.
    /// </summary>
    /// <NullableBase name="value">The initial wrapped value.</param>
    Nullable(const T& value)
        : _value(value)
        , _hasValue(true)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="NullableBase{T}"/> struct by moving the value.
    /// </summary>
    /// <NullableBase name="value">The initial wrapped value.</param>
    Nullable(T&& value) noexcept
        : _value(MoveTemp(value))
        , _hasValue(true)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="NullableBase{T}"/> struct by copying the value from another instance.
    /// </summary>
    /// <param name="other">The wrapped value to be copied.</param>
    Nullable(const Nullable& other)
        : _value(other._value)
        , _hasValue(other._hasValue)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="NullableBase{T}"/> struct by moving the value from another instance.
    /// </summary>
    /// <param name="other">The wrapped value to be moved.</param>
    Nullable(Nullable&& other) noexcept
    {
        _hasValue = other._hasValue;
        if (_hasValue)
        {
            new (&_value) T(MoveTemp(other._value)); // Placement new (move constructor)
        }

        other.Reset();
    }

    auto operator=(const T& value) -> Nullable&
    {
        KillOld();

        new (&_value) T(value); // Placement new (copy constructor)
        _hasValue = true;

        return *this;
    }

    auto operator=(T&& value) noexcept -> Nullable&
    {
        KillOld();

        new (&_value) T(MoveTemp(value)); // Placement new (move constructor)
        _hasValue = true;

        return *this;
    }

    auto operator=(const Nullable& other) -> Nullable&
    {
        KillOld();

        _hasValue = other._hasValue;
        if (_hasValue)
        {
            new (&_value) T(other._value); // Placement new (copy constructor)
        }

        return *this;
    }

    auto operator=(Nullable&& other) noexcept -> Nullable&
    {
        KillOld();

        _hasValue = other._hasValue;
        if (_hasValue)
        {
            new (&_value) T(MoveTemp(other._value)); // Placement new (move constructor)

            other.Reset();
        }

        return *this;
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
    /// Gets a reference to the value of the current NullableBase{T} object.
    /// If is assumed that the value is valid, otherwise the behavior is undefined.
    /// </summary>
    /// <remarks>In the past, this value returned a copy of the stored value. Be careful.</remarks>
    /// <returns>Reference to the value.</returns>
    FORCE_INLINE T& GetValue()
    {
        ASSERT(_hasValue);
        return _value;
    }

    /// <summary>
    /// Sets the wrapped value.
    /// </summary>
    /// <param name="value">The value to be copied.</param>
    FORCE_INLINE void SetValue(const T& value)
    {
        if (_hasValue)
        {
            _value.~T();
        }

        new (&_value) T(value); // Placement new (copy constructor)
        _hasValue = true;
    }

    /// <summary>
    /// Sets the wrapped value.
    /// </summary>
    /// <param name="value">The value to be moved.</param>
    FORCE_INLINE void SetValue(T&& value) noexcept
    {
        KillOld();

        new (&_value) T(MoveTemp(value)); // Placement new (move constructor)
        _hasValue = true;
    }

    /// <summary>
    /// Resets the value.
    /// </summary>
    FORCE_INLINE void Reset()
    {
        KillOld();
        _hasValue = false;
    }

    /// <summary>
    /// Moves the value from the current NullableBase{T} object and resets it.
    /// </summary>
    FORCE_INLINE void GetAndReset(T& value)
    {
        ASSERT(_hasValue);
        value = MoveTemp(_value);
        _value.~T(); // Move is not destructive.
        _hasValue = false;
    }

public:
    /// <summary>
    /// Indicates whether the current NullableBase{T} object is equal to a specified object.
    /// </summary>
    /// <param name="other">The other object.</param>
    /// <returns>True if both values are equal.</returns>
    FORCE_INLINE bool operator==(const Nullable& other) const
    {
        if (other._hasValue != _hasValue)
        {
            return false;
        }

        return _value == other._value;
    }

    /// <summary>
    /// Indicates whether the current NullableBase{T} object is not equal to a specified object.
    /// </summary>
    /// <param name="other">The other object.</param>
    /// <returns>True if both values are not equal.</returns>
    FORCE_INLINE bool operator!=(const Nullable& other) const
    {
        return !operator==(other);
    }

    /// <summary>
    /// Explicit conversion to boolean value.
    /// </summary>
    /// <returns>True if this object has a valid value, otherwise false</returns>
    /// <remarks>Hint: If-statements are able to use explicit cast implicitly (sic).</remarks>
    FORCE_INLINE explicit operator bool() const
    {
        return _hasValue;
    }
};

/// <summary>
/// Nullable value container that contains a boolean value or null.
/// </summary>
template<>
struct Nullable<bool>
{
private:
    enum class Value : uint8
    {
        False = 0,
        True = 1,
        Null = 2,
    };

    Value _value = Value::Null;

public:
    Nullable() = default;

    ~Nullable() = default;


    Nullable(Nullable&& value) = default;

    Nullable(const Nullable& value) = default;

    Nullable(const bool value) noexcept
    {
        _value = value ? Value::True : Value::False;
    }


    auto operator=(const bool value) noexcept -> Nullable&
    {
        _value = value ? Value::True : Value::False;
        return *this;
    }

    auto operator=(const Nullable& value) -> Nullable& = default;

    auto operator=(Nullable&& value) -> Nullable& = default;


    FORCE_INLINE bool HasValue() const noexcept
    {
        return _value != Value::Null;
    }

    FORCE_INLINE bool GetValue() const
    {
        ASSERT(_value != Value::Null);
        return _value == Value::True;
    }

    FORCE_INLINE void SetValue(const bool value) noexcept
    {
        _value = value ? Value::True : Value::False;
    }

    FORCE_INLINE void Reset() noexcept
    {
        _value = Value::Null;
    }

    FORCE_INLINE void GetAndReset(bool& value) noexcept
    {
        ASSERT(_value != Value::Null);
        value = _value == Value::True;
        _value = Value::Null;
    }


    /// <summary>
    /// Gets a value indicating whether the current Nullable{T} object has a valid value and it's set to true.
    /// </summary>
    /// <returns><c>true</c> if this object has a valid value set to true; otherwise, <c>false</c>.</returns>
    FORCE_INLINE bool IsTrue() const
    {
        return _value == Value::True;
    }

    /// <summary>
    /// Gets a value indicating whether the current Nullable{T} object has a valid value and it's set to false.
    /// </summary>
    /// <returns><c>true</c> if this object has a valid value set to false; otherwise, <c>false</c>.</returns>
    FORCE_INLINE bool IsFalse() const
    {
        return _value == Value::False;
    }

    /// <summary>
    /// Getting if provoke unacceptably ambiguous code. For template meta-programming use explicit HasValue() instead.
    /// </summary>
    explicit operator bool() const = delete;

    // Note: Even though IsTrue and IsFalse have been added for convenience, but they may be used for performance reasons.
};
