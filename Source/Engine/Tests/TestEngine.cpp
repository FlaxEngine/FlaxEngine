// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Engine/Engine.h"
#include <ThirdParty/catch2/catch.hpp>

TEST_CASE("Engine")
{
    SECTION("Test ForcedGCInterval")
    {
        // The default keeps the previous fixed interval of 60 update ticks
        CHECK(Engine::ForcedGCInterval == 60);

        // The interval can be changed at run time (0 disables the forced collections)
        const int32 interval = Engine::ForcedGCInterval;
        Engine::ForcedGCInterval = 0;
        CHECK(Engine::ForcedGCInterval == 0);
        Engine::ForcedGCInterval = 120;
        CHECK(Engine::ForcedGCInterval == 120);
        Engine::ForcedGCInterval = interval;
    }
}
