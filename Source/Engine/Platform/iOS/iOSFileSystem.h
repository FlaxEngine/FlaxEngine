// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_IOS

#include "Engine/Platform/Apple/AppleFileSystem.h"

/// <summary>
/// iOS platform implementation of filesystem service.
/// </summary>
class FLAXENGINE_API iOSFileSystem : public AppleFileSystem
{
public:
    // [AppleFileSystem]
    static bool FileExists(const StringView& path);
    static uint64 GetFileSize(const StringView& path);
    static bool IsReadOnly(const StringView& path);
};

#endif
