// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_UWP

#include "Engine/Platform/Win32/Win32FileSystem.h"

/// <summary>
/// Universal Windows Platform (UWP) implementation of filesystem service
/// </summary>
class FLAXENGINE_API UWPFileSystem : public Win32FileSystem
{
public:
    // [FileSystemBase]
    static void GetSpecialFolderPath(const SpecialFolder type, String& result);
};

#endif
