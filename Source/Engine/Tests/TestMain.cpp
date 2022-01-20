// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#if PLATFORM_WINDOWS || PLATFORM_LINUX || PLATFORM_MAC

#define CATCH_CONFIG_RUNNER
#include <ThirdParty/catch2/catch.hpp>

int main(int argc, char* argv[])
{
    int result = Catch::Session().run(argc, argv);
    return result;
}

#endif
