// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if USE_EDITOR

#include "BinaryAssetUpgrader.h"

/// <summary>
/// Skeleton Mask asset upgrader.
/// </summary>
/// <seealso cref="BinaryAssetUpgrader" />
class SkeletonMaskUpgrader : public BinaryAssetUpgrader
{
public:
    SkeletonMaskUpgrader()
    {
        const Upgrader upgraders[] =
        {
            {},
        };
        setup(upgraders, ARRAY_COUNT(upgraders));
    }
};

#endif
