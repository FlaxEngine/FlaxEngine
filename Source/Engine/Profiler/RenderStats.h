// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_PROFILER

#include "Engine/Core/Types/BaseTypes.h"

/// <summary>
/// Object that stores various render statistics.
/// </summary>
API_STRUCT() struct RenderStatsData
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(RenderStatsData);

    /// <summary>
    /// The draw calls count.
    /// </summary>
    API_FIELD() int64 DrawCalls;

    /// <summary>
    /// The compute shader dispatch calls count.
    /// </summary>
    API_FIELD() int64 DispatchCalls;

    /// <summary>
    /// The vertices drawn count.
    /// </summary>
    API_FIELD() int64 Vertices;

    /// <summary>
    /// The triangles drawn count.
    /// </summary>
    API_FIELD() int64 Triangles;

    /// <summary>
    /// The pipeline state changes count.
    /// </summary>
    API_FIELD() int64 PipelineStateChanges;

    /// <summary>
    /// Initializes a new instance of the <see cref="RenderStatsData"/> struct.
    /// </summary>
    RenderStatsData()
        : DrawCalls(0)
        , DispatchCalls(0)
        , Vertices(0)
        , Triangles(0)
        , PipelineStateChanges(0)
    {
    }

    /// <summary>
    /// The global rendering stats counter.
    /// </summary>
    static RenderStatsData Counter;

    /// <summary>
    /// Mixes the stats with the current state. Perform operation: this  = currentState - this, but without additional stack allocations.
    /// </summary>
    /// <param name="currentState">The current state.</param>
    void Mix(const RenderStatsData& currentState)
    {
#define MIX(name) name = currentState.name - name
        MIX(DrawCalls);
        MIX(DispatchCalls);
        MIX(Vertices);
        MIX(Triangles);
        MIX(PipelineStateChanges);
#undef MIX
    }
};

#define RENDER_STAT_DISPATCH_CALL() Platform::InterlockedIncrement(&RenderStatsData::Counter.DispatchCalls)
#define RENDER_STAT_PS_STATE_CHANGE() Platform::InterlockedIncrement(&RenderStatsData::Counter.PipelineStateChanges)
#define RENDER_STAT_DRAW_CALL(vertices, triangles) \
	Platform::InterlockedIncrement(&RenderStatsData::Counter.DrawCalls); \
	Platform::InterlockedAdd(&RenderStatsData::Counter.Vertices, vertices); \
	Platform::InterlockedAdd(&RenderStatsData::Counter.Triangles, triangles)

#else

#define RENDER_STAT_DISPATCH_CALL()
#define RENDER_STAT_PS_STATE_CHANGE()
#define RENDER_STAT_DRAW_CALL(vertices, primitives)

#endif
