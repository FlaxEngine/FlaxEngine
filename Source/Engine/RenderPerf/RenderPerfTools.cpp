// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_RENDER_PERF

#include "RenderPerfTools.h"
#include "Engine/Core/Types/StringBuilder.h"
#include "Engine/Graphics/GPUAdapter.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/Enums.h"

#if COMPILE_WITH_RENDER_PERF_NVPERF
#include "PerfSdkHud.h"
#endif

namespace
{
    static bool SessionActive = false;
    static String RegionName;
    static String RegionSummary;
    static String ErrorText;
    static float RegionGpuTimeMs = 0.0f;
    static int32 RegionDrawCalls = 0;
    static int32 RegionTriangles = 0;
    static int32 RegionVertices = 0;
#if COMPILE_WITH_RENDER_PERF_NVPERF
    static PerfSdkHud Hud;
#endif
}

Array<RenderPerfTools::DesignerGpuMetric, InlinedAllocation<32>> RenderPerfTools::DesignerMetrics;

bool RenderPerfTools::GetIsCompiled()
{
#if COMPILE_WITH_RENDER_PERF_NVPERF
    return true;
#else
    return false;
#endif
}

bool RenderPerfTools::GetIsSessionActive()
{
    return SessionActive;
}

String RenderPerfTools::GetRegionName()
{
    return RegionName;
}

String RenderPerfTools::GetStatusText()
{
#if COMPILE_WITH_RENDER_PERF_NVPERF
    if (Hud.IsActive())
        return Hud.GetStatusText();
#endif
    return String::Empty;
}

String RenderPerfTools::GetMetricsText()
{
    StringBuilder sb;
    if (!RegionSummary.IsEmpty())
    {
        sb.Append(RegionSummary);
        sb.AppendLine();
        sb.AppendLine();
    }

#if COMPILE_WITH_RENDER_PERF_NVPERF
    if (Hud.IsActive())
    {
        if (!Hud.GetGraphicsApiName().IsEmpty())
        {
            sb.AppendFormat(TEXT("Graphics API: {0}"), Hud.GetGraphicsApiName());
            sb.AppendLine();
        }
        if (!Hud.GetStatusText().IsEmpty())
        {
            sb.Append(Hud.GetStatusText());
            sb.AppendLine();
            sb.AppendLine();
        }
        if (!Hud.GetOverlayText().IsEmpty())
            sb.Append(Hud.GetOverlayText());
    }
#endif

    return sb.ToString();
}

String RenderPerfTools::GetChipAreasText()
{
#if COMPILE_WITH_RENDER_PERF_NVPERF
    if (Hud.IsActive())
        return Hud.GetChipAreasText();
#endif
    return String::Empty;
}

String RenderPerfTools::GetErrorText()
{
    return ErrorText;
}

float RenderPerfTools::GetRegionGpuTimeMs()
{
    return RegionGpuTimeMs;
}

int32 RenderPerfTools::GetRegionDrawCalls()
{
    return RegionDrawCalls;
}

int32 RenderPerfTools::GetRegionTriangles()
{
    return RegionTriangles;
}

int32 RenderPerfTools::GetRegionVertices()
{
    return RegionVertices;
}

bool RenderPerfTools::TryOpenRegionProfiler(const String& regionName, float gpuTimeMs, int32 drawCalls, int32 triangles, int32 vertices)
{
#if COMPILE_WITH_RENDER_PERF_NVPERF
    ErrorText.Clear();
    RegionName = regionName;
    RegionGpuTimeMs = gpuTimeMs;
    RegionDrawCalls = drawCalls;
    RegionTriangles = triangles;
    RegionVertices = vertices;
    RegionSummary = String::Format(TEXT("GPU Profiler Region: {0}\r\nGPU Time: {1:.3f} ms\r\nDraw Calls: {2}\r\nTriangles: {3}\r\nVertices: {4}"),
                                   regionName, gpuTimeMs, drawCalls, triangles, vertices);
    DesignerMetrics.Clear();

    GPUDevice* device = GPUDevice::Instance;
    if (!device)
    {
        ErrorText = TEXT("GPU device is not available.");
        SessionActive = false;
        return false;
    }

    GPUAdapter* adapter = device->GetAdapter();
    GPUContext* context = device->GetMainContext();
    if (!adapter || !context)
    {
        ErrorText = TEXT("GPU adapter or context is not available.");
        SessionActive = false;
        return false;
    }

    if (!Hud.IsActive())
    {
        if (!Hud.TryInitialize(device, adapter, context))
        {
            ErrorText = Hud.GetStatusText();
            if (ErrorText.IsEmpty())
                ErrorText = TEXT("Failed to initialize NVIDIA Perf SDK.");
            SessionActive = false;
            return false;
        }
    }

    SessionActive = true;
    return true;
#else
    ErrorText = TEXT("Engine was built without NVIDIA Perf SDK support.");
    SessionActive = false;
    return false;
#endif
}

void RenderPerfTools::CloseRegionProfiler()
{
    SessionActive = false;
#if COMPILE_WITH_RENDER_PERF_NVPERF
    Hud.Shutdown();
#endif
    RegionName.Clear();
    RegionSummary.Clear();
    ErrorText.Clear();
    RegionGpuTimeMs = 0.0f;
    RegionDrawCalls = 0;
    RegionTriangles = 0;
    RegionVertices = 0;
    DesignerMetrics.Clear();
}

void RenderPerfTools::OnGpuFrameBegin(GPUContext* context)
{
#if COMPILE_WITH_RENDER_PERF_NVPERF
    if (!SessionActive || !context || !Hud.IsActive())
        return;

    Hud.OnRenderPassBegin(context);
#endif
}

void RenderPerfTools::OnGpuFrameEnd(GPUContext* context)
{
#if COMPILE_WITH_RENDER_PERF_NVPERF
    if (!SessionActive || !context || !Hud.IsActive())
        return;

    Hud.OnRenderPassEnd(context);
    Hud.GetDesignerMetrics(DesignerMetrics);
#endif
}

#endif
