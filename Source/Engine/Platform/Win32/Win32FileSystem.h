// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WIN32

#include "Engine/Platform/Base/FileSystemBase.h"

/// <summary>
/// Win32 platform implementation of filesystem service
/// </summary>
class FLAXENGINE_API Win32FileSystem : public FileSystemBase
{
    // TODO: fix docs

public:

    // Creates a new directory
    // @param path Directory path
    // @returns True if cannot create directory, otherwise false
    static bool CreateDirectory(const StringView& path);

    // Deletes an existing directory
    // @param path Directory path
    // @param deleteSubdirectories True if delete all subdirectories and files, otherwise false
    // @returns True if cannot delete directory, otherwise false
    static bool DeleteDirectory(const String& path, bool deleteContents = true);

    // Check if directory exists
    // @param path Directory path to check
    // @returns True if directory exists, otherwise false
    static bool DirectoryExists(const StringView& path);

    // Finds the names of files (including their paths) that match the specified search pattern in the specified directory, using a value to determine whether to search subdirectories
    // @param results When this method completes, this list contains list of all filenames that match the specified search pattern
    // @param path Path of the directory to search in it
    // @param searchPattern Custom search pattern to use during that operation
    // @param option Additional search options
    // @returns True if an error occurred, otherwise false
    static bool DirectoryGetFiles(Array<String, HeapAllocation>& results, const String& path, const Char* searchPattern, DirectorySearchOption option = DirectorySearchOption::AllDirectories);

    // Finds the names of directories (including their paths) that are inside the specified directory
    // @param results When this method completes, this list contains list of all filenames that match the specified search pattern
    // @param directory Path of the directory to search in it
    // @returns True if an error occurred, otherwise false
    static bool GetChildDirectories(Array<String, HeapAllocation>& results, const String& directory);

public:

    // Check if file exists
    // @param path File path to check
    // @returns True if file exists, otherwise false
    static bool FileExists(const StringView& path);

    // Deletes an existing file
    // @param path File path
    // @returns True if cannot delete file, otherwise false
    static bool DeleteFile(const StringView& path);

    // Tries to get size of the file
    // @param path File path to check
    // @returns Amount of bytes in file
    static uint64 GetFileSize(const StringView& path);

    // Check if file is read-only
    // @param path File path to check
    // @returns True if file is read-only
    static bool IsReadOnly(const StringView& path);

    // Sets file read-only flag
    // @param path File path
    // @returns True if cannot update file
    static bool SetReadOnly(const StringView& path, bool isReadOnly);

    // Move file
    // @param dst Destination path
    // @param src Source path
    // @returns True if cannot move file
    static bool MoveFile(const StringView& dst, const StringView& src, bool overwrite = false);

    // Clone file
    // @param dst Destination path
    // @param src Source path
    // @returns True if cannot copy file
    static bool CopyFile(const StringView& dst, const StringView& src);

public:

    /// <summary>
    /// Converts the UNIX style line endings into DOS style (from \n into \r\n).
    /// </summary>
    /// <param name="text">The input text data.</param>
    /// <param name="output">The output result.</param>
    static void ConvertLineEndingsToDos(const StringView& text, Array<Char, HeapAllocation>& output);

public:

    /// <summary>
    /// Gets last time when file has been modified (in UTC).
    /// </summary>
    /// <param name="path">The file path to check.</param>
    /// <returns>The last write time or DateTime::MinValue() if cannot get data.</returns>
    static DateTime GetFileLastEditTime(const StringView& path);

private:

    static bool getFilesFromDirectoryTop(Array<String, HeapAllocation>& results, const String& directory, const Char* searchPattern);
    static bool getFilesFromDirectoryAll(Array<String, HeapAllocation>& results, const String& directory, const Char* searchPattern);
};

#endif
