// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config.h"

/// <summary>
/// CSG brush mode
/// </summary>
API_ENUM() enum class BrushMode
{
    /// <summary>
    /// Brush adds
    /// </summary>
    Additive,

    /// <summary>
    /// Brush subtracts
    /// </summary>
    Subtractive,
};
