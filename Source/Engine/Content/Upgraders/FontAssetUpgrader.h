// Copyright (c) Wojciech Figat. All rights reserved.

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
    FontAssetUpgrader()
    {
        const Upgrader upgraders[] =
        {
            { 3, 4, &Upgrade_3_To_4 },
        };
        setup(upgraders, ARRAY_COUNT(upgraders));
    }

private:
    struct FontOptionsOld
    {
        FontHinting Hinting;
        FontFlags Flags;
    };

    static bool Upgrade_3_To_4(AssetMigrationContext& context)
    {
        ASSERT(context.Input.SerializedVersion == 3 && context.Output.SerializedVersion == 4);

        FontOptionsOld optionsOld;
        Platform::MemoryCopy(&optionsOld, context.Input.CustomData.Get(), sizeof(FontOptionsOld));

        FontOptions options;
        options.Hinting = optionsOld.Hinting;
        options.Flags = optionsOld.Flags;
        options.RasterMode = FontRasterMode::Bitmap;
        context.Output.CustomData.Copy(&options);

        return CopyChunk(context, 0);
    }
};

#endif
