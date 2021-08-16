// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if PLATFORM_LINUX

#include "LinuxFileSystemWatcher.h"
#include "Engine/Platform/CriticalSection.h"
#include "Engine/Platform/Thread.h"
#include "Engine/Threading/ThreadSpawner.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Core/Types/Pair.h"
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/inotify.h>

namespace FileSystemWatchers
{
    CriticalSection Locker;
    Dictionary<int, LinuxFileSystemWatcher*> RootWatchers;
    Dictionary<int, Pair<int, Pair<String, LinuxFileSystemWatcher*>>> Watchers;

    LinuxThread* Thread = nullptr;
    bool ThreadActive;
    int WacherFileDescriptor;

    constexpr int EVENT_SIZE = sizeof (inotify_event);
    constexpr int BUF_LEN = 1024 * ( EVENT_SIZE + 16 );

    void AddDirWatcher(const int rootFileDesc, const String& path);
    void DeleteDirWatcher(const int parentWacherFileDesc, String& dirName);
    LinuxFileSystemWatcher* GetRootWatcher(int watcherFileDesc);

    int32 Run()
    {
        char buffer[BUF_LEN];
        fd_set set;
        timeval timeout;

        while (ThreadActive)
        {
            FD_ZERO(&set);
            FD_SET(WacherFileDescriptor, &set);

            timeout.tv_sec = 0;
            timeout.tv_usec = 10000;

            int retVal = select(WacherFileDescriptor + 1, &set, NULL, NULL, &timeout);

            if(retVal == -1)
                LOG(Error, "WacherFileDescriptor select failed.");
            else if(retVal == 0)
            {
                continue;
            }
            else
            {
                int length = read( WacherFileDescriptor, buffer, BUF_LEN );  

                if ( length < 0 )
                {
                    return 0;
                    LOG(Error, "Failed to read WacherFileDescriptor.");
                }
                
                int event_offset = 0;
                while ( event_offset < length )
                {

                    LinuxFileSystemWatcher* watcher;
                    auto action = FileSystemAction::Unknown;

                    inotify_event* event = reinterpret_cast<inotify_event*>(&buffer[ event_offset ]);
                    if ( event->len )
                    {
                        auto wacher = Watchers[event->wd];
                        String name(event->name);

                        if ( event->mask & IN_CREATE )
                        {
                            if ( event->mask & IN_ISDIR )
                            {
                                AddDirWatcher(GetRootWatcher(event->wd)->GetWachedDirectoryDescriptor(), wacher.Second.First/name);                                
                            } else
                            {
                                GetRootWatcher(event->wd)->OnEvent(name, FileSystemAction::Create);
                            }
                        }
                        else if ( event->mask & IN_DELETE )
                        {
                            if ( event->mask & IN_ISDIR )
                            {
                                DeleteDirWatcher(event->wd, name);
                            } else
                            {
                                GetRootWatcher(event->wd)->OnEvent(name, FileSystemAction::Delete);
                            }
                        }
                        else if ( event->mask & IN_MODIFY )
                        {
                            if ( event->mask & IN_ISDIR )
                            {
                                // Do we need this?
                            } else
                            {                                                                
                                GetRootWatcher(event->wd)->OnEvent(name, FileSystemAction::Modify);
                            }
                        }
                    }            
                    event_offset += EVENT_SIZE + event->len;
                }
            } 
        }
        return 0;
    }

    void AddSubDirWatcher (const int rootFileDesc, const String& parent)
    {
        DIR *dir;
        struct dirent *entry;
        
        char c_path[parent.Length() + 1];
        Platform::MemoryCopy(c_path, parent.ToStringAnsi().GetText(),parent.Length());
        c_path[parent.Length()]=0;

        if (!(dir = opendir(c_path)))
            return;
        
        while ((entry = readdir(dir)) != NULL)
        {
            if (entry->d_type == DT_DIR)
            {
                char path[1024];
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                    continue;

               AddDirWatcher(rootFileDesc, parent / String(entry->d_name));                
            }
        }
        closedir(dir);
    };

    LinuxFileSystemWatcher* GetRootWatcher(int watcherFileDesc)
    {
        if (RootWatchers.ContainsKey(watcherFileDesc)) return RootWatchers[watcherFileDesc];

        return RootWatchers[Watchers[watcherFileDesc].First];
    }
    
    void AddDirWatcher(const int rootFileDesc, const String& path)
    {
        auto watcher = New<FileSystemWatcher>(path, false, rootFileDesc);
        Pair<String, LinuxFileSystemWatcher*> subDir = Pair<String, LinuxFileSystemWatcher*>(path, watcher);
    }

    void DeleteDirWatcher(const int parentWacherFileDesc, String& dirName)
    {
        if ( Watchers.ContainsKey(parentWacherFileDesc))
        {
            auto fullPath = Watchers[parentWacherFileDesc].Second.Second->Directory/dirName;

            for (auto& watcher : Watchers)
            {
                if(watcher.Value.Second.First.Compare(fullPath) == 0)
                {
                    Delete<LinuxFileSystemWatcher>(watcher.Value.Second.Second);
                }
            }
        }
    }
};

LinuxFileSystemWatcher::LinuxFileSystemWatcher(const String& directory, bool withSubDirs, int rootWatcher)
    : FileSystemWatcherBase(directory, withSubDirs)
{
    // Register
    FileSystemWatchers::Locker.Lock();

    if(withSubDirs)
    {
        for (const auto& watcher : FileSystemWatchers::Watchers)
        {
            LOG(Info, "Watcher:{} .", watcher.Key);
        }
    }
    if (!FileSystemWatchers::Thread)
    {
        FileSystemWatchers::WacherFileDescriptor = inotify_init();

        if ( FileSystemWatchers::WacherFileDescriptor > 0 )
        {
            FileSystemWatchers::ThreadActive = true;
            FileSystemWatchers::Thread = ThreadSpawner::Start(FileSystemWatchers::Run, TEXT("File System Watchers"), ThreadPriority::BelowNormal);
        }
    }

    if ( FileSystemWatchers::WacherFileDescriptor > 0 )
    {
        char c_path[directory.Length() + 1];
        Platform::MemoryCopy(c_path, directory.ToStringAnsi().GetText(),directory.Length());
        c_path[directory.Length()]=0;

        LOG(Info, "Adding watcher for directory {}.", Directory);
        WachedDirectory = inotify_add_watch( FileSystemWatchers::WacherFileDescriptor, c_path, IN_MODIFY | IN_CREATE | IN_DELETE );
        
        FileSystemWatchers::Watchers[WachedDirectory] = Pair<int, Pair<String, LinuxFileSystemWatcher*>>(rootWatcher, Pair<String, LinuxFileSystemWatcher*>(directory, this));
        
        if (withSubDirs)
        {
            FileSystemWatchers::RootWatchers[WachedDirectory] = this;
            FileSystemWatchers::AddSubDirWatcher(WachedDirectory, directory);
        }
    }
    FileSystemWatchers::Locker.Unlock();
}

LinuxFileSystemWatcher::~LinuxFileSystemWatcher()
{
    if (FileSystemWatchers::WacherFileDescriptor > 0)
    {
        LOG(Info, "Removing watcher for directory {}.", Directory);
        inotify_rm_watch( FileSystemWatchers::WacherFileDescriptor, WachedDirectory );

        FileSystemWatchers::Locker.Lock();
        if (FileSystemWatchers::RootWatchers.ContainsKey(WachedDirectory))
        {
            FileSystemWatchers::RootWatchers.Remove(WachedDirectory);
        }
        FileSystemWatchers::Watchers.Remove(WachedDirectory);
        FileSystemWatchers::Locker.Unlock();
    }

    FileSystemWatchers::Locker.Lock();
    if (FileSystemWatchers::Watchers.IsEmpty() && FileSystemWatchers::Thread)
    {
        FileSystemWatchers::ThreadActive = false;
        FileSystemWatchers::Thread->Join();
        Delete(FileSystemWatchers::Thread);
        FileSystemWatchers::Thread = nullptr;
        close( FileSystemWatchers::WacherFileDescriptor );
    }
    FileSystemWatchers::Locker.Unlock();
}

#endif

