// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if USE_EDITOR

#include "BinaryAssetUpgrader.h"

/// <summary>
/// Material Instance Upgrader
/// </summary>
/// <seealso cref="BinaryAssetUpgrader" />
class MaterialInstanceUpgrader : public BinaryAssetUpgrader
{
public:
    /// <summary>
    /// Initializes a new instance of the <see cref="MaterialInstanceUpgrader"/> class.
    /// </summary>
    MaterialInstanceUpgrader()
    {
        static const Upgrader upgraders[] =
        {
            {},
        };
        setup(upgraders, ARRAY_COUNT(upgraders));
    }
};

#endif
