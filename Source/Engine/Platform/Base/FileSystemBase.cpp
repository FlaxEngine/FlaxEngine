// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/File.h"
#include "Engine/Core/Types/Guid.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/StringView.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Engine/Globals.h"

bool FileSystemBase::ShowOpenFileDialog(Window* parentWindow, const StringView& initialDirectory, const StringView& filter, bool multiSelect, const StringView& title, Array<String, HeapAllocation>& filenames)
{
    // No supported
    return true;
}

bool FileSystemBase::ShowSaveFileDialog(Window* parentWindow, const StringView& initialDirectory, const StringView& filter, bool multiSelect, const StringView& title, Array<String, HeapAllocation>& filenames)
{
    // No supported
    return true;
}

bool FileSystemBase::ShowBrowseFolderDialog(Window* parentWindow, const StringView& initialDirectory, const StringView& title, StringView& path)
{
    // No supported
    return true;
}

bool FileSystemBase::ShowFileExplorer(const StringView& path)
{
    // No supported
    return true;
}

void FileSystemBase::SaveBitmapToFile(byte* data, uint32 width, uint32 height, uint32 bitsPerPixel, const uint32 padding, const String& path)
{
    // Try to open file
    auto file = File::Open(path, FileMode::CreateAlways, FileAccess::Write, FileShare::None);
    if (file == nullptr)
        return;

    const unsigned long headers_size = 54;
    const unsigned long pixel_data_size = height * ((width * bitsPerPixel / 8) + padding);
    const unsigned int filesize = headers_size + pixel_data_size;
    uint8 bmpfileheader[14] = { 'B', 'M', 0, 0, 0, 0, 0, 0, 0, 0, 54, 0, 0, 0 };
    uint8 bmpinfoheader[40] = { 40, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 24, 0 };
    uint8 bmppad[3] = { 0, 0, 0 };

    // Init the file header
    bmpfileheader[2] = (uint8)(filesize);
    bmpfileheader[3] = (uint8)(filesize >> 8);
    bmpfileheader[4] = (uint8)(filesize >> 16);
    bmpfileheader[5] = (uint8)(filesize >> 24);

    // Init the bitmap info header
    bmpinfoheader[4] = (uint8)(width);
    bmpinfoheader[5] = (uint8)(width >> 8);
    bmpinfoheader[6] = (uint8)(width >> 16);
    bmpinfoheader[7] = (uint8)(width >> 24);
    bmpinfoheader[8] = (uint8)(height);
    bmpinfoheader[9] = (uint8)(height >> 8);
    bmpinfoheader[10] = (uint8)(height >> 16);
    bmpinfoheader[11] = (uint8)(height >> 24);

    // Write the file header
    file->Write(bmpfileheader, sizeof(bmpfileheader));

    // Write the bitmap info header  
    file->Write(bmpinfoheader, sizeof(bmpinfoheader));

    // Write the RGB Data
    file->Write(data, pixel_data_size);

    // Close the file
    Delete(file);
}

bool FileSystemBase::AreFilePathsEqual(const StringView& path1, const StringView& path2)
{
    if (path1.Compare(path2, StringSearchCase::CaseSensitive) == 0)
        return true;

    // Normalize file paths
    String filename1(path1);
    String filename2(path2);
    NormalizePath(filename1);
    NormalizePath(filename2);

    return filename1.Compare(filename2, StringSearchCase::IgnoreCase) == 0;
}

void FileSystemBase::NormalizePath(String& path)
{
    path.Replace('\\', '/');
    if (path.Length() > 2 && StringUtils::IsAlpha(path[0]) && path[1] == ':')
    {
        path[2] = '\\';
    }
}

bool FileSystemBase::IsRelative(const StringView& path)
{
    const bool isRooted =
            (path.Length() >= 2 && StringUtils::IsAlpha(path[0]) && path[1] == ':') ||
            path.StartsWith(StringView(TEXT("\\\\"), 2), StringSearchCase::CaseSensitive) ||
            path.StartsWith('/') ||
            path.StartsWith('\\') ||
            path.StartsWith('/');
    return !isRooted;
}

String FileSystemBase::GetExtension(const StringView& path)
{
    Char chr;
    int32 length = path.Length();
    int32 num = length;
    do
    {
        num--;
        if (num < 0)
        {
            break;
        }
        chr = path[num];
        if (chr != '.')
        {
            continue;
        }
        if (num == length - 1)
        {
            return String::Empty;
        }
        num++;
        return path.Substring(num, length - num);
    } while (chr != TEXT('\\') && chr != TEXT('/') && chr != TEXT(':'));

    return String::Empty;
}

void FileSystemBase::GetTempFilePath(String& tmpPath)
{
    tmpPath = Globals::TemporaryFolder / Guid::New().ToString(Guid::FormatType::N);
}

static void SplitPath(const String& path, Array<String>& splitPath)
{
    int32 start = 0;
    int32 separatorPos;

    do
    {
        separatorPos = path.FindFirstOf(TEXT("\\/"), start);
        if (separatorPos == -1)
            splitPath.Add(path.Substring(start));
        else
            splitPath.Add(path.Substring(start, separatorPos - start));
        start = separatorPos + 1;
    } while (separatorPos != -1);
}

String FileSystemBase::ConvertAbsolutePathToRelative(const String& basePath, const String& path)
{
    Array<String> toDirs;
    Array<String> fromDirs;

    SplitPath(path, toDirs);
    SplitPath(basePath, fromDirs);

    String output;

    Array<String>::Iterator toIt = toDirs.Begin(), fromIt = fromDirs.Begin();
    const Array<String>::Iterator toEnd = toDirs.End(), fromEnd = fromDirs.End();

    while (toIt != toEnd && fromIt != fromEnd && *toIt == *fromIt)
    {
        ++toIt;
        ++fromIt;
    }

    while (fromIt != fromEnd)
    {
        output += TEXT("../");
        ++fromIt;
    }

    while (toIt != toEnd)
    {
        output += *toIt;
        ++toIt;

        if (toIt != toEnd)
            output += '/';
    }

    return output;
}

bool FileSystemBase::CopyFile(const String& dst, const String& src)
{
    // Open and create files
    const auto srcFile = File::Open(src, FileMode::OpenExisting, FileAccess::Read);
    if (!srcFile)
    {
        return true;
    }
    const auto dstFile = File::Open(dst, FileMode::CreateAlways, FileAccess::Write);
    if (!dstFile)
    {
        Delete(srcFile);
        return true;
    }

    // Skip for empty file
    uint32 size = srcFile->GetSize();
    if (size == 0)
    {
        Delete(srcFile);
        Delete(dstFile);
        return false;
    }

    // Copy data
    const uint32 bufferSize = Math::Min<uint32>(1024 * 1024, size);
    byte* buffer = (byte*)Allocator::Allocate(bufferSize);
    while (size)
    {
        const uint32 readSize = Math::Min<uint32>(bufferSize, size);
        srcFile->Read(buffer, readSize);
        dstFile->Write(buffer, readSize);
        size -= readSize;
    }
    Allocator::Free(buffer);

    // Cleanup
    Delete(srcFile);
    Delete(dstFile);

    return false;
}

bool FileSystemBase::CopyDirectory(const String& dst, const String& src, bool withSubDirectories)
{
    // Check if source exists
    if (!FileSystem::DirectoryExists(*src))
        return false;

    // Copy
    return FileSystemBase::DirectoryCopyHelper(dst, src, withSubDirectories);
}

String FileSystemBase::ConvertRelativePathToAbsolute(const String& path)
{
    return ConvertRelativePathToAbsolute(Globals::StartupFolder, path);
}

String FileSystemBase::ConvertRelativePathToAbsolute(const String& basePath, const String& path)
{
    String fullyPathed;
    if (IsRelative(path))
    {
        fullyPathed = basePath;
    }

    fullyPathed /= path;

    NormalizePath(fullyPathed);
    return fullyPathed;
}

String FileSystemBase::ConvertAbsolutePathToRelative(const String& path)
{
    return ConvertAbsolutePathToRelative(Globals::StartupFolder, path);
}

bool FileSystemBase::DirectoryCopyHelper(const String& dst, const String& src, bool withSubDirectories)
{
    // Create dst directory
    if (!FileSystem::DirectoryExists(dst))
    {
        if (FileSystem::CreateDirectory(dst))
            return true;
    }

    // Copy all files
    Array<String> cache(32);
    if (FileSystem::DirectoryGetFiles(cache, *src, TEXT("*"), DirectorySearchOption::TopDirectoryOnly))
        return true;
    for (int32 i = 0; i < cache.Count(); i++)
    {
        String dstFile = dst / StringUtils::GetFileName(cache[i]);
        if (FileSystem::CopyFile(*dstFile, *cache[i]))
            return true;
    }

    // Copy all subdirectories (if need to)
    if (withSubDirectories)
    {
        cache.Clear();
        if (FileSystem::GetChildDirectories(cache, src))
            return true;
        for (int32 i = 0; i < cache.Count(); i++)
        {
            String dstDir = dst / StringUtils::GetFileName(cache[i]);
            if (DirectoryCopyHelper(dstDir, cache[i], true))
                return true;
        }
    }

    return false;
}
