// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if PLATFORM_MAC

#include "Engine/Platform/StringUtils.h"
#include <wctype.h>
#include <cctype>
#include <wchar.h>
#include <cstring>
#include <stdlib.h>

// Reuse Unix-impl
#define PLATFORM_UNIX 1
#include "../Unix/UnixStringUtils.cpp"

#endif
