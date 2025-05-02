// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_MAC

#include "MacFileSystem.h"
#include "Engine/Platform/Apple/AppleUtils.h"
#include "Engine/Platform/File.h"
#include "Engine/Platform/Platform.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/StringView.h"
#include "Engine/Core/Types/TimeSpan.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Log.h"
#include "Engine/Utilities/StringConverter.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <cerrno>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <AppKit/AppKit.h>

void InitMacDialog(NSSavePanel* dialog, const StringView& initialDirectory, const StringView& filter, const StringView& title)
{
    if (initialDirectory.HasChars())
    {
        [dialog setDirectoryURL:[NSURL fileURLWithPath:(NSString*)AppleUtils::ToString(initialDirectory) isDirectory:YES]];
    }
    if (filter.HasChars())
    {
        // TODO: finish file tpye filter support
        /*NSMutableArray* fileTypes = [[NSMutableArray alloc] init];
        Array<String> entries;
        String(filter).Split('\0', entries);
        for (int32 i = 1; i < entries.Count(); i += 2)
        {
            String extension = entries[i];
            if (extension.StartsWith(TEXT("*.")))
                extension = extension.Substring(2);
            [fileTypes addObject:(NSString*)AppleUtils::ToString(extension)];
        }
        [dialog setAllowedFileTypes:fileTypes];*/
    }
    if (title.HasChars())
    {
        [dialog setMessage:(NSString*)AppleUtils::ToString(title)];
    }
}

bool MacFileSystem::ShowOpenFileDialog(Window* parentWindow, const StringView& initialDirectory, const StringView& filter, bool multiSelect, const StringView& title, Array<String>& filenames)
{
    bool result = true;
    @autoreleasepool {
        NSWindow* focusedWindow = [[NSApplication sharedApplication] keyWindow];

        NSOpenPanel* dialog = [NSOpenPanel openPanel];
        [dialog setCanChooseFiles:YES];
        [dialog setCanChooseDirectories:NO];
        [dialog setAllowsMultipleSelection:multiSelect ? YES : NO];
        InitMacDialog(dialog, initialDirectory, filter, title);

        if ([dialog runModal] == NSModalResponseOK)
        {
            if (multiSelect)
            {
                const NSArray* urls = [dialog URLs];
                for (int32 i = 0; i < [urls count]; i++)
                    filenames.Add(AppleUtils::ToString((CFStringRef)[[urls objectAtIndex:i] path]));
            }
            else
            {
                const NSURL* url = [dialog URL];
                filenames.Add(AppleUtils::ToString((CFStringRef)[url path]));
            }
            result = false;
        }

        [focusedWindow makeKeyAndOrderFront:nil];
    }
    return result;
}

bool MacFileSystem::ShowSaveFileDialog(Window* parentWindow, const StringView& initialDirectory, const StringView& filter, bool multiSelect, const StringView& title, Array<String>& filenames)
{
    bool result = true;
    @autoreleasepool {
        NSWindow* focusedWindow = [[NSApplication sharedApplication] keyWindow];

        NSSavePanel* dialog = [NSSavePanel savePanel];
        [dialog setExtensionHidden:NO];
        InitMacDialog(dialog, initialDirectory, filter, title);

        if ([dialog runModal] == NSModalResponseOK)
        {
            const NSURL* url = [dialog URL];
            filenames.Add(AppleUtils::ToString((CFStringRef)[url path]));
            result = false;
        }

        [focusedWindow makeKeyAndOrderFront:nil];
    }
    return result;
}

bool MacFileSystem::ShowBrowseFolderDialog(Window* parentWindow, const StringView& initialDirectory, const StringView& title, String& path)
{
    bool result = true;
    @autoreleasepool {
        NSWindow* focusedWindow = [[NSApplication sharedApplication] keyWindow];

        NSOpenPanel* dialog = [NSOpenPanel openPanel];
        [dialog setCanChooseFiles:NO];
        [dialog setCanChooseDirectories:YES];
        [dialog setCanCreateDirectories:YES];
        [dialog setAllowsMultipleSelection:NO];
        InitMacDialog(dialog, initialDirectory, StringView::Empty, title);

        if ([dialog runModal] == NSModalResponseOK)
        {
            const NSURL* url = [dialog URL];
            path = AppleUtils::ToString((CFStringRef)[url path]);
            result = false;
        }

        [focusedWindow makeKeyAndOrderFront:nil];
    }
    return result;
}

bool MacFileSystem::ShowFileExplorer(const StringView& path)
{
    return [[NSWorkspace sharedWorkspace] selectFile: AppleUtils::ToNSString(FileSystem::ConvertRelativePathToAbsolute(path)) inFileViewerRootedAtPath: @""];
}

#endif
