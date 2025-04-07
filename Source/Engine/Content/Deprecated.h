// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Core.h"

#if USE_EDITOR

// Editor-only utility for marking content as deprecated during load. Used to auto-upgrade (by resaving) data during development in editor or during game cooking.
API_CLASS(Static) class FLAXENGINE_API ContentDeprecated
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(ContentDeprecated);

public:
    // Marks content as deprecated (for the current thread).
    API_FUNCTION() static void Mark();
    // Reads and clears deprecation flag (for the current thread).
    API_FUNCTION() static bool Clear(bool newValue = false);
};

// Marks content as deprecated (for the current thread).
#define MARK_CONTENT_DEPRECATED() ContentDeprecated::Mark()

#else

#define MARK_CONTENT_DEPRECATED()

#endif
