// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_ANDROID

#include "Engine/Platform/Base/FileSystemBase.h"

/// <summary>
/// Android platform implementation of filesystem service.
/// </summary>
class FLAXENGINE_API AndroidFileSystem : public FileSystemBase
{
public:
    static bool CreateDirectory(const StringView& path);
    static bool DeleteDirectory(const String& path, bool deleteContents = true);
    static bool DirectoryExists(const StringView& path);
    static bool DirectoryGetFiles(Array<String, HeapAllocation>& results, const String& path, const Char* searchPattern = TEXT("*"), DirectorySearchOption option = DirectorySearchOption::AllDirectories);
    static bool GetChildDirectories(Array<String, HeapAllocation>& results, const String& path);
    static bool FileExists(const StringView& path);
    static bool DeleteFile(const StringView& path);
    static uint64 GetFileSize(const StringView& path);
    static bool IsReadOnly(const StringView& path);
    static bool SetReadOnly(const StringView& path, bool isReadOnly);
    static bool MoveFile(const StringView& dst, const StringView& src, bool overwrite = false);
    static bool CopyFile(const StringView& dst, const StringView& src);
    static DateTime GetFileLastEditTime(const StringView& path);
    static void GetSpecialFolderPath(const SpecialFolder type, String& result);
    
private:
    static bool getFilesFromDirectoryTop(Array<String, HeapAllocation>& results, const char* path, const char* searchPattern);
    static bool getFilesFromDirectoryAll(Array<String, HeapAllocation>& results, const char* path, const char* searchPattern);
};

#endif
