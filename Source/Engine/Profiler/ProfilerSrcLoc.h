// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_PROFILER

#include "Engine/Core/Types/BaseTypes.h"

struct FLAXENGINE_API SourceLocationData
{
    const char* name;
    const char* function;
    const char* file;
    uint32 line;
    uint32 color;
};

#endif
