// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if USE_EDITOR

#include "BinaryAssetUpgrader.h"

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
            {},
        };
        setup(upgraders, ARRAY_COUNT(upgraders));
    }
};

#endif
