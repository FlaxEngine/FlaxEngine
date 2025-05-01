// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Templates.h"
#include "Engine/Core/Collections/HashFunctions.h"

/// <summary>
/// Pair of two values.
/// </summary>
template<typename T, typename U>
class Pair
{
public:
    /// <summary>
    /// The first element.
    /// </summary>
    T First;

    /// <summary>
    /// The second element.
    /// </summary>
    U Second;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="Pair"/> class.
    /// </summary>
    Pair()
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Pair"/> class.
    /// </summary>
    /// <param name="key">The key.</param>
    /// <param name="value">The value.</param>
    Pair(const T& key, const U& value)
        : First(key)
        , Second(value)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Pair"/> class.
    /// </summary>
    /// <param name="other">The other pair.</param>
    Pair(const Pair& other)
        : First(other.First)
        , Second(other.Second)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Pair"/> class.
    /// </summary>
    /// <param name="other">The other pair.</param>
    Pair(Pair&& other) noexcept
        : First(MoveTemp(other.First))
        , Second(MoveTemp(other.Second))
    {
    }

public:
    Pair& operator=(const Pair& other)
    {
        if (this == &other)
            return *this;
        First = other.First;
        Second = other.Second;
        return *this;
    }

    Pair& operator=(Pair&& other) noexcept
    {
        if (this != &other)
        {
            First = MoveTemp(other.First);
            Second = MoveTemp(other.Second);
        }
        return *this;
    }

    bool operator==(const Pair& other) const
    {
        return First == other.First && Second == other.Second;
    }

    bool operator!=(const Pair& other) const
    {
        return First != other.First || Second != other.Second;
    }
};

template<class T, class U>
inline uint32 GetHash(const Pair<T, U>& key)
{
    uint32 hash = GetHash(key.First);
    CombineHash(hash, GetHash(key.Second));
    return hash;
}

template<class T, class U>
inline constexpr Pair<T, U> ToPair(const T& key, const U& value)
{
    return Pair<T, U>(key, value);
}
