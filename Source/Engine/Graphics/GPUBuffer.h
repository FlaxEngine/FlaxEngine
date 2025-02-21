// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "GPUBufferDescription.h"
#include "GPUResource.h"

class Task;
template<typename T>
class DataContainer;
typedef DataContainer<byte> BytesContainer;

/// <summary>
/// Defines a view for the <see cref="GPUBuffer"/>. Used to bind buffer to the shaders (for input as shader resource or for input/output as unordered access).
/// </summary>
API_CLASS(Sealed, NoSpawn) class FLAXENGINE_API GPUBufferView : public GPUResourceView
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(GPUBufferView);
protected:
    GPUBufferView();
};

/// <summary>
/// All-in-one GPU buffer class. This class is able to create index buffers, vertex buffers, structured buffer and argument buffers.
/// </summary>
/// <seealso cref="GPUResource" />
API_CLASS(Sealed) class FLAXENGINE_API GPUBuffer : public GPUResource
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(GPUBuffer);
    static GPUBuffer* Spawn(const SpawnParams& params);
    static GPUBuffer* New();

protected:
    GPUBufferDescription _desc;
    bool _isLocked = false;

    GPUBuffer();

public:
    /// <summary>
    /// Gets a value indicating whether this buffer has been allocated. 
    /// </summary>
    API_PROPERTY() FORCE_INLINE bool IsAllocated() const
    {
        return _desc.Size > 0;
    }

    /// <summary>
    /// Gets buffer size in bytes.
    /// </summary>
    API_PROPERTY() FORCE_INLINE uint32 GetSize() const
    {
        return _desc.Size;
    }

    /// <summary>
    /// Gets buffer stride in bytes.
    /// </summary>
    API_PROPERTY() FORCE_INLINE uint32 GetStride() const
    {
        return _desc.Stride;
    }

    /// <summary>
    /// Gets buffer data format (if used).
    /// </summary>
    API_PROPERTY() FORCE_INLINE PixelFormat GetFormat() const
    {
        return _desc.Format;
    }

    /// <summary>
    /// Gets buffer elements count (size divided by the stride).
    /// </summary>
    API_PROPERTY() FORCE_INLINE uint32 GetElementsCount() const
    {
        return _desc.Stride > 0 ? _desc.Size / _desc.Stride : 0;
    }

    /// <summary>
    /// Gets buffer flags.
    /// </summary>
    FORCE_INLINE GPUBufferFlags GetFlags() const
    {
        return _desc.Flags;
    }

    /// <summary>
    /// Checks if buffer is a staging buffer (supports CPU access).
    /// </summary>
    API_PROPERTY() bool IsStaging() const;

    /// <summary>
    /// Checks if buffer is a dynamic buffer.
    /// </summary>
    API_PROPERTY() bool IsDynamic() const;

    /// <summary>
    /// Gets a value indicating whether this buffer is a shader resource.
    /// </summary>
    API_PROPERTY() FORCE_INLINE bool IsShaderResource() const
    {
        return _desc.IsShaderResource();
    }

    /// <summary>
    /// Gets a value indicating whether this buffer is a unordered access.
    /// </summary>
    API_PROPERTY() FORCE_INLINE bool IsUnorderedAccess() const
    {
        return _desc.IsUnorderedAccess();
    }

    /// <summary>
    /// Gets buffer description structure.
    /// </summary>
    API_PROPERTY() const GPUBufferDescription& GetDescription() const
    {
        return _desc;
    }

    /// <summary>
    /// Gets the view for the whole buffer.
    /// </summary>
    API_FUNCTION() virtual GPUBufferView* View() const = 0;

public:
    /// <summary>
    /// Creates new buffer.
    /// </summary>
    /// <param name="desc">The buffer description.</param>
    /// <returns>True if cannot create buffer, otherwise false.</returns>
    API_FUNCTION() bool Init(API_PARAM(Ref) const GPUBufferDescription& desc);

    /// <summary>
    /// Creates new staging readback buffer with the same dimensions and properties as a source buffer (but without a data transferred; warning: caller must delete object).
    /// </summary>
    /// <returns>Staging readback buffer.</returns>
    GPUBuffer* ToStagingReadback() const;

    /// <summary>
    /// Creates new staging upload buffer with the same dimensions and properties as a source buffer (but without a data transferred; warning: caller must delete object).
    /// </summary>
    /// <returns>Staging upload buffer.</returns>
    GPUBuffer* ToStagingUpload() const;

    /// <summary>
    /// Tries to resize the buffer (warning: contents will be lost).
    /// </summary>
    /// <param name="newSize">The new size (in bytes).</param>
    /// <returns>True if cannot resize buffer, otherwise false.</returns>
    API_FUNCTION() bool Resize(uint32 newSize);

public:
    /// <summary>
    /// Stops current thread execution to gather buffer data from the GPU. Cannot be called from main thread if the buffer is not a dynamic nor staging readback.
    /// </summary>
    /// <param name="result">The result data.</param>
    /// <returns>True if cannot download data, otherwise false.</returns>
    API_FUNCTION() bool DownloadData(API_PARAM(Out) BytesContainer& result);

    /// <summary>
    /// Creates GPU async task that will gather buffer data from the GPU.
    /// </summary>
    /// <param name="result">The result data.</param>
    /// <returns>Download data task (not started yet).</returns>
    Task* DownloadDataAsync(BytesContainer& result);

    /// <summary>
    /// Gets the buffer data via map/memcpy/unmap sequence. Always supported for dynamic and staging buffers (other types support depends on graphics backend implementation).
    /// </summary>
    /// <param name="output">The output data container.</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() bool GetData(API_PARAM(Out) BytesContainer& output);

    /// <summary>
    /// Sets the buffer data via map/memcpy/unmap sequence. Always supported for dynamic buffers (other types support depends on graphics backend implementation).
    /// </summary>
    /// <param name="data">The source data to upload.</param>
    /// <param name="size">The size of data (in bytes).</param>
    API_FUNCTION() void SetData(const void* data, uint32 size);

    /// <summary>
    /// Gets a CPU pointer to the resource by mapping its contents. Denies the GPU access to that resource.
    /// </summary>
    /// <remarks>Always call Unmap if the returned pointer is valid to release resources.</remarks>
    /// <param name="mode">The map operation mode.</param>
    /// <returns>The pointer of the mapped CPU buffer with resource data or null if failed.</returns>
    API_FUNCTION() virtual void* Map(GPUResourceMapMode mode) = 0;

    /// <summary>
    /// Invalidates the mapped pointer to a resource and restores the GPU's access to that resource.
    /// </summary>
    API_FUNCTION() virtual void Unmap() = 0;

protected:
    virtual bool OnInit() = 0;

public:
    // [GPUResource]
    String ToString() const override;
    GPUResourceType GetResourceType() const final override;

protected:
    // [GPUResource]
    void OnReleaseGPU() override;
};
