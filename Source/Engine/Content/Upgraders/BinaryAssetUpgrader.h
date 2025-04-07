// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if USE_EDITOR

#include "IAssetUpgrader.h"
#include "Engine/Content/Storage/AssetHeader.h"
#include "Engine/Core/Log.h"

/// <summary>
/// Binary asset upgrading context structure.
/// </summary>
struct FLAXENGINE_API AssetMigrationContext
{
    /// <summary>
    /// The input data.
    /// </summary>
    AssetInitData Input;

    /// <summary>
    /// The output data.
    /// </summary>
    AssetInitData Output;

public:
    /// <summary>
    /// Allocates the chunk in the output data so upgrader can write to it.
    /// </summary>
    /// <param name="index">The index of the chunk.</param>
    /// <returns>True if cannot allocate it.</returns>
    bool AllocateChunk(int32 index)
    {
        // Check index
        if (index < 0 || index >= ASSET_FILE_DATA_CHUNKS)
        {
            LOG(Warning, "Invalid asset chunk index {0}.", index);
            return true;
        }

        // Check if chunk has been already allocated
        if (Output.Header.Chunks[index] != nullptr)
        {
            LOG(Warning, "Asset chunk {0} has been already allocated.", index);
            return true;
        }

        // Create new chunk
        Output.Header.Chunks[index] = New<FlaxChunk>();
        return false;
    }
};

typedef bool (*UpgradeHandler)(AssetMigrationContext& context);

/// <summary>
/// Binary Assets Upgrader base class
/// </summary>
/// <seealso cref="IAssetUpgrader" />
class FLAXENGINE_API BinaryAssetUpgrader : public IAssetUpgrader
{
public:
    struct Upgrader
    {
        uint32 CurrentVersion;
        uint32 TargetVersion;
        UpgradeHandler Handler;
    };

private:
    Array<Upgrader, InlinedAllocation<8>> _upgraders;

public:
    /// <summary>
    /// Upgrades the specified asset data serialized version.
    /// </summary>
    /// <param name="serializedVersion">The serialized version.</param>
    /// <param name="context">The context.</param>
    /// <returns>True if cannot upgrade or upgrade failed, otherwise false</returns>
    bool Upgrade(uint32 serializedVersion, AssetMigrationContext& context) const
    {
        for (auto& upgrader : _upgraders)
        {
            if (upgrader.CurrentVersion == serializedVersion)
            {
                // Set target version and preserve metadata
                context.Output.SerializedVersion = upgrader.TargetVersion;
#if USE_EDITOR
                context.Output.Metadata = context.Input.Metadata;
                context.Output.Dependencies = context.Input.Dependencies;
#endif

                // Upgrade
                LOG(Info, "Converting \'{0}\' from version {1} to {2}...", context.Input.Header.ToString(), context.Input.SerializedVersion, context.Output.SerializedVersion);
                return upgrader.Handler(context);
            }
        }
        return true;
    }

public:
    /// <summary>
    /// Copies all the chunks from the input data to the output container.
    /// </summary>
    /// <param name="context">The context.</param>
    /// <returns>True if failed, otherwise false.</returns>
    static bool CopyChunks(AssetMigrationContext& context)
    {
        for (int32 chunkIndex = 0; chunkIndex < ASSET_FILE_DATA_CHUNKS; chunkIndex++)
        {
            const auto srcChunk = context.Input.Header.Chunks[chunkIndex];
            if (srcChunk != nullptr && srcChunk->IsLoaded())
            {
                if (context.AllocateChunk(chunkIndex))
                    return true;
                context.Output.Header.Chunks[chunkIndex]->Data.Copy(srcChunk->Data);
            }
        }
        return false;
    }

    /// <summary>
    /// Copies single chunk from the input data to the output container.
    /// </summary>
    /// <param name="context">The context.</param>
    /// <param name="index">The chunk index.</param>
    /// <returns>True if failed, otherwise false.</returns>
    static bool CopyChunk(AssetMigrationContext& context, int32 index)
    {
        return CopyChunk(context, index, index);
    }

    /// <summary>
    /// Copies single chunk from the input data to the output container.
    /// </summary>
    /// <param name="context">The context.</param>
    /// <param name="srcIndex">The source chunk index.</param>
    /// <param name="dstIndex">The destination chunk index.</param>
    /// <returns>True if failed, otherwise false.</returns>
    static bool CopyChunk(AssetMigrationContext& context, int32 srcIndex, int32 dstIndex)
    {
        // Check index
        if (srcIndex < 0 || srcIndex >= ASSET_FILE_DATA_CHUNKS)
        {
            LOG(Warning, "Invalid asset chunk index {0}.", srcIndex);
            return true;
        }

        // Check if loaded and valid
        const auto srcChunk = context.Input.Header.Chunks[srcIndex];
        if (srcChunk != nullptr && srcChunk->IsLoaded())
        {
            // Allocate memory
            if (context.AllocateChunk(dstIndex))
                return true;

            // Copy data
            context.Output.Header.Chunks[dstIndex]->Data.Copy(srcChunk->Data);
        }

        return false;
    }

protected:
    BinaryAssetUpgrader()
    {
    }

    void setup(Upgrader const* upgraders, int32 upgradersCount)
    {
        _upgraders.Add(upgraders, upgradersCount);
    }

public:
    // [IAssetUpgrader]
    bool ShouldUpgrade(uint32 serializedVersion) const override
    {
        for (auto& upgrader : _upgraders)
        {
            if (upgrader.CurrentVersion == serializedVersion)
                return true;
        }
        return false;
    }
};

#endif
