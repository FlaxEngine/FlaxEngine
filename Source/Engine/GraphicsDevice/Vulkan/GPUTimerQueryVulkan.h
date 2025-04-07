// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_VULKAN

#include "Engine/Graphics/GPUTimerQuery.h"
#include "GPUDeviceVulkan.h"

/// <summary>
/// GPU timer query object for Vulkan backend.
/// </summary>
class GPUTimerQueryVulkan : public GPUResourceVulkan<GPUTimerQuery>
{
private:
    struct Query
    {
        BufferedQueryPoolVulkan* Pool;
        uint32 Index;
        uint64 Result;
    };

    struct QueryPair
    {
        Query Begin;
        Query End;
    };

    bool _hasResult = false;
    bool _endCalled = false;
    bool _interrupted = false;
    float _timeDelta = 0.0f;
    int32 _queryIndex;
    Array<QueryPair, InlinedAllocation<8>> _queries;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="GPUTimerQueryVulkan"/> class.
    /// </summary>
    /// <param name="device">The graphics device.</param>
    GPUTimerQueryVulkan(GPUDeviceVulkan* device);

public:
    /// <summary>
    /// Interrupts an in-progress query, allowing the command buffer to submitted. Interrupted queries must be resumed using Resume().
    /// </summary>
    /// <param name="cmdBuffer">The GPU commands buffer.</param>
    void Interrupt(CmdBufferVulkan* cmdBuffer);

    /// <summary>
    /// Resumes an interrupted query, restoring it back to its original in-progress state.
    /// </summary>
    /// <param name="cmdBuffer">The GPU commands buffer.</param>
    void Resume(CmdBufferVulkan* cmdBuffer);

private:
    bool GetResult(Query& query);
    void WriteTimestamp(CmdBufferVulkan* cmdBuffer, Query& query, VkPipelineStageFlagBits stage) const;
    bool TryGetResult();
    bool UseQueries();

public:
    // [GPUTimerQuery]
    void Begin() override;
    void End() override;
    bool HasResult() override;
    float GetResult() override;

protected:
    // [GPUResourceVulkan]
    void OnReleaseGPU() override;
};

#endif
