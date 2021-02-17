// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Log.h"
#include "Engine/Core/Enums.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Collections/HashSet.h"
#include "Engine/Core/Collections/Dictionary.h"

class GameCooker;
class PlatformTools;

/// <summary>
/// Game building options. Used as flags.
/// </summary>
API_ENUM(Attributes="Flags") enum class BuildOptions
{
    /// <summary>
    /// No special options declared.
    /// </summary>
    None = 0,

    /// <summary>
    /// Shows the output directory folder on building end.
    /// </summary>
    ShowOutput = 1 << 0,
};

DECLARE_ENUM_OPERATORS(BuildOptions);

/// <summary>
/// Game build target platform.
/// </summary>
API_ENUM() enum class BuildPlatform
{
    /// <summary>
    /// Windows (32-bit architecture)
    /// </summary>
    API_ENUM(Attributes="EditorDisplay(null, \"Windows 32bit\")")
    Windows32 = 1,

    /// <summary>
    /// Windows (64-bit architecture)
    /// </summary>
    API_ENUM(Attributes="EditorDisplay(null, \"Windows 64bit\")")
    Windows64 = 2,

    /// <summary>
    /// Universal Windows Platform (UWP) (x86 architecture)
    /// </summary>
    API_ENUM(Attributes="EditorDisplay(null, \"Windows Store x86\")")
    UWPx86 = 3,

    /// <summary>
    /// Universal Windows Platform (UWP) (x64 architecture)
    /// </summary>
    API_ENUM(Attributes="EditorDisplay(null, \"Windows Store x64\")")
    UWPx64 = 4,

    /// <summary>
    /// Xbox One
    /// </summary>
    API_ENUM(Attributes="EditorDisplay(null, \"Xbox One\")")
    XboxOne = 5,

    /// <summary>
    /// Linux (64-bit architecture)
    /// </summary>
    API_ENUM(Attributes="EditorDisplay(null, \"Linux x64\")")
    LinuxX64 = 6,

    /// <summary>
    /// PlayStation 4
    /// </summary>
    API_ENUM(Attributes="EditorDisplay(null, \"PlayStation 4\")")
    PS4 = 7,

    /// <summary>
    /// Xbox Series X.
    /// </summary>
    API_ENUM(Attributes="EditorDisplay(null, \"Xbox Scarlett\")")
    XboxScarlett = 8,

    /// <summary>
    /// Android ARM64 (arm64-v8a).
    /// </summary>
    API_ENUM(Attributes="EditorDisplay(null, \"Android ARM64 (arm64-v8a)\")")
    AndroidARM64 = 9,
};

inline const Char* ToString(const BuildPlatform platform)
{
    switch (platform)
    {
    case BuildPlatform::Windows32:
        return TEXT("Windows x86");
    case BuildPlatform::Windows64:
        return TEXT("Windows x64");
    case BuildPlatform::UWPx86:
        return TEXT("Windows Store x86");
    case BuildPlatform::UWPx64:
        return TEXT("Windows Store x64");
    case BuildPlatform::XboxOne:
        return TEXT("Xbox One");
    case BuildPlatform::LinuxX64:
        return TEXT("Linux x64");
    case BuildPlatform::PS4:
        return TEXT("PlayStation 4");
    case BuildPlatform::XboxScarlett:
        return TEXT("Xbox Scarlett");
    case BuildPlatform::AndroidARM64:
        return TEXT("Android ARM64");
    default:
        return TEXT("?");
    }
}

/// <summary>
/// Game build configuration modes.
/// </summary>
API_ENUM() enum class BuildConfiguration
{
    /// <summary>
    /// Debug configuration. Without optimizations but with full debugging information.
    /// </summary>
    Debug = 0,

    /// <summary>
    /// Development configuration. With basic optimizations and partial debugging data.
    /// </summary>
    Development = 1,

    /// <summary>
    /// Shipping configuration. With full optimization and no debugging data.
    /// </summary>
    Release = 2,
};

inline const Char* ToString(const BuildConfiguration configuration)
{
    switch (configuration)
    {
    case BuildConfiguration::Debug:
        return TEXT("Debug");
    case BuildConfiguration::Development:
        return TEXT("Development");
    case BuildConfiguration::Release:
        return TEXT("Release");
    default:
        return TEXT("?");
    }
}

#define BUILD_STEP_CANCEL_CHECK if (GameCooker::IsCancelRequested()) return true

/// <summary>
/// Game cooking temporary data.
/// </summary>
struct FLAXENGINE_API CookingData
{
    /// <summary>
    /// The platform.
    /// </summary>
    BuildPlatform Platform;

    /// <summary>
    /// The configuration.
    /// </summary>
    BuildConfiguration Configuration;

    /// <summary>
    /// The options.
    /// </summary>
    BuildOptions Options;

    /// <summary>
    /// The list of custom defines passed to the build tool when compiling project scripts. Can be used in build scripts for configuration (Configuration.CustomDefines).
    /// </summary>
    Array<String> CustomDefines;

    /// <summary>
    /// The original output path (actual OutputPath could be modified by the Platform Tools or a plugin for additional layout customizations or packaging). This path is preserved.
    /// </summary>
    String OriginalOutputPath;

    /// <summary>
    /// The output path.
    /// </summary>
    String OutputPath;

    /// <summary>
    /// The platform tools.
    /// </summary>
    PlatformTools* Tools;

public:

    /// <summary>
    /// The asset type build stats storage.
    /// </summary>
    struct AssetTypeStatistics
    {
        /// <summary>
        /// The asset type name.
        /// </summary>
        String TypeName;

        /// <summary>
        /// The amount of assets of that type in a build.
        /// </summary>
        int32 Count = 0;

        /// <summary>
        /// The final output size of the assets of that type in a build.
        /// </summary>
        uint64 ContentSize = 0;

        bool operator<(const AssetTypeStatistics& other) const;
    };

    /// <summary>
    /// The build stats storage.
    /// </summary>
    struct Statistics
    {
        /// <summary>
        /// The total assets amount in the build.
        /// </summary>
        int32 TotalAssets;

        /// <summary>
        /// The cooked assets (TotalAssets - CookedAssets is amount of reused cached assets).
        /// </summary>
        int32 CookedAssets;

        /// <summary>
        /// The final output content size in MB.
        /// </summary>
        int32 ContentSizeMB;

        /// <summary>
        /// The asset type stats. Key is the asset typename, value is the stats container.
        /// </summary>
        Dictionary<String, AssetTypeStatistics> AssetStats;

        Statistics();
    };

    /// <summary>
    /// The build stats.
    /// </summary>
    Statistics Stats;

public:

    /// <summary>
    /// The temporary directory used for the building cache. Can be used for the incremental building.
    /// </summary>
    String CacheDirectory;

    /// <summary>
    /// The root assets collection to include in build in the first place (can be used only before CollectAssetsStep).
    /// Game cooker will find dependant assets and deploy them as well.
    /// </summary>
    HashSet<Guid> RootAssets;

    /// <summary>
    /// The final assets collection to include in build (valid only after CollectAssetsStep).
    /// </summary>
    HashSet<Guid> Assets;

    struct BinaryModule
    {
        String Name;
        String NativePath;
        String ManagedPath;
    };

    /// <summary>
    /// The binary modules used in the build. Valid after scripts compilation step. This list includes game, all plugins modules and engine module.
    /// </summary>
    Array<BinaryModule, InlinedAllocation<64>> BinaryModules;

public:

    void Init()
    {
        RootAssets.Clear();
        Assets.Clear();
        Stats = Statistics();
    }

    /// <summary>
    /// Gets the absolute path to the Platform Data folder that contains the binary files used by the current build configuration.
    /// </summary>
    /// <returns>The platform data folder path.</returns>
    String GetGameBinariesPath() const;

    /// <summary>
    /// Gets the absolute path to the platform folder that contains the dependency files used by the current build configuration.
    /// </summary>
    /// <returns>The platform deps folder path.</returns>
    String GetPlatformBinariesRoot() const;

public:

    /// <summary>
    /// The total amount of baking steps to perform.
    /// </summary>
    int32 StepsCount;

    /// <summary>
    /// The current step index.
    /// </summary>
    int32 CurrentStepIndex;

    /// <summary>
    /// Initializes the progress.
    /// </summary>
    /// <param name="stepsCount">The total steps count.</param>
    void InitProgress(const int32 stepsCount)
    {
        StepsCount = stepsCount;
        CurrentStepIndex = 0;
    }

    /// <summary>
    /// Moves the progress reporting to the next step
    /// </summary>
    void NextStep()
    {
        CurrentStepIndex++;
    }

    /// <summary>
    /// Reports the current step progress (normalized 0-1 value)
    /// </summary>
    /// <param name="info">The step info message.</param>
    /// <param name="stepProgress">The step progress.</param>
    void StepProgress(const String& info, const float stepProgress) const;

public:

    /// <summary>
    /// Adds the asset to the build.
    /// </summary>
    /// <param name="id">The asset id.</param>
    void AddRootAsset(const Guid& id);

    /// <summary>
    /// Adds the asset to the build.
    /// </summary>
    /// <param name="path">The absolute asset path.</param>
    void AddRootAsset(const String& path);

    /// <summary>
    /// Adds the internal engine asset to the build.
    /// </summary>
    /// <param name="internalPath">The internal path (relative to the engine content path).</param>
    void AddRootEngineAsset(const String& internalPath);

public:

    void Error(const String& msg);

    void Error(const Char* msg)
    {
        Error(String(msg));
    }

    template<typename... Args>
    void Error(const Char* format, const Args& ... args)
    {
        const String msg = String::Format(format, args...);
        Error(msg);
    }
};
