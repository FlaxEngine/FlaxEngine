// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if PLATFORM_WINDOWS

#include "WindowsFileSystem.h"
#include "Engine/Platform/File.h"
#include "Engine/Platform/Window.h"
#include "Engine/Platform/Windows/ComPtr.h"
#include "Engine/Core/Types/StringView.h"
#include "../Win32/IncludeWindowsHeaders.h"

// Hack this stuff (the problem is that GDI has function named Rectangle -> like one of the Flax core types)
#define _PRSHT_H_
#define __shobjidl_h__
#define LPFNADDPROPSHEETPAGE void*
#include <ShlObj.h>
#include <ShellAPI.h>
#include <KnownFolders.h>

namespace Windows
{
    typedef UINT_PTR (CALLBACK *LPOFNHOOKPROC)(HWND, UINT, WPARAM, LPARAM);

    typedef struct tagOFNW
    {
        DWORD lStructSize;
        HWND hwndOwner;
        HINSTANCE hInstance;
        LPCWSTR lpstrFilter;
        LPWSTR lpstrCustomFilter;
        DWORD nMaxCustFilter;
        DWORD nFilterIndex;
        LPWSTR lpstrFile;
        DWORD nMaxFile;
        LPWSTR lpstrFileTitle;
        DWORD nMaxFileTitle;
        LPCWSTR lpstrInitialDir;
        LPCWSTR lpstrTitle;
        DWORD Flags;
        WORD nFileOffset;
        WORD nFileExtension;
        LPCWSTR lpstrDefExt;
        LPARAM lCustData;
        LPOFNHOOKPROC lpfnHook;
        LPCWSTR lpTemplateName;
        void* pvReserved;
        DWORD dwReserved;
        DWORD FlagsEx;
    } OPENFILENAMEW, *LPOPENFILENAMEW;

    typedef OPENFILENAMEW OPENFILENAME;

    WIN_API BOOL WIN_API_CALLCONV GetOpenFileNameW(LPOPENFILENAMEW);
    WIN_API BOOL WIN_API_CALLCONV GetSaveFileNameW(LPOPENFILENAMEW);

#define OFN_READONLY                 0x00000001
#define OFN_OVERWRITEPROMPT          0x00000002
#define OFN_HIDEREADONLY             0x00000004
#define OFN_NOCHANGEDIR              0x00000008
#define OFN_SHOWHELP                 0x00000010
#define OFN_ENABLEHOOK               0x00000020
#define OFN_ENABLETEMPLATE           0x00000040
#define OFN_ENABLETEMPLATEHANDLE     0x00000080
#define OFN_NOVALIDATE               0x00000100
#define OFN_ALLOWMULTISELECT         0x00000200
#define OFN_EXTENSIONDIFFERENT       0x00000400
#define OFN_PATHMUSTEXIST            0x00000800
#define OFN_FILEMUSTEXIST            0x00001000
#define OFN_CREATEPROMPT             0x00002000
#define OFN_SHAREAWARE               0x00004000
#define OFN_NOREADONLYRETURN         0x00008000
#define OFN_NOTESTFILECREATE         0x00010000
#define OFN_NONETWORKBUTTON          0x00020000
#define OFN_NOLONGNAMES              0x00040000
#define OFN_EXPLORER                 0x00080000
#define OFN_NODEREFERENCELINKS       0x00100000
#define OFN_LONGNAMES                0x00200000
#define OFN_ENABLEINCLUDENOTIFY      0x00400000
#define OFN_ENABLESIZING             0x00800000
#define OFN_DONTADDTORECENT          0x02000000
#define OFN_FORCESHOWHIDDEN          0x10000000
}

static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    if (uMsg == BFFM_INITIALIZED)
    {
        if (lParam)
        {
            SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lParam);
        }
    }

    return 0;
}

bool WindowsFileSystem::MoveFileToRecycleBin(const StringView& path)
{
    ASSERT(path.Length() < MAX_PATH);
    Char pathNullNull[MAX_PATH + 1];
    Platform::MemoryCopy(pathNullNull, *path, sizeof(Char) * path.Length() + 1);
    pathNullNull[path.Length()] = 0;
    pathNullNull[path.Length() + 1] = 0;

    SHFILEOPSTRUCT op;
    op.hwnd = nullptr;
    op.wFunc = FO_DELETE;
    op.pFrom = pathNullNull;
    op.pTo = nullptr;
    op.fFlags = FOF_ALLOWUNDO | FOF_NO_UI;
    op.fAnyOperationsAborted = FALSE;
    op.hNameMappings = nullptr;
    op.lpszProgressTitle = nullptr;

    return SHFileOperation(&op) != 0;
}

bool SameFile(HANDLE h1, HANDLE h2)
{
    BY_HANDLE_FILE_INFORMATION bhfi1 = { 0 };
    BY_HANDLE_FILE_INFORMATION bhfi2 = { 0 };

    if (::GetFileInformationByHandle(h1, &bhfi1) && ::GetFileInformationByHandle(h2, &bhfi2))
    {
        return ((bhfi1.nFileIndexHigh == bhfi2.nFileIndexHigh) && (bhfi1.nFileIndexLow == bhfi2.nFileIndexLow) && (bhfi1.dwVolumeSerialNumber == bhfi2.dwVolumeSerialNumber));
    }

    return false;
}

bool WindowsFileSystem::AreFilePathsEqual(const StringView& path1, const StringView& path2)
{
    if (path1.Compare(path2, StringSearchCase::CaseSensitive) == 0)
        return true;

    // Normalize file paths
    String filename1(path1);
    String filename2(path2);
    NormalizePath(filename1);
    NormalizePath(filename2);

    HANDLE file1 = CreateFileW(*filename1, GENERIC_READ, (DWORD)FileShare::All, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    HANDLE file2 = CreateFileW(*filename2, GENERIC_READ, (DWORD)FileShare::All, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

    bool result = SameFile(file1, file2);

    CloseHandle(file1);
    CloseHandle(file2);

    return result;
}

void WindowsFileSystem::GetSpecialFolderPath(const SpecialFolder type, String& result)
{
    KNOWNFOLDERID rfid;
    switch (type)
    {
    case SpecialFolder::Desktop:
        rfid = FOLDERID_Desktop;
        break;
    case SpecialFolder::Documents:
        rfid = FOLDERID_Documents;
        break;
    case SpecialFolder::Pictures:
        rfid = FOLDERID_Pictures;
        break;
    case SpecialFolder::AppData:
        rfid = FOLDERID_RoamingAppData;
        break;
    case SpecialFolder::LocalAppData:
        rfid = FOLDERID_LocalAppData;
        break;
    case SpecialFolder::ProgramData:
        rfid = FOLDERID_ProgramData;
        break;
    case SpecialFolder::Temporary:
    {
        Char buffer[MAX_PATH];
        if (GetTempPathW(MAX_PATH, buffer))
        {
            result = buffer;
            NormalizePath(result);
        }
        return;
    }
    }

    PWSTR path = nullptr;
    HRESULT hr = SHGetKnownFolderPath(rfid, 0, nullptr, &path);
    if (SUCCEEDED(hr))
    {
        result = path;
        NormalizePath(result);
    }

    CoTaskMemFree(path);
}

bool WindowsFileSystem::ShowOpenFileDialog(Window* parentWindow, const StringView& initialDirectory, const StringView& filter, bool multiSelect, const StringView& title, Array<String, HeapAllocation>& filenames)
{
    bool result = true;

    // Allocate memory for the filenames
    const int32 maxFilenamesSize = (multiSelect ? 200 : 2) * MAX_PATH;
    Array<Char> fileNamesBuffer;
    fileNamesBuffer.Resize(maxFilenamesSize);
    fileNamesBuffer[0] = 0;

    // Setup description
    Windows::OPENFILENAME of;
    ZeroMemory(&of, sizeof(of));
    of.lStructSize = sizeof(of);
    of.lpstrFilter = filter.HasChars() ? filter.Get() : nullptr;
    of.lpstrFile = fileNamesBuffer.Get();
    of.nMaxFile = maxFilenamesSize;
    of.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_ENABLESIZING;
    of.lpstrTitle = title.HasChars() ? title.Get() : nullptr;
    of.lpstrInitialDir = initialDirectory.HasChars() ? initialDirectory.Get() : nullptr;
    if (parentWindow)
        of.hwndOwner = static_cast<HWND>(parentWindow->GetNativePtr());
    if (multiSelect)
        of.Flags |= OFN_ALLOWMULTISELECT;

    // Show dialog
    if (GetOpenFileNameW(&of) != 0)
    {
        // Get filenames
        Char* ptr = of.lpstrFile;
        ptr[of.nFileOffset - 1] = 0;
        String directory = String(ptr);
        ptr += of.nFileOffset;
        while (*ptr)
        {
            filenames.Add(directory / ptr);
            ptr += lstrlenW(ptr);
            if (multiSelect)
                ptr++;
        }

        result = false;
    }

    return result;
}

bool WindowsFileSystem::ShowSaveFileDialog(Window* parentWindow, const StringView& initialDirectory, const StringView& filter, bool multiSelect, const StringView& title, Array<String, HeapAllocation>& filenames)
{
    bool result = true;

    // Allocate memory for the filenames
    int32 maxFilenamesSize = (multiSelect ? 200 : 2) * MAX_PATH;
    Array<Char> fileNamesBuffer;
    fileNamesBuffer.Resize(maxFilenamesSize);
    fileNamesBuffer[0] = 0;

    // Setup description
    Windows::OPENFILENAME of;
    ZeroMemory(&of, sizeof(of));
    of.lStructSize = sizeof(of);
    of.lpstrFilter = filter.HasChars() ? filter.Get() : nullptr;
    of.lpstrFile = fileNamesBuffer.Get();
    of.nMaxFile = maxFilenamesSize;
    of.Flags = OFN_EXPLORER | OFN_ENABLESIZING;
    of.lpstrTitle = title.HasChars() ? title.Get() : nullptr;
    of.lpstrInitialDir = initialDirectory.HasChars() ? initialDirectory.Get() : nullptr;
    if (parentWindow)
        of.hwndOwner = static_cast<HWND>(parentWindow->GetNativePtr());
    if (multiSelect)
        of.Flags |= OFN_ALLOWMULTISELECT;

    // Show dialog
    if (GetSaveFileNameW(&of) != 0)
    {
        // Get filenames
        Char* ptr = of.lpstrFile;
        ptr[of.nFileOffset - 1] = 0;
        String directory = String(ptr);
        ptr += of.nFileOffset;
        while (*ptr)
        {
            filenames.Add(directory / ptr);
            ptr += lstrlenW(ptr);
            if (multiSelect)
                ptr++;
        }

        result = false;
    }

    return result;
}

bool WindowsFileSystem::ShowBrowseFolderDialog(Window* parentWindow, const StringView& initialDirectory, const StringView& title, StringView& path)
{
    bool result = true;

    // Randomly generated GUID used for storing the last location of this dialog
    const Guid folderGuid(0x53890ed9, 0xa55e47ba, 0xa970bdae, 0x72acedff);

    ComPtr<IFileOpenDialog> fd;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&fd))))
    {
        DWORD options;
        fd->GetOptions(&options);
        fd->SetOptions(options | FOS_PICKFOLDERS | FOS_NOCHANGEDIR);

        if (title.HasChars())
            fd->SetTitle(title.Get());

        // Associate the last selected folder with this GUID instead of overwriting the global one
        fd->SetClientGuid(*reinterpret_cast<const GUID*>(&folderGuid));

        ComPtr<IShellItem> defaultFolder;
        if (SUCCEEDED(SHCreateItemFromParsingName(initialDirectory.Get(), NULL, IID_PPV_ARGS(&defaultFolder))))
            fd->SetFolder(defaultFolder);

        if (SUCCEEDED(fd->Show(parentWindow->GetHWND())))
        {
            ComPtr<IShellItem> si;
            if (SUCCEEDED(fd->GetResult(&si)))
            {
                LPWSTR resultPath;
                if (SUCCEEDED(si->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &resultPath)))
                {
                    path = resultPath;
                    CoTaskMemFree(resultPath);
                    result = false;
                }
            }
        }
    }

    return result;
}

bool WindowsFileSystem::ShowFileExplorer(const StringView& path)
{
    return Platform::StartProcess(path, StringView::Empty, StringView::Empty) != 0;
}

#endif
