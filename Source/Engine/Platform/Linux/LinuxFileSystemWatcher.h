// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_LINUX

#include "Engine/Platform/Base/FileSystemWatcherBase.h"

/// <summary>
/// Linux platform implementation of the file system watching object.
/// </summary>
class FLAXENGINE_API LinuxFileSystemWatcher : public FileSystemWatcherBase
{
public:

    /// <summary>
    /// Initializes a new instance of the <see cref="LinuxFileSystemWatcher"/> class.
    /// </summary>
    /// <param name="directory">The directory to watch.</param>
    /// <param name="withSubDirs">True if monitor the directory tree rooted at the specified directory or just a given directory.</param>
    /// <param name="rootWatcher">Linux specific root directory watcher file descriptor.</param>
    LinuxFileSystemWatcher(const String& directory, bool withSubDirs, int rootWatcher = -1);

    /// <summary>
    /// Finalizes an instance of the <see cref="LinuxFileSystemWatcher"/> class.
    /// </summary>
    ~LinuxFileSystemWatcher();

public:

    FORCE_INLINE int GetWachedDirectoryDescriptor()
    {
        return WachedDirectory;
    }
    
private:

    int WachedDirectory;
    int RootWatcher;
};

#endif
