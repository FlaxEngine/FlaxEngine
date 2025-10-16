// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_LINUX || PLATFORM_MAC || PLATFORM_IOS

#include "Engine/Engine/Engine.h"
#include "Engine/Main/MainUtil.h"

int main(int argc, char* argv[])
{
    return Engine::Main(GetCommandLine(argc, argv));
}

#endif
