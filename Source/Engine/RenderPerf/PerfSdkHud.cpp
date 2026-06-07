// Copyright (c) Flax Engine. Nsight Perf SDK HUD helper for RenderPerfSDK.

#include "PerfSdkHud.h"
#include "Engine/Core/Types/StringBuilder.h"

#if defined(COMPILE_WITH_RENDER_PERF_NVPERF) && defined(PLATFORM_WINDOWS)

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef NOGDI
#define NOGDI
#endif

#include <cstdarg>
#include <cstdio>
#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include <d3d11.h>
#include <d3d12.h>
#include <dxgi.h>
#include <wrl/client.h>

#if PLATFORM_WIN32
#include "Engine/Platform/Win32/IncludeWindowsHeaders.h"
#endif
#include <ThirdParty/volk/volk.h>

#define RYML_SINGLE_HDR_DEFINE_NOW
#include <ryml_all.hpp>
#ifndef _RYML_SINGLE_HEADER_AMALGAMATED_HPP_
#define _RYML_SINGLE_HEADER_AMALGAMATED_HPP_
#endif
#undef RYML_SINGLE_HDR_DEFINE_NOW

#include <NvPerfHudDataModel.h>
#include <NvPerfHudTextRenderer.h>

#include <NvPerfD3D11.h>
#include <NvPerfD3D12.h>
#include <NvPerfVulkan.h>
#include <NvPerfMetricConfigurationsHAL.h>
#include <NvPerfPeriodicSamplerGpu.h>
#include <NvPerfPeriodicSamplerD3D12.h>
#include <NvPerfPeriodicSamplerVulkan.h>
#include <NvPerfCounterData.h>
#include <NvPerfScopeExitGuard.h>

#include "nvperf_host_impl.h"

#ifdef Rectangle
#undef Rectangle
#endif

#include "Engine/Graphics/GPUAdapter.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/Enums.h"

namespace
{
    using Microsoft::WRL::ComPtr;

    static String RendererTypeToDisplayName(RendererType type)
    {
        switch (type)
        {
        case RendererType::DirectX11: return TEXT("DirectX 11");
        case RendererType::DirectX12: return TEXT("DirectX 12");
        case RendererType::Vulkan: return TEXT("Vulkan");
        case RendererType::DirectX10: return TEXT("DirectX 10");
        case RendererType::DirectX10_1: return TEXT("DirectX 10.1");
        case RendererType::OpenGL4_1: return TEXT("OpenGL 4.1");
        case RendererType::OpenGL4_4: return TEXT("OpenGL 4.4");
        case RendererType::OpenGLES3: return TEXT("OpenGL ES 3");
        case RendererType::OpenGLES3_1: return TEXT("OpenGL ES 3.1");
        case RendererType::Null: return TEXT("Null");
        case RendererType::WebGPU: return TEXT("WebGPU");
        default: return TEXT("Unknown");
        }
    }

    enum class PerfSdkBackend
    {
        D3D11,
        D3D12,
        Vulkan,
    };

    struct PerfSdkHudImpl
    {
        PerfSdkBackend backend = PerfSdkBackend::D3D11;

        nv::perf::hud::HudPresets hudPresets;
        nv::perf::hud::HudDataModel hudDataModel;
        nv::perf::hud::HudTextRenderer hudTextRenderer;
        std::string capturedHudText;

        // D3D11-only path (GpuPeriodicSampler + timestamp queries)
        nv::perf::sampler::GpuPeriodicSampler d3d11PeriodicSampler;
        nv::perf::sampler::RingBufferCounterData d3d11CounterData;
        ComPtr<ID3D11Device> d3d11Device;
        ComPtr<ID3D11DeviceContext> d3d11Context;
        ComPtr<ID3D11Query> d3d11TimestampQuery;
        ComPtr<ID3D11Query> d3d11TimestampDisjointQuery;
        uint32_t d3d11MaxTriggerLatency = 0;
        bool d3d11InSession = false;
        uint64_t d3d11PendingFrameEndTime = 0;

        // D3D12 / Vulkan (PeriodicSamplerTimeHistory + mini trace)
        nv::perf::sampler::PeriodicSamplerTimeHistoryD3D12 d3d12Sampler;
        ComPtr<ID3D12CommandQueue> d3d12Queue;
        nv::perf::sampler::PeriodicSamplerTimeHistoryVulkan vkSampler;

        uint32_t samplingFrequencyHz = 20;
    };

    static PerfSdkHudImpl* gHudTextCapture = nullptr;

    static bool TrySetNvPerfLibraryPath()
    {
        std::vector<std::wstring> candidates;
        wchar_t envPath[1024];
        if (GetEnvironmentVariableW(L"NVPERF_SDK_PATH", envPath, 1024) > 0)
            candidates.emplace_back(envPath);
        candidates.emplace_back(L"C:\\Users\\nproc\\Downloads\\PerfSDK_2025_5");

        for (const std::wstring& root : candidates)
        {
            const std::wstring binPath = root + L"\\NvPerf\\bin\\x64";
            if (GetFileAttributesW(binPath.c_str()) == INVALID_FILE_ATTRIBUTES)
                continue;

            const wchar_t* paths[] = { binPath.c_str() };
            NVPW_SetLibraryLoadPathsW_Params params = { NVPW_SetLibraryLoadPathsW_Params_STRUCT_SIZE };
            params.numPaths = 1;
            params.ppwPaths = paths;
            if (NVPW_SetLibraryLoadPathsW(&params) == NVPA_STATUS_SUCCESS)
                return true;
        }
        return true;
    }

    static void HudPrintCapture(const char* format, va_list list)
    {
        if (!gHudTextCapture)
            return;
        char buffer[512];
        const int written = vsnprintf(buffer, sizeof(buffer), format, list);
        if (written > 0)
            gHudTextCapture->capturedHudText.append(buffer, static_cast<size_t>(written));
    }

    static bool IsTimestampTableHeader(const std::string& line)
    {
        return line.find("Timestamp") != std::string::npos && line.find('|') != std::string::npos;
    }

    static bool IsTimestampTableDataRow(const std::string& line)
    {
        if (line.empty())
            return false;
        if (line.find("===") != std::string::npos)
            return false;
        if (line.find("Timestamp") != std::string::npos)
            return false;
        if (line.find('|') == std::string::npos)
            return false;
        if (line.find("---") != std::string::npos)
            return false;
        return true;
    }

    static void KeepOnlyLatestTimestampRows(std::string& text)
    {
        std::vector<std::string> lines;
        size_t lineStart = 0;
        for (size_t i = 0; i <= text.size(); ++i)
        {
            if (i == text.size() || text[i] == '\n')
            {
                lines.push_back(text.substr(lineStart, i - lineStart));
                lineStart = i + 1;
            }
        }

        std::string filtered;
        filtered.reserve(text.size());
        std::string latestDataRow;
        bool inTimestampTable = false;

        auto flushLatestRow = [&]() {
            if (!latestDataRow.empty())
            {
                filtered.append(latestDataRow);
                filtered.push_back('\n');
                latestDataRow.clear();
            }
        };

        for (const std::string& line : lines)
        {
            if (IsTimestampTableHeader(line))
            {
                if (inTimestampTable)
                    flushLatestRow();
                inTimestampTable = true;
                filtered.append(line);
                filtered.push_back('\n');
                continue;
            }
            if (inTimestampTable && IsTimestampTableDataRow(line))
            {
                latestDataRow = line;
                continue;
            }
            if (inTimestampTable)
            {
                flushLatestRow();
                inTimestampTable = false;
            }
            if (!line.empty() || !filtered.empty())
            {
                filtered.append(line);
                filtered.push_back('\n');
            }
        }
        if (inTimestampTable)
            flushLatestRow();
        while (!filtered.empty() && filtered.back() == '\n')
            filtered.pop_back();
        text.swap(filtered);
    }

    static void RenderHudText(PerfSdkHudImpl& impl)
    {
        impl.capturedHudText.clear();
        impl.hudTextRenderer.Render();
        KeepOnlyLatestTimestampRows(impl.capturedHudText);
    }

    static bool TryGetLatestHudMetricValueInPanel(nv::perf::hud::HudDataModel& model, const char* panelName, const char* signalLabel, float& outValue)
    {
        if (!signalLabel)
            return false;

        for (const nv::perf::hud::HudConfiguration& configuration : model.GetConfigurations())
        {
            for (const nv::perf::hud::Panel& panel : configuration.panels)
            {
                if (panelName && panel.name != panelName)
                    continue;

                for (const std::unique_ptr<nv::perf::hud::Widget>& widget : panel.widgets)
                {
                    if (!widget)
                        continue;

                    if (widget->type == nv::perf::hud::Widget::Type::ScalarText)
                    {
                        nv::perf::hud::ScalarText& scalar = *static_cast<nv::perf::hud::ScalarText*>(widget.get());
                        if (scalar.label.text != signalLabel || scalar.signal.valBuffer.Size() == 0)
                            continue;
                        outValue = static_cast<float>(scalar.signal.valBuffer.Back());
                        return true;
                    }

                    if (widget->type == nv::perf::hud::Widget::Type::TimePlot)
                    {
                        nv::perf::hud::TimePlot& plot = *static_cast<nv::perf::hud::TimePlot*>(widget.get());
                        for (nv::perf::hud::MetricSignal& signal : plot.signals)
                        {
                            if (signal.label.text != signalLabel || signal.valBuffer.Size() == 0)
                                continue;
                            outValue = static_cast<float>(signal.valBuffer.Back());
                            return true;
                        }
                    }
                }
            }
        }

        return false;
    }

    static bool TryGetLatestHudMetricValue(nv::perf::hud::HudDataModel& model, const char* signalLabel, float& outValue)
    {
        return TryGetLatestHudMetricValueInPanel(model, nullptr, signalLabel, outValue);
    }

    static float NormalizeDesignerMetricValue(float value)
    {
        if (std::isnan(value) || std::isinf(value))
            return 0.0f;
        return static_cast<float>(Math::Clamp(value, 0.0f, 100.0f));
    }

    enum class ChipMetricFormat
    {
        Percent,
        Warps,
        Pixels,
        Bandwidth,
    };

    struct ChipAreaDefinition
    {
        const Char* GroupName;
        const Char* AreaName;
        const char* PanelName;
        const char* HudLabel;
        const Char* MetricLabel;
        ChipMetricFormat Format;
        float BarScale;
        bool HigherIsBetter;
    };

    static const ChipAreaDefinition gChipAreaDefinitions[] =
    {
        // GPU Engines
        { TEXT("GPU Engines"), TEXT("3D Engine"), "Frame Level Stats", "GR Active", TEXT("GR Active"), ChipMetricFormat::Percent, 100.0f, false },
        { TEXT("GPU Engines"), TEXT("Copy Engine"), "Engine Active (%)", "Sync CE", TEXT("Sync Copy Engine"), ChipMetricFormat::Percent, 100.0f, false },
        { TEXT("GPU Engines"), TEXT("Compute"), "Warp Occupancy (warps)", "Compute", TEXT("Compute Warps"), ChipMetricFormat::Warps, 24.0f, false },
        { TEXT("GPU Engines"), TEXT("Fill Rate"), "ROP Throughput (%)", "CROP Write", TEXT("CROP Write"), ChipMetricFormat::Percent, 100.0f, false },
        { TEXT("GPU Engines"), TEXT("Depth Output"), "ROP Throughput (%)", "ZROP Write", TEXT("ZROP Write"), ChipMetricFormat::Percent, 100.0f, false },

        // Shaders
        { TEXT("Shaders"), TEXT("Shader Load"), "Frame Level Stats", "SM Active", TEXT("SM Active"), ChipMetricFormat::Percent, 100.0f, false },
        { TEXT("Shaders"), TEXT("Complexity"), "Shader Activity (%)", "Instructions", TEXT("Instructions"), ChipMetricFormat::Percent, 100.0f, false },
        { TEXT("Shaders"), TEXT("Heavy Math (FMA)"), "Shader Activity (%)", "FMA-Heavy Instructions", TEXT("FMA-Heavy"), ChipMetricFormat::Percent, 100.0f, false },
        { TEXT("Shaders"), TEXT("ALU Instructions"), "Shader Activity (%)", "ALU Instructions", TEXT("ALU Instructions"), ChipMetricFormat::Percent, 100.0f, false },
        { TEXT("Shaders"), TEXT("Pixel Shaders"), "Warp Occupancy (warps)", "Pixel", TEXT("Pixel Warps"), ChipMetricFormat::Warps, 24.0f, false },
        { TEXT("Shaders"), TEXT("Vertex Shaders"), "Warp Occupancy (warps)", "Vertex/Tess/Geometry", TEXT("VTG Warps"), ChipMetricFormat::Warps, 24.0f, false },
        { TEXT("Shaders"), TEXT("GPU Headroom"), "Warp Occupancy (warps)", "Unused, SM Active", TEXT("Idle Warps"), ChipMetricFormat::Warps, 24.0f, true },

        // VRAM
        { TEXT("VRAM"), TEXT("Memory Throughput"), "Frame Level Stats", "VRAM Throughput", TEXT("VRAM Throughput"), ChipMetricFormat::Percent, 100.0f, false },
        { TEXT("VRAM"), TEXT("Read"), "VRAM Throughput (%)", "Read", TEXT("VRAM Read"), ChipMetricFormat::Percent, 100.0f, false },
        { TEXT("VRAM"), TEXT("Write"), "VRAM Throughput (%)", "Write", TEXT("VRAM Write"), ChipMetricFormat::Percent, 100.0f, false },
        { TEXT("VRAM"), TEXT("System RAM (PCI)"), "Frame Level Stats", "PCI Throughput", TEXT("PCI Throughput"), ChipMetricFormat::Percent, 100.0f, false },
        { TEXT("VRAM"), TEXT("PCI Read"), "PCI Bandwidth (GB/s)", "Read", TEXT("PCI Read"), ChipMetricFormat::Bandwidth, 64.0f, false },
        { TEXT("VRAM"), TEXT("PCI Write"), "PCI Bandwidth (GB/s)", "Write", TEXT("PCI Write"), ChipMetricFormat::Bandwidth, 64.0f, false },

        // Caches
        { TEXT("Caches"), TEXT("L2 Cache"), "L2 Hit-Rates (%)", "Total", TEXT("L2 Hit Rate"), ChipMetricFormat::Percent, 100.0f, true },
        { TEXT("Caches"), TEXT("Texture L2"), "L2 Hit-Rates (%)", "Tex", TEXT("Texture L2 Hit"), ChipMetricFormat::Percent, 100.0f, true },
        { TEXT("Caches"), TEXT("TRAM Stalls"), "TRAM Allocation Stalls (%)", "TRAM Allocation Stalls", TEXT("TRAM Stalls"), ChipMetricFormat::Percent, 100.0f, false },
    };

    static const Char* gChipAreaGroupOrder[] =
    {
        TEXT("GPU Engines"),
        TEXT("Shaders"),
        TEXT("VRAM"),
        TEXT("Caches"),
    };

    static float ComputeBarValue(const ChipAreaDefinition& def, float rawValue)
    {
        if (def.Format == ChipMetricFormat::Percent)
            return NormalizeDesignerMetricValue(rawValue);

        if (def.BarScale <= 0.0f)
            return 0.0f;

        return NormalizeDesignerMetricValue(rawValue / def.BarScale * 100.0f);
    }

    static String FormatChipMetricDetail(const ChipAreaDefinition& def, float rawValue)
    {
        switch (def.Format)
        {
        case ChipMetricFormat::Percent:
            return String::Format(TEXT("{0}: {1:.1f}%"), def.MetricLabel, rawValue);
        case ChipMetricFormat::Warps:
            return String::Format(TEXT("{0}: {1:.1f} warps"), def.MetricLabel, rawValue);
        case ChipMetricFormat::Pixels:
            return String::Format(TEXT("{0}: {1:.0f}"), def.MetricLabel, rawValue);
        case ChipMetricFormat::Bandwidth:
            return String::Format(TEXT("{0}: {1:.2f} GB/s"), def.MetricLabel, rawValue);
        default:
            return String::Format(TEXT("{0}: {1:.1f}"), def.MetricLabel, rawValue);
        }
    }

    static bool TryGetChipMetricValue(PerfSdkHudImpl& impl, const ChipAreaDefinition& def, float& rawValue)
    {
        return TryGetLatestHudMetricValueInPanel(impl.hudDataModel, def.PanelName, def.HudLabel, rawValue);
    }

    static void AppendChipAreaRows(StringBuilder& sb, PerfSdkHudImpl& impl)
    {
        for (const Char* groupName : gChipAreaGroupOrder)
        {
            sb.AppendFormat(TEXT("#GROUP\t{0}\n"), groupName);

            for (const ChipAreaDefinition& def : gChipAreaDefinitions)
            {
                if (String(def.GroupName) != groupName)
                    continue;

                float rawValue = 0.0f;
                const bool hasData = TryGetChipMetricValue(impl, def, rawValue);
                const float throughput = hasData ? ComputeBarValue(def, rawValue) : 0.0f;
                const String detail = hasData ? FormatChipMetricDetail(def, rawValue) : TEXT("Collecting samples...");
                sb.AppendFormat(TEXT("{0}\t{1:.1f}\t{2}\t{3}\n"),
                                def.AreaName,
                                throughput,
                                detail,
                                def.HigherIsBetter ? 1 : 0);
            }
        }
    }

    static bool InitHudDataModel(PerfSdkHudImpl& impl, const char* chipName, String& statusText)
    {
        if (!chipName)
        {
            statusText = TEXT("Nsight Perf: unknown GPU chip.");
            return false;
        }

        impl.hudPresets.Initialize(chipName);
        const char* hudConfigurationName = "Graphics General Triage";
        impl.hudDataModel.Load(impl.hudPresets.GetPreset(hudConfigurationName));

        nv::perf::MetricConfigObject metricConfigObject;
        std::string metricConfigName;
        if (nv::perf::MetricConfigurations::GetMetricConfigNameBasedOnHudConfigurationName(
                metricConfigName, chipName, hudConfigurationName))
        {
            if (!nv::perf::MetricConfigurations::LoadMetricConfigObject(metricConfigObject, chipName, metricConfigName))
            {
                statusText = TEXT("Nsight Perf: metric config load failed.");
                return false;
            }
        }

        const double samplingPeriod = 1.0 / static_cast<double>(impl.samplingFrequencyHz);
        if (!impl.hudDataModel.Initialize(samplingPeriod, 4.0, metricConfigObject))
        {
            statusText = TEXT("Nsight Perf: HudDataModel init failed.");
            return false;
        }

        impl.hudTextRenderer.SetConsoleOutput([](const char* format, va_list list) {
            HudPrintCapture(format, list);
        });
        impl.hudTextRenderer.SetColumnSeparator("|");
        impl.hudTextRenderer.Initialize(impl.hudDataModel);
        return true;
    }

    static bool CreateD3D11TimestampQueries(ID3D11Device* device, ComPtr<ID3D11Query>& disjointQuery, ComPtr<ID3D11Query>& timestampQuery)
    {
        D3D11_QUERY_DESC disjointDesc = {};
        disjointDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
        if (FAILED(device->CreateQuery(&disjointDesc, &disjointQuery)))
            return false;
        D3D11_QUERY_DESC tsDesc = {};
        tsDesc.Query = D3D11_QUERY_TIMESTAMP;
        return SUCCEEDED(device->CreateQuery(&tsDesc, &timestampQuery));
    }

    static uint64_t ReadD3D11GpuTimestamp(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11Query* disjointQuery, ID3D11Query* timestampQuery)
    {
        if (!device || !context || !timestampQuery)
            return 0;
        context->End(timestampQuery);
        if (disjointQuery)
            context->End(disjointQuery);
        nv::perf::D3D11Finish(device, context);
        if (disjointQuery)
        {
            D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointData = {};
            while (context->GetData(disjointQuery, &disjointData, sizeof(disjointData), 0) == S_FALSE)
            {
            }
        }
        uint64_t timestamp = 0;
        while (context->GetData(timestampQuery, &timestamp, sizeof(timestamp), 0) == S_FALSE)
        {
        }
        return timestamp;
    }

    static ID3D12CommandQueue* GetFlaxGraphicsCommandQueue(GPUDevice* gpuDevice)
    {
        if (!gpuDevice)
            return nullptr;
        return static_cast<ID3D12CommandQueue*>(gpuDevice->GetNativeCommandQueue());
    }

    static bool DecodeAndUpdateHudD3D11(PerfSdkHudImpl& impl)
    {
        nv::perf::sampler::GpuPeriodicSampler::GetRecordBufferStatusParams status = {};
        status.queryOverflow = true;
        status.queryNumUnreadBytes = true;
        if (!impl.d3d11PeriodicSampler.GetRecordBufferStatus(status))
            return false;
        if (status.overflow)
            return false;

        if (status.numUnreadBytes > 0)
        {
            NVPW_GPU_PeriodicSampler_DecodeStopReason decodeStopReason = NVPW_GPU_PERIODIC_SAMPLER_DECODE_STOP_REASON_OTHER;
            size_t numSamplesMerged = 0;
            size_t numBytesConsumed = 0;
            if (!impl.d3d11PeriodicSampler.DecodeCounters(
                    impl.d3d11CounterData.GetCounterData(),
                    status.numUnreadBytes,
                    decodeStopReason,
                    numSamplesMerged,
                    numBytesConsumed))
                return false;
            if (!impl.d3d11PeriodicSampler.AcknowledgeRecordBuffer(numBytesConsumed))
                return false;
            if (!impl.d3d11CounterData.UpdatePut())
                return false;
        }

        uint32_t numRangesConsumed = 0;
        if (!impl.d3d11CounterData.ConsumeData([&](const uint8_t* pCounterDataImage, size_t counterDataImageSize, uint32_t rangeIndex, bool& stop) {
                stop = false;
                if (!impl.hudDataModel.AddSample(pCounterDataImage, counterDataImageSize, rangeIndex))
                    return false;
                ++numRangesConsumed;
                return true;
            }))
            return false;

        if (!impl.d3d11CounterData.UpdateGet(numRangesConsumed))
            return false;

        if (impl.d3d11PendingFrameEndTime != 0)
            impl.hudDataModel.AddFrameDelimiter(impl.d3d11PendingFrameEndTime);

        RenderHudText(impl);
        return true;
    }

    template<typename TSampler>
    static bool DecodeAndUpdateHudTimeHistory(TSampler& sampler, PerfSdkHudImpl& impl)
    {
        if (!sampler.DecodeCounters())
            return false;

        if (!sampler.ConsumeSamples([&](const uint8_t* pCounterDataImage, size_t counterDataImageSize, uint32_t rangeIndex, bool& stop) {
                stop = false;
                return impl.hudDataModel.AddSample(pCounterDataImage, counterDataImageSize, rangeIndex);
            }))
            return false;

        for (const auto& frameDelimiter : sampler.GetFrameDelimiters())
            impl.hudDataModel.AddFrameDelimiter(frameDelimiter.frameEndTime);

        RenderHudText(impl);
        return true;
    }

    static bool BeginGpuPeriodicSession(nv::perf::sampler::GpuPeriodicSampler& sampler, size_t deviceIndex, uint32_t maxTriggerLatency, uint32_t samplingIntervalNs)
    {
        const auto pulse = sampler.GetGpuPulseSamplingInterval(samplingIntervalNs);
        size_t recordBufferSize = 0;
        if (!nv::perf::sampler::GpuPeriodicSamplerCalculateRecordBufferSize(deviceIndex, std::vector<uint8_t>(), maxTriggerLatency, recordBufferSize))
            return false;
        return sampler.BeginSession(recordBufferSize, 1, { pulse.triggerSource }, pulse.samplingInterval);
    }

    static bool InitD3D11(PerfSdkHudImpl& impl, ID3D11Device* device, ID3D11DeviceContext* context, String& statusText)
    {
        if (!device || !context || context->GetType() != D3D11_DEVICE_CONTEXT_IMMEDIATE)
        {
            statusText = TEXT("Nsight Perf: D3D11 immediate context required.");
            return false;
        }
        if (!nv::perf::D3D11LoadDriver())
        {
            statusText = TEXT("Nsight Perf: D3D11LoadDriver failed.");
            return false;
        }
        if (!nv::perf::D3D11IsNvidiaDevice(device) || !nv::perf::profiler::D3D11IsGpuSupported(device))
        {
            statusText = TEXT("Nsight Perf: D3D11 NVIDIA GPU not supported.");
            return false;
        }

        const size_t deviceIndex = nv::perf::D3D11GetNvperfDeviceIndex(device);
        if (deviceIndex == size_t(~0))
        {
            statusText = TEXT("Nsight Perf: D3D11GetNvperfDeviceIndex failed.");
            return false;
        }

        impl.d3d11Device = device;
        impl.d3d11Context = context;
        if (!CreateD3D11TimestampQueries(device, impl.d3d11TimestampDisjointQuery, impl.d3d11TimestampQuery))
        {
            statusText = TEXT("Nsight Perf: D3D11 timestamp queries failed.");
            return false;
        }
        if (!impl.d3d11PeriodicSampler.Initialize(deviceIndex))
        {
            statusText = TEXT("Nsight Perf: GpuPeriodicSampler init failed.");
            return false;
        }

        const char* chipName = impl.d3d11PeriodicSampler.GetDeviceIdentifiers().pChipName;
        if (!InitHudDataModel(impl, chipName, statusText))
            return false;

        const uint32_t samplingIntervalNs = 1000u * 1000u * 1000u / impl.samplingFrequencyHz;
        const uint32_t maxDecodeLatencyNs = 1000u * 1000u * 1000u;
        impl.d3d11MaxTriggerLatency = maxDecodeLatencyNs / samplingIntervalNs;

        if (!BeginGpuPeriodicSession(impl.d3d11PeriodicSampler, deviceIndex, impl.d3d11MaxTriggerLatency, samplingIntervalNs))
        {
            statusText = TEXT("Nsight Perf: D3D11 BeginSession failed.");
            return false;
        }
        impl.d3d11InSession = true;

        const auto& counterConfig = impl.hudDataModel.GetCounterConfiguration();
        if (!impl.d3d11PeriodicSampler.SetConfig(counterConfig.configImage, 0))
        {
            statusText = TEXT("Nsight Perf: D3D11 SetConfig failed.");
            return false;
        }

        if (!impl.d3d11CounterData.Initialize(impl.d3d11MaxTriggerLatency, false, [&](uint32_t maxSamples, NVPW_PeriodicSampler_CounterData_AppendMode appendMode, std::vector<uint8_t>& counterData) {
                return nv::perf::sampler::GpuPeriodicSamplerCreateCounterData(
                    deviceIndex,
                    counterConfig.counterDataPrefix.data(),
                    counterConfig.counterDataPrefix.size(),
                    maxSamples,
                    appendMode,
                    counterData);
            }))
        {
            statusText = TEXT("Nsight Perf: D3D11 counter data init failed.");
            return false;
        }

        if (!impl.hudDataModel.PrepareSampleProcessing(impl.d3d11CounterData.GetCounterData()))
        {
            statusText = TEXT("Nsight Perf: PrepareSampleProcessing failed.");
            return false;
        }

        if (!impl.d3d11PeriodicSampler.StartSampling())
        {
            statusText = TEXT("Nsight Perf: StartSampling failed.");
            return false;
        }
        return true;
    }

    static bool InitD3D12(PerfSdkHudImpl& impl, GPUDevice* gpuDevice, String& statusText)
    {
        if (!gpuDevice)
        {
            statusText = TEXT("Nsight Perf: missing D3D12 device.");
            return false;
        }
        auto* device = static_cast<ID3D12Device*>(gpuDevice->GetNativePtr());
        if (!device)
        {
            statusText = TEXT("Nsight Perf: missing D3D12 native device.");
            return false;
        }
        if (!nv::perf::D3D12LoadDriver())
        {
            statusText = TEXT("Nsight Perf: D3D12LoadDriver failed.");
            return false;
        }
        if (!nv::perf::D3D12IsNvidiaDevice(device) || !nv::perf::profiler::D3D12IsGpuSupported(device))
        {
            statusText = TEXT("Nsight Perf: D3D12 NVIDIA GPU not supported.");
            return false;
        }

        if (!impl.d3d12Sampler.Initialize(device))
        {
            statusText = TEXT("Nsight Perf: D3D12 sampler init failed.");
            return false;
        }

        const char* chipName = impl.d3d12Sampler.GetGpuDeviceIdentifiers().pChipName;
        if (!InitHudDataModel(impl, chipName, statusText))
            return false;

        ComPtr<ID3D12CommandQueue> queue;
        ID3D12CommandQueue* flaxQueue = GetFlaxGraphicsCommandQueue(gpuDevice);
        if (!flaxQueue)
        {
            statusText = TEXT("Nsight Perf: D3D12 graphics queue not found.");
            return false;
        }
        queue = flaxQueue;
        impl.d3d12Queue = queue;

        const uint32_t samplingIntervalNs = 1000u * 1000u * 1000u / impl.samplingFrequencyHz;
        const uint32_t maxDecodeLatencyNs = 1000u * 1000u * 1000u;
        const size_t maxFrameLatency = 3;
        if (!impl.d3d12Sampler.BeginSession(queue.Get(), samplingIntervalNs, maxDecodeLatencyNs, maxFrameLatency))
        {
            statusText = TEXT("Nsight Perf: D3D12 BeginSession failed.");
            return false;
        }

        if (!impl.d3d12Sampler.SetConfig(&impl.hudDataModel.GetCounterConfiguration()))
        {
            statusText = TEXT("Nsight Perf: D3D12 SetConfig failed.");
            return false;
        }

        if (!impl.hudDataModel.PrepareSampleProcessing(impl.d3d12Sampler.GetCounterData()))
        {
            statusText = TEXT("Nsight Perf: PrepareSampleProcessing failed.");
            return false;
        }
        return true;
    }

    static bool InitVulkan(PerfSdkHudImpl& impl, GPUDevice* gpuDevice, GPUAdapter* adapter, String& statusText)
    {
        if (!gpuDevice || !adapter)
        {
            statusText = TEXT("Nsight Perf: missing Vulkan device or adapter.");
            return false;
        }

        void** handles = static_cast<void**>(gpuDevice->GetNativePtr());
        VkInstance instance = handles ? static_cast<VkInstance>(handles[0]) : VK_NULL_HANDLE;
        VkDevice device = handles ? static_cast<VkDevice>(handles[1]) : VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice = static_cast<VkPhysicalDevice>(adapter->GetNativePtr());
        if (!instance || !physicalDevice || !device)
        {
            statusText = TEXT("Nsight Perf: missing Vulkan handles.");
            return false;
        }
        if (!nv::perf::VulkanLoadDriver(instance))
        {
            statusText = TEXT("Nsight Perf: VulkanLoadDriver failed.");
            return false;
        }
        if (!adapter->IsNVIDIA())
        {
            statusText = TEXT("Nsight Perf: Vulkan NVIDIA GPU not supported.");
            return false;
        }

        if (!impl.vkSampler.Initialize(instance, physicalDevice, device, vkGetInstanceProcAddr, vkGetDeviceProcAddr))
        {
            statusText = TEXT("Nsight Perf: Vulkan sampler init failed.");
            return false;
        }

        const char* chipName = impl.vkSampler.GetGpuDeviceIdentifiers().pChipName;
        if (!InitHudDataModel(impl, chipName, statusText))
            return false;

        VkQueue queue = static_cast<VkQueue>(gpuDevice->GetNativeCommandQueue());
        const uint32_t queueFamilyIndex = gpuDevice->GetNativeCommandQueueFamilyIndex();
        if (!queue)
        {
            statusText = TEXT("Nsight Perf: Vulkan graphics queue not found.");
            return false;
        }

        const uint32_t samplingIntervalNs = 1000u * 1000u * 1000u / impl.samplingFrequencyHz;
        const uint32_t maxDecodeLatencyNs = 1000u * 1000u * 1000u;
        const size_t maxFrameLatency = 3;
        if (!impl.vkSampler.BeginSession(queue, queueFamilyIndex, samplingIntervalNs, maxDecodeLatencyNs, maxFrameLatency))
        {
            statusText = TEXT("Nsight Perf: Vulkan BeginSession failed. Fully restart the editor after rebuilding so PerfSDK Vulkan features/extensions are applied at device creation.");
            return false;
        }

        if (!impl.vkSampler.SetConfig(&impl.hudDataModel.GetCounterConfiguration()))
        {
            statusText = TEXT("Nsight Perf: Vulkan SetConfig failed.");
            return false;
        }

        if (!impl.hudDataModel.PrepareSampleProcessing(impl.vkSampler.GetCounterData()))
        {
            statusText = TEXT("Nsight Perf: PrepareSampleProcessing failed.");
            return false;
        }
        return true;
    }
}

bool PerfSdkHud::TryInitialize(GPUDevice* device, GPUAdapter* adapter, GPUContext* context)
{
    Shutdown();

    if (!device || !context)
    {
        _statusText = TEXT("Nsight Perf: missing GPU device or context.");
        return false;
    }

    const RendererType rendererType = device->GetRendererType();
    _graphicsApiName = RendererTypeToDisplayName(rendererType);

    if (rendererType != RendererType::DirectX11 &&
        rendererType != RendererType::DirectX12 &&
        rendererType != RendererType::Vulkan)
    {
        _statusText = String::Format(TEXT("Nsight Perf: unsupported graphics API ({0}). Use D3D11, D3D12, or Vulkan."), _graphicsApiName);
        return false;
    }

    TrySetNvPerfLibraryPath();
    if (!nv::perf::InitializeNvPerf())
    {
        _statusText = TEXT("Nsight Perf: NVPW_InitializeHost/Target failed.");
        return false;
    }

    auto impl = std::make_unique<PerfSdkHudImpl>();
    bool initOk = false;

    switch (rendererType)
    {
    case RendererType::DirectX11:
    {
        impl->backend = PerfSdkBackend::D3D11;
        auto* d3dContext = static_cast<ID3D11DeviceContext*>(context->GetNativePtr());
        ComPtr<ID3D11Device> d3dDevice;
        if (d3dContext)
            d3dContext->GetDevice(&d3dDevice);
        initOk = InitD3D11(*impl, d3dDevice.Get(), d3dContext, _statusText);
        break;
    }
    case RendererType::DirectX12:
    {
        impl->backend = PerfSdkBackend::D3D12;
        initOk = InitD3D12(*impl, device, _statusText);
        break;
    }
    case RendererType::Vulkan:
    {
        impl->backend = PerfSdkBackend::Vulkan;
        initOk = InitVulkan(*impl, device, adapter, _statusText);
        break;
    }
    default:
        break;
    }

    if (!initOk)
        return false;

    gHudTextCapture = impl.get();
    _impl = impl.release();
    _active = true;

    const char* chipName = nullptr;
    switch (rendererType)
    {
    case RendererType::DirectX11:
        chipName = static_cast<PerfSdkHudImpl*>(_impl)->d3d11PeriodicSampler.GetDeviceIdentifiers().pChipName;
        break;
    case RendererType::DirectX12:
        chipName = static_cast<PerfSdkHudImpl*>(_impl)->d3d12Sampler.GetGpuDeviceIdentifiers().pChipName;
        break;
    case RendererType::Vulkan:
        chipName = static_cast<PerfSdkHudImpl*>(_impl)->vkSampler.GetGpuDeviceIdentifiers().pChipName;
        break;
    default:
        break;
    }

    _statusText = String::Format(
        TEXT("Nsight Perf: {0} | {1} @ {2} Hz"),
        _graphicsApiName,
        chipName ? String(chipName) : String(TEXT("GPU")),
        static_cast<PerfSdkHudImpl*>(_impl)->samplingFrequencyHz);
    _overlayText = TEXT("Nsight Perf: warming up...");
    return true;
}

void PerfSdkHud::Shutdown()
{
    gHudTextCapture = nullptr;
    if (!_impl)
        return;

    auto* impl = static_cast<PerfSdkHudImpl*>(_impl);
    switch (impl->backend)
    {
    case PerfSdkBackend::D3D11:
        if (impl->d3d11InSession)
        {
            impl->d3d11PeriodicSampler.StopSampling();
            impl->d3d11PeriodicSampler.EndSession();
            impl->d3d11InSession = false;
        }
        impl->d3d11PeriodicSampler.Reset();
        impl->d3d11CounterData.Reset();
        break;
    case PerfSdkBackend::D3D12:
        impl->d3d12Sampler.EndSession();
        impl->d3d12Sampler.Reset();
        break;
    case PerfSdkBackend::Vulkan:
        impl->vkSampler.EndSession();
        impl->vkSampler.Reset();
        break;
    }
    delete impl;
    _impl = nullptr;
    _active = false;
}

void PerfSdkHud::OnRenderPassBegin(GPUContext* context)
{
    if (!_active || !_impl || !context)
        return;

    auto* impl = static_cast<PerfSdkHudImpl*>(_impl);
    switch (impl->backend)
    {
    case PerfSdkBackend::D3D11:
    {
        auto* d3dContext = static_cast<ID3D11DeviceContext*>(context->GetNativePtr());
        if (impl->d3d11TimestampDisjointQuery && d3dContext)
            d3dContext->Begin(impl->d3d11TimestampDisjointQuery.Get());
        DecodeAndUpdateHudD3D11(*impl);
        break;
    }
    case PerfSdkBackend::D3D12:
        DecodeAndUpdateHudTimeHistory(impl->d3d12Sampler, *impl);
        break;
    case PerfSdkBackend::Vulkan:
        DecodeAndUpdateHudTimeHistory(impl->vkSampler, *impl);
        break;
    }

    if (!impl->capturedHudText.empty())
        _overlayText = impl->capturedHudText.c_str();
}

void PerfSdkHud::OnRenderPassEnd(GPUContext* context)
{
    if (!_active || !_impl || !context)
        return;

    auto* impl = static_cast<PerfSdkHudImpl*>(_impl);
    switch (impl->backend)
    {
    case PerfSdkBackend::D3D11:
    {
        auto* d3dContext = static_cast<ID3D11DeviceContext*>(context->GetNativePtr());
        if (impl->d3d11Device && d3dContext)
        {
            impl->d3d11PendingFrameEndTime = ReadD3D11GpuTimestamp(
                impl->d3d11Device.Get(),
                d3dContext,
                impl->d3d11TimestampDisjointQuery.Get(),
                impl->d3d11TimestampQuery.Get());
            if (impl->d3d11InSession)
                impl->d3d11PeriodicSampler.StartSampling();
        }
        break;
    }
    case PerfSdkBackend::D3D12:
        impl->d3d12Sampler.OnFrameEnd();
        break;
    case PerfSdkBackend::Vulkan:
        if (!impl->vkSampler.OnFrameEnd())
            _overlayText = TEXT("Nsight Perf: Vulkan frame marker failed.");
        break;
    }
}

void PerfSdkHud::GetDesignerMetrics(Array<RenderPerfTools::DesignerGpuMetric, InlinedAllocation<32>>& metrics) const
{
    metrics.Clear();
    if (!_active || !_impl)
        return;

    auto* impl = static_cast<PerfSdkHudImpl*>(_impl);
    for (const ChipAreaDefinition& def : gChipAreaDefinitions)
    {
        float rawValue = 0.0f;
        if (!TryGetChipMetricValue(*impl, def, rawValue))
            continue;

        RenderPerfTools::DesignerGpuMetric metric;
        metric.Title = def.AreaName;
        metric.Hint = FormatChipMetricDetail(def, rawValue);
        metric.Value = ComputeBarValue(def, rawValue);
        metric.HigherIsBetter = def.HigherIsBetter;
        metrics.Add(metric);
    }
}

String PerfSdkHud::GetChipAreasText() const
{
    if (!_active || !_impl)
        return String::Empty;

    StringBuilder sb;
    AppendChipAreaRows(sb, *static_cast<PerfSdkHudImpl*>(_impl));
    return sb.ToString();
}

#else

bool PerfSdkHud::TryInitialize(GPUDevice*, GPUAdapter*, GPUContext*)
{
    _graphicsApiName = TEXT("Unknown");
    _statusText = TEXT("Nsight Perf: Windows + COMPILE_WITH_RENDER_PERF_NVPERF build required.");
    return false;
}

void PerfSdkHud::Shutdown()
{
    _active = false;
}

void PerfSdkHud::OnRenderPassBegin(GPUContext*)
{
}

void PerfSdkHud::OnRenderPassEnd(GPUContext*)
{
}

void PerfSdkHud::GetDesignerMetrics(Array<RenderPerfTools::DesignerGpuMetric, InlinedAllocation<32>>& metrics) const
{
    metrics.Clear();
}

String PerfSdkHud::GetChipAreasText() const
{
    return String::Empty;
}

#endif
