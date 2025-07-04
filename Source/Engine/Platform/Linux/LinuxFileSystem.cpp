// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_LINUX

#include "LinuxFileSystem.h"
#include "Engine/Platform/File.h"
#include "Engine/Core/Types/StringBuilder.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Log.h"
#include "Engine/Utilities/StringConverter.h"
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <stdio.h>
#include <cerrno>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

const DateTime UnixEpoch(1970, 1, 1);

bool LinuxFileSystem::ShowOpenFileDialog(Window* parentWindow, const StringView& initialDirectory, const StringView& filter, bool multiSelect, const StringView& title, Array<String, HeapAllocation>& filenames)
{
    const StringAsUTF8<> initialDirectoryAnsi(*initialDirectory, initialDirectory.Length());
    const StringAsUTF8<> titleAnsi(*title, title.Length());
    const char* initDir = initialDirectory.HasChars() ? initialDirectoryAnsi.Get() : ".";
    String xdgCurrentDesktop;
    StringBuilder fileFilter;
    Array<String> fileFilterEntries;
    Platform::GetEnvironmentVariable(TEXT("XDG_CURRENT_DESKTOP"), xdgCurrentDesktop);
    StringUtils::GetZZString(filter.Get()).Split('\0', fileFilterEntries);

    const bool zenitySupported = FileSystem::FileExists(TEXT("/usr/bin/zenity"));
    const bool kdialogSupported = FileSystem::FileExists(TEXT("/usr/bin/kdialog"));
    char cmd[2048];
    if (zenitySupported && (xdgCurrentDesktop != TEXT("KDE") || !kdialogSupported)) // Prefer kdialog when running on KDE
    {
        for (int32 i = 1; i < fileFilterEntries.Count(); i += 2)
        {
            String extensions(fileFilterEntries[i]);
            fileFilterEntries[i].Replace(TEXT(";"), TEXT(" "));
            fileFilter.Append(String::Format(TEXT("{0}--file-filter=\"{1}|{2}\""), i > 1 ? TEXT(" ") : TEXT(""), fileFilterEntries[i-1].Get(), extensions.Get()));
        }

        sprintf(cmd, "/usr/bin/zenity --modal --file-selection %s--filename=\"%s\" --title=\"%s\" %s ", multiSelect ? "--multiple --separator=$'\n' " : " ", initDir, titleAnsi.Get(), fileFilter.ToStringView().ToStringAnsi().GetText());
    }
    else if (kdialogSupported)
    {
        for (int32 i = 1; i < fileFilterEntries.Count(); i += 2)
        {
            String extensions(fileFilterEntries[i]);
            fileFilterEntries[i].Replace(TEXT(";"), TEXT(" "));
            fileFilter.Append(String::Format(TEXT("{0}\"{1}({2})\""), i > 1 ? TEXT(" ") : TEXT(""), fileFilterEntries[i-1].Get(), extensions.Get()));
        }
        fileFilter.Append(String::Format(TEXT("{0}\"{1}({2})\""), TEXT(" "), TEXT("many things"), TEXT("*.png *.jpg")));

        sprintf(cmd, "/usr/bin/kdialog --getopenfilename %s--title \"%s\" \"%s\" %s ", multiSelect ? "--multiple --separate-output " : " ", titleAnsi.Get(), initDir, fileFilter.ToStringView().ToStringAnsi().GetText());
    }
    else
    {
        LOG(Error, "Missing file picker (install zenity or kdialog).");
        return true;    
    }
    FILE* f = popen(cmd, "r");
    char buf[2048];
    char* writePointer = buf;
    int remainingCapacity = ARRAY_COUNT(buf);
    // make sure we read all output from kdialog
    while (remainingCapacity > 0 && fgets(writePointer, remainingCapacity, f))
    {
        int r = strlen(writePointer);
        writePointer += r;
        remainingCapacity -= r;
    }
    if (remainingCapacity <= 0)
    {
        LOG(Error, "You selected more files than an internal buffer can hold. Try selecting fewer files at a time.");
        // in case of an overflow we miss the closing null byte, add it after the rightmost linefeed
        while (*writePointer != '\n')
        {
            writePointer--;
            if (writePointer == buf)
            {
                *buf = 0;
                break;
            }
        }
        *(++writePointer) = 0;
    }
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

bool LinuxFileSystem::ShowBrowseFolderDialog(Window* parentWindow, const StringView& initialDirectory, const StringView& title, String& path)
{
    const StringAsUTF8<> titleAnsi(*title, title.Length());
    String xdgCurrentDesktop;
    Platform::GetEnvironmentVariable(TEXT("XDG_CURRENT_DESKTOP"), xdgCurrentDesktop);

    // TODO: support initialDirectory

    const bool zenitySupported = FileSystem::FileExists(TEXT("/usr/bin/zenity"));
    const bool kdialogSupported = FileSystem::FileExists(TEXT("/usr/bin/kdialog"));
    char cmd[2048];
    if (zenitySupported && (xdgCurrentDesktop != TEXT("KDE") || !kdialogSupported)) // Prefer kdialog when running on KDE
    {
        sprintf(cmd, "/usr/bin/zenity --modal --file-selection --directory --title=\"%s\" ", titleAnsi.Get());
    }
    else if (kdialogSupported)
    {
        sprintf(cmd, "/usr/bin/kdialog --getexistingdirectory --title \"%s\" ", titleAnsi.Get());
    }
    else
    {
        LOG(Error, "Missing file picker (install zenity or kdialog).");
        return true;    
    }
    FILE* f = popen(cmd, "r");
    char buf[2048];
    fgets(buf, ARRAY_COUNT(buf), f); 
    int result = pclose(f);
    if (result != 0)
        return true;

    const char* c = buf;
    while (*c)
    {
        const char* start = c;
        while (*c != '\n')
            c++;
        path = String(start, (int32)(c - start));
        if (path.HasChars())
            break;
        c++;
    }
    return false;
}

bool LinuxFileSystem::ShowFileExplorer(const StringView& path)
{
    const StringAsUTF8<> pathAnsi(*path, path.Length());
    char cmd[2048];
    sprintf(cmd, "xdg-open %s &", pathAnsi.Get());
    system(cmd);
    return false;
}

bool LinuxFileSystem::CopyFile(const StringView& dst, const StringView& src)
{
    const StringAsUTF8<> srcANSI(*src, src.Length());
    const StringAsUTF8<> dstANSI(*dst, dst.Length());

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

    // first try the kernel method
    struct stat statBuf;
    fstat(srcFile, &statBuf);
    readSize = 1;
    while (readSize > 0)
    {
        readSize = sendfile(dstFile, srcFile, 0, statBuf.st_size);
    }
    // sendfile could fail for example if the input file is not nmap'able
    // in this case we fall back to the read/write loop
    if (readSize < 0)
    {
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

bool LinuxFileSystem::MoveFileToRecycleBin(const StringView& path)
{
    String trashDir;
    GetSpecialFolderPath(SpecialFolder::LocalAppData, trashDir);
    trashDir += TEXT("/Trash");
    const String filesDir = trashDir + TEXT("/files");
    const String infoDir = trashDir + TEXT("/info");
    String trashName = getBaseName(path);
    String dst = filesDir / trashName;

    int fd = -1;
    if (FileExists(dst))
    {
        const String ext = GetExtension(path);
        dst = filesDir / getNameWithoutExtension(path) + TEXT("XXXXXX.") + ext;
        const char *templateString = dst.ToStringAnsi().Get();
        char writableName[strlen(templateString) + 1];
        strcpy(writableName, templateString);
        fd = mkstemps(writableName, ext.Length() + 1);
        if (fd < 0)
        {
            LOG(Error, "Cannot create a temporary file as {0}, errno={1}", String(writableName), errno);
            return true;
        }
        dst = String(writableName);
        trashName = getBaseName(dst);
    }
    if (fd != -1)
        close(fd);

    if (!MoveFile(dst, path, true))
    {
        // Not MoveFile means success so write the info file
        const String infoFile = infoDir / trashName + TEXT(".trashinfo");
        StringBuilder trashInfo;
        const char *ansiPath = path.ToStringAnsi().Get();
        const int maxLength = strlen(ansiPath) * 3 + 1; // in the worst case the length will be tripled
        char encoded[maxLength];
        if (!UrnEncodePath(ansiPath, encoded, maxLength))
        {
            // unlikely but better keep something
            strcpy(encoded, ansiPath);
        }
        const DateTime now = DateTime::Now();
        const String rfcDate = String::Format(TEXT("{0}-{1:0>2}-{2:0>2}T{3:0>2}:{4:0>2}:{5:0>2}"), now.GetYear(), now.GetMonth(), now.GetDay(), now.GetHour(), now.GetMinute(), now.GetSecond());
        trashInfo.AppendLine(TEXT("[Trash Info]")).Append(TEXT("Path=")).Append(encoded).Append(TEXT('\n'));
        trashInfo.Append(TEXT("DeletionDate=")).Append(rfcDate).Append(TEXT("\n\0"));

        // a failure to write the info file is considered non-fatal according to the FreeDesktop.org specification
        FileBase::WriteAllText(infoFile, trashInfo, Encoding::ANSI);
        // return false on success as the Windows pendant does
        return false;
    }
    return true;
}

String LinuxFileSystem::getBaseName(const StringView& path)
{
    String baseName = path.Substring(path.FindLast('/') + 1);
    if (baseName.IsEmpty())
        baseName = path;
    return baseName;
}

String LinuxFileSystem::getNameWithoutExtension(const StringView& path)
{
    String baseName = getBaseName(path);
    const int pos = baseName.FindLast('.');
    if (pos > 0)
        return baseName.Left(pos);
    return baseName;
}

bool LinuxFileSystem::UrnEncodePath(const char *path, char *result, const int maxLength)
{
    static auto digits = "0123456789ABCDEF";
    const char *src = path;
    char *dest = result;
    while (*src)
    {
        if (*src <= 0x20 || *src > 0x7f || *src == '%')
        {
            if (dest - result + 4 > maxLength)
                return false;
            *dest++ = '%';
            *dest++ = digits[*src>>4 & 0xf];
            *dest = digits[*src & 0xf];
        }
        else
        {
            *dest = *src;
        }
        src++;
        dest++;
        if (dest - result == maxLength)
            return false;
    }
    *dest = 0;
    return true;
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
    {
        String dataHome;
        if (!Platform::GetEnvironmentVariable(TEXT("XDG_DATA_HOME"), dataHome))
            result = dataHome;
        else
            result = home / TEXT(".local/share");
        break;
    }
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
