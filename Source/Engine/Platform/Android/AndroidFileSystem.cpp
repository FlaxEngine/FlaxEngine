// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_ANDROID

#include "AndroidFileSystem.h"
#include "Engine/Platform/File.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/StringView.h"
#include "Engine/Core/Types/TimeSpan.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Utilities/StringConverter.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include "Engine/Main/Android/android_native_app_glue.h"

namespace
{
    FORCE_INLINE AAssetManager* GetAssetManager()
    {
        return AndroidPlatform::GetApp()->activity->assetManager;
    }

    bool IsAsset(const StringView& path, const char* pathAnsi)
    {
        if (path.StartsWith(StringView(Globals::ProjectFolder), StringSearchCase::CaseSensitive))
        {
            AAsset* asset = AAssetManager_open(GetAssetManager(), pathAnsi + Globals::ProjectFolder.Length() + 1, AASSET_MODE_UNKNOWN);
            if (asset)
            {
                AAsset_close(asset);
                return true;
            }
        }
        return false;
    }

    bool DeleteUnixPathTree(const char* path)
    {
        size_t pathLength;
        struct stat statPath, statEntry;
        struct dirent* entry;

        // Stat for the path
        stat(path, &statPath);

        // If path does not exists or is not dir - exit with status -1
        if (S_ISDIR(statPath.st_mode) == 0)
        {
            // Is not directory
            return true;
        }

        // If not possible to read the directory for this user
        DIR* dir = dir = opendir(path);
        if (dir == nullptr)
        {
            // Cannot open directory
            return true;
        }

        // The length of the path
        pathLength = strlen(path);

        // Iteration through entries in the directory
        while ((entry = readdir(dir)) != nullptr)
        {
            // Skip entries "." and ".."
            if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
                continue;

            // Determinate a full path of an entry
            char full_path[256];
            ASSERT(pathLength + strlen(entry->d_name) < ARRAY_COUNT(full_path));
            strcpy(full_path, path);
            strcat(full_path, "/");
            strcat(full_path, entry->d_name);

            // Stat for the entry
            stat(full_path, &statEntry);

            // Recursively remove a nested directory
            if (S_ISDIR(statEntry.st_mode) != 0)
            {
                if (DeleteUnixPathTree(full_path))
                    return true;
                continue;
            }

            // Remove a file object
            if (unlink(full_path) != 0)
                return true;
        }

        // Remove the devastated directory and close the object of it
        if (rmdir(path) != 0)
            return true;

        closedir(dir);

        return false;
    }
}

bool AndroidFileSystem::CreateDirectory(const StringView& path)
{
    const StringAsANSI<> pathAnsi(*path, path.Length());

    // Skip if already exists
    struct stat fileInfo;
    if (stat(pathAnsi.Get(), &fileInfo) != -1 && S_ISDIR(fileInfo.st_mode))
    {
        return false;
    }

    // Recursively do it all again for the parent directory, if any
    const int32 slashIndex = path.FindLast('/');
    if (slashIndex > 1)
    {
        if (CreateDirectory(path.Substring(0, slashIndex)))
        {
            return true;
        }
    }

    // Create the last directory on the path (the recursive calls will have taken care of the parent directories by now)
    return mkdir(pathAnsi.Get(), 0755) != 0 && errno != EEXIST;
}

bool AndroidFileSystem::DeleteDirectory(const String& path, bool deleteContents)
{
    const StringAsANSI<> pathANSI(*path, path.Length());
    if (deleteContents)
    {
        return DeleteUnixPathTree(pathANSI.Get());
    }
    return rmdir(pathANSI.Get()) != 0;
}

bool AndroidFileSystem::DirectoryExists(const StringView& path)
{
    struct stat fileInfo;
    const StringAsANSI<> pathANSI(*path, path.Length());
    if (stat(pathANSI.Get(), &fileInfo) != -1)
    {
        return S_ISDIR(fileInfo.st_mode);
    }
    return false;
}

bool AndroidFileSystem::DirectoryGetFiles(Array<String>& results, const String& path, const Char* searchPattern, DirectorySearchOption option)
{
    const StringAsANSI<> pathANSI(*path, path.Length());
    const StringAsANSI<> searchPatternANSI(searchPattern);

    // Check if use only top directory
    if (option == DirectorySearchOption::TopDirectoryOnly)
        return getFilesFromDirectoryTop(results, pathANSI.Get(), searchPatternANSI.Get());
    return getFilesFromDirectoryAll(results, pathANSI.Get(), searchPatternANSI.Get());
}

bool AndroidFileSystem::GetChildDirectories(Array<String>& results, const String& path)
{
    size_t pathLength;
    struct stat statPath, statEntry;
    struct dirent* entry;
    const StringAsANSI<> pathANSI(*path, path.Length());
    const char* pathStr = pathANSI.Get();

    // Stat for the path
    stat(pathStr, &statPath);

    // If path does not exist or is not dir - exit with status -1
    if (S_ISDIR(statPath.st_mode) == 0)
    {
        // Is not directory
        return true;
    }

    // If not possible to read the directory for this user
    DIR* dir = opendir(pathStr);
    if (dir == nullptr)
    {
        // Cannot open directory
        return true;
    }

    // The length of the path
    pathLength = strlen(pathStr);

    // Iteration through entries in the directory
    while ((entry = readdir(dir)) != nullptr)
    {
        // Skip entries "." and ".."
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;

        // Determinate a full path of an entry
        char fullPath[256];
        ASSERT(pathLength + strlen(entry->d_name) < ARRAY_COUNT(fullPath));
        strcpy(fullPath, pathStr);
        strcat(fullPath, "/");
        strcat(fullPath, entry->d_name);

        // Stat for the entry
        stat(fullPath, &statEntry);

        // Check for directory
        if (S_ISDIR(statEntry.st_mode) != 0)
        {
            // Add directory
            results.Add(String(fullPath));
        }
    }

    closedir(dir);

    return false;
}

bool AndroidFileSystem::FileExists(const StringView& path)
{
    struct stat fileInfo;
    const StringAsANSI<> pathANSI(*path, path.Length());
    if (stat(pathANSI.Get(), &fileInfo) != -1)
    {
        return S_ISREG(fileInfo.st_mode);
    }
    if (IsAsset(path, pathANSI.Get()))
    {
        return true;
    }
    return false;
}

bool AndroidFileSystem::DeleteFile(const StringView& path)
{
    const StringAsANSI<> pathANSI(*path, path.Length());
    return unlink(pathANSI.Get()) != 0;
}

uint64 AndroidFileSystem::GetFileSize(const StringView& path)
{
    struct stat fileInfo;
    fileInfo.st_size = 0;
    const StringAsANSI<> pathANSI(*path, path.Length());
    if (stat(pathANSI.Get(), &fileInfo) != -1)
    {
        if (S_ISDIR(fileInfo.st_mode))
        {
            fileInfo.st_size = 0;
        }
    }
    else
    {
        AAsset* asset = AAssetManager_open(GetAssetManager(), pathANSI.Get(), AASSET_MODE_UNKNOWN);
        if (asset)
        {
            fileInfo.st_size = AAsset_getLength64(asset);
            AAsset_close(asset);
        }
    }
    return fileInfo.st_size;
}

bool AndroidFileSystem::IsReadOnly(const StringView& path)
{
    const StringAsANSI<> pathANSI(*path, path.Length());
    if (access(pathANSI.Get(), W_OK) == -1)
    {
        return errno == EACCES;
    }
    if (IsAsset(path, pathANSI.Get()))
    {
        return true;
    }
    return false;
}

bool AndroidFileSystem::SetReadOnly(const StringView& path, bool isReadOnly)
{
    const StringAsANSI<> pathANSI(*path, path.Length());
    struct stat fileInfo;
    if (stat(pathANSI.Get(), &fileInfo) != -1)
    {
        if (isReadOnly)
        {
            fileInfo.st_mode &= ~S_IWUSR;
        }
        else
        {
            fileInfo.st_mode |= S_IWUSR;
        }
        return chmod(pathANSI.Get(), fileInfo.st_mode) == 0;
    }
    return false;
}

bool AndroidFileSystem::MoveFile(const StringView& dst, const StringView& src, bool overwrite)
{
    if (!overwrite && FileExists(dst))
    {
        // Already exists
        return true;
    }

    return rename(StringAsANSI<>(*src, src.Length()).Get(), StringAsANSI<>(*dst, dst.Length()).Get()) != 0;
}

bool AndroidFileSystem::CopyFile(const StringView& dst, const StringView& src)
{
    const StringAsANSI<> srcANSI(*src, src.Length());
    const StringAsANSI<> dstANSI(*dst, dst.Length());

    int srcFile, dstFile;
    char buffer[4096];
    ssize_t readSize;
    int cachedError;

    srcFile = open(srcANSI.Get(), O_RDONLY);
    if (srcFile < 0)
        return true;
    dstFile = open(dstANSI.Get(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if (dstFile < 0)
        goto out_error;

    while (readSize = read(srcFile, buffer, sizeof(buffer)), readSize > 0)
    {
        char* ptr = buffer;
        ssize_t writeSize;

        do
        {
            writeSize = write(dstFile, ptr, readSize);
            if (writeSize >= 0)
            {
                readSize -= writeSize;
                ptr += writeSize;
            }
            else if (errno != EINTR)
            {
                goto out_error;
            }
        } while (readSize > 0);
    }

    if (readSize == 0)
    {
        if (close(dstFile) < 0)
        {
            dstFile = -1;
            goto out_error;
        }
        close(srcFile);

        // Success
        return false;
    }

out_error:
    cachedError = errno;
    close(srcFile);
    if (dstFile >= 0)
        close(dstFile);
    errno = cachedError;
    return true;
}

bool AndroidFileSystem::getFilesFromDirectoryTop(Array<String>& results, const char* path, const char* searchPattern)
{
    size_t pathLength;
    struct stat statPath, statEntry;
    struct dirent* entry;

    // Stat for the path
    stat(path, &statPath);

    // If path does not exists or is not dir - exit with status -1
    if (S_ISDIR(statPath.st_mode) == 0)
    {
        // Is not directory
        return true;
    }

    // If not possible to read the directory for this user
    DIR* dir = opendir(path);
    if (dir == nullptr)
    {
        // Cannot open directory
        return true;
    }

    // The length of the path
    pathLength = strlen(path);

    // Iteration through entries in the directory
    while ((entry = readdir(dir)) != nullptr)
    {
        // Skip entries "." and ".."
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;

        // Determinate a full path of an entry
        char fullPath[256];
        ASSERT(pathLength + strlen(entry->d_name) <= 255);
        strcpy(fullPath, path);
        strcat(fullPath, "/");
        strcat(fullPath, entry->d_name);

        // Stat for the entry
        stat(fullPath, &statEntry);

        // Check for file
        if (S_ISREG(statEntry.st_mode) != 0)
        {
            // Validate with filter
            const int32 fullPathLength = StringUtils::Length(fullPath);
            const int32 searchPatternLength = StringUtils::Length(searchPattern);
            if (searchPatternLength == 0 || StringUtils::Compare(searchPattern, "*") == 0)
            {
                // All files
            }
            else if (searchPattern[0] == '*' && searchPatternLength < fullPathLength && StringUtils::Compare(fullPath + fullPathLength - searchPatternLength + 1, searchPattern + 1) == 0)
            {
                // Path ending
            }
            else
            {
                // TODO: implement all cases in a generic way
                continue;
            }

            // Add file
            results.Add(String(fullPath));
        }
    }

    closedir(dir);

    return false;
}

bool AndroidFileSystem::getFilesFromDirectoryAll(Array<String>& results, const char* path, const char* searchPattern)
{
    // Find all files in this directory
    getFilesFromDirectoryTop(results, path, searchPattern);

    size_t pathLength;
    struct stat statPath, statEntry;
    struct dirent* entry;

    // Stat for the path
    stat(path, &statPath);

    // If path does not exists or is not dir - exit with status -1
    if (S_ISDIR(statPath.st_mode) == 0)
    {
        // Is not directory
        return true;
    }

    // If not possible to read the directory for this user
    DIR* dir = opendir(path);
    if (dir == nullptr)
    {
        // Cannot open directory
        return true;
    }

    // The length of the path
    pathLength = strlen(path);

    // Iteration through entries in the directory
    while ((entry = readdir(dir)) != nullptr)
    {
        // Skip entries "." and ".."
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;

        // Determinate a full path of an entry
        char full_path[256];
        ASSERT(pathLength + strlen(entry->d_name) < ARRAY_COUNT(full_path));
        strcpy(full_path, path);
        strcat(full_path, "/");
        strcat(full_path, entry->d_name);

        // Stat for the entry
        stat(full_path, &statEntry);

        // Check for directory
        if (S_ISDIR(statEntry.st_mode) != 0)
        {
            if (getFilesFromDirectoryAll(results, full_path, searchPattern))
            {
                closedir(dir);
                return true;
            }
        }
    }

    closedir(dir);

    return false;
}

DateTime AndroidFileSystem::GetFileLastEditTime(const StringView& path)
{
    struct stat fileInfo;
    const StringAsANSI<> pathANSI(*path, path.Length());
    if (stat(pathANSI.Get(), &fileInfo) == -1)
        return DateTime::MinValue();
    const TimeSpan timeSinceEpoch(0, 0, 0, fileInfo.st_mtime);
    const DateTime UnixEpoch(1970, 1, 1);
    return UnixEpoch + timeSinceEpoch;
}

AndroidFile::AndroidFile(int32 handle)
    : UnixFile(handle)
{
}

AndroidFile::~AndroidFile()
{
}

AndroidFile* AndroidFile::Open(const StringView& path, FileMode mode, FileAccess access, FileShare share)
{
    // Check if use AAssetManager API from NDK to access assets bounded in APK
    const StringAsANSI<> pathANSI(*path, path.Length());
    if (mode == FileMode::OpenExisting && path.StartsWith(StringView(Globals::ProjectFolder), StringSearchCase::CaseSensitive))
    {
        AAsset* asset = AAssetManager_open(GetAssetManager(), pathANSI.Get() + Globals::ProjectFolder.Length() + 1, AASSET_MODE_RANDOM);
        if (asset)
        {
            return New<AndroidAssetFile>(asset);
        }
    }

    // Fallback to default Unix file access
    return (AndroidFile*)UnixFile::Open(path, mode, access, share);
}

AndroidAssetFile::AndroidAssetFile(AAsset* asset)
    : AndroidFile(-1)
    , _asset(asset)
{
}

AndroidAssetFile::~AndroidAssetFile()
{
    Close();
}

bool AndroidAssetFile::Read(void* buffer, uint32 bytesToRead, uint32* bytesRead)
{
    const ssize_t tmp = AAsset_read(_asset, buffer, bytesToRead);
    if (tmp != -1)
    {
        if (bytesRead)
            *bytesRead = tmp;
        return false;
    }
    if (bytesRead)
        *bytesRead = 0;
    return true;
}

bool AndroidAssetFile::Write(const void* buffer, uint32 bytesToWrite, uint32* bytesWritten)
{
    return true;
}

void AndroidAssetFile::Close()
{
    if (_asset)
    {
        AAsset_close(_asset);
        _asset = nullptr;
    }
}

uint32 AndroidAssetFile::GetSize() const
{
    return AAsset_getLength(_asset);
}

DateTime AndroidAssetFile::GetLastWriteTime() const
{
    return DateTime::MinValue();
}

uint32 AndroidAssetFile::GetPosition() const
{
    return AAsset_seek(_asset, 0, SEEK_CUR);
}

void AndroidAssetFile::SetPosition(uint32 seek)
{
    AAsset_seek(_asset, (off_t)seek, SEEK_SET);
}

bool AndroidAssetFile::IsOpened() const
{
    return _asset != nullptr;
}

#endif
