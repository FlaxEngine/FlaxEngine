// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config/Settings.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Content/Asset.h"
#include "Engine/Content/AssetReference.h"

/// <summary>
/// The game building rendering settings.
/// </summary>
class BuildSettings : public SettingsBase<BuildSettings>
{
public:

    /// <summary>
    /// The maximum amount of assets to include into a single assets package. Asset packages will split into several packages if need to.
    /// </summary>
    int32 MaxAssetsPerPackage = 1024;

    /// <summary>
    /// The maximum size of the single assets package (in megabytes). Asset packages will split into several packages if need to.
    /// </summary>
    int32 MaxPackageSizeMB = 1024;

    /// <summary>
    /// The game content cooking keycode. Use the same value for a game and DLC packages to support loading them by the build game. Use 0 to randomize it during building.
    /// </summary>
    int32 ContentKey = 0;

    /// <summary>
    /// If checked, the builds produced by the Game Cooker will be treated as for final game distribution (eg. for game store upload). Builds done this way cannot be tested on console devkits (eg. Xbox One, Xbox Scarlett).
    /// </summary>
    bool ForDistribution = false;

    /// <summary>
    /// If checked, the output build files won't be packaged for the destination platform. Useful when debugging build from local PC.
    /// </summary>
    bool SkipPackaging = false;

    /// <summary>
    /// The list of additional assets to include into build (into root assets set).
    /// </summary>
    Array<AssetReference<Asset>> AdditionalAssets;

    /// <summary>
    /// The list of additional folders with assets to include into build (into root assets set). Paths relative to the project directory (or absolute).
    /// </summary>
    Array<String> AdditionalAssetFolders;

    /// <summary>
    /// Disables shaders compiler optimizations in cooked game. Can be used to debug shaders on a target platform or to speed up the shaders compilation time.
    /// </summary>
    bool ShadersNoOptimize = false;

    /// <summary>
    /// Enables shader debug data generation for shaders in cooked game (depends on the target platform rendering backend).
    /// </summary>
    bool ShadersGenerateDebugData = false;

public:

    // [SettingsBase]
    void RestoreDefault() final override
    {
        MaxAssetsPerPackage = 1024;
        MaxPackageSizeMB = 1024;
        ContentKey = 0;
        ForDistribution = false;
        SkipPackaging = false;
        AdditionalAssets.Clear();
        AdditionalAssetFolders.Clear();
        ShadersNoOptimize = false;
        ShadersGenerateDebugData = false;
    }

    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) final override
    {
        DESERIALIZE(MaxAssetsPerPackage);
        DESERIALIZE(MaxPackageSizeMB);
        DESERIALIZE(ContentKey);
        DESERIALIZE(ForDistribution);
        DESERIALIZE(SkipPackaging);
        DESERIALIZE(AdditionalAssets);
        DESERIALIZE(AdditionalAssetFolders);
        DESERIALIZE(ShadersNoOptimize);
        DESERIALIZE(ShadersGenerateDebugData);
    }
};
