// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_LINUX

#include "Engine/Platform/Base/FileSystemBase.h"

/// <summary>
/// Linux platform implementation of filesystem service.
/// </summary>
class FLAXENGINE_API LinuxFileSystem : public FileSystemBase
{
public:

    // [FileSystemBase]
    static bool ShowOpenFileDialog(Window* parentWindow, const StringView& initialDirectory, const StringView& filter, bool multiSelect, const StringView& title, Array<String, HeapAllocation>& filenames);
    static bool ShowFileExplorer(const StringView& path);
    static bool CreateDirectory(const StringView& path);
    static bool DeleteDirectory(const String& path, bool deleteContents = true);
    static bool DirectoryExists(const StringView& path);
    static bool DirectoryGetFiles(Array<String, HeapAllocation>& results, const String& path, const Char* searchPattern, DirectorySearchOption option = DirectorySearchOption::AllDirectories);
    static bool GetChildDirectories(Array<String, HeapAllocation>& results, const String& directory);
    static bool FileExists(const StringView& path);
    static bool DeleteFile(const StringView& path);
    static uint64 GetFileSize(const StringView& path);
    static bool IsReadOnly(const StringView& path);
    static bool SetReadOnly(const StringView& path, bool isReadOnly);
    static bool MoveFile(const StringView& dst, const StringView& src, bool overwrite = false);
    static bool CopyFile(const StringView& dst, const StringView& src);

public:

    /// <summary>
    /// Gets last time when file has been modified (in UTC).
    /// </summary>
    /// <param name="path">The file path to check.</param>
    /// <returns>The last write time or DateTime::MinValue() if cannot get data.</returns>
    static DateTime GetFileLastEditTime(const StringView& path);

    /// <summary>
    /// Gets the special folder path.
    /// </summary>
    /// <param name="type">The folder type.</param>
    /// <param name="result">The result full path.</param>
    static void GetSpecialFolderPath(const SpecialFolder type, String& result);
    
private:

    static bool getFilesFromDirectoryTop(Array<String, HeapAllocation>& results, const char* path, const char* searchPattern);
    static bool getFilesFromDirectoryAll(Array<String, HeapAllocation>& results, const char* path, const char* searchPattern);
};

#endif
