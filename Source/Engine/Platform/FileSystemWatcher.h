// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WINDOWS
#include "Windows/WindowsFileSystemWatcher.h"
#elif PLATFORM_LINUX
#include "Linux/LinuxFileSystemWatcher.h"
#else
#include "Base/FileSystemWatcherBase.h"
#endif

#include "Types.h"
