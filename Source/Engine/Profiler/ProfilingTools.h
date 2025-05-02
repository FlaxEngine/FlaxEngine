// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_PROFILER

#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Platform/MemoryStats.h"
#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Profiler/Profiler.h"

/// <summary>
/// Profiler tools for development. Allows to gather profiling data and events from the engine.
/// </summary>
API_CLASS(Static) class FLAXENGINE_API ProfilingTools
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(ProfilingTools);
public:
    /// <summary>
    /// The GPU memory stats.
    /// </summary>
    API_STRUCT(NoDefault) struct MemoryStatsGPU
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
    API_STRUCT(NoDefault) struct MainStats
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
    API_STRUCT(NoDefault) struct ThreadStats
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

    /// <summary>
    /// The network stat.
    /// </summary>
    API_STRUCT(NoDefault) struct NetworkEventStat
    {
        DECLARE_SCRIPTING_TYPE_MINIMAL(NetworkEventStat);

        // Amount of occurrences.
        API_FIELD() uint16 Count;
        // Transferred data size (in bytes).
        API_FIELD() uint16 DataSize;
        // Transferred message (data+header) size (in bytes).
        API_FIELD() uint16 MessageSize;
        // Amount of peers that will receive this message.
        API_FIELD() uint16 Receivers;
        API_FIELD(Private, NoArray) byte Name[120];
    };

public:
    /// <summary>
    /// Controls the engine profiler (CPU, GPU, etc.) usage.
    /// </summary>
    API_PROPERTY(Attributes="DebugCommand") static bool GetEnabled();

    /// <summary>
    /// Controls the engine profiler (CPU, GPU, etc.) usage.
    /// </summary>
    API_PROPERTY() static void SetEnabled(bool enabled);

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

    /// <summary>
    /// The networking profiler events.
    /// </summary>
    API_FIELD(ReadOnly) static Array<NetworkEventStat> EventsNetwork;
};

#endif
