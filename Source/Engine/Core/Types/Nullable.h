// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Templates.h"
#include "Engine/Platform/Platform.h"

/// <summary>
/// Wrapper for a value type that can be assigned null, controlling the lifetime of the wrapped value.
/// </summary>
/// <typeparam name="T">
/// The type of the wrapped value. It must be move-constructible but does not have to be copy-constructible. Value is never reassigned.
/// </typeparam>
template<typename T>
struct Nullable
{
private:
    struct Dummy { Dummy() {} };

    union
    {
        T _value;
        Dummy _dummy;
    };
    bool _hasValue;

public:
    /// <summary>
    /// Initializes <see cref="Nullable{T}"/> by setting the wrapped value to null.
    /// </summary>
    Nullable()
        : _dummy()
        , _hasValue(false)
    {
    }

    ~Nullable()
    {
        if (_hasValue)
            _value.~T();
    }

    /// <summary>
    /// Initializes <see cref="Nullable{T}"/> by copying the wrapped value.
    /// </summary>
    /// <NullableBase name="value">The initial wrapped value to be copied.</param>
    template<typename U = T, typename = typename TEnableIf<TIsCopyConstructible<U>::Value>::Type>
    Nullable(const T& value)
        : _value(value)
        , _hasValue(true)
    {
    }

    /// <summary>
    /// Initializes <see cref="Nullable{T}"/> by moving the wrapped value.
    /// </summary>
    /// <NullableBase name="value">The initial wrapped value to be moved.</param>
    Nullable(T&& value) noexcept
        : _value(MoveTemp(value))
        , _hasValue(true)
    {
    }

    /// <summary>
    /// Initializes <see cref="Nullable{T}"/> by copying another <see cref="Nullable{T}"/>.
    /// </summary>
    /// <param name="other">The wrapped value to be copied.</param>
    template<typename U = T, typename = typename TEnableIf<TIsCopyConstructible<U>::Value>::Type>
    Nullable(const Nullable& other)
        : _value(other._value)
        , _hasValue(other._hasValue)
    {
    }

    /// <summary>
    /// Initializes <see cref="Nullable{T}"/> by moving another <see cref="Nullable{T}"/>.
    /// </summary>
    /// <param name="other">The wrapped value to be moved.</param>
    Nullable(Nullable&& other) noexcept
    {
        if (other._hasValue)
            new (&_value) T(MoveTemp(other._value)); // Placement new (move constructor)
        _hasValue = other._hasValue;

        other.Reset();
    }

    /// <summary>
    /// Reassigns the wrapped value by copying.
    /// </summary>
    template<typename U = T, typename = typename TEnableIf<TIsCopyConstructible<U>::Value>::Type>
    Nullable& operator=(const T& value)
    {
        if (_hasValue)
            _value.~T();

        new (&_value) T(value); // Placement new (copy constructor)
        _hasValue = true;

        return *this;
    }

    /// <summary>
    /// Reassigns the wrapped value by moving.
    /// </summary>
    Nullable& operator=(T&& value) noexcept
    {
        if (_hasValue)
            _value.~T();

        new (&_value) T(MoveTemp(value)); // Placement new (move constructor)
        _hasValue = true;

        return *this;
    }

    /// <summary>
    /// Reassigns the wrapped value by copying another <see cref="Nullable{T}"/>.
    /// </summary>
    template<typename U = T, typename = typename TEnableIf<TIsCopyConstructible<U>::Value>::Type>
    Nullable& operator=(const Nullable& other)
    {
        if (_hasValue)
            _value.~T();

        if (other._hasValue)
            new (&_value) T(other._value); // Placement new (copy constructor)

        _hasValue = other._hasValue; // Set the flag AFTER the value is copied.

        return *this;
    }

    /// <summary>
    /// Reassigns the wrapped value by moving another <see cref="Nullable{T}"/>.
    /// </summary>
    Nullable& operator=(Nullable&& other) noexcept
    {
        if (this == &other)
            return *this;

        if (_hasValue)
            _value.~T();

        if (other._hasValue)
        {
            new (&_value) T(MoveTemp(other._value)); // Placement new (move constructor)

            other._value.~T(); // Kill the old value in the source object.
            other._hasValue = false;
        }

        _hasValue = other._hasValue; // Set the flag AFTER the value is moved.

        return *this;
    }

    /// <summary>
    /// Checks if wrapped object has a valid value.
    /// </summary>
    /// <returns><c>true</c> if this object has a valid value; otherwise, <c>false</c>.</returns>
    FORCE_INLINE bool HasValue() const
    {
        return _hasValue;
    }

    /// <summary>
    /// Gets a const reference to the wrapped value. If the value is not valid, the behavior is undefined.
    /// </summary>
    /// <returns>Reference to the wrapped value.</returns>
    FORCE_INLINE const T& GetValue() const
    {
        ASSERT(_hasValue);
        return _value;
    }

    /// <summary>
    /// Gets a reference to the wrapped value. If the value is not valid, the behavior is undefined.
    /// This method can be used to reassign the wrapped value.
    /// </summary>
    /// <returns>Reference to the wrapped value.</returns>
    FORCE_INLINE T& GetValue()
    {
        ASSERT(_hasValue);
        return _value;
    }

    /// <summary>
    /// Gets a const reference to the wrapped value or a default value if the value is not valid.
    /// </summary>
    /// <returns>Reference to the wrapped value or the default value.</returns>
    FORCE_INLINE const T& GetValueOr(const T& defaultValue) const
    {
        return _hasValue ? _value : defaultValue;
    }

    /// <summary>
    /// Gets a mutable reference to the wrapped value or a default value if the value is not valid.
    /// </summary>
    /// <returns>Reference to the wrapped value or the default value.</returns>
    FORCE_INLINE T& GetValueOr(T& defaultValue) const
    {
        return _hasValue ? _value : defaultValue;
    }

    /// <summary>
    /// Sets the wrapped value by copying.
    /// </summary>
    /// <param name="value">The value to be copied.</param>
    template<typename U = T, typename = typename TEnableIf<TIsCopyConstructible<U>::Value>::Type>
    FORCE_INLINE void SetValue(const T& value)
    {
        if (_hasValue)
            _value.~T();
        new (&_value) T(value); // Placement new (copy constructor)
        _hasValue = true; // Set the flag AFTER the value is copied.
    }

    /// <summary>
    /// Sets the wrapped value by moving.
    /// </summary>
    /// <param name="value">The value to be moved.</param>
    FORCE_INLINE void SetValue(T&& value) noexcept
    {
        if (_hasValue)
            _value.~T();
        new (&_value) T(MoveTemp(value)); // Placement new (move constructor)
        _hasValue = true; // Set the flag AFTER the value is moved.
    }

    /// <summary>
    /// If the wrapped value is not valid, sets it by copying. Otherwise, does nothing.
    /// </summary>
    /// <returns>True if the wrapped value was changed, otherwise false.</returns>
    template<typename U = T, typename = typename TEnableIf<TIsCopyConstructible<U>::Value>::Type>
    FORCE_INLINE bool TrySet(const T& value)
    {
        if (_hasValue)
            return false;
        new (&_value) T(value); // Placement new (copy constructor)
        _hasValue = true; // Set the flag AFTER the value is copied.
        return true;
    }

    /// <summary>
    /// If the wrapped value is not valid, sets it by moving. Otherwise, does nothing.
    /// </summary>
    /// <returns>True if the wrapped value was changed, otherwise false.</returns>
    FORCE_INLINE bool TrySet(T&& value) noexcept
    {
        if (_hasValue)
            return false;
        new (&_value) T(MoveTemp(value)); // Placement new (move constructor)
        _hasValue = true; // Set the flag AFTER the value is moved.
        return true;
    }

    /// <summary>
    /// Disposes the wrapped value and sets the wrapped value to null. If the wrapped value is not valid, does nothing.
    /// </summary>
    FORCE_INLINE void Reset()
    {
        if (!_hasValue)
            return;
        _hasValue = false; // Reset the flag BEFORE the value is (potentially) destructed.
        _value.~T();
    }

    /// <summary>
    /// Moves the wrapped value to the output parameter and sets the wrapped value to null. If the wrapped value is not valid, the behavior is undefined.
    /// </summary>
    /// <param name="value">The output parameter that will receive the wrapped value.</param>
    FORCE_INLINE void GetAndReset(T& value)
    {
        ASSERT(_hasValue);
        value = MoveTemp(_value);
        Reset();
    }

    /// <summary>
    /// Indicates whether this instance is equal to other one.
    /// </summary>
    /// <param name="other">The other object.</param>
    /// <returns><c>true</c> if both values are equal.</returns>
    FORCE_INLINE bool operator==(const Nullable& other) const
    {
        return other._hasValue == _hasValue && _value == other._value;
    }

    /// <summary>
    /// Indicates whether this instance is NOT equal to other one.
    /// </summary>
    /// <param name="other">The other object.</param>
    /// <returns><c>true</c> if both values are not equal.</returns>
    FORCE_INLINE bool operator!=(const Nullable& other) const
    {
        return !operator==(other);
    }

    /// <summary>
    /// Explicit conversion to boolean value. Allows to check if the wrapped value is valid in if-statements without casting.
    /// </summary>
    /// <returns><c>true</c> if this object has a valid value, otherwise false</returns>
    FORCE_INLINE explicit operator bool() const
    {
        return _hasValue;
    }

    /// <summary>
    /// Matches the wrapped value with a handler for the value or a handler for the null value.
    /// </summary>
    /// <param name="valueHandler">Value visitor handling valid nullable value.</param>
    /// <param name="nullHandler">Null visitor handling invalid nullable value.</param>
    /// <returns>Result of the call of one of handlers. Handlers must share the same result type.</returns>
    template<typename ValueVisitor, typename NullVisitor>
    FORCE_INLINE auto Match(ValueVisitor valueHandler, NullVisitor nullHandler) const
    {
        if (_hasValue)
            return valueHandler(_value);
        return nullHandler();
    }
};

/// <summary>
/// Specialization of <see cref="Nullable{T}"/> for <see cref="bool"/> type.
/// </summary>
template<>
struct Nullable<bool>
{
private:
    /// <summary>
    /// Underlying value of the nullable boolean. Uses only one byte to optimize memory usage.
    /// </summary>
    enum class Value : uint8
    {
        Null,
        False,
        True,
    };

    Value _value = Value::Null;

public:
    /// <summary>
    /// Initializes nullable boolean by setting the wrapped value to null.
    /// </summary>
    Nullable() = default;

    ~Nullable() = default;

    /// <summary>
    /// Initializes nullable boolean by moving another nullable boolean.
    /// </summary>
    Nullable(Nullable&& value) = default;

    /// <summary>
    /// Initializes nullable boolean by copying another nullable boolean.
    /// </summary>
    Nullable(const Nullable& value) = default;

    /// <summary>
    /// Initializes nullable boolean by implicitly casting a boolean value.
    /// </summary>
    Nullable(const bool value) noexcept
    {
        _value = value ? Value::True : Value::False;
    }

    /// <summary>
    /// Reassigns the wrapped value by implicitly casting a boolean value.
    /// </summary>
    Nullable& operator=(const bool value) noexcept
    {
        _value = value ? Value::True : Value::False;
        return *this;
    }

    /// <summary>
    /// Reassigns the wrapped value by copying another nullable boolean.
    /// </summary>
    Nullable& operator=(const Nullable& value) = default;

    /// <summary>
    /// Reassigns the wrapped value by moving another nullable boolean.
    /// </summary>
    Nullable& operator=(Nullable&& value) = default;

    /// <summary>
    /// Checks if wrapped bool has a valid value.
    /// </summary>
    FORCE_INLINE bool HasValue() const noexcept
    {
        return _value != Value::Null;
    }

    /// <summary>
    /// Gets the wrapped boolean value. If the value is not valid, the behavior is undefined.
    /// </summary>
    FORCE_INLINE bool GetValue() const
    {
        ASSERT(_value != Value::Null);
        return _value == Value::True;
    }

    /// <summary>
    /// Gets the wrapped boolean value. If the value is not valid, returns the default value.
    /// </summary>
    FORCE_INLINE bool GetValueOr(const bool defaultValue) const noexcept
    {
        return _value == Value::Null ? defaultValue : _value == Value::True;
    }

    /// <summary>
    /// Sets the wrapped value to a valid boolean.
    /// </summary>
    FORCE_INLINE void SetValue(const bool value) noexcept
    {
        _value = value ? Value::True : Value::False;
    }

    /// <summary>
    /// If the wrapped value is not valid, sets it to a valid boolean.
    /// </summary>
    FORCE_INLINE bool TrySet(const bool value) noexcept
    {
        if (_value != Value::Null)
        {
            return false;
        }

        _value = value ? Value::True : Value::False;
        return true;
    }

    /// <summary>
    /// Sets the wrapped bool to null.
    /// </summary>
    FORCE_INLINE void Reset() noexcept
    {
        _value = Value::Null;
    }

    /// <summary>
    /// Moves the wrapped value to the output parameter and sets the wrapped value to null. If the wrapped value is not valid, the behavior is undefined.
    /// </summary>
    FORCE_INLINE void GetAndReset(bool& value) noexcept
    {
        ASSERT(_value != Value::Null);
        value = _value == Value::True;
        _value = Value::Null;
    }

    /// <summary>
    /// Checks if the current object has a valid value and it's set to <c>true</c>. If the value is <c>false</c> or not valid, the method returns <c>false</c>.
    /// </summary>
    FORCE_INLINE bool IsTrue() const
    {
        return _value == Value::True;
    }

    /// <summary>
    /// Checks if the current object has a valid value and it's set to <c>false</c>. If the value is <c>true</c> or not valid, the method returns <c>false</c>.
    /// </summary>
    FORCE_INLINE bool IsFalse() const
    {
        return _value == Value::False;
    }

    /// <summary>
    /// Deletes implicit conversion to bool to prevent ambiguous code.
    /// </summary>
    /// <remarks>
    /// Implicit cast from nullable bool to a bool produces unacceptably ambiguous code. For template meta-programming use explicit <c>HasValue</c> instead.
    /// </remarks>
    explicit operator bool() const = delete;
};
