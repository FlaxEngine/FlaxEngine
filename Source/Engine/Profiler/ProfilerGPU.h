// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/NonCopyable.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Scripting/ScriptingType.h"
#include "RenderStats.h"

class GPUTimerQuery;

#if COMPILE_WITH_PROFILER

// Profiler events buffers capacity (tweaked manually)
#define PROFILER_GPU_EVENTS_FRAMES 6

/// <summary>
/// Provides GPU performance measuring methods.
/// </summary>
API_CLASS(Static) class FLAXENGINE_API ProfilerGPU
{
DECLARE_SCRIPTING_TYPE_NO_SPAWN(ProfilerGPU);
public:

    /// <summary>
    /// Represents single CPU profiling event data.
    /// </summary>
    API_STRUCT() struct Event
    {
    DECLARE_SCRIPTING_TYPE_MINIMAL(Event);

        /// <summary>
        /// The name of the event.
        /// </summary>
        API_FIELD() const Char* Name;

        /// <summary>
        /// The timer query used to get the exact event time on a GPU. Assigned and managed by the internal profiler layer.
        /// </summary>
        API_FIELD() GPUTimerQuery* Timer;

        /// <summary>
        /// The rendering stats for this event. When event is active it holds the stats on event begin.
        /// </summary>
        API_FIELD() RenderStatsData Stats;

        /// <summary>
        /// The event execution time on a GPU (in milliseconds).
        /// </summary>
        API_FIELD() float Time;

        /// <summary>
        /// The event depth. Value 0 is used for the root events.
        /// </summary>
        API_FIELD() int32 Depth;
    };

    /// <summary>
    /// Implements simple profiling events buffer that holds single frame data.
    /// </summary>
    class EventBuffer : public NonCopyable
    {
    private:

        bool _isResolved = true;
        Array<Event> _data;

    public:

        /// <summary>
        /// The index of the frame buffer was used for recording events (for the last time).
        /// </summary>
        uint64 FrameIndex;

        /// <summary>
        /// Determines whether this buffer has ready data (resolved and not empty).
        /// </summary>
        bool HasData() const;

        /// <summary>
        /// Ends all used timer queries.
        /// </summary>
        void EndAll();

        /// <summary>
        /// Tries the resolve this frame. Skips if already resolved or has no collected events.
        /// </summary>
        void TryResolve();

        /// <summary>
        /// Gets the event at the specified index.
        /// </summary>
        /// <param name="index">The index.</param>
        /// <returns>The event</returns>
        Event* Get(const int32 index)
        {
            return &_data[index];
        }

        /// <summary>
        /// Adds new event to the buffer.
        /// </summary>
        /// <param name="e">The initial event data.</param>
        /// <returns>The event index.</returns>
        int32 Add(const Event& e);

        /// <summary>
        /// Extracts the buffer data.
        /// </summary>
        /// <param name="data">The output data.</param>
        void Extract(Array<Event>& data) const;

        /// <summary>
        /// Clears this buffer.
        /// </summary>
        void Clear();
    };

private:

    static int32 _depth;

    static Array<GPUTimerQuery*> _timerQueriesPool;
    static Array<GPUTimerQuery*> _timerQueriesFree;

    static GPUTimerQuery* GetTimerQuery();

public:

    /// <summary>
    /// True if GPU profiling is enabled, otherwise false to disable events collecting and GPU timer queries usage. Can be changed during rendering.
    /// </summary>
    static bool Enabled;

    /// <summary>
    /// The current frame buffer to collect events.
    /// </summary>
    static int32 CurrentBuffer;

    /// <summary>
    /// The events buffers (one per frame).
    /// </summary>
    static EventBuffer Buffers[PROFILER_GPU_EVENTS_FRAMES];

public:

    /// <summary>
    /// Begins the event. Call EndEvent with index parameter equal to the returned value by BeginEvent function.
    /// </summary>
    /// <param name="name">The event name.</param>
    /// <returns>The event token index</returns>
    static int32 BeginEvent(const Char* name);

    /// <summary>
    /// Ends the active event.
    /// </summary>
    /// <param name="index">The event token index returned by the BeginEvent method.</param>
    static void EndEvent(int32 index);

    /// <summary>
    /// Begins the new frame rendering. Called by the engine to sync profiling data.
    /// </summary>
    static void BeginFrame();

    /// <summary>
    /// Called when just before flushing current frame GPU commands (via Present or Flush). Call active timer queries should be ended now.
    /// </summary>
    static void OnPresent();

    /// <summary>
    /// Ends the frame rendering. Called by the engine to sync profiling data.
    /// </summary>
    static void EndFrame();

    /// <summary>
    /// Tries to get the rendering stats from the last frame drawing (that has been resolved and has valid data).
    /// </summary>
    /// <param name="drawTimeMs">The draw execution time on a GPU (in milliseconds).</param>
    /// <param name="statsData">The rendering stats data.</param>
    /// <returns>True if got the data, otherwise false.</returns>
    static bool GetLastFrameData(float& drawTimeMs, RenderStatsData& statsData);

    /// <summary>
    /// Releases resources. Calls to the profiling API after Dispose are not valid
    /// </summary>
    static void Dispose();
};

/// <summary>
/// Helper structure used to call BeginEvent/EndEvent within single code block.
/// </summary>
struct ScopeProfileBlockGPU
{
    int32 Index;

    FORCE_INLINE ScopeProfileBlockGPU(const Char* name)
    {
        Index = ProfilerGPU::BeginEvent(name);
    }
 
    FORCE_INLINE ~ScopeProfileBlockGPU()
    {
        ProfilerGPU::EndEvent(Index);
    }
};

template<>
struct TIsPODType<ProfilerGPU::Event>
{
    enum { Value = true };
};

// Shortcut macro for profiling rendering on GPU
#define PROFILE_GPU(name) ScopeProfileBlockGPU ProfileBlockGPU(TEXT(name))

#else

// Empty macros for disabled profiler
#define PROFILE_GPU(name)

#endif
