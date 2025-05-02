// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Platform/Types.h"

/// <summary>
/// Specifies whether to search the current directory, or the current directory and all subdirectories.
/// </summary>
enum class DirectorySearchOption
{
    /// <summary>
    /// Includes the current directory and all its subdirectories in a search operation.This option includes reparse points such as mounted drives and symbolic links in the search.
    /// </summary>
    AllDirectories,

    /// <summary>
    /// Includes only the current directory in a search operation.
    /// </summary>
    TopDirectoryOnly,
};

/// <summary>
/// Special system folder types.
/// </summary>
enum class SpecialFolder
{
    Desktop,
    Documents,
    Pictures,
    AppData,
    LocalAppData,
    ProgramData,
    Temporary,
};

API_INJECT_CODE(cpp, "#include \"Engine/Platform/FileSystem.h\"");

/// <summary>
/// Platform implementation of filesystem service.
/// </summary>
API_CLASS(Static, Name="FileSystem", Tag="NativeInvokeUseName")
class FLAXENGINE_API FileSystemBase
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(FileSystemBase);

    /// <summary>
    /// Creates a new directory.
    /// </summary>
    /// <param name="path">Directory path</param>
    /// <returns>True if failed to create directory, otherwise false.</returns>
    static bool CreateDirectory(const StringView& path) = delete;

    /// <summary>
    /// Deletes an existing directory.
    /// </summary>
    /// <param name="path">Directory path</param>
    /// <param name="deleteContents">True if delete all subdirectories and files, otherwise false.</param>
    /// <returns>True if failed to delete directory, otherwise false.</returns>
    static bool DeleteDirectory(const String& path, bool deleteContents = true) = delete;

    /// <summary>
    /// Checks if directory exists.
    /// </summary>
    /// <param name="path">Directory path.</param>
    /// <returns>True if directory exists, otherwise false.</returns>
    static bool DirectoryExists(const StringView& path) = delete;

    /// <summary>
    /// Finds the paths of files that match the specified search pattern in the specified directory, using a value to determine whether to search subdirectories.
    /// </summary>
    /// <param name="results">Output list with all found file paths. Items are appended without clearing the list first.</param>
    /// <param name="path">Path of the directory to search in it.</param>
    /// <param name="searchPattern">Custom search pattern to use during that operation Use asterisk character (*) for name-based filtering (eg. `*.txt` to find all files with `.txt` extension)..</param>
    /// <param name="option">Additional search options that define rules.</param>
    /// <returns>True if an error occurred, otherwise false.</returns>
    static bool DirectoryGetFiles(Array<String, HeapAllocation>& results, const String& path, const Char* searchPattern = TEXT("*"), DirectorySearchOption option = DirectorySearchOption::AllDirectories) = delete;

    /// <summary>
    /// Finds the paths of directories that are inside the specified directory.
    /// </summary>
    /// <param name="results">Output list with all found directory paths. Items are appended without clearing the list first.</param>
    /// <param name="path">Path of the directory to search in it.</param>
    /// <returns>True if an error occurred, otherwise false.</returns>
    static bool GetChildDirectories(Array<String, HeapAllocation>& results, const String& path) = delete;

    /// <summary>
    /// Copies the directory.
    /// </summary>
    /// <param name="dst">Destination path.</param>
    /// <param name="src">Source directory path.</param>
    /// <param name="withSubDirectories">True if copy subdirectories of the source folder, otherwise only top-level files will be cloned.</param>
    /// <returns>True if failed, otherwise false.</returns>
    static bool CopyDirectory(const String& dst, const String& src, bool withSubDirectories = true);

    /// <summary>
    /// Gets the size of the directory (in bytes) defined by size of all files contained by it.
    /// </summary>
    /// <param name="path">Directory path.</param>
    /// <returns>Amount of bytes in directory, or 0 if failed.</returns>
    static uint64 GetDirectorySize(const StringView& path);

public:
    /// <summary>
    /// Checks if a given file exists.
    /// </summary>
    /// <param name="path">File path to check.</param>
    /// <returns>True if file exists, otherwise false.</returns>
    static bool FileExists(const StringView& path) = delete;

    /// <summary>
    /// Deletes an existing file.
    /// </summary>
    /// <param name="path">File path</param>
    /// <returns>True if operation failed, otherwise false.</returns>
    static bool DeleteFile(const StringView& path) = delete;

    /// <summary>
    /// Gets the size of the file (in bytes).
    /// </summary>
    /// <param name="path">File path</param>
    /// <returns>Amount of bytes in file, or 0 if failed.</returns>
    static uint64 GetFileSize(const StringView& path) = delete;

    /// <summary>
    /// Checks if file is read-only.
    /// </summary>
    /// <param name="path">File path.</param>
    /// <returns>True if file is read-only, otherwise false. Returns false if failed or path is invalid.</returns>
    static bool IsReadOnly(const StringView& path) = delete;

    /// <summary>
    /// Sets file read-only flag.
    /// </summary>
    /// <param name="path">File path.</param>
    /// <param name="isReadOnly">Read-only flag value to set.</param>
    /// <returns>True if operation failed, otherwise false.</returns>
    static bool SetReadOnly(const StringView& path, bool isReadOnly) = delete;

    /// <summary>
    /// Moves the file.
    /// </summary>
    /// <param name="dst">Destination path.</param>
    /// <param name="src">Source file path.</param>
    /// <param name="overwrite">True if allow overriding destination file if it already exists, otherwise action will fail.</param>
    /// <returns>True if failed, otherwise false.</returns>
    static bool MoveFile(const StringView& dst, const StringView& src, bool overwrite = false) = delete;

    /// <summary>
    /// Copies the file.
    /// </summary>
    /// <param name="dst">Destination path.</param>
    /// <param name="src">Source file path.</param>
    /// <returns>True if failed, otherwise false.</returns>
    static bool CopyFile(const StringView& dst, const StringView& src);

    /// <summary>
    /// Gets last time when file has been modified (in UTC).
    /// </summary>
    /// <param name="path">The file path to check.</param>
    /// <returns>The last write time or DateTime::MinValue() if cannot get data.</returns>
    static DateTime GetFileLastEditTime(const StringView& path) = delete;

    /// <summary>
    /// Gets the special folder path.
    /// </summary>
    /// <param name="type">The folder type.</param>
    /// <param name="result">The result full path.</param>
    static void GetSpecialFolderPath(const SpecialFolder type, String& result) = delete;

public:
    /// <summary>
    /// Displays a standard dialog box that prompts the user to open a file(s).
    /// </summary>
    /// <param name="parentWindow">The parent window or null.</param>
    /// <param name="initialDirectory">The initial directory.</param>
    /// <param name="filter">The file filter string as null-terminated pairs of name and list of extensions. Multiple file extensions must be separated with semicolon.</param>
    /// <param name="multiSelect">True if allow multiple files to be selected, otherwise use single-file mode.</param>
    /// <param name="title">The dialog title.</param>
    /// <param name="filenames">The output names of the files picked by the user.</param>
    /// <returns>True if failed, otherwise false.</returns>
    /// <remarks>
    /// Example file filters:
    ///    "All Files\0*.*"
    ///    "All Files\0*.*\0Image Files\0*.png;*.jpg"
    /// </remarks>
    API_FUNCTION() static bool ShowOpenFileDialog(Window* parentWindow, const StringView& initialDirectory, const StringView& filter, bool multiSelect, const StringView& title, API_PARAM(Out) Array<String, HeapAllocation>& filenames);

    /// <summary>
    /// Displays a standard dialog box that prompts the user to save a file(s).
    /// </summary>
    /// <param name="parentWindow">The parent window.</param>
    /// <param name="initialDirectory">The initial directory.</param>
    /// <param name="filter">The file filter string as null-terminated pairs of name and list of extensions. Multiple file extensions must be separated with semicolon.</param>
    /// <param name="multiSelect">True if allow multiple files to be selected, otherwise use single-file mode.</param>
    /// <param name="title">The title.</param>
    /// <param name="filenames">The output names of the files picked by the user.</param>
    /// <returns>True if failed, otherwise false.</returns>
    /// <remarks>
    /// Example file filters:
    ///    "All Files\0*.*"
    ///    "All Files\0*.*\0Image Files\0*.png;*.jpg"
    /// </remarks>
    API_FUNCTION() static bool ShowSaveFileDialog(Window* parentWindow, const StringView& initialDirectory, const StringView& filter, bool multiSelect, const StringView& title, API_PARAM(Out) Array<String, HeapAllocation>& filenames);

    /// <summary>
    /// Displays a standard dialog box that prompts the user to select a folder.
    /// </summary>
    /// <param name="parentWindow">The parent window.</param>
    /// <param name="initialDirectory">The initial directory.</param>
    /// <param name="title">The dialog title.</param>
    /// <param name="path">The output path.</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() static bool ShowBrowseFolderDialog(Window* parentWindow, const StringView& initialDirectory, const StringView& title, API_PARAM(Out) String& path);

    /// <summary>
    /// Opens a standard file explorer application and navigates to the given directory.
    /// </summary>
    /// <param name="path">The path.</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() static bool ShowFileExplorer(const StringView& path);

public:
    static void SaveBitmapToFile(byte* data, uint32 width, uint32 height, uint32 bitsPerPixel, const uint32 padding, const String& path);

public:
    static bool AreFilePathsEqual(const StringView& path1, const StringView& path2);

    /// <summary>
    /// Normalizes input path for valid path name for current platform file system.
    /// </summary>
    /// <param name="path">Path to normalize</param>
    static void NormalizePath(String& path);

    /// <summary>
    /// Checks if path type is relative.
    /// </summary>
    /// <param name="path">Input path to check</param>
    /// <returns>True if input path is relative one, otherwise false</returns>
    static bool IsRelative(const StringView& path);

    /// <summary>
    /// Retrieves file extension (without a dot).
    /// </summary>
    /// <param name="path">Input path to process</param>
    /// <returns>File extension</returns>
    static String GetExtension(const StringView& path);

    /// <summary>
    /// Gets the file path to the temporary file that can be created and used.
    /// </summary>
    /// <param name="tmpPath">The temporary path.</param>
    static void GetTempFilePath(String& tmpPath);

public:
    /// <summary>
    /// Converts path relative to the engine startup folder into absolute path.
    /// </summary>
    /// <param name="path">Path relative to the engine directory</param>
    /// <returns>Absolute path</returns>
    static String ConvertRelativePathToAbsolute(const String& path);

    /// <summary>
    /// Converts path relative to basePath into absolute path.
    /// </summary>
    /// <param name="basePath">Base path</param>
    /// <param name="path">Path relative to basePath</param>
    /// <returns>Absolute path</returns>
    static String ConvertRelativePathToAbsolute(const String& basePath, const String& path);

    /// <summary>
    /// Converts absolute path into relative path to engine startup folder.
    /// </summary>
    /// <param name="path">Absolute path</param>
    /// <returns>Relative path</returns>
    static String ConvertAbsolutePathToRelative(const String& path);

    /// <summary>
    /// Converts absolute path into relative path to basePath.
    /// </summary>
    /// <param name="basePath">Base path</param>
    /// <param name="path">Absolute path</param>
    /// <returns>Relative path</returns>
    static String ConvertAbsolutePathToRelative(const String& basePath, const String& path);

private:
    static bool DirectoryCopyHelper(const String& dst, const String& src, bool withSubDirectories);
};
