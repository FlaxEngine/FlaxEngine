// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"

/// <summary>
/// Contains information about current memory usage and capacity.
/// </summary>
API_STRUCT() struct MemoryStats
{
DECLARE_SCRIPTING_TYPE_MINIMAL(MemoryStats);

    /// <summary>
    /// Total amount of physical memory in bytes.
    /// </summary>
    API_FIELD() uint64 TotalPhysicalMemory;

    /// <summary>
    /// Amount of used physical memory in bytes.
    /// </summary>
    API_FIELD() uint64 UsedPhysicalMemory;

    /// <summary>
    /// Total amount of virtual memory in bytes.
    /// </summary>
    API_FIELD() uint64 TotalVirtualMemory;

    /// <summary>
    /// Amount of used virtual memory in bytes.
    /// </summary>
    API_FIELD() uint64 UsedVirtualMemory;
};

/// <summary>
/// Contains information about current memory usage by the process.
/// </summary>
API_STRUCT() struct ProcessMemoryStats
{
DECLARE_SCRIPTING_TYPE_MINIMAL(ProcessMemoryStats);

    /// <summary>
    /// Amount of used physical memory in bytes.
    /// </summary>
    API_FIELD() uint64 UsedPhysicalMemory;

    /// <summary>
    /// Amount of used virtual memory in bytes.
    /// </summary>
    API_FIELD() uint64 UsedVirtualMemory;
};
