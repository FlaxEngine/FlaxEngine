// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_PROFILER

#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Platform/MemoryStats.h"
#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Profiler/Profiler.h"

/// <summary>
/// Profiler tools for development. Allows to gather profiling data and events from the engine.
/// </summary>
API_CLASS(Static) class ProfilingTools
{
DECLARE_SCRIPTING_TYPE_NO_SPAWN(ProfilingTools);
public:

    /// <summary>
    /// The GPU memory stats.
    /// </summary>
    API_STRUCT() struct MemoryStatsGPU
    {
    DECLARE_SCRIPTING_TYPE_MINIMAL(MemoryStatsGPU);

        /// <summary>
        /// The total amount of memory in bytes (as reported by the driver).
        /// </summary>
        API_FIELD() uint64 Total;

        /// <summary>
        /// The used by the game amount of memory in bytes (estimated).
        /// </summary>
        API_FIELD() uint64 Used;
    };

    /// <summary>
    /// Engine profiling data header. Contains main info and stats.
    /// </summary>
    API_STRUCT() struct MainStats
    {
    DECLARE_SCRIPTING_TYPE_MINIMAL(MainStats);

        /// <summary>
        /// The process memory stats.
        /// </summary>
        API_FIELD() ProcessMemoryStats ProcessMemory;

        /// <summary>
        /// The CPU memory stats.
        /// </summary>
        API_FIELD() MemoryStats MemoryCPU;

        /// <summary>
        /// The GPU memory stats.
        /// </summary>
        API_FIELD() MemoryStatsGPU MemoryGPU;

        /// <summary>
        /// The frames per second (fps counter).
        /// </summary>
        API_FIELD() int32 FPS;

        /// <summary>
        /// The update time on CPU (in milliseconds).
        /// </summary>
        API_FIELD() float UpdateTimeMs;

        /// <summary>
        /// The fixed update time on CPU (in milliseconds).
        /// </summary>
        API_FIELD() float PhysicsTimeMs;

        /// <summary>
        /// The draw time on CPU (in milliseconds).
        /// </summary>
        API_FIELD() float DrawCPUTimeMs;

        /// <summary>
        /// The draw time on GPU (in milliseconds).
        /// </summary>
        API_FIELD() float DrawGPUTimeMs;

        /// <summary>
        /// The last rendered frame stats.
        /// </summary>
        API_FIELD() RenderStatsData DrawStats;
    };

    /// <summary>
    /// The CPU thread stats.
    /// </summary>
    API_STRUCT() struct ThreadStats
    {
    DECLARE_SCRIPTING_TYPE_MINIMAL(ThreadStats);

        /// <summary>
        /// The thread name.
        /// </summary>
        API_FIELD() String Name;

        /// <summary>
        /// The events list.
        /// </summary>
        API_FIELD() Array<ProfilerCPU::Event> Events;
    };

public:

    /// <summary>
    /// The current collected main stats by the profiler from the local session. Updated every frame.
    /// </summary>
    API_FIELD(ReadOnly) static MainStats Stats;

    /// <summary>
    /// The CPU threads profiler events.
    /// </summary>
    API_FIELD(ReadOnly) static Array<ThreadStats, InlinedAllocation<64>> EventsCPU;

    /// <summary>
    /// The GPU rendering profiler events.
    /// </summary>
    API_FIELD(ReadOnly) static Array<ProfilerGPU::Event> EventsGPU;
};

#endif
