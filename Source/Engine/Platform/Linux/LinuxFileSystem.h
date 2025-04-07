// Copyright (c) Wojciech Figat. All rights reserved.

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
    static bool ShowBrowseFolderDialog(Window* parentWindow, const StringView& initialDirectory, const StringView& title, String& path);
    static bool ShowFileExplorer(const StringView& path);
    static bool CreateDirectory(const StringView& path);
    static bool DeleteDirectory(const String& path, bool deleteContents = true);
    static bool DirectoryExists(const StringView& path);
    static bool DirectoryGetFiles(Array<String, HeapAllocation>& results, const String& path, const Char* searchPattern = TEXT("*"), DirectorySearchOption option = DirectorySearchOption::AllDirectories);
    static bool GetChildDirectories(Array<String, HeapAllocation>& results, const String& path);
    static bool FileExists(const StringView& path);
    static bool DeleteFile(const StringView& path);
    static bool MoveFileToRecycleBin(const StringView& path);
    static uint64 GetFileSize(const StringView& path);
    static bool IsReadOnly(const StringView& path);
    static bool SetReadOnly(const StringView& path, bool isReadOnly);
    static bool MoveFile(const StringView& dst, const StringView& src, bool overwrite = false);
    static bool CopyFile(const StringView& dst, const StringView& src);
    static DateTime GetFileLastEditTime(const StringView& path);
    static void GetSpecialFolderPath(const SpecialFolder type, String& result);

private:
    static bool UrnEncodePath(const char *path, char *result, int maxLength);
    static bool getFilesFromDirectoryTop(Array<String, HeapAllocation>& results, const char* path, const char* searchPattern);
    static bool getFilesFromDirectoryAll(Array<String, HeapAllocation>& results, const char* path, const char* searchPattern);
    static String getBaseName(const StringView& path);
    static String getNameWithoutExtension(const StringView& path);
};

#endif
