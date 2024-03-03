// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_UWP

#include "Engine/Platform/Win32/Win32FileSystem.h"

/// <summary>
/// Universal Windows Platform (UWP) implementation of filesystem service
/// </summary>
class FLAXENGINE_API UWPFileSystem : public Win32FileSystem
{
public:

    /// <summary>
    /// Gets the special folder path.
    /// </summary>
    /// <param name="type">The folder type.</param>
    /// <param name="result">The result full path.</param>
    static void GetSpecialFolderPath(const SpecialFolder type, String& result);
};

#endif
