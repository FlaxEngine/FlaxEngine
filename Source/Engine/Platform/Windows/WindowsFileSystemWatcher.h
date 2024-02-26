// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WINDOWS

#include "Engine/Platform/Base/FileSystemWatcherBase.h"
#include "../Win32/WindowsMinimal.h"

/// <summary>
/// Windows platform implementation of the file system watching object.
/// </summary>
class FLAXENGINE_API WindowsFileSystemWatcher : public FileSystemWatcherBase
{
public:

    /// <summary>
    /// Initializes a new instance of the <see cref="WindowsFileSystemWatcher"/> class.
    /// </summary>
    /// <param name="directory">The directory to watch.</param>
    /// <param name="withSubDirs">True if monitor the directory tree rooted at the specified directory or just a given directory.</param>
    WindowsFileSystemWatcher(const String& directory, bool withSubDirs);

    /// <summary>
    /// Finalizes an instance of the <see cref="WindowsFileSystemWatcher"/> class.
    /// </summary>
    ~WindowsFileSystemWatcher();

public:

    Windows::OVERLAPPED Overlapped;
    Windows::HANDLE DirectoryHandle;
    bool StopNow;
    int32 CurrentBuffer;
    static const int32 BufferSize = 32 * 1024;
    byte Buffer[2][BufferSize];
};

#endif
