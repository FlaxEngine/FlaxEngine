// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
    /// <summary>
    /// Initializes a new instance of the <see cref="SkeletonMaskUpgrader"/> class.
    /// </summary>
    SkeletonMaskUpgrader()
    {
        static const Upgrader upgraders[] =
        {
            {},
        };
        setup(upgraders, ARRAY_COUNT(upgraders));
    }
};

#endif
