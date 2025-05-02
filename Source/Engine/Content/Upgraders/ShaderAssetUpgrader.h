// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if USE_EDITOR

#include "BinaryAssetUpgrader.h"

/// <summary>
/// Material Asset and Shader Asset Upgrader
/// </summary>
/// <seealso cref="BinaryAssetUpgrader" />
class ShaderAssetUpgrader : public BinaryAssetUpgrader
{
public:
    ShaderAssetUpgrader()
    {
        const Upgrader upgraders[] =
        {
            {},
        };
        setup(upgraders, ARRAY_COUNT(upgraders));
    }
};

#endif
