// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WINDOWS
#include "Windows/WindowsClipboard.h"
#elif PLATFORM_LINUX
#include "Linux/LinuxClipboard.h"
#elif PLATFORM_MAC
#include "Mac/MacClipboard.h"
#else
#include "Base/ClipboardBase.h"
#endif

#include "Types.h"
