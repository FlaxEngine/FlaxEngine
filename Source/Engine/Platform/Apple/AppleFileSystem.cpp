// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_MAC || PLATFORM_IOS

#include "AppleFileSystem.h"
#include "AppleUtils.h"
#include "Engine/Platform/File.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/StringView.h"
#include "Engine/Utilities/StringConverter.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <cerrno>
#include <dirent.h>
#include <stdlib.h>
#include <fcntl.h>

bool AppleFileSystem::CopyFile(const StringView& dst, const StringView& src)
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

void AppleFileSystem::GetSpecialFolderPath(const SpecialFolder type, String& result)
{
    String home;
    Platform::GetEnvironmentVariable(TEXT("HOME"), home);
    switch (type)
    {
    case SpecialFolder::Desktop:
        result = home / TEXT("/Desktop");
        break;
    case SpecialFolder::Documents:
        result = home / TEXT("/Documents");
        break;
    case SpecialFolder::Pictures:
        result = home / TEXT("/Pictures");
        break;
    case SpecialFolder::AppData:
    case SpecialFolder::LocalAppData:
        result = home / TEXT("/Library/Caches");
        break;
    case SpecialFolder::ProgramData:
        result = home / TEXT("/Library/Application Support");
        break;
    case SpecialFolder::Temporary:
        Platform::GetEnvironmentVariable(TEXT("TMPDIR"), result);
        break;
    }
}

#endif
