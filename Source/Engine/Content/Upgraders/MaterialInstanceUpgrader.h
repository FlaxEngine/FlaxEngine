// Copyright (c) Wojciech Figat. All rights reserved.

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
    MaterialInstanceUpgrader()
    {
        const Upgrader upgraders[] =
        {
            {},
        };
        setup(upgraders, ARRAY_COUNT(upgraders));
    }
};

#endif
