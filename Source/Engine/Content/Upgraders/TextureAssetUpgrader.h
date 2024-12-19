// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
    /// <summary>
    /// Initializes a new instance of the <see cref="TextureAssetUpgrader"/> class.
    /// </summary>
    TextureAssetUpgrader()
    {
        static const Upgrader upgraders[] =
        {
            {},
        };
        setup(upgraders, ARRAY_COUNT(upgraders));
    }
};

#endif
