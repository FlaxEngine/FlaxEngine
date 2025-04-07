// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if USE_EDITOR

#include "BinaryAssetUpgrader.h"

/// <summary>
/// Audio Clip asset upgrader.
/// </summary>
/// <seealso cref="BinaryAssetUpgrader" />
class AudioClipUpgrader : public BinaryAssetUpgrader
{
public:
    AudioClipUpgrader()
    {
        const Upgrader upgraders[] =
        {
            {},
        };
        setup(upgraders, ARRAY_COUNT(upgraders));
    }

private:
};

#endif
