// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "../Config.h"

#if COMPILE_WITH_SHADER_COMPILER

#include "Engine/Utilities/TextProcessing.h"

namespace ShaderProcessing
{
    typedef TextProcessing Reader;
    typedef Reader::Token Token;
    typedef Reader::SeparatorData Separator;
}

#endif

// Don't count ending '\0' character
#define MACRO_LENGTH(macro) (ARRAY_COUNT(macro) - 1)
