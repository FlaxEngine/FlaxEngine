// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Renderer/GI/GlobalSurfaceAtlasPass.h"
#include <ThirdParty/catch2/catch.hpp>

TEST_CASE("GlobalSurfaceAtlasPass")
{
    SECTION("Test GetTileResolution")
    {
        // 10 texels per world unit
        CHECK(GlobalSurfaceAtlasPass::GetTileResolution(1.0f, 10.0f) == 10);
        CHECK(GlobalSurfaceAtlasPass::GetTileResolution(0.2f, 10.0f) < 4); // Too small: skipped

        // Sides above 6,553.5 units (eg. terrains, sea planes) exceed the uint16 range: they get the largest resolution
        // (a cast before the clamp gave 0, so the side was skipped as too small, or a wrapped size)
        CHECK(GlobalSurfaceAtlasPass::GetTileResolution(6553.6f, 10.0f) == MAX_uint16);
        CHECK(GlobalSurfaceAtlasPass::GetTileResolution(100000.0f, 10.0f) == MAX_uint16);
        CHECK(GlobalSurfaceAtlasPass::GetTileResolution(160000.0f, 10.0f) == MAX_uint16);
    }
}
