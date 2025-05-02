// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"

#if COMPILE_WITH_ASSETS_IMPORTER

/// <summary>
/// Creating animation graph utility
/// </summary>
class CreateAnimationGraph
{
public:
    /// <summary>
    /// Creates the asset.
    /// </summary>
    /// <param name="context">The importing context.</param>
    /// <returns>Result.</returns>
    static CreateAssetResult Create(CreateAssetContext& context);
};

#endif
