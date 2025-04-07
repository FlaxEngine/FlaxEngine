// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_WINDOWS

#include "WindowsClipboard.h"
#include "Engine/Core/Collections/Array.h"
#include "../Win32/IncludeWindowsHeaders.h"

typedef struct _DROPFILES
{
    DWORD pFiles;
    POINT pt;
    BOOL fNC;
    BOOL fWide;
} DROPFILES, *LPDROPFILES;

void WindowsClipboard::Clear()
{
    OpenClipboard(nullptr);
    EmptyClipboard();
    CloseClipboard();
}

void WindowsClipboard::SetText(const StringView& text)
{
    const int32 sizeWithoutNull = text.Length() * sizeof(Char);
    const HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, sizeWithoutNull + sizeof(Char));

    Char* pMem = static_cast<Char*>(GlobalLock(hMem));
    Platform::MemoryCopy(pMem, text.GetText(), sizeWithoutNull);
    Platform::MemorySet(pMem + text.Length(), sizeof(Char), 0);
    GlobalUnlock(hMem);

    OpenClipboard(nullptr);
    EmptyClipboard();
    SetClipboardData(CF_UNICODETEXT, hMem);
    CloseClipboard();
}

void WindowsClipboard::SetRawData(const Span<byte>& data)
{
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, data.Length());
    Platform::MemoryCopy(GlobalLock(hMem), data.Get(), data.Length());
    GlobalUnlock(hMem);

    OpenClipboard(nullptr);
    EmptyClipboard();
    SetClipboardData(CF_PENDATA, hMem);
    CloseClipboard();
}

void WindowsClipboard::SetFiles(const Array<String>& files)
{
    int32 size = sizeof(DROPFILES);
    for (int32 i = 0; i < files.Count(); i++)
    {
        size += (files[i].Length() + 1) * sizeof(Char);
    }
    size += 2 * sizeof(Char);

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);
    DROPFILES* data = (DROPFILES*)GlobalLock(hMem);
    ZeroMemory(data, size);
    data->pFiles = sizeof(DROPFILES);
    data->fWide = TRUE;
    LPWSTR ptr = (LPWSTR)(data + 1);
    for (int32 i = 0; i < files.Count(); i++)
    {
        Platform::MemoryCopy(ptr, files[i].Get(), (files[i].Length() + 1) * sizeof(Char));
        ptr += files[i].Length() + 1;
    }
    *ptr++ = 0;
    *ptr = 0;
    GlobalUnlock(hMem);

    OpenClipboard(nullptr);
    EmptyClipboard();
    SetClipboardData(CF_HDROP, hMem);
    CloseClipboard();
}

String WindowsClipboard::GetText()
{
    String result;
    OpenClipboard(nullptr);
    HANDLE hMem = GetClipboardData(CF_UNICODETEXT);
    if (hMem)
    {
        Char* data = static_cast<Char*>(GlobalLock(hMem));
        result = data;
        GlobalUnlock(data);
    }
    else
    {
        hMem = GetClipboardData(CF_TEXT);
        if (hMem)
        {
            char* data = static_cast<char*>(GlobalLock(hMem));
            result = data;
            GlobalUnlock(data);
        }
    }
    CloseClipboard();
    return result;
}

Array<byte> WindowsClipboard::GetRawData()
{
    Array<byte> result;
    OpenClipboard(nullptr);
    HANDLE hMem = GetClipboardData(CF_PENDATA);
    if (hMem)
    {
        auto data = GlobalLock(hMem);
        auto size = static_cast<int32>(GlobalSize(hMem));

        if (size > 0)
        {
            result.Set(static_cast<byte*>(data), size);
        }

        GlobalUnlock(data);
    }
    CloseClipboard();
    return result;
}

Array<String> WindowsClipboard::GetFiles()
{
    Array<String> result;
    OpenClipboard(nullptr);
    HANDLE hMem = GetClipboardData(CF_HDROP);
    if (hMem)
    {
        auto data = (DROPFILES*)GlobalLock(hMem);
        auto size = static_cast<int32>(GlobalSize(hMem));

        if (size > 0)
        {
            if (data->fWide)
            {
                Char* ptr = (Char*)(data + 1);
                while (*ptr)
                {
                    int32 length = StringUtils::Length(ptr);
                    result.Add(String(ptr, length));
                    ptr += length + 1;
                }
            }
            else
            {
                char* ptr = (char*)(data + 1);
                while (*ptr)
                {
                    int32 length = StringUtils::Length(ptr);
                    result.Add(String(ptr, length));
                    ptr += length + 1;
                }
            }
        }

        GlobalUnlock(data);
    }
    CloseClipboard();
    return result;
}

#endif
