// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "Engine/Core/Math/Vector3.h"
#include "Engine/Level/LargeWorlds.h"
#include <ThirdParty/catch2/catch.hpp>

TEST_CASE("LargeWorlds")
{
    SECTION("UpdateOrigin")
    {
        LargeWorlds::Enable = true;
        Vector3 origin = Vector3::Zero;
        LargeWorlds::UpdateOrigin(origin, Vector3::Zero);
        CHECK(origin == Vector3::Zero);
        LargeWorlds::UpdateOrigin(origin, Vector3(LargeWorlds::ChunkSize * 0.5, LargeWorlds::ChunkSize * 1.0001, LargeWorlds::ChunkSize * 1.5));
        CHECK(origin == Vector3(0, 0, LargeWorlds::ChunkSize * 1));
    }
}
