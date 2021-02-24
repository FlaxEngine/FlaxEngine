// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WINDOWS
#include "Windows/WindowsClipboard.h"
#elif PLATFORM_UWP
#include "Base/ClipboardBase.h"
#elif PLATFORM_LINUX
#include "Linux/LinuxClipboard.h"
#elif PLATFORM_PS4
#include "Base/ClipboardBase.h"
#elif PLATFORM_XBOX_SCARLETT
#include "Base/ClipboardBase.h"
#elif PLATFORM_ANDROID
#include "Base/ClipboardBase.h"
#else
#error Missing Clipboard implementation!
#endif

#include "Types.h"
