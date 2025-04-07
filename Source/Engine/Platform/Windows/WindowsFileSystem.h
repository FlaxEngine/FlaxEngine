// Copyright (c) Wojciech Figat. All rights reserved.

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

    static bool AreFilePathsEqual(const StringView& path1, const StringView& path2);

public:
    // [Win32FileSystem]
    static bool ShowOpenFileDialog(Window* parentWindow, const StringView& initialDirectory, const StringView& filter, bool multiSelect, const StringView& title, Array<String, HeapAllocation>& filenames);
    static bool ShowSaveFileDialog(Window* parentWindow, const StringView& initialDirectory, const StringView& filter, bool multiSelect, const StringView& title, Array<String, HeapAllocation>& filenames);
    static bool ShowBrowseFolderDialog(Window* parentWindow, const StringView& initialDirectory, const StringView& title, String& path);
    static bool ShowFileExplorer(const StringView& path);
    static void GetSpecialFolderPath(const SpecialFolder type, String& result);
};

#endif
