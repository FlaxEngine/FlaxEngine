// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_LINUX

#include "Engine/Platform/Unix/UnixFileSystem.h"

/// <summary>
/// Linux platform implementation of filesystem service.
/// </summary>
class FLAXENGINE_API LinuxFileSystem : public UnixFileSystem
{
public:
    // [FileSystemBase]
    static bool ShowOpenFileDialog(Window* parentWindow, const StringView& initialDirectory, const StringView& filter, bool multiSelect, const StringView& title, Array<String, HeapAllocation>& filenames);
    static bool ShowBrowseFolderDialog(Window* parentWindow, const StringView& initialDirectory, const StringView& title, String& path);
    static bool ShowFileExplorer(const StringView& path);
    static bool CopyFile(const StringView& dst, const StringView& src);
    static bool MoveFileToRecycleBin(const StringView& path);
    static void GetSpecialFolderPath(const SpecialFolder type, String& result);

private:
    static bool UrnEncodePath(const char *path, char *result, int maxLength);
    static String getBaseName(const StringView& path);
    static String getNameWithoutExtension(const StringView& path);
};

#endif
