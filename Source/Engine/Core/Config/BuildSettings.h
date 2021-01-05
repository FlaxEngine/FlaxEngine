// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config/Settings.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Content/Asset.h"
#include "Engine/Content/AssetReference.h"

/// <summary>
/// The game building rendering settings.
/// </summary>
API_CLASS(sealed, Namespace="FlaxEditor.Content.Settings") class FLAXENGINE_API BuildSettings : public SettingsBase
{
DECLARE_SCRIPTING_TYPE_MINIMAL(BuildSettings);
public:

    /// <summary>
    /// The maximum amount of assets to include into a single assets package. Asset packages will split into several packages if need to.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10), DefaultValue(4096), Limit(1, 32, ushort.MaxValue), EditorDisplay(\"General\", \"Max assets per package\")")
    int32 MaxAssetsPerPackage = 4096;

    /// <summary>
    /// The maximum size of the single assets package (in megabytes). Asset packages will split into several packages if need to.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(20), DefaultValue(1024), Limit(1, 16, ushort.MaxValue), EditorDisplay(\"General\", \"Max package size (in MB)\")")
    int32 MaxPackageSizeMB = 1024;

    /// <summary>
    /// The game content cooking keycode. Use the same value for a game and DLC packages to support loading them by the build game. Use 0 to randomize it during building.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(30), DefaultValue(0), Limit(1, 16, ushort.MaxValue), EditorDisplay(\"General\")")
    int32 ContentKey = 0;

    /// <summary>
    /// If checked, the builds produced by the Game Cooker will be treated as for final game distribution (eg. for game store upload). Builds done this way cannot be tested on console devkits (eg. Xbox One, Xbox Scarlett).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(40), DefaultValue(false), EditorDisplay(\"General\")")
    bool ForDistribution = false;

    /// <summary>
    /// If checked, the output build files won't be packaged for the destination platform. Useful when debugging build from local PC.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(50), DefaultValue(false), EditorDisplay(\"General\")")
    bool SkipPackaging = false;

    /// <summary>
    /// The list of additional assets to include into build (into root assets set).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1000), EditorDisplay(\"Additional Data\")")
    Array<AssetReference<Asset>> AdditionalAssets;

    /// <summary>
    /// The list of additional folders with assets to include into build (into root assets set). Paths relative to the project directory (or absolute).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1010), EditorDisplay(\"Additional Data\")")
    Array<String> AdditionalAssetFolders;

    /// <summary>
    /// Disables shaders compiler optimizations in cooked game. Can be used to debug shaders on a target platform or to speed up the shaders compilation time.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(2000), DefaultValue(false), EditorDisplay(\"Content\", \"Shaders No Optimize\")")
    bool ShadersNoOptimize = false;

    /// <summary>
    /// Enables shader debug data generation for shaders in cooked game (depends on the target platform rendering backend).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(2010), DefaultValue(false), EditorDisplay(\"Content\")")
    bool ShadersGenerateDebugData = false;

public:

    /// <summary>
    /// Gets the instance of the settings asset (default value if missing). Object returned by this method is always loaded with valid data to use.
    /// </summary>
    static BuildSettings* Get();

    // [SettingsBase]
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
