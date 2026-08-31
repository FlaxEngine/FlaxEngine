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
            { 4, 5, &Upgrade_4_To_5 },
        };
        setup(upgraders, ARRAY_COUNT(upgraders));
    }

private:
    struct FontOptions3
    {
        FontHinting Hinting;
        FontFlags Flags;
    };

    static bool Upgrade_3_To_4(AssetMigrationContext& context)
    {
        ASSERT(context.Input.SerializedVersion == 3 && context.Output.SerializedVersion == 4);

        FontOptions3 optionsOld;
        Platform::MemoryCopy(&optionsOld, context.Input.CustomData.Get(), sizeof(FontOptions3));

        FontOptions options;
        options.Hinting = optionsOld.Hinting;
        options.Flags = optionsOld.Flags;
        options.RasterMode = FontRasterMode::Bitmap;
        context.Output.CustomData.Copy(&options);

        return CopyChunk(context, 0);
    }

    struct FontOptions4
    {
        FontHinting Hinting;
        FontFlags Flags;
        FontRasterMode RasterMode;
    };

    static bool Upgrade_4_To_5(AssetMigrationContext& context)
    {
        ASSERT(context.Input.SerializedVersion == 4 && context.Output.SerializedVersion == 5);

        FontOptions4 optionsOld;
        Platform::MemoryCopy(&optionsOld, context.Input.CustomData.Get(), sizeof(FontOptions4));

        FontOptions options;
        options.Hinting = optionsOld.Hinting;
        options.Flags = optionsOld.Flags;
        options.RasterMode = optionsOld.RasterMode;
        options.MSDFSize = 32.0f;
        context.Output.CustomData.Copy(&options);

        return CopyChunk(context, 0);
    }
};

#endif
