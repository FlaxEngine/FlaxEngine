// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"

/// <summary>
/// Contains information about current memory usage and capacity.
/// </summary>
API_STRUCT(NoDefault) struct MemoryStats
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(MemoryStats);

    /// <summary>
    /// Total amount of physical memory in bytes.
    /// </summary>
    API_FIELD() uint64 TotalPhysicalMemory = 0;

    /// <summary>
    /// Amount of used physical memory in bytes.
    /// </summary>
    API_FIELD() uint64 UsedPhysicalMemory = 0;

    /// <summary>
    /// Total amount of virtual memory in bytes.
    /// </summary>
    API_FIELD() uint64 TotalVirtualMemory = 0;

    /// <summary>
    /// Amount of used virtual memory in bytes.
    /// </summary>
    API_FIELD() uint64 UsedVirtualMemory = 0;

    /// <summary>
    /// Amount of memory used initially by the program data (executable code, exclusive shared libraries and global static data sections).
    /// </summary>
    API_FIELD() uint64 ProgramSizeMemory = 0;

    /// <summary>
    /// Amount of extra memory assigned by the platform for development. Only used on platforms with fixed memory and no paging.
    /// </summary>
    API_FIELD() uint64 ExtraDevelopmentMemory = 0;
};

/// <summary>
/// Contains information about current memory usage by the process.
/// </summary>
API_STRUCT(NoDefault) struct ProcessMemoryStats
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(ProcessMemoryStats);

    /// <summary>
    /// Amount of used physical memory in bytes.
    /// </summary>
    API_FIELD() uint64 UsedPhysicalMemory = 0;

    /// <summary>
    /// Amount of used virtual memory in bytes.
    /// </summary>
    API_FIELD() uint64 UsedVirtualMemory = 0;
};
