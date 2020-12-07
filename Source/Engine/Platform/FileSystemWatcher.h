// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WINDOWS
#include "Windows/WindowsFileSystemWatcher.h"
#elif PLATFORM_UWP
#include "Base/FileSystemWatcherBase.h"
#elif PLATFORM_LINUX
#include "Base/FileSystemWatcherBase.h"
#elif PLATFORM_PS4
#include "Base/FileSystemWatcherBase.h"
#elif PLATFORM_XBOX_SCARLETT
#include "Base/FileSystemWatcherBase.h"
#elif PLATFORM_ANDROID
#include "Base/FileSystemWatcherBase.h"
#else
#error Missing File System Watcher implementation!
#endif

#include "Types.h"
