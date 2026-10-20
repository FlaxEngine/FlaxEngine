// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Platform/Platform.h"
#include "Engine/Platform/CreateProcessSettings.h"
#if PLATFORM_WINDOWS
#include "Engine/Platform/Win32/IncludeWindowsHeaders.h"
#endif
#include <ThirdParty/catch2/catch.hpp>

TEST_CASE("Platform")
{
#if PLATFORM_WINDOWS
    SECTION("Test CreateProcess with ShellExecute")
    {
        // Start processes through ShellExecute without waiting for them: no process handle must stay open
        CreateProcessSettings settings;
        settings.FileName = TEXT("cmd.exe");
        settings.Arguments = TEXT("/c exit 0");
        settings.ShellExecute = true;
        settings.WaitForEnd = false;
        settings.HiddenWindow = true;
        CHECK(Platform::CreateProcess(settings) == 0); // The first call also loads the shell, which opens its own handles
        constexpr int32 count = 50;
        DWORD handlesBefore = 0, handlesAfter = 0;
        GetProcessHandleCount(GetCurrentProcess(), &handlesBefore);
        for (int32 i = 0; i < count; i++)
        {
            CHECK(Platform::CreateProcess(settings) == 0);
        }
        GetProcessHandleCount(GetCurrentProcess(), &handlesAfter);
        CHECK(handlesAfter < handlesBefore + count / 2);
    }
#endif
}
