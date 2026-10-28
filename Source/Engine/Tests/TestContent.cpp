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
        // Other spellings of the same file: one asset, and the file keeps its ID
        String slashes = path;
        slashes.Replace('\\', '/');
        String backslashes = path;
        backslashes.Replace('/', '\\');
        CHECK(Content::GetAssetInfo(slashes, info));
        CHECK(info.ID == id);
        CHECK(Content::GetAssetInfo(backslashes, info));
        CHECK(info.ID == id);
        REQUIRE(Content::GetAssetInfo(path, info));
        CHECK(info.ID == id);
#endif

        Content::DeleteAsset(path);
    }

    SECTION("Test formatted path check")
    {
        // IsFormattedPath lets a path skip FormatPath, so it must never be true for a path that FormatPath changes
        String project = Globals::ProjectFolder / TEXT("Content/Materials/Base.flax");
        ContentStorageManager::FormatPath(project);
        CHECK(ContentStorageManager::IsFormattedPath(project));
        const String paths[] =
        {
            project,
            String(TEXT("Content/A.flax")),
            String(TEXT("./Content/A.flax")),
            String(TEXT("/home/user/Project/Content/A.flax")),
            String(TEXT("/home/user/Project//Content/A.flax")),
            String(TEXT("/home/user/Project/Content/../A.flax")),
            String(TEXT("/home/user/Project/./Content/A.flax")),
            String(TEXT("/home/user/Project/Content/")),
            String(TEXT("C:\\Project/Content/A.flax")),
            String(TEXT("C:/Project/Content/A.flax")),
            String(TEXT("C:\\Project\\Content\\A.flax")),
            String(TEXT("C:\\Project/Content/.hidden/A.flax")),
            String(TEXT("C:\\Project/Content/..A.flax")),
            String(TEXT("\\\\server/share/A.flax")),
            String(TEXT("C:\\")),
        };
        for (const String& path : paths)
        {
            String formatted = path;
            ContentStorageManager::FormatPath(formatted);
            CAPTURE(path, formatted);
            if (ContentStorageManager::IsFormattedPath(path))
                CHECK(formatted == path);
        }
        CHECK(ContentStorageManager::IsFormattedPath(String(TEXT("/home/user/Project/Content/A.flax"))));
        CHECK(ContentStorageManager::IsFormattedPath(String(TEXT("C:\\Project/Content/A.flax"))));
        CHECK(!ContentStorageManager::IsFormattedPath(String(TEXT("C:/Project/Content/A.flax"))));
        CHECK(!ContentStorageManager::IsFormattedPath(String(TEXT("C:\\Project\\Content\\A.flax"))));
        CHECK(!ContentStorageManager::IsFormattedPath(String(TEXT("Content/A.flax"))));
    }
}

#endif
