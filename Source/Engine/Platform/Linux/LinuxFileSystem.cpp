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

bool LinuxFileSystem::ShowOpenFileDialog(Window* parentWindow, const StringView& initialDirectory, const StringView& filter, bool multiSelect, const StringView& title, Array<String, HeapAllocation>& filenames)
{
    const StringAsANSI<> titleAnsi(*title, title.Length());
    char cmd[2048];
    // TODO: multiSelect support
    // TODO: filter support
    sprintf(cmd, "/usr/bin/zenity --modal --file-selection --title=\"%s\" ", titleAnsi.Get());
    FILE* f = popen(cmd, "r");
    char buf[2048];
    fgets(buf, ARRAY_COUNT(buf), f); 
    int result = pclose(f);
    if (result != 0)
    {
        return true;
    }
    const char* c = buf;
    while (*c)
    {
        const char* start = c;
        while (*c != '\n')
            c++;
        filenames.Add(String(start, (int32)(c - start)));
        c++;
    }
    return false;
}

bool LinuxFileSystem::ShowFileExplorer(const StringView& path)
{
    const StringAsANSI<> pathAnsi(*path, path.Length());
    char cmd[2048];
    sprintf(cmd, "nautilus %s &", pathAnsi.Get());
    system(cmd);
    return false;
}

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
        ASSERT(pathLength + strlen(entry->d_name) < ARRAY_COUNT(full_path));
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

bool LinuxFileSystem::getFilesFromDirectoryTop(Array<String>& results, const char* path, const char* searchPattern)
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
            if (searchPatternLength == 0 || StringUtils::Compare(searchPattern, "*") == 0)
            {
                // All files
            }
            else if (searchPattern[0] == '*' && searchPatternLength < fullPathLength && StringUtils::Compare(fullPath + fullPathLength - searchPatternLength + 1, searchPattern + 1, searchPatternLength - 1) == 0)
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
