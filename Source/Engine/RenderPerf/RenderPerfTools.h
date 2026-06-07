// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_RENDER_PERF

#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Scripting/ScriptingType.h"

class GPUContext;

/// <summary>
/// NVIDIA Nsight Perf SDK integration for GPU profiler region inspection.
/// </summary>
API_CLASS(Static) class FLAXENGINE_API RenderPerfTools
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(RenderPerfTools);

public:
    /// <summary>
    /// Game-designer friendly GPU throughput metric (0-100%).
    /// </summary>
    API_STRUCT(NoDefault) struct DesignerGpuMetric
    {
        DECLARE_SCRIPTING_TYPE_MINIMAL(DesignerGpuMetric);

        /// <summary>
        /// Short metric title for game developers.
        /// </summary>
        API_FIELD() String Title;

        /// <summary>
        /// Plain-language explanation and guidance.
        /// </summary>
        API_FIELD() String Hint;

        /// <summary>
        /// Normalized value in range [0, 100].
        /// </summary>
        API_FIELD() float Value;

        /// <summary>
        /// True if higher values are better (e.g. cache hit rate).
        /// </summary>
        API_FIELD() bool HigherIsBetter;
    };

    /// <summary>
    /// True if engine was built with NVPerf SDK support (COMPILE_WITH_RENDER_PERF_NVPERF).
    /// </summary>
    API_PROPERTY() static bool GetIsCompiled();

    /// <summary>
    /// True if a region profiling session is active.
    /// </summary>
    API_PROPERTY() static bool GetIsSessionActive();

    /// <summary>
    /// The selected GPU profiler region name.
    /// </summary>
    API_PROPERTY() static String GetRegionName();

    /// <summary>
    /// Perf SDK status line (chip, API, sampling rate).
    /// </summary>
    API_PROPERTY() static String GetStatusText();

    /// <summary>
    /// Combined region summary and live Perf SDK HUD text.
    /// </summary>
    API_PROPERTY() static String GetMetricsText();

    /// <summary>
    /// Live chip-area rows for the editor metrics table. Each line: Area\tThroughput\tDetail\tHigherIsBetter.
    /// </summary>
    API_PROPERTY() static String GetChipAreasText();

    /// <summary>
    /// Game-designer friendly GPU throughput metrics (0-100%) updated while a session is active.
    /// </summary>
    API_FIELD(ReadOnly) static Array<DesignerGpuMetric, InlinedAllocation<32>> DesignerMetrics;

    /// <summary>
    /// GPU time in ms for the selected profiler region (from the click that opened this session).
    /// </summary>
    API_PROPERTY() static float GetRegionGpuTimeMs();

    /// <summary>
    /// Draw calls for the selected profiler region.
    /// </summary>
    API_PROPERTY() static int32 GetRegionDrawCalls();

    /// <summary>
    /// Triangles for the selected profiler region.
    /// </summary>
    API_PROPERTY() static int32 GetRegionTriangles();

    /// <summary>
    /// Vertices for the selected profiler region.
    /// </summary>
    API_PROPERTY() static int32 GetRegionVertices();

    /// <summary>
    /// Last error message from TryOpenRegionProfiler.
    /// </summary>
    API_PROPERTY() static String GetErrorText();

    /// <summary>
    /// Starts (or reuses) a Perf SDK sampling session for the selected GPU profiler region.
    /// Returns false if Perf SDK is unavailable or initialization failed.
    /// </summary>
    API_FUNCTION() static bool TryOpenRegionProfiler(const String& regionName, float gpuTimeMs, int32 drawCalls, int32 triangles, int32 vertices);

    /// <summary>
    /// Stops the active Perf SDK sampling session.
    /// </summary>
    API_FUNCTION() static void CloseRegionProfiler();

    /// <summary>
    /// Called from the GPU frame before scene rendering to begin Perf SDK sampling for the frame.
    /// </summary>
    static void OnGpuFrameBegin(GPUContext* context);

    /// <summary>
    /// Called from the GPU frame after scene rendering to tick live Perf SDK sampling.
    /// </summary>
    static void OnGpuFrameEnd(GPUContext* context);
};

#endif
