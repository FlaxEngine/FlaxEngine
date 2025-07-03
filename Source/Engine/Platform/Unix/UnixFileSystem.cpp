// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_UNIX

#include "UnixFileSystem.h"
#include "Engine/Platform/File.h"
#include "Engine/Core/Types/TimeSpan.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Log.h"
#include "Engine/Utilities/StringConverter.h"
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <cerrno>
#include <dirent.h>

#if PLATFORM_MAC || PLATFORM_IOS
typedef StringAsANSI<> UnixString;
#else
typedef StringAsUTF8<> UnixString;
#endif

const DateTime UnixEpoch(1970, 1, 1);

bool UnixFileSystem::CreateDirectory(const StringView& path)
{
    const UnixString pathAnsi(*path, path.Length());

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

bool DeleteUnixPathTree(const char* path)
{
    size_t pathLength;
    DIR* dir;
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
    if ((dir = opendir(path)) == NULL)
    {
        // Cannot open directory
        return true;
    }

    // The length of the path
    pathLength = strlen(path);

    // Iteration through entries in the directory
    while ((entry = readdir(dir)) != NULL)
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

bool UnixFileSystem::DeleteDirectory(const String& path, bool deleteContents)
{
    const UnixString pathANSI(*path, path.Length());
    if (deleteContents)
    {
        return DeleteUnixPathTree(pathANSI.Get());
    }
    else
    {
        return rmdir(pathANSI.Get()) != 0;
    }
}

bool UnixFileSystem::DirectoryExists(const StringView& path)
{
    struct stat fileInfo;
    const UnixString pathANSI(*path, path.Length());
    if (stat(pathANSI.Get(), &fileInfo) != -1)
    {
        return S_ISDIR(fileInfo.st_mode);
    }
    return false;
}

bool UnixFileSystem::DirectoryGetFiles(Array<String>& results, const String& path, const Char* searchPattern, DirectorySearchOption option)
{
    const UnixString pathANSI(*path, path.Length());
    const UnixString searchPatternANSI(searchPattern);

    // Check if use only top directory
    if (option == DirectorySearchOption::TopDirectoryOnly)
        return getFilesFromDirectoryTop(results, pathANSI.Get(), searchPatternANSI.Get());
    return getFilesFromDirectoryAll(results, pathANSI.Get(), searchPatternANSI.Get());
}

bool UnixFileSystem::GetChildDirectories(Array<String>& results, const String& path)
{
    size_t pathLength;
    DIR* dir;
    struct stat statPath, statEntry;
    struct dirent* entry;
    const UnixString pathANSI(*path, path.Length());
    const char* pathStr = pathANSI.Get();

    // Stat for the path
    stat(pathStr, &statPath);

    // If path does not exists or is not dir - exit with status -1
    if (S_ISDIR(statPath.st_mode) == 0)
    {
        // Is not directory
        return true;
    }

    // If not possible to read the directory for this user
    if ((dir = opendir(pathStr)) == NULL)
    {
        // Cannot open directory
        return true;
    }

    // The length of the path
    pathLength = strlen(pathStr);

    // Iteration through entries in the directory
    while ((entry = readdir(dir)) != NULL)
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

bool UnixFileSystem::FileExists(const StringView& path)
{
    struct stat fileInfo;
    const UnixString pathANSI(*path, path.Length());
    if (stat(pathANSI.Get(), &fileInfo) != -1)
    {
        return S_ISREG(fileInfo.st_mode);
    }
    return false;
}

bool UnixFileSystem::DeleteFile(const StringView& path)
{
    const UnixString pathANSI(*path, path.Length());
    return unlink(pathANSI.Get()) != 0;
}

uint64 UnixFileSystem::GetFileSize(const StringView& path)
{
    struct stat fileInfo;
    fileInfo.st_size = 0;
    const UnixString pathANSI(*path, path.Length());
    if (stat(pathANSI.Get(), &fileInfo) != -1)
    {
        // Check for directories
        if (S_ISDIR(fileInfo.st_mode))
        {
            fileInfo.st_size = 0;
        }
    }
    return fileInfo.st_size;
}

bool UnixFileSystem::IsReadOnly(const StringView& path)
{
    const UnixString pathANSI(*path, path.Length());
    if (access(pathANSI.Get(), W_OK) == -1)
    {
        return errno == EACCES;
    }
    return false;
}

bool UnixFileSystem::SetReadOnly(const StringView& path, bool isReadOnly)
{
    const UnixString pathANSI(*path, path.Length());
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

bool UnixFileSystem::MoveFile(const StringView& dst, const StringView& src, bool overwrite)
{
    if (!overwrite && FileExists(dst))
    {
        // Already exists
        return true;
    }

    if (overwrite)
    {
        unlink(UnixString(*dst, dst.Length()).Get());
    }
    if (rename(UnixString(*src, src.Length()).Get(), UnixString(*dst, dst.Length()).Get()) != 0)
    {
        if (errno == EXDEV)
        {
            if (!CopyFile(dst, src))
            {
                unlink(UnixString(*src, src.Length()).Get());
                return false;
            }
        }
        return true;
    }
    return false;
}

bool UnixFileSystem::getFilesFromDirectoryTop(Array<String>& results, const char* path, const char* searchPattern)
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
    if (dir == NULL)
    {
        // Cannot open directory
        return true;
    }

    // The length of the path
    pathLength = strlen(path);

    // Iteration through entries in the directory
    while ((entry = readdir(dir)) != NULL)
    {
        // Skip entries "." and ".."
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;

        // Determinate a full path of an entry
        char fullPath[256];
        const int32 pathLength = strlen(entry->d_name);
        ASSERT(pathLength + strlen(entry->d_name) < ARRAY_COUNT(fullPath));
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
            if (searchPatternLength == 0 ||
                StringUtils::Compare(searchPattern, "*") == 0 ||
                StringUtils::Compare(searchPattern, "*.*") == 0)
            {
                // All files
            }
            else if (searchPattern[0] == '*' && searchPatternLength < fullPathLength && StringUtils::Compare(fullPath + fullPathLength - searchPatternLength + 1, searchPattern + 1, searchPatternLength - 1) == 0)
            {
                // Path ending
            }
            else if (searchPattern[0] == '*' && searchPatternLength > 2 && searchPattern[searchPatternLength-1] == '*')
            {
                // Contains pattern
                bool match = false;
                for (int32 i = 0; i < pathLength - searchPatternLength - 1; i++)
                {
                    int32 len = Math::Min(searchPatternLength - 2, pathLength - i);
                    if (StringUtils::Compare(&entry->d_name[i], &searchPattern[1], len) == 0)
                    {
                        match = true;
                        break;
                    }
                }
                if (!match)
                    continue;
            }
            else
            {
                // TODO: implement all cases in a generic way
                LOG(Warning, "DirectoryGetFiles: Wildcard filter is not implemented");
                continue;
            }

            // Add file
            results.Add(String(fullPath));
        }
    }

    closedir(dir);

    return false;
}

bool UnixFileSystem::getFilesFromDirectoryAll(Array<String>& results, const char* path, const char* searchPattern)
{
    // Find all files in this directory
    getFilesFromDirectoryTop(results, path, searchPattern);

    size_t pathLength;
    DIR* dir;
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
    if ((dir = opendir(path)) == NULL)
    {
        // Cannot open directory
        return true;
    }

    // The length of the path
    pathLength = strlen(path);

    // Iteration through entries in the directory
    while ((entry = readdir(dir)) != NULL)
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

DateTime UnixFileSystem::GetFileLastEditTime(const StringView& path)
{
    struct stat fileInfo;
    const UnixString pathANSI(*path, path.Length());
    if (stat(pathANSI.Get(), &fileInfo) == -1)
    {
        return DateTime::MinValue();
    }

    const TimeSpan timeSinceEpoch(0, 0, 0, fileInfo.st_mtime);
    return UnixEpoch + timeSinceEpoch;
}

#endif
