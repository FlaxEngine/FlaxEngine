// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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

API_INJECT_CPP_CODE("#include \"Engine/Platform/FileSystem.h\"");

/// <summary>
/// Platform implementation of filesystem service.
/// </summary>
API_CLASS(Static, Name="FileSystem") class FLAXENGINE_API FileSystemBase
{
DECLARE_SCRIPTING_TYPE_MINIMAL(FileSystemBase);

    /// <summary>
    /// Displays a standard dialog box that prompts the user to open a file(s).
    /// </summary>
    /// <param name="parentWindow">The parent window or null.</param>
    /// <param name="initialDirectory">The initial directory.</param>
    /// <param name="filter">The custom filter.</param>
    /// <param name="multiSelect">True if allow multiple files to be selected, otherwise use single-file mode.</param>
    /// <param name="title">The dialog title.</param>
    /// <param name="filenames">The output names of the files picked by the user.</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() static bool ShowOpenFileDialog(Window* parentWindow, const StringView& initialDirectory, const StringView& filter, bool multiSelect, const StringView& title, API_PARAM(Out) Array<String, HeapAllocation>& filenames);

    /// <summary>
    /// Displays a standard dialog box that prompts the user to save a file(s).
    /// </summary>
    /// <param name="parentWindow">The parent window.</param>
    /// <param name="initialDirectory">The initial directory.</param>
    /// <param name="filter">The filter.</param>
    /// <param name="multiSelect">True if allow multiple files to be selected, otherwise use single-file mode.</param>
    /// <param name="title">The title.</param>
    /// <param name="filenames">The output names of the files picked by the user.</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() static bool ShowSaveFileDialog(Window* parentWindow, const StringView& initialDirectory, const StringView& filter, bool multiSelect, const StringView& title, API_PARAM(Out) Array<String, HeapAllocation>& filenames);

    /// <summary>
    /// Displays a standard dialog box that prompts the user to select a folder.
    /// </summary>
    /// <param name="parentWindow">The parent window.</param>
    /// <param name="initialDirectory">The initial directory.</param>
    /// <param name="title">The dialog title.</param>
    /// <param name="path">The output path.</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() static bool ShowBrowseFolderDialog(Window* parentWindow, const StringView& initialDirectory, const StringView& title, API_PARAM(Out) StringView& path);

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
    /// Normalize input path for valid path name for current platform file system
    /// </summary>
    /// <param name="path">Path to normalize</param>
    static void NormalizePath(String& path);

    /// <summary>
    /// Check if path type is relative
    /// </summary>
    /// <param name="path">Input path to check</param>
    /// <returns>True if input path is relative one, otherwise false</returns>
    static bool IsRelative(const StringView& path);

    /// <summary>
    /// Retrieves file extension (without a dot)
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

    static bool CopyFile(const String& dst, const String& src);
    static bool CopyDirectory(const String& dst, const String& src, bool withSubDirectories);

public:

    /// <summary>
    /// Converts path relative to the engine startup folder into absolute path
    /// </summary>
    /// <param name="path">Path relative to the engine directory</param>
    /// <returns>Absolute path</returns>
    static String ConvertRelativePathToAbsolute(const String& path);

    /// <summary>
    /// Converts path relative to basePath into absolute path
    /// </summary>
    /// <param name="basePath">Base path</param>
    /// <param name="path">Path relative to basePath</param>
    /// <returns>Absolute path</returns>
    static String ConvertRelativePathToAbsolute(const String& basePath, const String& path);

    /// <summary>
    /// Converts absolute path into relative path to engine startup folder
    /// </summary>
    /// <param name="path">Absolute path</param>
    /// <returns>Relative path</returns>
    static String ConvertAbsolutePathToRelative(const String& path);

    /// <summary>
    /// Converts absolute path into relative path to basePath
    /// </summary>
    /// <param name="basePath">Base path</param>
    /// <param name="path">Absolute path</param>
    /// <returns>Relative path</returns>
    static String ConvertAbsolutePathToRelative(const String& basePath, const String& path);

private:

    static bool DirectoryCopyHelper(const String& dst, const String& src, bool withSubDirectories);
};
