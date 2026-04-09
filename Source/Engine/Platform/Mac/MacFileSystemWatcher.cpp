// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_MAC

#include "MacFileSystemWatcher.h"
#include "Engine/Platform/Apple/AppleUtils.h"
#include "Engine/Platform/CriticalSection.h"
#include "Engine/Platform/Thread.h"
#include "Engine/Threading/ThreadSpawner.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Types/StringView.h"

void DirectoryWatchCallback( ConstFSEventStreamRef StreamRef, void* FileWatcherPtr, size_t EventCount, void* EventPaths, const FSEventStreamEventFlags EventFlags[], const FSEventStreamEventId EventIDs[])
{
    MacFileSystemWatcher* macFileSystemWatcher = (MacFileSystemWatcher*)FileWatcherPtr;
    if (macFileSystemWatcher)
    {
        CFArrayRef EventPathArray = (CFArrayRef)EventPaths;
        for (size_t eventIndex = 0; eventIndex < EventCount; eventIndex++)
        {
            const FSEventStreamEventFlags flags = EventFlags[eventIndex];
            if (!(flags & kFSEventStreamEventFlagItemIsFile) && !(flags & kFSEventStreamEventFlagItemIsDir))
            {
                // Events about symlinks don't concern us
                continue;
            }

            auto action = FileSystemAction::Unknown;
            if (flags & kFSEventStreamEventFlagItemCreated)
                action = FileSystemAction::Create;
            if (flags & kFSEventStreamEventFlagItemRenamed)
                action = FileSystemAction::Rename;
            if (flags & kFSEventStreamEventFlagItemModified)
                action = FileSystemAction::Modify;
            if (flags & kFSEventStreamEventFlagItemRemoved)
                action = FileSystemAction::Delete;

            const String resolvedPath = AppleUtils::ToString((CFStringRef)CFArrayGetValueAtIndex(EventPathArray, eventIndex));
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

    _eventStream = FSEventStreamCreate(NULL,
        &DirectoryWatchCallback,
        &Context,
        PathsToWatch,
        kFSEventStreamEventIdSinceNow,
        Latency,
        kFSEventStreamCreateFlagUseCFTypes | kFSEventStreamCreateFlagNoDefer | kFSEventStreamCreateFlagFileEvents
    );
    
    CFRelease(PathsToWatch);
    CFRelease(FullPathMac);

    _queue = dispatch_queue_create("MacFileSystemWatcher", NULL);
	FSEventStreamSetDispatchQueue(_eventStream, _queue);
    FSEventStreamStart(_eventStream);
}

MacFileSystemWatcher::~MacFileSystemWatcher()
{
    if (_eventStream)
    {
        FSEventStreamStop(_eventStream);
        FSEventStreamSetDispatchQueue(_eventStream, nullptr);
        FSEventStreamInvalidate(_eventStream);
        FSEventStreamRelease(_eventStream);
        _eventStream = nullptr;
    }
    if (_queue != nullptr)
    {
        dispatch_release(_queue);
        _queue = nullptr;
    }
}

#endif
