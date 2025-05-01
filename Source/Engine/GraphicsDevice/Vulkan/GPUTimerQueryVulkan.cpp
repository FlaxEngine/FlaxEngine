// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_VULKAN

#include "GPUTimerQueryVulkan.h"
#include "GPUContextVulkan.h"
#include "CmdBufferVulkan.h"

GPUTimerQueryVulkan::GPUTimerQueryVulkan(GPUDeviceVulkan* device)
    : GPUResourceVulkan<GPUTimerQuery>(device, String::Empty)
{
}

void GPUTimerQueryVulkan::Interrupt(CmdBufferVulkan* cmdBuffer)
{
    if (!_interrupted)
    {
        _interrupted = true;
        WriteTimestamp(cmdBuffer, _queries[_queryIndex].End, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    }
}

void GPUTimerQueryVulkan::Resume(CmdBufferVulkan* cmdBuffer)
{
    ASSERT(_interrupted);

    QueryPair e;
    e.End.Pool = nullptr;

    _interrupted = false;
    WriteTimestamp(cmdBuffer, e.Begin, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

    _queries.Add(e);
    _queryIndex++;
}

bool GPUTimerQueryVulkan::GetResult(Query& query)
{
    if (query.Pool)
    {
        const auto context = (GPUContextVulkan*)_device->GetMainContext();
        if (query.Pool->GetResults(context, query.Index, query.Result))
        {
            // Release query
            query.Pool->ReleaseQuery(query.Index);
            query.Pool = nullptr;
        }
        else
        {
            // No results
            return true;
        }
    }

    // Has result
    return false;
}

void GPUTimerQueryVulkan::WriteTimestamp(CmdBufferVulkan* cmdBuffer, Query& query, VkPipelineStageFlagBits stage) const
{
    auto pool = _device->FindAvailableTimestampQueryPool();
    uint32 index;
    if (pool->AcquireQuery(cmdBuffer, index))
    {
        vkCmdWriteTimestamp(cmdBuffer->GetHandle(), stage, pool->GetHandle(), index);
        pool->MarkQueryAsStarted(index);
        query.Pool = pool;
        query.Index = index;
    }
    else
    {
        query.Pool = nullptr;
        query.Index = 0;
    }
}

bool GPUTimerQueryVulkan::TryGetResult()
{
#if VULKAN_USE_QUERIES
    // Try get queries value (if not already)
    for (int32 i = 0; i < _queries.Count(); i++)
    {
        auto& e = _queries[i];

        if (GetResult(e.Begin))
            return false;
        if (GetResult(e.End))
            return false;
    }

    // Calculate delta time (accumulated)
    uint64 delta = 0;
    for (int32 i = 0; i < _queries.Count(); i++)
    {
        auto& e = _queries[i];

        if (e.End.Result > e.Begin.Result)
        {
            delta += e.End.Result - e.Begin.Result;
        }
    }

    // Calculate event duration in milliseconds
    const double frequency = double(_device->PhysicalDeviceLimits.timestampPeriod) / 1e6;
    _timeDelta = static_cast<float>((delta * frequency));

    // Clear data for next usage
    _hasResult = true;
    for (int32 i = 0; i < _queries.Count(); i++)
    {
        auto& e = _queries[i];
        if (e.Begin.Pool)
            e.Begin.Pool->ReleaseQuery(e.Begin.Index);
        if (e.End.Pool)
            e.End.Pool->ReleaseQuery(e.End.Index);
    }
    _queries.Clear();
#else
    _timeDelta = 0.0f;
    _hasResult = true;
#endif
    return true;
}

bool GPUTimerQueryVulkan::UseQueries()
{
#if VULKAN_USE_QUERIES
    return _device->PhysicalDeviceLimits.timestampComputeAndGraphics == VK_TRUE;
#else
    return false;
#endif
}

void GPUTimerQueryVulkan::OnReleaseGPU()
{
    _hasResult = false;
    _endCalled = false;
    _timeDelta = 0.0f;

    for (int32 i = 0; i < _queries.Count(); i++)
    {
        auto& e = _queries[i];
        if (e.Begin.Pool)
            e.Begin.Pool->ReleaseQuery(e.Begin.Index);
        if (e.End.Pool)
            e.End.Pool->ReleaseQuery(e.End.Index);
    }
    _queries.Clear();
}

void GPUTimerQueryVulkan::Begin()
{
#if VULKAN_USE_QUERIES
    if (UseQueries())
    {
        const auto context = (GPUContextVulkan*)_device->GetMainContext();
        const auto cmdBuffer = context->GetCmdBufferManager()->GetCmdBuffer();

        QueryPair e;
        e.End.Pool = nullptr;

        _queryIndex = 0;
        _interrupted = false;
        WriteTimestamp(cmdBuffer, e.Begin, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
        context->GetCmdBufferManager()->OnQueryBegin(this);

        ASSERT(_queries.IsEmpty());
        _queries.Add(e);
    }
#endif

    _hasResult = false;
    _endCalled = false;
}

void GPUTimerQueryVulkan::End()
{
    if (_endCalled)
        return;

#if VULKAN_USE_QUERIES
    if (UseQueries())
    {
        const auto context = (GPUContextVulkan*)_device->GetMainContext();
        const auto cmdBuffer = context->GetCmdBufferManager()->GetCmdBuffer();

        if (!_interrupted)
        {
            WriteTimestamp(cmdBuffer, _queries[_queryIndex].End, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
        }
        context->GetCmdBufferManager()->OnQueryEnd(this);
    }
#endif

    _endCalled = true;
}

bool GPUTimerQueryVulkan::HasResult()
{
    if (!_endCalled)
        return false;
    if (_hasResult)
        return true;
    return TryGetResult();
}

float GPUTimerQueryVulkan::GetResult()
{
    if (_hasResult)
        return _timeDelta;
    TryGetResult();
    return _timeDelta;
}

#endif
