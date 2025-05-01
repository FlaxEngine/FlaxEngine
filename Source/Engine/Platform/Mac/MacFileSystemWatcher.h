// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_MAC
#include "Engine/Platform/Base/FileSystemWatcherBase.h"

#include <CoreServices/CoreServices.h>


/// <summary>
/// Mac platform implementation of the file system watching object.
/// </summary>
class FLAXENGINE_API MacFileSystemWatcher : public FileSystemWatcherBase
{
public:

    /// <summary>
    /// Initializes a new instance of the <see cref="MacFileSystemWatcher"/> class.
    /// </summary>
    /// <param name="directory">The directory to watch.</param>
    /// <param name="withSubDirs">True if monitor the directory tree rooted at the specified directory or just a given directory.</param>
    MacFileSystemWatcher(const String& directory, bool withSubDirs);

    /// <summary>
    /// Finalizes an instance of the <see cref="MacFileSystemWatcher"/> class.
    /// </summary>
    ~MacFileSystemWatcher();

public:


    
private:

    FSEventStreamRef	EventStream;
    bool                IsRunning;
};
#endif
