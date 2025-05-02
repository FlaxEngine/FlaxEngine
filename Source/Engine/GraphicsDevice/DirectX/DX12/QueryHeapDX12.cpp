// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX12

#include "QueryHeapDX12.h"
#include "GPUDeviceDX12.h"
#include "GPUContextDX12.h"
#include "../RenderToolsDX.h"

QueryHeapDX12::QueryHeapDX12(GPUDeviceDX12* device, const D3D12_QUERY_HEAP_TYPE& queryHeapType, int32 queryHeapCount)
    : _device(device)
    , _queryHeap(nullptr)
    , _resultBuffer(nullptr)
    , _queryHeapType(queryHeapType)
    , _currentIndex(0)
    , _queryHeapCount(queryHeapCount)
{
    if (queryHeapType == D3D12_QUERY_HEAP_TYPE_OCCLUSION)
    {
        _resultSize = sizeof(uint64);
        _queryType = D3D12_QUERY_TYPE_OCCLUSION;
    }
    else if (queryHeapType == D3D12_QUERY_HEAP_TYPE_TIMESTAMP)
    {
        _resultSize = sizeof(uint64);
        _queryType = D3D12_QUERY_TYPE_TIMESTAMP;
    }
    else
    {
        MISSING_CODE("Not support D3D12 query heap type.");
    }
}

bool QueryHeapDX12::Init()
{
    _resultData.Resize(_resultSize * _queryHeapCount);

    // Create the query heap
    D3D12_QUERY_HEAP_DESC heapDesc;
    heapDesc.Type = _queryHeapType;
    heapDesc.Count = _queryHeapCount;
    heapDesc.NodeMask = 0;
    HRESULT result = _device->GetDevice()->CreateQueryHeap(&heapDesc, IID_PPV_ARGS(&_queryHeap));
    LOG_DIRECTX_RESULT_WITH_RETURN(result, true);
    DX_SET_DEBUG_NAME(_queryHeap, "Query Heap");

    // Create the result buffer
    D3D12_HEAP_PROPERTIES heapProperties;
    heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;
    D3D12_RESOURCE_DESC resourceDesc;
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Alignment = 0;
    resourceDesc.Width = _resultData.Count();
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    result = _device->GetDevice()->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&_resultBuffer));
    LOG_DIRECTX_RESULT_WITH_RETURN(result, true);
    DX_SET_DEBUG_NAME(_resultBuffer, "Query Heap Result Buffer");

    // Start out with an open query batch
    _currentBatch.Open = false;
    StartQueryBatch();

    return false;
}

void QueryHeapDX12::Destroy()
{
    SAFE_RELEASE(_resultBuffer);
    SAFE_RELEASE(_queryHeap);
    _currentBatch.Clear();
    _resultData.SetCapacity(0);
}

void QueryHeapDX12::EndQueryBatchAndResolveQueryData(GPUContextDX12* context)
{
    ASSERT(_currentBatch.Open);
    if (_currentBatch.Count == 0)
        return;

    // Close the current batch
    _currentBatch.Open = false;

    // Resolve the batch
    const int32 offset = _currentBatch.Start * _resultSize;
    context->GetCommandList()->ResolveQueryData(_queryHeap, _queryType, _currentBatch.Start, _currentBatch.Count, _resultBuffer, offset);
    _currentBatch.Sync = _device->GetCommandQueue()->GetSyncPoint();

    // Begin a new query batch
    _batches.Add(_currentBatch);
    StartQueryBatch();
}

void QueryHeapDX12::AllocQuery(GPUContextDX12* context, ElementHandle& handle)
{
    ASSERT(_currentBatch.Open);

    // Check if need to start from the buffer head
    if (_currentIndex >= GetQueryHeapCount())
    {
        // We're in the middle of a batch, but we're at the end of the heap so split the batch in two
        EndQueryBatchAndResolveQueryData(context);
    }

    // Allocate element into the current batch
    handle = _currentIndex++;
    _currentBatch.Count++;
}

void QueryHeapDX12::BeginQuery(GPUContextDX12* context, ElementHandle& handle)
{
    AllocQuery(context, handle);

    context->GetCommandList()->BeginQuery(_queryHeap, _queryType, handle);
}

void QueryHeapDX12::EndQuery(GPUContextDX12* context, ElementHandle& handle)
{
    AllocQuery(context, handle);

    context->GetCommandList()->EndQuery(_queryHeap, _queryType, handle);
}

bool QueryHeapDX12::IsReady(ElementHandle& handle)
{
    // Current batch is not ready (not ended)
    if (_currentBatch.ContainsElement(handle))
        return false;

    for (int32 i = 0; i < _batches.Count(); i++)
    {
        auto& batch = _batches[i];
        if (batch.ContainsElement(handle))
        {
            ASSERT(batch.Sync.IsValid());
            return batch.Sync.IsComplete();
        }
    }

    return true;
}

void* QueryHeapDX12::ResolveQuery(ElementHandle& handle)
{
    // Prevent queries from the current batch
    ASSERT(!_currentBatch.ContainsElement(handle));

    // Find the batch that contains this element to resolve it
    for (int32 i = 0; i < _batches.Count(); i++)
    {
        auto& batch = _batches[i];
        if (batch.ContainsElement(handle))
        {
            ASSERT(batch.Sync.IsValid());

            // Ensure that end point has been already executed
            if (!batch.Sync.IsComplete())
            {
                if (batch.Sync.IsOpen())
                {
                    // The query is on a command list that hasn't been submitted yet
                    LOG(Warning, "Stalling the rendering and flushing GPU commands to wait for a query that hasn't been submitted to the GPU yet.");
                    _device->WaitForGPU();
                }

                batch.Sync.WaitForCompletion();
            }

            // Map the query values readback buffer
            D3D12_RANGE range;
            range.Begin = batch.Start * _resultSize;
            range.End = range.Begin + batch.Count * _resultSize;
            void* mapped = nullptr;
            VALIDATE_DIRECTX_CALL(_resultBuffer->Map(0, &range, &mapped));

            // Copy the results data
            Platform::MemoryCopy(_resultData.Get() + range.Begin, (byte*)mapped + range.Begin, batch.Count * _resultSize);

            // Unmap with an empty range to indicate nothing was written by the CPU
            _resultBuffer->Unmap(0, nullptr);

            // All elements got its results so we can remove this batch
            _batches.RemoveAt(i);

            break;
        }
    }

    return _resultData.Get() + handle * _resultSize;
}

void QueryHeapDX12::StartQueryBatch()
{
    ASSERT(!_currentBatch.Open);

    // Clear the current batch
    _currentBatch.Clear();

    // Loop active index on overflow
    if (_currentIndex >= GetQueryHeapCount())
    {
        _currentIndex = 0;
    }

    // Start a new batch
    _currentBatch.Start = _currentIndex;
    _currentBatch.Open = true;
}

#endif
