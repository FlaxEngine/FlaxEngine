// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Content/Cache/AssetsCache.h"
#include "Engine/Core/Types/StringView.h"
#include <ThirdParty/catch2/catch.hpp>

TEST_CASE("AssetsCache")
{
    SECTION("Test IsSamePath")
    {
        CHECK(AssetsCache::IsSamePath(TEXT("C:/Project/Content/A.flax"), TEXT("C:/Project/Content/A.flax")));
        CHECK(!AssetsCache::IsSamePath(TEXT("C:/Project/Content/A.flax"), TEXT("C:/Project/Content/B.flax")));
        CHECK(!AssetsCache::IsSamePath(TEXT("C:/Project/Content/A.flax"), TEXT("C:/Project/Content/A.flax2")));
#if PLATFORM_WINDOWS
        // On Windows, the letter case and the separator do not change the file. Before the fix, the two paths with
        // mixed separators compared as different files, so the registry gave the asset a new ID.
        CHECK(AssetsCache::IsSamePath(TEXT("c:/Project/Content/A.flax"), TEXT("C:/Project/Content/A.flax")));
        CHECK(AssetsCache::IsSamePath(TEXT("C:/Project/Content/A.flax"), TEXT("C:\\Project/Content/A.flax")));
        CHECK(AssetsCache::IsSamePath(TEXT("C:\\Project\\Content\\A.flax"), TEXT("C:/Project/Content/A.flax")));
        CHECK(!AssetsCache::IsSamePath(TEXT("C:\\Project\\Content\\A.flax"), TEXT("C:/Project/Content/B.flax")));
#endif
    }
}
