// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Log.h"
#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Collections/HashSet.h"
#include "Engine/Platform/Platform.h"
#include "Engine/Platform/CriticalSection.h"

/// <summary>
/// The utility for finding double-free or invalid malloc calls.
/// </summary>
class MallocTester
{
private:
    bool _entry = true;
    CriticalSection _locker;
    HashSet<void*> _allocs;

public:
    bool OnMalloc(void* ptr, uint64 size)
    {
        if (ptr == nullptr)
            return false;
        bool failed = false;
        _locker.Lock();
        if (_entry)
        {
            _entry = false;
            if (_allocs.Contains(ptr))
            {
                failed = true;
            }
            _allocs.Add(ptr);
            _entry = true;
        }
        _locker.Unlock();
        return failed;
    }

    bool OnFree(void* ptr)
    {
        if (ptr == nullptr)
            return false;
        bool failed = false;
        _locker.Lock();
        if (_entry)
        {
            _entry = false;
            if (!_allocs.Remove(ptr))
            {
                failed = true;
            }
            _entry = true;
        }
        _locker.Unlock();
        return failed;
    }
};
