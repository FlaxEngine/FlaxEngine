// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if PLATFORM_MAC

#include "../File.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/DateTime.h"
#include "Engine/Core/Types/TimeSpan.h"
#include "Engine/Utilities/StringConverter.h"
#include "Engine/Core/Log.h"
#include <sys/file.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>

MacFile::MacFile(int32 handle)
    : _handle(handle)
{
}

MacFile::~MacFile()
{
    Close();
}

MacFile* MacFile::Open(const StringView& path, FileMode mode, FileAccess access, FileShare share)
{
    int flags = O_CLOEXEC;
    switch (access)
    {
    case FileAccess::Read:
        flags |= O_RDONLY;
        break;
    case FileAccess::Write:
        flags |= O_WRONLY;
        break;
    case FileAccess::ReadWrite:
        flags |= O_RDWR;
        break;
    default: ;
    }
    switch (mode)
    {
    case FileMode::CreateAlways:
        flags |= O_CREAT | O_TRUNC;
        break;
    case FileMode::CreateNew:
        flags |= O_CREAT | O_EXCL;
        break;
    case FileMode::OpenAlways:
        break;
    case FileMode::OpenExisting:
        break;
    case FileMode::TruncateExisting:
        flags |= O_TRUNC;
        break;
    default: ;
    }

    mode_t omode = S_IRUSR | S_IWUSR;
    if ((uint32)share & (uint32)FileShare::Delete)
        omode |= 0;
    if ((uint32)share & (uint32)FileShare::Read)
        omode |= (mode_t)(S_IRGRP | S_IROTH);
    if ((uint32)share & (uint32)FileShare::Write)
        omode |= (mode_t)(S_IWGRP | S_IWOTH);
    if ((uint32)share & (uint32)FileShare::Delete)
        omode |= 0;

    const StringAsANSI<> pathANSI(*path, path.Length());
    auto handle = open(pathANSI.Get(), flags, omode);
    if (handle == -1)
    {
        return nullptr;
    }

    return New<MacFile>(handle);
}

bool MacFile::Read(void* buffer, uint32 bytesToRead, uint32* bytesRead)
{
    const ssize_t tmp = read(_handle, buffer, bytesToRead);
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

bool MacFile::Write(const void* buffer, uint32 bytesToWrite, uint32* bytesWritten)
{
    const ssize_t tmp = write(_handle, buffer, bytesToWrite);
    if (tmp != -1)
    {
        if (bytesWritten)
            *bytesWritten = tmp;
        return false;
    }
    if (bytesWritten)
        *bytesWritten = 0;
    return true;
}

void MacFile::Close()
{
    if (_handle != -1)
    {
        close(_handle);
        _handle = -1;
    }
}

uint32 MacFile::GetSize() const
{
    struct stat fileInfo;
    fstat(_handle, &fileInfo);
    return fileInfo.st_size;
}

DateTime MacFile::GetLastWriteTime() const
{
    struct stat fileInfo;
    if (fstat(_handle, &fileInfo) == -1)
    {
        return DateTime::MinValue();
    }
    const TimeSpan timeSinceEpoch(0, 0, fileInfo.st_mtime);
    const DateTime MacEpoch(1970, 1, 1);
    return MacEpoch + timeSinceEpoch;
}

uint32 MacFile::GetPosition() const
{
    return lseek(_handle, 0, SEEK_CUR);
}

void MacFile::SetPosition(uint32 seek)
{
    lseek(_handle, seek, SEEK_SET);
}

bool MacFile::IsOpened() const
{
    return _handle != -1;
}

#endif
