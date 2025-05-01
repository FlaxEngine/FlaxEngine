// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_WIN32

#include "Win32FileSystem.h"
#include "Engine/Core/Types/DateTime.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/StringView.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Utilities/StringConverter.h"
#include "IncludeWindowsHeaders.h"

const DateTime WindowsEpoch(1970, 1, 1);

#define WIN32_INIT_BUFFER(path, buffer) StringAsTerminated<> buffer(path.Get(), path.Length())

bool Win32FileSystem::CreateDirectory(const StringView& path)
{
    WIN32_INIT_BUFFER(path, buffer);

    // If the specified directory name doesn't exist, do our thing
    const DWORD fileAttributes = GetFileAttributesW(buffer);
    if (fileAttributes == INVALID_FILE_ATTRIBUTES)
    {
        const auto error = GetLastError();
        if (error == ERROR_ACCESS_DENIED)
            return false;

        // Recursively do it all again for the parent directory, if any
        const int32 slashIndex = path.FindLast('/');
        if (slashIndex != INVALID_INDEX)
        {
            if (CreateDirectory(path.Substring(0, slashIndex)))
            {
                return true;
            }
        }

        // Create the last directory on the path (the recursive calls will have taken care of the parent directories by now)
        const BOOL result = ::CreateDirectoryW(buffer, nullptr);
        if (result == FALSE)
        {
            return true;
        }
    }
    else
    {
        // Special case if directory name already exists as a file or directory
        if (!((fileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 || (fileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0))
        {
            return true;
        }
    }

    return false;
}

bool Win32FileSystem::DeleteDirectory(const String& path, bool deleteContents)
{
    // Check if delete contents
    if (deleteContents)
    {
        // Create search pattern
        String pattern = path / '*';

        WIN32_FIND_DATA info;
        HANDLE hp;
        hp = FindFirstFileW(*pattern, &info);
        if (INVALID_HANDLE_VALUE == hp)
        {
            // Check if no files at all
            return GetLastError() != ERROR_FILE_NOT_FOUND;
        }

        do
        {
            // Check if it isn't a special case
            if (StringUtils::Compare(info.cFileName, TEXT(".")) == 0 || StringUtils::Compare(info.cFileName, TEXT("..")) == 0)
                continue;

            // Check if its a directory of a file
            String tmpPath = path / info.cFileName;
            if (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                // Delete sub directory recursively
                if (DeleteDirectory(*tmpPath, true))
                {
                    FindClose(hp);
                    return true;
                }
            }
            else
            {
                if (!DeleteFileW(*tmpPath))
                {
                    FindClose(hp);
                    return true;
                }
            }
        } while (FindNextFileW(hp, &info) != 0);
        FindClose(hp);

        if (GetLastError() != ERROR_NO_MORE_FILES)
        {
            return true;
        }
    }

    // Remove directory
    RemoveDirectoryW(*path);

    // Check if still exists
    const DWORD result = GetFileAttributesW(*path);
    return result != 0xFFFFFFFF && result & FILE_ATTRIBUTE_DIRECTORY;
}

bool Win32FileSystem::DirectoryExists(const StringView& path)
{
    WIN32_INIT_BUFFER(path, buffer);
    const DWORD result = GetFileAttributesW(buffer);
    return result != 0xFFFFFFFF && result & FILE_ATTRIBUTE_DIRECTORY;
}

bool Win32FileSystem::DirectoryGetFiles(Array<String>& results, const String& path, const Char* searchPattern, DirectorySearchOption option)
{
    if (option == DirectorySearchOption::TopDirectoryOnly)
        return getFilesFromDirectoryTop(results, path, searchPattern);
    return getFilesFromDirectoryAll(results, path, searchPattern);
}

bool Win32FileSystem::GetChildDirectories(Array<String>& results, const String& path)
{
    // Try to find first file
    WIN32_FIND_DATA info;
    String pattern = path / TEXT('*');
    const HANDLE handle = FindFirstFileW(*pattern, &info);
    if (INVALID_HANDLE_VALUE == handle)
    {
        // Check if no files at all
        return GetLastError() != ERROR_FILE_NOT_FOUND;
    }

    do
    {
        // Check if it isn't a special case
        if (StringUtils::Compare(info.cFileName, TEXT(".")) == 0 || StringUtils::Compare(info.cFileName, TEXT("..")) == 0)
            continue;

        // Check if its a directory of a file
        if (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            // Add directory
            results.Add(path / info.cFileName);
        }
    } while (FindNextFileW(handle, &info) != 0);
    FindClose(handle);

    return GetLastError() != ERROR_NO_MORE_FILES;
}

bool Win32FileSystem::FileExists(const StringView& path)
{
    WIN32_INIT_BUFFER(path, buffer);
    const DWORD result = GetFileAttributesW(buffer);
    return result != 0xFFFFFFFF && !(result & FILE_ATTRIBUTE_DIRECTORY);
}

bool Win32FileSystem::DeleteFile(const StringView& path)
{
    return DeleteFileW(*path) == 0;
}

uint64 Win32FileSystem::GetFileSize(const StringView& path)
{
    WIN32_INIT_BUFFER(path, buffer);
    WIN32_FILE_ATTRIBUTE_DATA info;
    if (!!GetFileAttributesExW(buffer, GetFileExInfoStandard, &info))
    {
        if ((info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
        {
            LARGE_INTEGER li;
            li.HighPart = info.nFileSizeHigh;
            li.LowPart = info.nFileSizeLow;
            return li.QuadPart;
        }
    }
    return 0;
}

bool Win32FileSystem::IsReadOnly(const StringView& path)
{
    WIN32_INIT_BUFFER(path, buffer);
    const DWORD result = GetFileAttributesW(buffer);
    return result != 0xFFFFFFFF ? !!(result & FILE_ATTRIBUTE_READONLY) : false;
}

bool Win32FileSystem::SetReadOnly(const StringView& path, bool isReadOnly)
{
    WIN32_INIT_BUFFER(path, buffer);
    return SetFileAttributesW(buffer, isReadOnly ? FILE_ATTRIBUTE_READONLY : FILE_ATTRIBUTE_NORMAL) == 0;
}

bool Win32FileSystem::MoveFile(const StringView& dst, const StringView& src, bool overwrite)
{
    const DWORD flags = MOVEFILE_COPY_ALLOWED | (overwrite ? MOVEFILE_REPLACE_EXISTING : 0);
    WIN32_INIT_BUFFER(dst, bufferDst);
    WIN32_INIT_BUFFER(src, bufferSrc);

    // If paths are almost the same but some characters have different case we need to use a proxy file
    if (dst.Length() == src.Length() && StringUtils::CompareIgnoreCase(*dst, *src) == 0)
    {
        String tmp;
        GetTempFilePath(tmp);
        return MoveFileExW(bufferSrc, *tmp, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING) == 0 || MoveFileExW(*tmp, bufferDst, flags) == 0;
    }

    return MoveFileExW(bufferSrc, bufferDst, flags) == 0;
}

bool Win32FileSystem::CopyFile(const StringView& dst, const StringView& src)
{
    WIN32_INIT_BUFFER(dst, bufferDst);
    WIN32_INIT_BUFFER(src, bufferSrc);
#if PLATFORM_UWP
	const bool overwrite = true;
	COPYFILE2_EXTENDED_PARAMETERS param = { 0 };
	param.dwSize = sizeof(COPYFILE2_EXTENDED_PARAMETERS);
	param.dwCopyFlags = (!overwrite) ? COPY_FILE_FAIL_IF_EXISTS : 0;
	return FAILED(CopyFile2(bufferSrc, bufferDst, &param));
#else
    return CopyFileW(bufferSrc, bufferDst, FALSE) == 0;
#endif
}

void Win32FileSystem::ConvertLineEndingsToDos(const StringView& text, Array<Char>& output)
{
    // Prepare output (add some space for \r characters, just guess ~1%)
    output.Clear();
    output.EnsureCapacity(Math::CeilToInt((float)text.Length() * 1.01f));

    // Perform conversion
    auto readPtr = text.Get();
    for (int32 i = 0; i < text.Length(); i++)
    {
        // Try to find LF
        if (*readPtr == '\n')
        {
            // Check if next isn't CR
            if (*(readPtr + 1) != '\r')
            {
                // Append CR
                output.Add('\r');
            }
        }

        // Copy character
        output.Add(*readPtr++);
    }
}

bool Win32FileSystem::getFilesFromDirectoryTop(Array<String>& results, const String& directory, const Char* searchPattern)
{
    // Try to find first file
    WIN32_FIND_DATA info;
    String pattern = directory / searchPattern;
    const HANDLE hp = FindFirstFileW(*pattern, &info);
    if (INVALID_HANDLE_VALUE == hp)
    {
        // Check if no files at all
        return GetLastError() != ERROR_FILE_NOT_FOUND;
    }

    do
    {
        // Check if it isn't a special case
        if (StringUtils::Compare(info.cFileName, TEXT(".")) == 0 || StringUtils::Compare(info.cFileName, TEXT("..")) == 0)
            continue;

        // Check if its a directory or a file
        if (!(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            // Add file
            results.Add(directory / info.cFileName);
        }
    } while (FindNextFileW(hp, &info) != 0);
    FindClose(hp);

    if (GetLastError() != ERROR_NO_MORE_FILES)
    {
        return true;
    }

    return false;
}

bool Win32FileSystem::getFilesFromDirectoryAll(Array<String>& results, const String& directory, const Char* searchPattern)
{
    // Find all files in this directory
    getFilesFromDirectoryTop(results, directory, searchPattern);

    // Try to find first file/directory
    WIN32_FIND_DATA info;
    const HANDLE hp = FindFirstFileW(*(directory / TEXT('*')), &info);
    if (INVALID_HANDLE_VALUE == hp)
    {
        // Check if no files at all
        return GetLastError() != ERROR_FILE_NOT_FOUND;
    }

    do
    {
        // Check if it isn't a special case
        if (StringUtils::Compare(info.cFileName, TEXT(".")) == 0 || StringUtils::Compare(info.cFileName, TEXT("..")) == 0)
            continue;

        // Check if its a directory or a file
        String tmpPath = directory / info.cFileName;
        if (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (getFilesFromDirectoryAll(results, tmpPath, searchPattern))
            {
                FindClose(hp);
                return true;
            }
        }
    } while (FindNextFileW(hp, &info) != 0);
    FindClose(hp);

    return GetLastError() != ERROR_NO_MORE_FILES;
}

DateTime Win32FileSystem::GetFileLastEditTime(const StringView& path)
{
    DateTime result = DateTime::MinValue();

#if 0
	// Universal way to get file write time but 2x slower on Windows
	auto file = File::Open(path, FileMode::OpenExisting, FileAccess::Read, FileShare::All);
	if (file)
	{
		result = file->GetLastWriteTime();
		Delete(file);
	}
#endif

    WIN32_INIT_BUFFER(path, buffer);
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (!!GetFileAttributesExW(buffer, GetFileExInfoStandard, &data))
    {
        SYSTEMTIME lpSystemTime;
        FileTimeToSystemTime(&data.ftLastWriteTime, &lpSystemTime);

        result = DateTime(lpSystemTime.wYear, lpSystemTime.wMonth, lpSystemTime.wDay, lpSystemTime.wHour, lpSystemTime.wMinute, lpSystemTime.wSecond, lpSystemTime.wMilliseconds);
    }

    return result;
}

#endif
