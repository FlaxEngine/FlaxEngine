// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_UWP

#include "UWPFileSystem.h"
#include "Engine/Core/Types/String.h"

void UWPFileSystem::GetSpecialFolderPath(const SpecialFolder type, String& result)
{
    Char buffer[260];
    int length = CUWPPlatform->GetSpecialFolderPath((UWPPlatformImpl::SpecialFolder)type, buffer, 260);

    result = buffer;
    NormalizePath(result);
}

#endif
