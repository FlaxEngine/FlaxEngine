// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Content/Storage/FlaxStorage.h"

class GPUTexture;
class Task;

/// <summary>
/// Interface for objects that can manage streamable texture
/// </summary>
class FLAXENGINE_API ITextureOwner
{
public:
    /// <summary>
    /// Gets the texture owner mutex used to synchronize texture logic.
    /// </summary>
    virtual CriticalSection& GetOwnerLocker() const = 0;

    /// <summary>
    /// Get texture mip map data
    /// </summary>
    /// <param name="mipIndex">Mip map index</param>
    /// <returns>Task that will get asset data (may be null if data already loaded).</returns>
    virtual Task* RequestMipDataAsync(int32 mipIndex) = 0;

    /// <summary>
    /// Prepares texture data. May lock data chunks to be keep in cache for a while.
    /// </summary>
    virtual FlaxStorage::LockData LockData() = 0;

    /// <summary>
    /// Gets texture mip map data. May fail if not data requested. See RequestMipDataAsync.
    /// </summary>
    /// <param name="mipIndex">Mip map index</param>
    /// <param name="data">Result data</param>
    virtual void GetMipData(int32 mipIndex, BytesContainer& data) const = 0;

    /// <summary>
    /// Gets texture mip map data. Performs loading if data is not in memory (may stall the callee thread).
    /// </summary>
    /// <param name="mipIndex">Mip map index</param>
    /// <param name="data">Result data</param>
    virtual void GetMipDataWithLoading(int32 mipIndex, BytesContainer& data) const
    {
        GetMipData(mipIndex, data);
    }

    /// <summary>
    /// Gets texture mip map data row and slice pitch. Cna be used to override the default values.
    /// </summary>
    /// <param name="mipIndex">Mip map index</param>
    /// <param name="rowPitch">Data row pitch (in bytes).</param>
    /// <param name="slicePitch">Data slice pitch (in bytes).</param>
    /// <returns>True if has a custom row/slice pitch values, otherwise false (to use default values).</returns>
    virtual bool GetMipDataCustomPitch(int32 mipIndex, uint32& rowPitch, uint32& slicePitch) const
    {
        return false;
    }
};
