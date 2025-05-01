// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_UWP && !USE_EDITOR

#include "UWPGame.h"
#include "Engine/Platform/UWP/UWPPlatformImpl.h"

void UWPGame::InitMainWindowSettings(CreateWindowSettings& settings)
{
    // Link UWP window implementation (Flax has no access to modify window settings here)
    settings.Data = CUWPPlatform->GetMainWindowImpl();
}

#endif
