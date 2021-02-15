// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Common.h"

enum class PixelFormat : unsigned;
enum class DirectorySearchOption;
class TextureData;

/// <summary>
/// Helper functions for the editor.
/// </summary>
class EditorUtilities
{
public:

    enum class ApplicationImageType
    {
        Icon,
        SplashScreen,
    };

    /// <summary>
    /// Updates the Win32 executable file icon.
    /// </summary>
    /// <param name="path">The exe path.</param>
    /// <param name="icon">The icon image data.</param>
    /// <returns>True if fails, otherwise false.</returns>
    static bool UpdateExeIcon(const String& path, const TextureData& icon);

    static bool GetApplicationImage(const Guid& imageId, TextureData& imageData, ApplicationImageType type = ApplicationImageType::Icon);
    static bool GetTexture(const Guid& textureId, TextureData& textureData);
    static bool ExportApplicationImage(const Guid& iconId, int32 width, int32 height, PixelFormat format, const String& path, ApplicationImageType type = ApplicationImageType::Icon);
    static bool ExportApplicationImage(const TextureData& icon, int32 width, int32 height, PixelFormat format, const String& path);

public:

    static bool FindWDKBin(String& outputWdkBinPath);
    static bool GenerateCertificate(const String& name, const String& outputPfxFilePath);
    static bool GenerateCertificate(const String& name, const String& outputPfxFilePath, const String& outputCerFilePath, const String& outputPvkFilePath);

public:

    /// <summary>
    /// Determines whether the specified path character is invalid.
    /// </summary>
    /// <param name="c">The path character.</param>
    /// <returns><c>true</c> if the given character cannot be used as a path because it is illegal character; otherwise, <c>false</c>.</returns>
    static bool IsInvalidPathChar(Char c);

    /// <summary>
    /// Replaces the given text with other one in the files.
    /// </summary>
    /// <param name="folderPath">The relative or absolute path to the directory to search.</param>
    /// <param name="searchPattern">The search string to match against the names of files in <paramref name="folderPath" />. This parameter can contain a combination of valid literal path and wildcard (* and ?) characters (see Remarks), but doesn't support regular expressions.</param>
    /// <param name="searchOption">One of the enumeration values that specifies whether the search operation should include all subdirectories or only the current directory.</param>
    /// <param name="findWhat">The text to replace.</param>
    /// <param name="replaceWith">The value to replace to.</param>
    /// <returns>True if failed, otherwise false.</returns>
    static bool ReplaceInFiles(const String& folderPath, const Char* searchPattern, DirectorySearchOption searchOption, const String& findWhat, const String& replaceWith);

    /// <summary>
    /// Replaces the given text with other one in the file.
    /// </summary>
    /// <param name="file">The file to process.</param>
    /// <param name="findWhat">The text to replace.</param>
    /// <param name="replaceWith">The value to replace to.</param>
    /// <returns>True if failed, otherwise false.</returns>
    static bool ReplaceInFile(const StringView& file, const StringView& findWhat, const StringView& replaceWith);
};
