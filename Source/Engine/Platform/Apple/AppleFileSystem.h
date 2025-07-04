// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_MAC || PLATFORM_IOS

#include "Engine/Platform/Unix/UnixFileSystem.h"

/// <summary>
/// Apple platform implementation of filesystem service.
/// </summary>
class FLAXENGINE_API AppleFileSystem : public UnixFileSystem
{
public:
    // [FileSystemBase]
    static bool CopyFile(const StringView& dst, const StringView& src);
    static void GetSpecialFolderPath(const SpecialFolder type, String& result);
};

#endif
