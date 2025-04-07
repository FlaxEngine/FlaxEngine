// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_IOS

#include "Engine/Platform/Unix/UnixFile.h"

/// <summary>
/// iOS platform file object implementation.
/// </summary>
class FLAXENGINE_API iOSFile : public UnixFile
{
public:
    iOSFile(int32 handle)
        : UnixFile(handle)
    {
    }

    static iOSFile* Open(const StringView& path, FileMode mode, FileAccess access = FileAccess::ReadWrite, FileShare share = FileShare::None);
};

#endif
