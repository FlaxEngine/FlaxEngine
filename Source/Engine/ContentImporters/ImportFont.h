// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"

#if COMPILE_WITH_ASSETS_IMPORTER

/// <summary>
/// Importing fonts utility
/// </summary>
class ImportFont
{
public:
    /// <summary>
    /// Imports the font file.
    /// </summary>
    /// <param name="context">The importing context.</param>
    /// <returns>Result.</returns>
    static CreateAssetResult Import(CreateAssetContext& context);
};

#endif
