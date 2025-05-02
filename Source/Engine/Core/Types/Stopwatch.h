// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "BaseTypes.h"
#include "Engine/Platform/Platform.h"

/// <summary>
/// High-resolution performance counter based on Platform::GetTimeSeconds.
/// </summary>
API_STRUCT(InBuild, Namespace="System.Diagnostics") struct FLAXENGINE_API Stopwatch
{
private:
    double _start, _end;

public:
    Stopwatch()
    {
        _start = _end = Platform::GetTimeSeconds();
    }

public:
    // Starts the counter.
    void Start()
    {
        _start = Platform::GetTimeSeconds();
    }

    // Stops the counter.
    void Stop()
    {
        _end = Platform::GetTimeSeconds();
    }

    /// <summary>
    /// Gets the milliseconds time.
    /// </summary>
    FORCE_INLINE int32 GetMilliseconds() const
    {
        return (int32)((_end - _start) * 1000.0);
    }

    /// <summary>
    /// Gets the total number of milliseconds.
    /// </summary>
    FORCE_INLINE double GetTotalMilliseconds() const
    {
        return (float)((_end - _start) * 1000.0);
    }

    /// <summary>
    /// Gets the total number of seconds.
    /// </summary>
    FORCE_INLINE float GetTotalSeconds() const
    {
        return (float)(_end - _start);
    }
};

template<>
struct TIsPODType<Stopwatch>
{
    enum { Value = true };
};
