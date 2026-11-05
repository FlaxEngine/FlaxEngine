// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Particles/Particles.h"
#include <ThirdParty/catch2/catch.hpp>

TEST_CASE("Particles")
{
    SECTION("Test WriteRibbonSegmentIndices")
    {
        // The first segment: vertices 0 to 3
        Particles::RibbonIndex indices[6] = {};
        Particles::WriteRibbonSegmentIndices(indices, 0);
        CHECK(indices[0] == 0);
        CHECK(indices[1] == 1);
        CHECK(indices[2] == 2);
        CHECK(indices[3] == 1);
        CHECK(indices[4] == 3);
        CHECK(indices[5] == 2);

        // Segment 32,767 of a long ribbon: its vertices go past 65,535 (16-bit indices wrap to 0 and 1, and the triangles
        // go back to the start of the ribbon)
        Particles::WriteRibbonSegmentIndices(indices, 2 * 32767);
        CHECK(indices[0] == 65534);
        CHECK(indices[1] == 65535);
        CHECK(indices[2] == 65536);
        CHECK(indices[3] == 65535);
        CHECK(indices[4] == 65537);
        CHECK(indices[5] == 65536);
    }
}
