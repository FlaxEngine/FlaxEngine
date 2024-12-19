// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if USE_EDITOR

#include "BinaryAssetUpgrader.h"

/// <summary>
/// Model Asset Upgrader
/// </summary>
/// <seealso cref="BinaryAssetUpgrader" />
class ModelAssetUpgrader : public BinaryAssetUpgrader
{
public:
    /// <summary>
    /// Initializes a new instance of the <see cref="ModelAssetUpgrader"/> class.
    /// </summary>
    ModelAssetUpgrader()
    {
        static const Upgrader upgraders[] =
        {
            {},
        };
        setup(upgraders, ARRAY_COUNT(upgraders));
    }
};

#endif
