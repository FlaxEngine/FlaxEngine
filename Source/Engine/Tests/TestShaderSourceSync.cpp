// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Core/ScopeExit.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Platform/File.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/ShadersCompilation/ShadersCompilation.h"
#include <ThirdParty/catch2/catch.hpp>

#if COMPILE_WITH_SHADER_COMPILER && USE_EDITOR

TEST_CASE("Shader source asset synchronization ignores timestamps")
{
    const String sourcePath = Globals::StartupFolder / TEXT("Source/Shaders/VolumetricFog.shader");
    const String assetPath = Globals::EngineContentFolder / TEXT("Shaders/VolumetricFog.flax");
    REQUIRE(FileSystem::FileExists(sourcePath));
    REQUIRE(FileSystem::FileExists(assetPath));
    CHECK(ShadersCompilation::IsShaderSourceAssetUpToDate(sourcePath, assetPath));

    const String tempRoot = Globals::TemporaryFolder / (TEXT("ShaderSourceSync-") + Guid::New().ToString(Guid::FormatType::N));
    REQUIRE(!FileSystem::CreateDirectory(tempRoot));
    SCOPE_EXIT
    {
        FileSystem::DeleteDirectory(tempRoot, true);
    };

    DataContainer<byte> assetData;
    StringAnsi modifiedSource;
    REQUIRE(!File::ReadAllBytes(assetPath, assetData));
    REQUIRE(!File::ReadAllText(sourcePath, modifiedSource));
    modifiedSource.Append("// Deliberately different source\n");

    const String tempSourcePath = tempRoot / TEXT("VolumetricFog.shader");
    const String tempAssetPath = tempRoot / TEXT("VolumetricFog.flax");
    REQUIRE(!File::WriteAllBytes(tempSourcePath, modifiedSource.Get(), modifiedSource.Length()));
    REQUIRE(!File::WriteAllBytes(tempAssetPath, assetData.Get(), assetData.Length()));
    CHECK_FALSE(FileSystem::GetFileLastEditTime(tempSourcePath) > FileSystem::GetFileLastEditTime(tempAssetPath));
    CHECK_FALSE(ShadersCompilation::IsShaderSourceAssetUpToDate(tempSourcePath, tempAssetPath));
}

#endif
