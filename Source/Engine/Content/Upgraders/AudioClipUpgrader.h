// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
    /// <summary>
    /// Initializes a new instance of the <see cref="AudioClipUpgrader"/> class.
    /// </summary>
    AudioClipUpgrader()
    {
        static const Upgrader upgraders[] =
        {
            {},
        };
        setup(upgraders, ARRAY_COUNT(upgraders));
    }

private:
};

#endif
