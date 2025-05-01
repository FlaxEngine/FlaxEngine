// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WINDOWS
#include "Windows/WindowsFileSystemWatcher.h"
#elif PLATFORM_LINUX
#include "Linux/LinuxFileSystemWatcher.h"
#elif PLATFORM_MAC
#include "Mac/MacFileSystemWatcher.h"
#else
#include "Base/FileSystemWatcherBase.h"
#endif

#include "Types.h"
