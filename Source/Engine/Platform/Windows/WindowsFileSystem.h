// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WINDOWS

#include "Engine/Platform/Win32/Win32FileSystem.h"

/// <summary>
/// Windows platform implementation of filesystem service
/// </summary>
class FLAXENGINE_API WindowsFileSystem : public Win32FileSystem
{
public:

    /// <summary>
    /// Moves a file to the recycle bin with possible undo instead of removing it permanently.
    /// </summary>
    /// <param name="path">The path to the file to delete.</param>
    /// <returns>True if cannot perform that operation, otherwise false.</returns>
    static bool MoveFileToRecycleBin(const StringView& path);

public:

    static bool AreFilePathsEqual(const StringView& path1, const StringView& path2);

public:

    /// <summary>
    /// Gets the special folder path.
    /// </summary>
    /// <param name="type">The folder type.</param>
    /// <param name="result">The result full path.</param>
    static void GetSpecialFolderPath(const SpecialFolder type, String& result);

public:

    // [Win32FileSystem]
    static bool ShowOpenFileDialog(Window* parentWindow, const StringView& initialDirectory, const StringView& filter, bool multiSelect, const StringView& title, Array<String, HeapAllocation>& filenames);
    static bool ShowSaveFileDialog(Window* parentWindow, const StringView& initialDirectory, const StringView& filter, bool multiSelect, const StringView& title, Array<String, HeapAllocation>& filenames);
    static bool ShowBrowseFolderDialog(Window* parentWindow, const StringView& initialDirectory, const StringView& title, String& path);
    static bool ShowFileExplorer(const StringView& path);
};

#endif
