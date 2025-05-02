// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Delegate.h"
#include "Engine/Core/NonCopyable.h"
#include "Engine/Core/Types/String.h"

/// <summary>
/// Action types that file system watcher can listen for.
/// </summary>
enum class FileSystemAction
{
    Unknown = 0,
    Create = 1,
    Delete = 2,
    Modify = 4,
    Rename = 8,
};

/// <summary>
/// Base class for file system watcher objects.
/// </summary>
class FLAXENGINE_API FileSystemWatcherBase : public NonCopyable
{
public:

    FileSystemWatcherBase(const String& directory, bool withSubDirs)
        : Directory(directory)
        , WithSubDirs(withSubDirs)
        , Enabled(true)
    {
    }

public:

    /// <summary>
    /// The watcher directory path.
    /// </summary>
    const String Directory;

    /// <summary>
    /// The value whenever watcher is tracking changes in subdirectories.
    /// </summary>
    const bool WithSubDirs;

    /// <summary>
    /// The current watcher enable state.
    /// </summary>
    bool Enabled;

    /// <summary>
    /// Action fired when directory or file gets changed. Can be invoked from main or other thread depending on the platform.
    /// </summary>
    Delegate<const String&, FileSystemAction> OnEvent;
};
