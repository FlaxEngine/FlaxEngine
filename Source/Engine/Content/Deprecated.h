// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Core.h"

#if USE_EDITOR

// Utility for marking content as deprecated when loading it in Editor. Used to auto-upgrade (by resaving) data during development in editor or during game cooking.
class FLAXENGINE_API ContentDeprecated
{
public:
    // Marks content as deprecated (for the current thread).
    static void Mark();
    // Reads and clears deprecation flag (for the current thread).
    static bool Clear(bool newValue = false);
};

// Marks content as deprecated (for the current thread).
#define MARK_CONTENT_DEPRECATED() ContentDeprecated::Mark()

#else

#define MARK_CONTENT_DEPRECATED()

#endif
