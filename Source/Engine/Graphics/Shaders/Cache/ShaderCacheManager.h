// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_SHADER_CACHE_MANAGER

#include "../Config.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/Guid.h"
#include "Engine/Core/Types/DateTime.h"

/// <summary>
/// Shaders cache manager.
/// </summary>
class ShaderCacheManager
{
public:

    struct CachedEntryHandle
    {
        Guid ID;
        String Path;

        CachedEntryHandle()
        {
            ID = Guid::Empty;
        }

        bool IsValid() const
        {
            return ID.IsValid();
        }

        bool Exists() const
        {
            return FileSystem::FileExists(Path);
        }

        bool Delete() const
        {
            return FileSystem::DeleteFile(Path);
        }

        DateTime GetModificationDate() const
        {
            return FileSystem::GetFileLastEditTime(Path);
        }
    };

public:

    /// <summary>
    /// Tries to get cached shader entry for a given shader
    /// </summary>
    /// <param name="profile">Shader Profile</param>
    /// <param name="id">Shader ID</param>
    /// <param name="cachedEntry">Result entry if success</param>
    /// <returns>False if cannot get it, otherwise true</returns>
    static bool TryGetEntry(const ShaderProfile profile, const Guid& id, CachedEntryHandle& cachedEntry);

    /// <summary>
    /// Gets shader cache
    /// </summary>
    /// <param name="profile">Shader Profile</param>
    /// <param name="cachedEntry">Cached entry handle</param>
    /// <param name="outputShaderCache">Output data</param>
    /// <returns>True if cannot set cache data, otherwise false</returns>
    static bool GetCache(const ShaderProfile profile, const CachedEntryHandle& cachedEntry, BytesContainer& outputShaderCache);

    /// <summary>
    /// Sets shader cache
    /// </summary>
    /// <param name="profile">Shader Profile</param>
    /// <param name="cachedEntry">Cached entry handle</param>
    /// <param name="inputShaderCache">Input data</param>
    /// <returns>True if cannot set cache data, otherwise false</returns>
    static bool SetCache(const ShaderProfile profile, const CachedEntryHandle& cachedEntry, MemoryWriteStream& inputShaderCache);

    /// <summary>
    /// Removes shader cache.
    /// </summary>
    /// <param name="profile">Shader Profile</param>
    /// <param name="id">Shader ID</param>
    static void RemoveCache(const ShaderProfile profile, const Guid& id);

    /// <summary>
    /// Removes shader cache.
    /// </summary>
    /// <param name="id">Shader ID</param>
    static void RemoveCache(const Guid& id);

    /// <summary>
    /// Copies shader cache.
    /// </summary>
    /// <param name="dstId">Destination shader ID</param>
    /// <param name="srcId">Source shader ID</param>
    static void CopyCache(const Guid& dstId, const Guid& srcId);
};

#endif
