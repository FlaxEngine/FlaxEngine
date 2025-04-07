// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_DIRECTX12

class TimerQueryDX12;
class GPUDeviceDX12;
class GPUContextDX12;
class GPUBuffer;

#include "CommandQueueDX12.h"

/// <summary>
/// GPU queries heap for DirectX 12 backend.
/// </summary>
class QueryHeapDX12
{
public:

    /// <summary>
    /// The query element handle.
    /// </summary>
    typedef int32 ElementHandle;

private:

    struct QueryBatch
    {
        /// <summary>
        /// The synchronization point when query has been submitted to be executed.
        /// </summary>
        SyncPointDX12 Sync;

        /// <summary>
        /// The first element in the batch (inclusive).
        /// </summary>
        int32 Start = 0;

        /// <summary>
        /// The amount of elements added to this batch.
        /// </summary>
        int32 Count = 0;

        /// <summary>
        /// Is the batch still open for more begin/end queries.
        /// </summary>
        bool Open = false;

        /// <summary>
        /// Clears this batch.
        /// </summary>
        inline void Clear()
        {
            Sync = SyncPointDX12();
            Start = 0;
            Count = 0;
            Open = false;
        }

        /// <summary>
        /// Checks if this query batch contains a given element contains the element.
        /// </summary>
        /// <param name="elementIndex">The index of the element.</param>
        /// <returns>True if element is in this query, otherwise false.</returns>
        bool ContainsElement(int32 elementIndex) const
        {
            return elementIndex >= Start && elementIndex < Start + Count;
        }
    };

private:

    GPUDeviceDX12* _device;
    ID3D12QueryHeap* _queryHeap;
    ID3D12Resource* _resultBuffer;
    D3D12_QUERY_TYPE _queryType;
    D3D12_QUERY_HEAP_TYPE _queryHeapType;
    int32 _currentIndex;
    int32 _resultSize;
    int32 _queryHeapCount;
    QueryBatch _currentBatch;
    Array<QueryBatch> _batches;
    Array<byte> _resultData;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="QueryHeapDX12"/> class.
    /// </summary>
    /// <param name="device">The device.</param>
    /// <param name="queryHeapType">Type of the query heap.</param>
    /// <param name="queryHeapCount">The query heap count.</param>
    QueryHeapDX12(GPUDeviceDX12* device, const D3D12_QUERY_HEAP_TYPE& queryHeapType, int32 queryHeapCount);

public:

    /// <summary>
    /// Initializes this instance.
    /// </summary>
    ///	<returns>True if failed, otherwise false.</returns>
    bool Init();

    /// <summary>
    /// Destroys this instance.
    /// </summary>
    void Destroy();

public:

    /// <summary>
    /// Gets the query heap capacity.
    /// </summary>
    /// <returns>The queries count.</returns>
    FORCE_INLINE int32 GetQueryHeapCount() const
    {
        return _queryHeapCount;
    }

    /// <summary>
    /// Gets the size of the result value (in bytes).
    /// </summary>
    /// <returns>The size of the query result value (in bytes).</returns>
    FORCE_INLINE int32 GetResultSize() const
    {
        return _resultSize;
    }

    /// <summary>
    /// Gets the result buffer (CPU readable via Map/Unmap).
    /// </summary>
    /// <returns>The query results buffer.</returns>
    FORCE_INLINE ID3D12Resource* GetResultBuffer() const
    {
        return _resultBuffer;
    }

public:

    /// <summary>
    /// Stops tracking the current batch of begin/end query calls that will be resolved together. This implicitly starts a new batch.
    /// </summary>
    /// <param name="context">The context.</param>
    void EndQueryBatchAndResolveQueryData(GPUContextDX12* context);

    /// <summary>
    /// Allocates the query heap element.
    /// </summary>
    /// <param name="context">The context.</param>
    /// <param name="handle">The result handle.</param>
    void AllocQuery(GPUContextDX12* context, ElementHandle& handle);

    /// <summary>
    /// Calls BeginQuery on command list for the given query heap slot.
    /// </summary>
    /// <param name="context">The context.</param>
    /// <param name="handle">The query handle.</param>
    void BeginQuery(GPUContextDX12* context, ElementHandle& handle);

    /// <summary>
    /// Calls EndQuery on command list for the given query heap slot.
    /// </summary>
    /// <param name="context">The context.</param>
    /// <param name="handle">The query handle.</param>
    void EndQuery(GPUContextDX12* context, ElementHandle& handle);

    /// <summary>
    /// Determines whether the specified query handle is ready to read data (command list has been executed by the GPU).
    /// </summary>
    /// <param name="handle">The handle.</param>
    /// <returns><c>true</c> if the specified query handle is ready; otherwise, <c>false</c>.</returns>
    bool IsReady(ElementHandle& handle);

    /// <summary>
    /// Resolves the query (or skips if already resolved).
    /// </summary>
    /// <param name="handle">The result handle.</param>
    ///	<returns>The pointer to the resolved query data.</returns>
    void* ResolveQuery(ElementHandle& handle);

private:

    /// <summary>
    /// Starts tracking a new batch of begin/end query calls that will be resolved together
    /// </summary>
    void StartQueryBatch();
};

#endif
