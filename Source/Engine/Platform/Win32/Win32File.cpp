// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_WIN32

#include "Win32File.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Log.h"
#include "IncludeWindowsHeaders.h"

Win32File::~Win32File()
{
    Close();
}

Win32File* Win32File::Open(const StringView& path, FileMode mode, FileAccess access, FileShare share)
{
    // Try to open the file
    // Note: FileMode, FileAccess and FileShare enums are mapping directly to the Win32 types
#if PLATFORM_UWP
	CREATEFILE2_EXTENDED_PARAMETERS param = { 0 };
	param.dwSize = sizeof(CREATEFILE2_EXTENDED_PARAMETERS);
	param.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
	param.dwSecurityQosFlags = SECURITY_ANONYMOUS;
	auto handle = CreateFile2(*path, (DWORD)access, (DWORD)share, (DWORD)mode, &param);
#else
    auto handle = CreateFileW(*path, (DWORD)access, (DWORD)share, nullptr, (DWORD)mode, FILE_ATTRIBUTE_NORMAL, nullptr);
#endif
    if (handle == INVALID_HANDLE_VALUE)
    {
        LOG_WIN32_LAST_ERROR;
        return nullptr;
    }

    return New<Win32File>((void*)handle);
}

bool Win32File::Read(void* buffer, uint32 bytesToRead, uint32* bytesRead)
{
    // Try to read data
    DWORD tmp;
    if (ReadFile(_handle, buffer, bytesToRead, &tmp, nullptr))
    {
        if (bytesRead)
            *bytesRead = tmp;
        return false;
    }

    if (bytesRead)
        *bytesRead = 0;
    return true;
}

bool Win32File::Write(const void* buffer, uint32 bytesToWrite, uint32* bytesWritten)
{
    // Try to write data
    DWORD tmp;
    if (WriteFile(_handle, buffer, bytesToWrite, &tmp, nullptr))
    {
        if (bytesWritten)
            *bytesWritten = tmp;
        return false;
    }

    if (bytesWritten)
        *bytesWritten = 0;
    return true;
}

void Win32File::Close()
{
    if (_handle)
    {
        CloseHandle(_handle);
        _handle = nullptr;
    }
}

uint32 Win32File::GetSize() const
{
    LARGE_INTEGER result;
    GetFileSizeEx(_handle, &result);
    return (uint32)result.QuadPart;
}

DateTime Win32File::GetLastWriteTime() const
{
    FILETIME lpLastWriteTime;
    GetFileTime(_handle, nullptr, nullptr, &lpLastWriteTime);

    SYSTEMTIME lpSystemTime;
    FileTimeToSystemTime(&lpLastWriteTime, &lpSystemTime);

    return DateTime(lpSystemTime.wYear, lpSystemTime.wMonth, lpSystemTime.wDay, lpSystemTime.wHour, lpSystemTime.wMinute, lpSystemTime.wSecond, lpSystemTime.wMilliseconds);
}

uint32 Win32File::GetPosition() const
{
    return SetFilePointer(_handle, 0, nullptr, FILE_CURRENT);
}

void Win32File::SetPosition(uint32 seek)
{
    SetFilePointer(_handle, seek, nullptr, FILE_BEGIN);
}

bool Win32File::IsOpened() const
{
    return _handle != nullptr;
}

#endif
