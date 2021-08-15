// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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

#define CATCH_CONFIG_RUNNER
#include <ThirdParty/catch2/catch.hpp>


int main(int argc, char* argv[])
{
#ifdef USE_VS_MEM_LEAKS_CHECK
    // Memory leaks detect inside VS
    int flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    flag |= _CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF;
    _CrtSetDbgFlag(flag);
#endif

#ifdef USE_VLD_MEM_LEAKS_CHECK
    VLDGlobalEnable();
#endif

    int result = Catch::Session().run(argc, argv);
    return result;
}

#endif
