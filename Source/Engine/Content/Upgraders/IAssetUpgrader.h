// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if USE_EDITOR

#include "Engine/Core/Types/BaseTypes.h"

/// <summary>
/// The assets upgrading objects interface.
/// </summary>
class FLAXENGINE_API IAssetUpgrader
{
public:
    /// <summary>
    /// Finalizes an instance of the <see cref="IAssetUpgrader"/> class.
    /// </summary>
    virtual ~IAssetUpgrader()
    {
    }

public:
    /// <summary>
    /// Checks if given asset version should be converted.
    /// </summary>
    /// <param name="serializedVersion">The serialized version.</param>
    /// <returns>True if perform conversion, otherwise false.</returns>
    virtual bool ShouldUpgrade(uint32 serializedVersion) const = 0;
};

#endif
