// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"

/// <summary>
/// Contains information about CPU (Central Processing Unit).
/// </summary>
API_STRUCT(NoDefault) struct CPUInfo
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(CPUInfo);

    /// <summary>
    /// The number of physical processor packages.
    /// </summary>
    API_FIELD() uint32 ProcessorPackageCount;

    /// <summary>
    /// The number of processor cores (physical).
    /// </summary>
    API_FIELD() uint32 ProcessorCoreCount;

    /// <summary>
    /// The number of logical processors (including hyper-threading).
    /// </summary>
    API_FIELD() uint32 LogicalProcessorCount;

    /// <summary>
    /// The size of processor L1 caches (in bytes).
    /// </summary>
    API_FIELD() uint32 L1CacheSize;

    /// <summary>
    /// The size of processor L2 caches (in bytes).
    /// </summary>
    API_FIELD() uint32 L2CacheSize;

    /// <summary>
    /// The size of processor L3 caches (in bytes).
    /// </summary>
    API_FIELD() uint32 L3CacheSize;

    /// <summary>
    /// The CPU memory page size (in bytes).
    /// </summary>
    API_FIELD() uint32 PageSize;

    /// <summary>
    /// The CPU clock speed (in Hz).
    /// </summary>
    API_FIELD() uint64 ClockSpeed;

    /// <summary>
    /// The CPU cache line size (in bytes).
    /// </summary>
    API_FIELD() uint32 CacheLineSize;
};
