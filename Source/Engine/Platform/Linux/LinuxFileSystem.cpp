// Copyright (c) 2012-2019 Wojciech Figat. All rights reserved.

#if PLATFORM_LINUX

#include "LinuxFileSystem.h"
#include "Engine/Platform/File.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/StringView.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Utilities/StringConverter.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <cerrno>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

const DateTime UnixEpoch(1970, 1, 1);

bool LinuxFileSystem::CreateDirectory(const StringView& path)
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
        ASSERT(pathLength + strlen(entry->d_name) <= 255);
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

bool LinuxFileSystem::DeleteDirectory(const String& path, bool deleteContents)
{
    const StringAsANSI<> pathANSI(*path, path.Length());
    if (deleteContents)
    {
        return DeleteUnixPathTree(pathANSI.Get());
    }
    else
    {
        return rmdir(pathANSI.Get()) != 0;
    }
}

bool LinuxFileSystem::DirectoryExists(const StringView& path)
{
    struct stat fileInfo;
    const StringAsANSI<> pathANSI(*path, path.Length());
    if (stat(pathANSI.Get(), &fileInfo) != -1)
    {
        return S_ISDIR(fileInfo.st_mode);
    }
    return false;
}

bool LinuxFileSystem::DirectoryGetFiles(Array<String>& results, const String& path, const Char* searchPattern, DirectorySearchOption option)
{
    const StringAsANSI<> pathANSI(*path, path.Length());
    const StringAsANSI<> searchPatternANSI(searchPattern);

    // Check if use only top directory
    if (option == DirectorySearchOption::TopDirectoryOnly)
        return getFilesFromDirectoryTop(results, pathANSI.Get(), searchPatternANSI.Get());
    return getFilesFromDirectoryAll(results, pathANSI.Get(), searchPatternANSI.Get());
}

bool LinuxFileSystem::GetChildDirectories(Array<String>& results, const String& directory)
{
    size_t pathLength;
    DIR* dir;
    struct stat statPath, statEntry;
    struct dirent* entry;
    const StringAsANSI<> pathANSI(*directory, directory.Length());
    const char* path = pathANSI.Get();

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
        ASSERT(pathLength + strlen(entry->d_name) <= 255);
        strcpy(full_path, path);
        strcat(full_path, "/");
        strcat(full_path, entry->d_name);

        // Stat for the entry
        stat(full_path, &statEntry);

        // Check for directory
        if (S_ISDIR(statEntry.st_mode) != 0)
        {
            // Add directory
            results.Add(String(full_path));
        }
    }

    closedir(dir);

    return false;
}

bool LinuxFileSystem::FileExists(const StringView& path)
{
    struct stat fileInfo;
    const StringAsANSI<> pathANSI(*path, path.Length());
    if (stat(pathANSI.Get(), &fileInfo) != -1)
    {
        return S_ISREG(fileInfo.st_mode);
    }
    return false;
}

bool LinuxFileSystem::DeleteFile(const StringView& path)
{
    const StringAsANSI<> pathANSI(*path, path.Length());
    return unlink(pathANSI.Get()) == 0;
}

uint64 LinuxFileSystem::GetFileSize(const StringView& path)
{
    struct stat fileInfo;
    fileInfo.st_size = -1;
    const StringAsANSI<> pathANSI(*path, path.Length());
    if (stat(pathANSI.Get(), &fileInfo) != -1)
    {
        // Check for directories
        if (S_ISDIR(fileInfo.st_mode))
        {
            fileInfo.st_size = -1;
        }
    }
    return fileInfo.st_size;
}

bool LinuxFileSystem::IsReadOnly(const StringView& path)
{
    const StringAsANSI<> pathANSI(*path, path.Length());
    if (access(pathANSI.Get(), W_OK) == -1)
    {
        return errno == EACCES;
    }
    return false;
}

bool LinuxFileSystem::SetReadOnly(const StringView& path, bool isReadOnly)
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

bool LinuxFileSystem::MoveFile(const StringView& dst, const StringView& src, bool overwrite)
{
    if (!overwrite && FileExists(dst))
    {
        // Already exists
        return true;
    }

    return rename(StringAsANSI<>(*src, src.Length()).Get(), StringAsANSI<>(*dst, dst.Length()).Get()) != 0;
}

bool LinuxFileSystem::CopyFile(const StringView& dst, const StringView& src)
{
    const StringAsANSI<> srcANSI(*src, src.Length());
    const StringAsANSI<> dstANSI(*dst, dst.Length());
    const char* from = srcANSI.Get();
    const char* to = dstANSI.Get();

    int fd_to, fd_from;
    char buf[4096];
    ssize_t nread;
    int saved_errno;

    fd_from = open(from, O_RDONLY);
    if (fd_from < 0)
        return true;

    fd_to = open(to, O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (fd_to < 0)
        goto out_error;

    while (nread = read(fd_from, buf, sizeof buf), nread > 0)
    {
        char* out_ptr = buf;
        ssize_t nwritten;

        do
        {
            nwritten = write(fd_to, out_ptr, nread);

            if (nwritten >= 0)
            {
                nread -= nwritten;
                out_ptr += nwritten;
            }
            else if (errno != EINTR)
            {
                goto out_error;
            }
        } while (nread > 0);
    }

    if (nread == 0)
    {
        if (close(fd_to) < 0)
        {
            fd_to = -1;
            goto out_error;
        }
        close(fd_from);

        // Success
        return false;
    }

out_error:
    saved_errno = errno;

    close(fd_from);
    if (fd_to >= 0)
        close(fd_to);

    errno = saved_errno;
    return true;
}

bool LinuxFileSystem::getFilesFromDirectoryTop(Array<String>& results, const char* path, const char* searchPattern)
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
            else if (searchPattern[0] == '*' && searchPatternLength < fullPathLength && StringUtils::Compare(fullPath + fullPathLength - searchPatternLength, searchPattern + 1) == 0)
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

bool LinuxFileSystem::getFilesFromDirectoryAll(Array<String>& results, const char* path, const char* searchPattern)
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
        ASSERT(pathLength + strlen(entry->d_name) <= 255);
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

DateTime LinuxFileSystem::GetFileLastEditTime(const StringView& path)
{
    struct stat fileInfo;
    const StringAsANSI<> pathANSI(*path, path.Length());
    if (stat(pathANSI.Get(), &fileInfo) == -1)
    {
        return DateTime::MinValue();
    }

    const TimeSpan timeSinceEpoch(0, 0, fileInfo.st_mtime);
    return UnixEpoch + timeSinceEpoch;
}

void LinuxFileSystem::GetSpecialFolderPath(const SpecialFolder type, String& result)
{
    const String& home = LinuxPlatform::GetHomeDirectory();
    switch (type)
    {
    case SpecialFolder::Desktop:
        result = home / TEXT("Desktop");
        break;
    case SpecialFolder::Documents:
        result = String::Empty;
        break;
    case SpecialFolder::Pictures:
        result = home / TEXT("Pictures");
        break;
    case SpecialFolder::AppData:
        result = TEXT("/usr/share");
        break;
    case SpecialFolder::LocalAppData:
        result = home;
        break;
    case SpecialFolder::ProgramData:
        result = String::Empty;
        break;
    case SpecialFolder::Temporary:
        result = TEXT("/tmp");
        break;
    default:
        CRASH;
        break;
    }
}

#endif
