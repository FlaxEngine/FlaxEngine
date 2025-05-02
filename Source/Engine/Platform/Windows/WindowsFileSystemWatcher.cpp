// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_WINDOWS

#include "WindowsFileSystemWatcher.h"
#include "Engine/Platform/CriticalSection.h"
#include "Engine/Platform/Thread.h"
#include "Engine/Threading/ThreadSpawner.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Collections/Array.h"
#include "../Win32/IncludeWindowsHeaders.h"

namespace FileSystemWatchers
{
    CriticalSection Locker;
    Array<WindowsFileSystemWatcher*, InlinedAllocation<16>> Watchers;
    Win32Thread* Thread = nullptr;
    Windows::HANDLE IoHandle = INVALID_HANDLE_VALUE;
    bool ThreadActive;

    int32 Run()
    {
        DWORD numBytes = 0;
        LPOVERLAPPED overlapped;
        ULONG_PTR compKey = 0;
        while (ThreadActive)
        {
            if (GetQueuedCompletionStatus(IoHandle, &numBytes, &compKey, &overlapped, INFINITE) && overlapped && numBytes != 0)
            {
                // Send further to the specific watcher
                Locker.Lock();
                for (auto watcher : Watchers)
                {
                    if ((OVERLAPPED*)&watcher->Overlapped == overlapped)
                    {
                        watcher->NotificationCompletion();
                        break;
                    }
                }
                Locker.Unlock();
            }
        }
        return 0;
    }
};

WindowsFileSystemWatcher::WindowsFileSystemWatcher(const String& directory, bool withSubDirs)
    : FileSystemWatcherBase(directory, withSubDirs)
{
    // Setup
    Platform::MemoryClear(&Overlapped, sizeof(Overlapped));

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
        FileSystemWatchers::IoHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1);
        FileSystemWatchers::ThreadActive = true;
        FileSystemWatchers::Thread = ThreadSpawner::Start(FileSystemWatchers::Run, TEXT("File System Watchers"), ThreadPriority::BelowNormal);
    }
    CreateIoCompletionPort(DirectoryHandle, FileSystemWatchers::IoHandle, 0, 1);
    FileSystemWatchers::Locker.Unlock();

    // Initialize filesystem events tracking
    ReadDirectoryChanges();
}

WindowsFileSystemWatcher::~WindowsFileSystemWatcher()
{
    FileSystemWatchers::Locker.Lock();
    FileSystemWatchers::Watchers.Remove(this);
    StopNow = true;
    FileSystemWatchers::Locker.Unlock();

    if (DirectoryHandle != INVALID_HANDLE_VALUE)
    {
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
        FileSystemWatchers::Locker.Unlock();
        PostQueuedCompletionStatus(FileSystemWatchers::IoHandle, 0, 0, nullptr);
        FileSystemWatchers::Thread->Join();
        FileSystemWatchers::Locker.Lock();
        Delete(FileSystemWatchers::Thread);
        FileSystemWatchers::Thread = nullptr;
        CloseHandle(FileSystemWatchers::IoHandle);
        FileSystemWatchers::IoHandle = INVALID_HANDLE_VALUE;
    }
    FileSystemWatchers::Locker.Unlock();
}

void WindowsFileSystemWatcher::ReadDirectoryChanges()
{
    BOOL result = ReadDirectoryChangesW(
        DirectoryHandle,
        Buffer,
        BufferSize,
        WithSubDirs ? TRUE : FALSE,
        FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_FILE_NAME,
        nullptr,
        (OVERLAPPED*)&Overlapped,
        nullptr
    );
    if (!result)
    {
        LOG_WIN32_LAST_ERROR;
        Sleep(1);
    }
}

void WindowsFileSystemWatcher::NotificationCompletion()
{
    ScopeLock lock(Locker);

    // Process notifications
    auto notify = (FILE_NOTIFY_INFORMATION*)Buffer;
    do
    {
        // Convert action type
        FileSystemAction action;
        switch (notify->Action)
        {
        case FILE_ACTION_RENAMED_NEW_NAME:
        case FILE_ACTION_RENAMED_OLD_NAME:
            action = FileSystemAction::Rename;
            break;
        case FILE_ACTION_ADDED:
            action = FileSystemAction::Create;
            break;
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
            path = Directory / path;

            // Send event
            OnEvent(path, action);
        }

        // Move to the next notify
        notify = (FILE_NOTIFY_INFORMATION*)((byte*)notify + notify->NextEntryOffset);
    } while (notify->NextEntryOffset != 0);

    // Get the new read issued as fast as possible
    if (!StopNow)
    {
        ReadDirectoryChanges();
    }
}

#endif
