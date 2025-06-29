// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Core.h"
#include "Engine/Core/Types/BaseTypes.h"

/// <summary>
/// Utility for guarding system data access from different threads depending on the resources usage (eg. block read on write).
/// </summary>
struct ConcurrentSystemLocker
{
private:
    volatile int64 _counters[2];

public:
    NON_COPYABLE(ConcurrentSystemLocker);
    ConcurrentSystemLocker();

    void Begin(bool write, bool exclusively = false);
    void End(bool write);

public:
    template<bool Write>
    struct Scope
    {
        NON_COPYABLE(Scope);

        Scope(ConcurrentSystemLocker& locker, bool exclusively = false)
            : _locker(locker)
        {
            _locker.Begin(Write, exclusively);
        }

        ~Scope()
        {
            _locker.End(Write);
        }

    private:
        ConcurrentSystemLocker& _locker;
    };

    typedef Scope<false> ReadScope;
    typedef Scope<true> WriteScope;
};
