// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "MTypes.h"

/// <summary>
/// Contains information about managed type
/// </summary>
class MType
{
public:

#if USE_MONO

    MonoType* _monoType;

    /// <summary>
    /// Initializes a new instance of the <see cref="MType"/> class.
    /// </summary>
    /// <param name="monoType">The Mono type.</param>
    MType(MonoType* monoType)
        : _monoType(monoType)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="MType"/> class.
    /// </summary>
    MType()
        : _monoType(nullptr)
    {
    }

#endif

    /// <summary>
    /// Finalizes an instance of the <see cref="MType"/> class.
    /// </summary>
    ~MType()
    {
    }

public:

    String ToString() const;

#if USE_MONO

    /// <summary>
    /// Gets mono type handle
    /// </summary>
    /// <returns>Mono type</returns>
    MonoType* GetNative() const
    {
        return _monoType;
    }

    bool IsStruct() const;
    bool IsVoid() const;
    bool IsPointer() const;
    bool IsReference() const;
    bool IsByRef() const;

public:

    FORCE_INLINE bool operator==(const MType& other) const
    {
        return _monoType == other._monoType;
    }

    FORCE_INLINE bool operator!=(const MType& other) const
    {
        return _monoType != other._monoType;
    }

    FORCE_INLINE operator bool() const
    {
        return _monoType != nullptr;
    }

#endif
};
