// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WINDOWS
#include "Windows/WindowsClipboard.h"
#elif PLATFORM_LINUX
#include "Linux/LinuxClipboard.h"
#else
#include "Base/ClipboardBase.h"
#endif

#include "Types.h"
