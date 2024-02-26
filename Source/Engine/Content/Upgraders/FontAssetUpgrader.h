// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if USE_EDITOR

#include "BinaryAssetUpgrader.h"
#include "Engine/Render2D/FontAsset.h"

/// <summary>
/// Font Asset Upgrader
/// </summary>
/// <seealso cref="BinaryAssetUpgrader" />
class FontAssetUpgrader : public BinaryAssetUpgrader
{
public:
    /// <summary>
    /// Initializes a new instance of the <see cref="FontAssetUpgrader"/> class.
    /// </summary>
    FontAssetUpgrader()
    {
        static const Upgrader upgraders[] =
        {
            { 1, 2, &Upgrade_1_To_2 },
            { 2, 3, &Upgrade_2_To_3 },
        };
        setup(upgraders, ARRAY_COUNT(upgraders));
    }

private:
    // ============================================
    //                  Version 1:
    // Designed: long time ago in a galaxy far far away
    // Custom Data: not used
    // Chunk 0: Header
    // Chunk 1: Font File data
    // ============================================
    //                  Version 2:
    // Designed: 4/20/2017
    // Custom Data: not used
    // Chunk 0: Font File data
    // ============================================
    //                  Version 3:
    // Designed: 10/24/2019
    // Custom Data: Header1
    // Chunk 0: Font File data
    // ============================================

    typedef FontOptions Header1;

    static bool Upgrade_1_To_2(AssetMigrationContext& context)
    {
        ASSERT(context.Input.SerializedVersion == 1 && context.Output.SerializedVersion == 2);

        if (context.AllocateChunk(0))
            return true;
        context.Output.Header.Chunks[0]->Data.Copy(context.Input.Header.Chunks[1]->Data);

        return false;
    }

    static bool Upgrade_2_To_3(AssetMigrationContext& context)
    {
        ASSERT(context.Input.SerializedVersion == 2 && context.Output.SerializedVersion == 3);

        Header1 header;
        header.Hinting = FontHinting::Default;
        header.Flags = FontFlags::AntiAliasing;
        context.Output.CustomData.Copy(&header);

        return CopyChunk(context, 0);
    }
};

#endif
