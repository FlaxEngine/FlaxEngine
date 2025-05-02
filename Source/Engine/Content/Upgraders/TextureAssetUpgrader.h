// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if USE_EDITOR

#include "BinaryAssetUpgrader.h"

/// <summary>
/// Texture Asset Upgrader
/// </summary>
/// <seealso cref="BinaryAssetUpgrader" />
class TextureAssetUpgrader : public BinaryAssetUpgrader
{
public:
    TextureAssetUpgrader()
    {
        const Upgrader upgraders[] =
        {
            {},
        };
        setup(upgraders, ARRAY_COUNT(upgraders));
    }
};

#endif
