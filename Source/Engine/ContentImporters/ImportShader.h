// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"

#if COMPILE_WITH_ASSETS_IMPORTER

/// <summary>
/// Importing shaders utility
/// </summary>
class ImportShader
{
public:
    /// <summary>
    /// Imports the shader file.
    /// </summary>
    /// <param name="context">The importing context.</param>
    /// <returns>Result.</returns>
    static CreateAssetResult Import(CreateAssetContext& context);
};

#endif
