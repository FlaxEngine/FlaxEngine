// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_DIRECTX12

class TimerQueryDX12;
class GPUDeviceDX12;
class GPUContextDX12;
class GPUBuffer;

#include "CommandQueueDX12.h"
#include "Engine/Graphics/Enums.h"

/// <summary>
/// GPU query ID packed into 64-bits.
/// </summary>
struct GPUQueryDX12
{
    union
    {
        struct
        {
            uint16 Type;
            uint16 Heap;
            uint16 Element;
            uint16 SecondaryElement;
        };
        uint64 Raw;
    };

    static int32 GetQueriesCount(GPUQueryType type)
    {
        // Timer queries need to know duration via GPU timer queries difference
        return type == GPUQueryType::Timer ? 2 : 1;
    }
};

/// <summary>
/// GPU queries heap for DirectX 12 backend.
/// </summary>
class QueryHeapDX12
{
public:
    /// <summary>
    /// The query element handle.
    /// </summary>
    typedef uint16 ElementHandle;

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
        uint32 Start = 0;

        /// <summary>
        /// The amount of elements added to this batch.
        /// </summary>
        uint32 Count = 0;

        /// <summary>
        /// The GPU clock frequency for timer queries.
        /// </summary>
        uint64 TimestampFrequency = 0;

        /// <summary>
        /// Is the batch still open for more begin/end queries.
        /// </summary>
        bool Open = false;

        /// <summary>
        /// Checks if this query batch contains a given element contains the element.
        /// </summary>
        /// <param name="elementIndex">The index of the element.</param>
        /// <returns>True if element is in this query, otherwise false.</returns>
        bool ContainsElement(uint32 elementIndex) const
        {
            return elementIndex >= Start && elementIndex < Start + Count;
        }
    };

private:
    GPUDeviceDX12* _device = nullptr;
    ID3D12Resource* _resultBuffer = nullptr;
    uint32 _currentIndex = 0;
    uint32 _resultSize = 0;
    uint32 _queryHeapCount = 0;
    QueryBatch _currentBatch;
    Array<QueryBatch> _batches;
    Array<byte> _resultData;
    uint64 _timestampFrequency;

public:
    /// <summary>
    /// Initializes this instance.
    /// </summary>
    /// <param name="device">The device.</param>
    /// <param name="type">Type of the query heap.</param>
    /// <param name="size">The size of the heap.</param>
    ///	<returns>True if failed, otherwise false.</returns>
    bool Init(GPUDeviceDX12* device, GPUQueryType type, uint32 size);

    /// <summary>
    /// Destroys this instance.
    /// </summary>
    void Destroy();

public:
    GPUQueryType Type;
    ID3D12QueryHeap* QueryHeap = nullptr;
    D3D12_QUERY_TYPE QueryType = D3D12_QUERY_TYPE_OCCLUSION;

    /// <summary>
    /// Gets the query heap capacity.
    /// </summary>
    FORCE_INLINE uint32 GetQueryHeapCount() const
    {
        return _queryHeapCount;
    }

    /// <summary>
    /// Gets the size of the result value (in bytes).
    /// </summary>
    FORCE_INLINE uint32 GetResultSize() const
    {
        return _resultSize;
    }

    /// <summary>
    /// Gets the result buffer (CPU readable via Map/Unmap).
    /// </summary>
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
    /// Checks if can alloc a new query (without rolling the existing batch).
    /// </summary>
    /// <param name="count">How many elements to allocate?</param>
    /// <returns>True if can alloc new query within the same batch.</returns>
    bool CanAlloc(int32 count = 1) const;

    /// <summary>
    /// Allocates the query heap element.
    /// </summary>
    /// <param name="handle">The result handle.</param>
    void Alloc(ElementHandle& handle);

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
    /// <param name="timestampFrequency">The optional pointer to GPU timestamps frequency value to store.</param>
    ///	<returns>The pointer to the resolved query data.</returns>
    void* Resolve(ElementHandle& handle, uint64* timestampFrequency = nullptr);

private:
    /// <summary>
    /// Starts tracking a new batch of begin/end query calls that will be resolved together
    /// </summary>
    void StartQueryBatch();
};

#endif
