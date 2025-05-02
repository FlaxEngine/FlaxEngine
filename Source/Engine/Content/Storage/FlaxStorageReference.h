// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "FlaxStorage.h"

/// <summary>
/// Flax Storage container reference.
/// </summary>
struct FLAXENGINE_API FlaxStorageReference
{
private:
    FlaxStorage* _storage;

public:
    FlaxStorageReference(FlaxStorage* storage)
        : _storage(storage)
    {
        if (_storage)
            _storage->AddRef();
    }

    FlaxStorageReference(const FlaxStorageReference& other)
        : _storage(other.Get())
    {
        if (_storage)
            _storage->AddRef();
    }

    ~FlaxStorageReference()
    {
        if (_storage)
            _storage->RemoveRef();
    }

public:
    FORCE_INLINE FlaxStorage* Get() const
    {
        return _storage;
    }

public:
    FlaxStorageReference& operator=(const FlaxStorageReference& other)
    {
        if (this != &other)
        {
            if (_storage)
                _storage->RemoveRef();
            _storage = other._storage;
            if (_storage)
                _storage->AddRef();
        }
        return *this;
    }

    FORCE_INLINE operator bool() const
    {
        return _storage != nullptr;
    }

    FORCE_INLINE bool operator==(const FlaxStorageReference& other) const
    {
        return _storage == other._storage;
    }

    FORCE_INLINE bool operator!=(const FlaxStorageReference& other) const
    {
        return _storage != other._storage;
    }

    FORCE_INLINE FlaxStorage* operator->() const
    {
        return _storage;
    }
};
