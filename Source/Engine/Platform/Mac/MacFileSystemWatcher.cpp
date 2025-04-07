// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_MAC
#include "MacFileSystemWatcher.h"
#include "Engine/Platform/Apple/AppleUtils.h"
#include "Engine/Platform/CriticalSection.h"
#include "Engine/Platform/Thread.h"
#include "Engine/Threading/ThreadSpawner.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Types/StringView.h"

void DirectoryWatchCallback( ConstFSEventStreamRef StreamRef, void* FileWatcherPtr, size_t EventCount, void* EventPaths, const FSEventStreamEventFlags EventFlags[], const FSEventStreamEventId EventIDs[] )
{
    MacFileSystemWatcher* macFileSystemWatcher = (MacFileSystemWatcher*)FileWatcherPtr;
    if (macFileSystemWatcher)
    {
        CFArrayRef EventPathArray = (CFArrayRef)EventPaths;
        for( size_t EventIndex = 0; EventIndex < EventCount; ++EventIndex )
        {
            const FSEventStreamEventFlags Flags = EventFlags[EventIndex];
            if( !(Flags & kFSEventStreamEventFlagItemIsFile) && !(Flags & kFSEventStreamEventFlagItemIsDir) )
            {
                // events about symlinks don't concern us
                continue;
            }
            
            auto action = FileSystemAction::Unknown;
            
            const bool added = ( Flags & kFSEventStreamEventFlagItemCreated );
            const bool renamed = ( Flags & kFSEventStreamEventFlagItemRenamed );
            const bool modified = ( Flags & kFSEventStreamEventFlagItemModified );
            const bool removed = ( Flags & kFSEventStreamEventFlagItemRemoved );
            
            if (added)
            {
                action = FileSystemAction::Create;
            }
            if (renamed)
            {
                action = FileSystemAction::Rename;
            }
            if (modified)
            {
                action = FileSystemAction::Modify;
            }
            if (removed)
            {
                action = FileSystemAction::Delete;
            }
            
            const String resolvedPath = AppleUtils::ToString((CFStringRef)CFArrayGetValueAtIndex(EventPathArray,EventIndex));
            
            macFileSystemWatcher->OnEvent(resolvedPath, action);
        }
    }
}

MacFileSystemWatcher::MacFileSystemWatcher(const String& directory, bool withSubDirs)
    : FileSystemWatcherBase(directory, withSubDirs)
{
        
    CFStringRef FullPathMac = AppleUtils::ToString(StringView(directory));
    CFArrayRef PathsToWatch = CFArrayCreate(NULL, (const void**)&FullPathMac, 1, NULL);
    
    CFAbsoluteTime Latency = 0.2;
    
    FSEventStreamContext Context;
    Context.version = 0;
    Context.info = this;
    Context.retain = NULL;
    Context.release = NULL;
    Context.copyDescription = NULL;

    EventStream = FSEventStreamCreate( NULL,
        &DirectoryWatchCallback,
        &Context,
        PathsToWatch,
        kFSEventStreamEventIdSinceNow,
        Latency,
        kFSEventStreamCreateFlagUseCFTypes | kFSEventStreamCreateFlagNoDefer | kFSEventStreamCreateFlagFileEvents
    );
    
    CFRelease(PathsToWatch);
    CFRelease(FullPathMac);
    
    FSEventStreamScheduleWithRunLoop( EventStream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode );
    FSEventStreamStart( EventStream );
    
    IsRunning = true;
}

MacFileSystemWatcher::~MacFileSystemWatcher()
{
    if (IsRunning)
    {
        FSEventStreamStop(EventStream);
        FSEventStreamUnscheduleFromRunLoop(EventStream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        FSEventStreamInvalidate(EventStream);
        FSEventStreamRelease(EventStream);
    }
}
#endif
