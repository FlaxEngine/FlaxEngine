// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Tools/ModelTool/ModelTool.h"
#include <ThirdParty/catch2/catch.hpp>

TEST_CASE("ModelTool")
{
    SECTION("Test DetectLodIndex")
    {
        CHECK(ModelTool::DetectLodIndex(TEXT("mesh")) == 0);
        CHECK(ModelTool::DetectLodIndex(TEXT("mesh LOD")) == 0);
        CHECK(ModelTool::DetectLodIndex(TEXT("mesh LOD0")) == 0);
        CHECK(ModelTool::DetectLodIndex(TEXT("mesh LOD1")) == 1);
        CHECK(ModelTool::DetectLodIndex(TEXT("mesh_LOD1")) == 1);
        CHECK(ModelTool::DetectLodIndex(TEXT("mesh_lod1")) == 1);
        CHECK(ModelTool::DetectLodIndex(TEXT("mesh_lod2")) == 2);
        CHECK(ModelTool::DetectLodIndex(TEXT("lod0")) == 0);
        CHECK(ModelTool::DetectLodIndex(TEXT("lod1")) == 1);
        CHECK(ModelTool::DetectLodIndex(TEXT("lod_2")) == 2);
        CHECK(ModelTool::DetectLodIndex(TEXT("mesh_lod_0")) == 0);
        CHECK(ModelTool::DetectLodIndex(TEXT("mesh_lod_1")) == 1);
        CHECK(ModelTool::DetectLodIndex(TEXT("mesh lod_2")) == 2);
    }
}
