// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WEB

#include "Engine/Platform/Unix/UnixFileSystem.h"

/// <summary>
/// Web platform implementation of filesystem service.
/// </summary>
class FLAXENGINE_API WebFileSystem : public UnixFileSystem
{
public:
    // [UnixFileSystem]
    static void GetSpecialFolderPath(const SpecialFolder type, String& result);
};

#endif
