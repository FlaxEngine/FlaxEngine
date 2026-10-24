// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/File.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/Guid.h"
#include "Engine/Engine/Globals.h"
#include <ThirdParty/catch2/catch.hpp>

TEST_CASE("FileSystem")
{
    SECTION("Test AreFilePathsEqual")
    {
        const String folder = Globals::TemporaryFolder / TEXT("TestAreFilePathsEqual_") + Guid::New().ToString(Guid::FormatType::N);
        REQUIRE(!FileSystem::CreateDirectory(folder));
        const String missing1 = folder / TEXT("Missing1.txt");
        const String missing2 = folder / TEXT("Missing2.txt");

        // Two paths of files that do not exist: not equal, and the comparison does not create them
        CHECK(!FileSystem::AreFilePathsEqual(missing1, missing2));
        CHECK(!FileSystem::FileExists(missing1));
        CHECK(!FileSystem::FileExists(missing2));

        // An existing and a missing file are not equal
        const String existing = folder / TEXT("Existing.txt");
        REQUIRE(!File::WriteAllText(existing, TEXT("Test"), Encoding::ANSI));
        CHECK(!FileSystem::AreFilePathsEqual(existing, missing1));
#if PLATFORM_WINDOWS
        // The same existing file under two spellings of the separator is equal
        String otherSpelling = existing;
        otherSpelling.Replace('/', '\\');
        CHECK(FileSystem::AreFilePathsEqual(existing, otherSpelling));
#endif
        CHECK(!FileSystem::FileExists(missing1));

        FileSystem::DeleteDirectory(folder, true);
    }
}
