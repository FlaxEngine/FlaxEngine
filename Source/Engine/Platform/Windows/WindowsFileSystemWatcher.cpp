// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#if PLATFORM_WINDOWS

#include "WindowsFileSystemWatcher.h"
#include "Engine/Platform/CriticalSection.h"
#include "Engine/Platform/Thread.h"
#include "Engine/Threading/ThreadSpawner.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Collections/Array.h"
#include "../Win32/IncludeWindowsHeaders.h"

BOOL RefreshWatch(WindowsFileSystemWatcher* watcher);

namespace FileSystemWatchers
{
    CriticalSection Locker;
    Array<WindowsFileSystemWatcher*, InlinedAllocation<16>> Watchers;
    Win32Thread* Thread = nullptr;
    bool ThreadActive;

    int32 Run()
    {
        while (ThreadActive)
        {
            SleepEx(INFINITE, true);
        }
        return 0;
    }

    static void CALLBACK StopProc(ULONG_PTR arg)
    {
        ThreadActive = false;
    }

    static void CALLBACK AddDirectoryProc(ULONG_PTR arg)
    {
        const auto watcher = (FileSystemWatcher*)arg;
        RefreshWatch(watcher);
    }
};

VOID CALLBACK NotificationCompletion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
{
    auto watcher = (FileSystemWatcher*)lpOverlapped->hEvent;
    if (dwErrorCode == ERROR_OPERATION_ABORTED ||
        dwNumberOfBytesTransfered <= 0 ||
        !watcher)
    {
        return;
    }

    // Swap buffers
    watcher->CurrentBuffer = (watcher->CurrentBuffer + 1) % 2;

    // Get the new read issued as fast as possible
    if (!watcher->StopNow)
    {
        RefreshWatch(watcher);
    }

    // Process notifications
    auto notify = (FILE_NOTIFY_INFORMATION*)watcher->Buffer[(watcher->CurrentBuffer + 1) % 2];
    do
    {
        // Convert action type
        auto action = FileSystemAction::Unknown;
        switch (notify->Action)
        {
        case FILE_ACTION_RENAMED_NEW_NAME:
        case FILE_ACTION_ADDED:
            action = FileSystemAction::Create;
            break;
        case FILE_ACTION_RENAMED_OLD_NAME:
        case FILE_ACTION_REMOVED:
            action = FileSystemAction::Delete;
            break;
        case FILE_ACTION_MODIFIED:
            action = FileSystemAction::Modify;
            break;
        default:
            action = FileSystemAction::Unknown;
            break;
        }
        if (action != FileSystemAction::Unknown)
        {
            // Build path
            String path(notify->FileName, notify->FileNameLength / sizeof(WCHAR));
            path = watcher->Directory / path;

            // Send event
            watcher->OnEvent(path, action);
        }

        // Move to the next notify
        notify = (FILE_NOTIFY_INFORMATION*)((byte*)notify + notify->NextEntryOffset);
    } while (notify->NextEntryOffset != 0);
}

// Refreshes the directory monitoring
BOOL RefreshWatch(WindowsFileSystemWatcher* watcher)
{
    DWORD dwBytesReturned = 0;
    return ReadDirectoryChangesW(
        watcher->DirectoryHandle,
        watcher->Buffer[watcher->CurrentBuffer],
        FileSystemWatcher::BufferSize,
        watcher->WithSubDirs ? TRUE : FALSE,
        FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_FILE_NAME,
        &dwBytesReturned,
        (OVERLAPPED*)&watcher->Overlapped,
        NotificationCompletion
    );
}

WindowsFileSystemWatcher::WindowsFileSystemWatcher(const String& directory, bool withSubDirs)
    : FileSystemWatcherBase(directory, withSubDirs)
    , StopNow(false)
    , CurrentBuffer(0)
{
    // Setup
    Platform::MemoryClear(&Overlapped, sizeof(Overlapped));
    ((OVERLAPPED&)Overlapped).hEvent = this;

    // Create directory handle for events handling
    DirectoryHandle = CreateFileW(
        *directory,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        nullptr
    );
    if (DirectoryHandle == INVALID_HANDLE_VALUE)
    {
        LOG_WIN32_LAST_ERROR;
        return;
    }

    // Register
    FileSystemWatchers::Locker.Lock();
    FileSystemWatchers::Watchers.Add(this);
    if (!FileSystemWatchers::Thread)
    {
        FileSystemWatchers::ThreadActive = true;
        FileSystemWatchers::Thread = ThreadSpawner::Start(FileSystemWatchers::Run, TEXT("File System Watchers"), ThreadPriority::BelowNormal);
    }
    FileSystemWatchers::Locker.Unlock();

    // Issue the first read
    QueueUserAPC(FileSystemWatchers::AddDirectoryProc, FileSystemWatchers::Thread->GetHandle(), (ULONG_PTR)this);
}

WindowsFileSystemWatcher::~WindowsFileSystemWatcher()
{
    FileSystemWatchers::Locker.Lock();
    FileSystemWatchers::Watchers.Remove(this);
    FileSystemWatchers::Locker.Unlock();

    if (DirectoryHandle != INVALID_HANDLE_VALUE)
    {
        StopNow = true;

#if WINVER >= 0x600
        CancelIoEx(DirectoryHandle, (OVERLAPPED*)&Overlapped);
#else
        CancelIo(DirectoryHandle);
#endif

        const HANDLE handle = DirectoryHandle;
        DirectoryHandle = INVALID_HANDLE_VALUE;
        WaitForSingleObjectEx(handle, 0, true);

        CloseHandle(handle);
    }

    FileSystemWatchers::Locker.Lock();
    if (FileSystemWatchers::Watchers.IsEmpty() && FileSystemWatchers::Thread)
    {
        FileSystemWatchers::ThreadActive = false;
        QueueUserAPC(FileSystemWatchers::StopProc, FileSystemWatchers::Thread->GetHandle(), 0);
        FileSystemWatchers::Thread->Join();
        Delete(FileSystemWatchers::Thread);
        FileSystemWatchers::Thread = nullptr;
    }
    FileSystemWatchers::Locker.Unlock();
}

#endif
