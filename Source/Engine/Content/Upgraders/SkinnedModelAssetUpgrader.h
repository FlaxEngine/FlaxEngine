// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if USE_EDITOR

#include "BinaryAssetUpgrader.h"

/// <summary>
/// Skinned Model Asset Upgrader
/// </summary>
/// <seealso cref="BinaryAssetUpgrader" />
class SkinnedModelAssetUpgrader : public BinaryAssetUpgrader
{
public:
    /// <summary>
    /// Initializes a new instance of the <see cref="SkinnedModelAssetUpgrader"/> class.
    /// </summary>
    SkinnedModelAssetUpgrader()
    {
        static const Upgrader upgraders[] =
        {
            {},
        };
        setup(upgraders, ARRAY_COUNT(upgraders));
    }
};

#endif
