// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WINDOWS
#include "Windows/WindowsScreenUtilities.h"
#elif PLATFORM_LINUX
#include "Linux/LinuxScreenUtilities.h"
#elif PLATFORM_MAC
#include "Mac/MacScreenUtilities.h"
#else
#include "Base/ScreenUtilitiesBase.h"
#endif

#include "Types.h"
