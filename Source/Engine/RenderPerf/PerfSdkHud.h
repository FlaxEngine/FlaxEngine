// Copyright (c) Flax Engine. Nsight Perf SDK HUD helper for RenderPerfSDK.

#pragma once

#if COMPILE_WITH_RENDER_PERF

#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Types/String.h"
#include "Engine/RenderPerf/RenderPerfTools.h"

class GPUAdapter;
class GPUContext;
class GPUDevice;

/// <summary>
/// Live PerfSDK metrics (GPU periodic sampler + HUD) for supported graphics APIs.
/// </summary>
class PerfSdkHud
{
public:
    bool TryInitialize(GPUDevice* device, GPUAdapter* adapter, GPUContext* context);
    void Shutdown();

    void OnRenderPassBegin(GPUContext* context);
    void OnRenderPassEnd(GPUContext* context);

    bool IsActive() const { return _active; }
    const String& GetGraphicsApiName() const { return _graphicsApiName; }
    const String& GetOverlayText() const { return _overlayText; }
    const String& GetStatusText() const { return _statusText; }

    /// <summary>
    /// Fills designer-friendly throughput metrics from the live HUD data model.
    /// </summary>
    void GetDesignerMetrics(Array<RenderPerfTools::DesignerGpuMetric, InlinedAllocation<32>>& metrics) const;

    /// <summary>
    /// Tab-separated chip area rows for the editor metrics table (Area, throughput %, detail, higher-is-better flag).
    /// </summary>
    String GetChipAreasText() const;

private:
    bool _active = false;
    String _graphicsApiName;
    String _overlayText;
    String _statusText;
    void* _impl = nullptr;
};

#endif
