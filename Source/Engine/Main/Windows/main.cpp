// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_WINDOWS

#ifdef USE_VLD_MEM_LEAKS_CHECK
#include <vld.h>
#endif
#ifdef USE_VS_MEM_LEAKS_CHECK
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#else
#include <stdlib.h>
#endif
#include "Engine/Engine/Engine.h"
#include "Engine/Platform/Win32/IncludeWindowsHeaders.h"

// Favor the high performance NVIDIA GPU if there are multiple GPUs
extern "C" {
_declspec(dllexport) uint32 NvOptimusEnablement = 0x00000001;
}

// Favor the high performance AMD GPU if there are multiple GPUs
extern "C" {
__declspec(dllexport) int32 AmdPowerXpressRequestHighPerformance = 1;
}

#if FLAX_TESTS
int main(int argc, char* argv[])
#else
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
#endif
{
#if FLAX_TESTS
    HINSTANCE hInstance = GetModuleHandle(NULL);
    LPTSTR lpCmdLine = GetCommandLineW();
#endif

#ifdef USE_VS_MEM_LEAKS_CHECK
    // Memory leaks detect inside VS
    int flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    flag |= _CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF;
    _CrtSetDbgFlag(flag);
#endif

#ifdef USE_VLD_MEM_LEAKS_CHECK
    VLDGlobalEnable();
#endif

    Platform::PreInit(hInstance);
    __try
    {
        return Engine::Main(lpCmdLine);
    }
    __except (Platform::SehExceptionHandler(GetExceptionInformation()))
    {
        return -1;
    }
}

#endif
