// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Delegate.h"
#include "Engine/Core/NonCopyable.h"
#include "Engine/Core/Types/String.h"

/// <summary>
/// Action types that file system watcher can listen for.
/// </summary>
enum class FileSystemAction
{
    Unknown,
    Create,
    Delete,
    Modify,
};

/// <summary>
/// Base class for file system watcher objects.
/// </summary>
class FLAXENGINE_API FileSystemWatcherBase : public NonCopyable
{
protected:

    bool _isEnabled;
    bool _withSubDirs;
    String _directory;

public:

    FileSystemWatcherBase(const String& directory, bool withSubDirs)
        : _isEnabled(true)
        , _withSubDirs(withSubDirs)
        , _directory(directory)
    {
    }

    /// <summary>
    /// Action fired when directory or file gets changed. Can be invoked from main or other thread depending on the platform.
    /// </summary>
    Delegate<const String&, FileSystemAction> OnEvent;

public:

    /// <summary>
    /// Gets the watcher directory string.
    /// </summary>
    /// <returns>The target directory path.</returns>
    const String& GetDirectory() const
    {
        return _directory;
    }

    /// <summary>
    /// Gets the value whenever watcher is tracking changes in subdirectories.
    /// </summary>
    /// <returns>True if watcher is tracking changes in subdirectories, otherwise false.</returns>
    bool WithSubDirs() const
    {
        return _withSubDirs;
    }

    /// <summary>
    /// Gets the current watcher enable state.
    /// </summary>
    /// <returns>True if watcher is enabled, otherwise false.</returns>
    bool GetEnabled() const
    {
        return _isEnabled;
    }

    /// <summary>
    /// Sets the current enable state.
    /// </summary>
    /// <param name="value">A state to assign.</param>
    void SetEnabled(bool value)
    {
        _isEnabled = value;
    }
};
