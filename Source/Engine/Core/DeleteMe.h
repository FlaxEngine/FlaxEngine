// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Compiler.h"

/// <summary>
/// Helper class used to delete another object.
/// </summary>
template<class T, class MemoryAllocator = Allocator>
class DeleteMe
{
private:
    T* _toDelete;

    DeleteMe()
    {
    }

    DeleteMe(const DeleteMe&)
    {
    };

    DeleteMe(DeleteMe&&)
    {
    };

    DeleteMe& operator=(const DeleteMe&)
    {
        return *this;
    };

    DeleteMe& operator=(DeleteMe&&)
    {
        return *this;
    };

public:
    /// <summary>
    /// Init
    /// </summary>
    /// <param name="toDelete">Object to delete</param>
    explicit DeleteMe(T* toDelete)
        : _toDelete(toDelete)
    {
    }

    /// <summary>
    /// Destructor
    /// </summary>
    ~DeleteMe()
    {
        if (_toDelete)
            ::Delete<T, MemoryAllocator>(_toDelete);
    }

public:
    /// <summary>
    /// Delete linked object and assign new one
    /// </summary>
    /// <param name="other">New object</param>
    void DeleteAndSetNew(T* other)
    {
        ASSERT(other);
        if (_toDelete)
        {
            ::Delete<T, MemoryAllocator>(_toDelete);
        }
        _toDelete = other;
    }

    /// <summary>
    /// Delete linked object
    /// </summary>
    void Delete()
    {
        if (_toDelete)
        {
            ::Delete<T, MemoryAllocator>(_toDelete);
            _toDelete = nullptr;
        }
    }

    FORCE_INLINE T* Get() const
    {
        return _toDelete;
    }

public:
    T* operator->()
    {
        return _toDelete;
    }

    const T* operator->() const
    {
        return _toDelete;
    }

    T* operator*()
    {
        return _toDelete;
    }

    const T& operator*() const
    {
        return *_toDelete;
    }
};
