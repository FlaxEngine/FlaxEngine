// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_MAC

#include "../Apple/AppleFileSystem.h"

/// <summary>
/// Mac platform implementation of filesystem service.
/// </summary>
class FLAXENGINE_API MacFileSystem : public AppleFileSystem
{
public:
    // [AppleFileSystem]
    static bool ShowOpenFileDialog(Window* parentWindow, const StringView& initialDirectory, const StringView& filter, bool multiSelect, const StringView& title, Array<String, HeapAllocation>& filenames);
    static bool ShowSaveFileDialog(Window* parentWindow, const StringView& initialDirectory, const StringView& filter, bool multiSelect, const StringView& title, Array<String, HeapAllocation>& filenames);
    static bool ShowBrowseFolderDialog(Window* parentWindow, const StringView& initialDirectory, const StringView& title, String& path);
    static bool ShowFileExplorer(const StringView& path);
};

#endif
