// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_UNIX

#include "Engine/Platform/Base/PlatformBase.h"

/// <summary>
/// The Unix platform implementation and application management utilities.
/// </summary>
class FLAXENGINE_API UnixPlatform : public PlatformBase
{
public:
    
    // [PlatformBase]
    static void* Allocate(uint64 size, uint64 alignment);
    static void Free(void* ptr);
    static void* AllocatePages(uint64 numPages, uint64 pageSize);
    static void FreePages(void* ptr);
    static uint64 GetCurrentProcessId();
};

#endif
