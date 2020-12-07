// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

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
    /// <param name="key">The key.</param>
    Pair(const T& key)
        : First(key)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Pair"/> class.
    /// </summary>
    /// <param name="p">The other pair.</param>
    Pair(const Pair& p)
        : First(p.First)
        , Second(p.Second)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Pair"/> class.
    /// </summary>
    /// <param name="p">The other pair.</param>
    Pair(Pair* p)
        : First(p->First)
        , Second(p->Second)
    {
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="Pair"/> class.
    /// </summary>
    ~Pair()
    {
    }

public:

    friend bool operator==(const Pair& a, const Pair& b)
    {
        return a.First == b.First && a.Second == b.Second;
    }

    friend bool operator!=(const Pair& a, const Pair& b)
    {
        return a.First != b.First || a.Second != b.Second;
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
