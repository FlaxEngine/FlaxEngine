// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Content/Content.h"
#include "Engine/Content/Storage/ContentStorageManager.h"
#include "Engine/Engine/Globals.h"
#if COMPILE_WITH_ASSETS_IMPORTER
#include "Engine/ContentImporters/AssetsImportingManager.h"
#endif
#include <ThirdParty/catch2/catch.hpp>

#if COMPILE_WITH_ASSETS_IMPORTER

TEST_CASE("Content")
{
    SECTION("Test asset path spelling")
    {
        // A binary asset at a path in the canonical format (FormatPath)
        String path = Globals::TemporaryFolder / TEXT("TestAssetPathSpelling.flax");
        ContentStorageManager::FormatPath(path);
        Guid id = Guid::New();
        REQUIRE(!AssetsImportingManager::Create(AssetsImportingManager::CreateMaterialInstanceTag, path, id));
        AssetInfo info;
        REQUIRE(Content::GetAssetInfo(path, info));
        CHECK(info.ID == id);

#if PLATFORM_WINDOWS
        // Other spellings of the same file: one asset, one storage, and the file keeps its ID
        String slashes = path;
        slashes.Replace('\\', '/');
        String backslashes = path;
        backslashes.Replace('/', '\\');
        CHECK(Content::GetAssetInfo(slashes, info));
        CHECK(info.ID == id);
        CHECK(Content::GetAssetInfo(backslashes, info));
        CHECK(info.ID == id);
        CHECK(ContentStorageManager::GetStorage(slashes).Get() == ContentStorageManager::GetStorage(path).Get());
        REQUIRE(Content::GetAssetInfo(path, info));
        CHECK(info.ID == id);
#endif

        Content::DeleteAsset(path);
    }
}

#endif
