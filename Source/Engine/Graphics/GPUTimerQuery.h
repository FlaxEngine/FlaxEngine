// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "GPUResource.h"

/// <summary>
/// Represents a GPU query that measures execution time of GPU operations.
/// The query will measure any GPU operations that take place between its Begin() and End() calls.
/// </summary>
/// <seealso cref="GPUResource" />
class FLAXENGINE_API GPUTimerQuery : public GPUResource
{
public:
    /// <summary>
    /// Finalizes an instance of the <see cref="GPUTimerQuery"/> class.
    /// </summary>
    virtual ~GPUTimerQuery()
    {
    }

public:
    /// <summary>
    /// Starts the counter.
    /// </summary>
    virtual void Begin() = 0;

    /// <summary>
    /// Stops the counter. Can be called more than once without failing.
    /// </summary>
    virtual void End() = 0;

    /// <summary>
    /// Determines whether this query has been completed and has valid result to gather.
    /// </summary>
    /// <returns><c>true</c> if this query has result; otherwise, <c>false</c>.</returns>
    virtual bool HasResult() = 0;

    /// <summary>
    /// Gets the query result time (in milliseconds) it took to execute GPU commands between Begin/End calls.
    /// </summary>
    /// <returns>The time in milliseconds.</returns>
    virtual float GetResult() = 0;

public:
    // [GPUResource]
    String ToString() const override
    {
        return TEXT("TimerQuery");
    }
    GPUResourceType GetResourceType() const final override
    {
        return GPUResourceType::Query;
    }
};
