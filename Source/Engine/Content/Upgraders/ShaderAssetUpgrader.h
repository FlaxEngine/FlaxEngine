// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if USE_EDITOR

#include "BinaryAssetUpgrader.h"
#include "Engine/Platform/Platform.h"
#include "Engine/Graphics/Shaders/Cache/ShaderStorage.h"

/// <summary>
/// Material Asset and Shader Asset Upgrader
/// </summary>
/// <seealso cref="BinaryAssetUpgrader" />
class ShaderAssetUpgrader : public BinaryAssetUpgrader
{
public:
    /// <summary>
    /// Initializes a new instance of the <see cref="ShaderAssetUpgrader"/> class.
    /// </summary>
    ShaderAssetUpgrader()
    {
        static const Upgrader upgraders[] =
        {
            { 18, 19, &Upgrade_18_To_19 },
            { 19, 20, &Upgrade_19_To_20 },
        };
        setup(upgraders, ARRAY_COUNT(upgraders));
    }

private:
    // ============================================
    //              Versions 18,19,20:
    // Designed: 7/24/2019
    // Custom Data: Header
    // Chunk 0: Material Params
    // Chunk 1: Internal SM5 cache
    // Chunk 2: Internal SM4 cache
    // Chunk 14: Visject Surface data
    // Chunk 15: Source code: ANSI text (encrypted)
    // ============================================

    typedef ShaderStorage::Header Header;
    typedef ShaderStorage::Header20 Header20;
    typedef ShaderStorage::Header19 Header19;
    typedef ShaderStorage::Header18 Header18;

    static bool Upgrade_19_To_20(AssetMigrationContext& context)
    {
        ASSERT(context.Input.SerializedVersion == 19 && context.Output.SerializedVersion == 20);

        // Convert header
        if (context.Input.CustomData.IsInvalid())
            return true;
        auto& oldHeader = *(Header19*)context.Input.CustomData.Get();
        Header20 newHeader;
        Platform::MemoryClear(&newHeader, sizeof(newHeader));
        if (context.Input.Header.TypeName == TEXT("FlaxEngine.ParticleEmitter"))
        {
            newHeader.ParticleEmitter.GraphVersion = oldHeader.ParticleEmitter.GraphVersion;
            newHeader.ParticleEmitter.CustomDataSize = oldHeader.ParticleEmitter.CustomDataSize;
        }
        else if (context.Input.Header.TypeName == TEXT("FlaxEngine.Material"))
        {
            newHeader.Material.GraphVersion = oldHeader.Material.GraphVersion;
            newHeader.Material.Info = oldHeader.Material.Info;
        }
        else if (context.Input.Header.TypeName == TEXT("FlaxEngine.Shader"))
        {
        }
        else
        {
            LOG(Warning, "Unknown input asset type.");
            return true;
        }
        context.Output.CustomData.Copy(&newHeader);

        // Copy all chunks
        return CopyChunks(context);
    }

    static bool Upgrade_18_To_19(AssetMigrationContext& context)
    {
        ASSERT(context.Input.SerializedVersion == 18 && context.Output.SerializedVersion == 19);

        // Convert header
        if (context.Input.CustomData.IsInvalid())
            return true;
        auto& oldHeader = *(Header18*)context.Input.CustomData.Get();
        Header19 newHeader;
        Platform::MemoryClear(&newHeader, sizeof(newHeader));
        if (context.Input.Header.TypeName == TEXT("FlaxEngine.ParticleEmitter"))
        {
            newHeader.ParticleEmitter.GraphVersion = oldHeader.MaterialVersion;
            newHeader.ParticleEmitter.CustomDataSize = oldHeader.MaterialInfo.MaxTessellationFactor;
        }
        else if (context.Input.Header.TypeName == TEXT("FlaxEngine.Material"))
        {
            newHeader.Material.GraphVersion = oldHeader.MaterialVersion;
            newHeader.Material.Info = oldHeader.MaterialInfo;
        }
        else if (context.Input.Header.TypeName == TEXT("FlaxEngine.Shader"))
        {
        }
        else
        {
            LOG(Warning, "Unknown input asset type.");
            return true;
        }
        context.Output.CustomData.Copy(&newHeader);

        // Copy all chunks
        return CopyChunks(context);
    }
};

#endif
