// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Log.h"
#include "Engine/Core/Types/Guid.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Collections/HashSet.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Scripting/ScriptingObject.h"

class GameCooker;
class PlatformTools;

#if OFFICIAL_BUILD
// Use the fixed .NET SDK version in packaged builds for compatibility (FlaxGame is precompiled with it)
#define GAME_BUILD_DOTNET_VER TEXT("-dotnet=8")
#else
#define GAME_BUILD_DOTNET_VER TEXT("")
#endif

// Range of dotnet runtime versions
#define GAME_BUILD_DOTNET_RUNTIME_MIN_VER 8
#define GAME_BUILD_DOTNET_RUNTIME_MAX_VER 9

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

    /// <summary>
    /// Starts the cooked game build on building end.
    /// </summary>
    AutoRun = 1 << 1,

    /// <summary>
    /// Skips cooking logic and uses already cooked data (eg. to only use AutoRun or ShowOutput feature).
    /// </summary>
    NoCook = 1 << 2,
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

    /// <summary>
    /// Switch.
    /// </summary>
    Switch = 10,

    /// <summary>
    /// PlayStation 5
    /// </summary>
    API_ENUM(Attributes="EditorDisplay(null, \"PlayStation 5\")")
    PS5 = 11,

    /// <summary>
    /// MacOS (x86-64 Intel)
    /// </summary>
    API_ENUM(Attributes="EditorDisplay(null, \"Mac x64\")")
    MacOSx64 = 12,

    /// <summary>
    /// MacOS (ARM64 Apple Silicon)
    /// </summary>
    API_ENUM(Attributes="EditorDisplay(null, \"Mac ARM64\")")
    MacOSARM64 = 13,

    /// <summary>
    /// iOS (ARM64)
    /// </summary>
    API_ENUM(Attributes="EditorDisplay(null, \"iOS ARM64\")")
    iOSARM64 = 14,

    /// <summary>
    /// Windows (ARM64)
    /// </summary>
    API_ENUM(Attributes = "EditorDisplay(null, \"Windows ARM64\")")
    WindowsARM64 = 15,
};

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

/// <summary>
/// .NET Ahead of Time Compilation (AOT) modes.
/// </summary>
enum class DotNetAOTModes
{
    /// <summary>
    /// AOT is not used.
    /// </summary>
    None,

    /// <summary>
    /// Use .NET Native IL Compiler (shorten as ILC) to convert all C# assemblies in native platform executable binary.
    /// </summary>
    ILC,

    /// <summary>
    /// Use Mono AOT to cross-compile all used C# assemblies into native platform shared libraries.
    /// </summary>
    MonoAOTDynamic,

    /// <summary>
    /// Use Mono AOT to cross-compile all used C# assemblies into native platform static libraries which can be linked into a single shared library.
    /// </summary>
    MonoAOTStatic,
};

extern FLAXENGINE_API const Char* ToString(const BuildPlatform platform);
extern FLAXENGINE_API const Char* ToString(const BuildConfiguration configuration);
extern FLAXENGINE_API const Char* ToString(const DotNetAOTModes mode);

#define BUILD_STEP_CANCEL_CHECK if (GameCooker::IsCancelRequested()) return true

/// <summary>
/// Game cooking temporary data.
/// </summary>
API_CLASS(Sealed, Namespace="FlaxEditor") class FLAXENGINE_API CookingData : public ScriptingObject
{
DECLARE_SCRIPTING_TYPE(CookingData);
public:

    /// <summary>
    /// The platform.
    /// </summary>
    API_FIELD(ReadOnly) BuildPlatform Platform;

    /// <summary>
    /// The configuration.
    /// </summary>
    API_FIELD(ReadOnly) BuildConfiguration Configuration;

    /// <summary>
    /// The options.
    /// </summary>
    API_FIELD(ReadOnly) BuildOptions Options;

    /// <summary>
    /// The name of build preset used for cooking (can be used by editor and game plugins).
    /// </summary>
    API_FIELD(ReadOnly) String Preset;

    /// <summary>
    /// The name of build preset target used for cooking (can be used by editor and game plugins).
    /// </summary>
    API_FIELD(ReadOnly) String PresetTarget;

    /// <summary>
    /// The list of custom defines passed to the build tool when compiling project scripts. Can be used in build scripts for configuration (Configuration.CustomDefines).
    /// </summary>
    API_FIELD(ReadOnly) Array<String> CustomDefines;

    /// <summary>
    /// The original output path (actual OutputPath could be modified by the Platform Tools or a plugin for additional layout customizations or packaging). This path is preserved.
    /// </summary>
    API_FIELD(ReadOnly) String OriginalOutputPath;

    /// <summary>
    /// The output path for data files (Content, Dotnet, Mono, etc.).
    /// </summary>
    API_FIELD(ReadOnly) String DataOutputPath;

    /// <summary>
    /// The output path for binaries (native executable and native code libraries).
    /// </summary>
    API_FIELD(ReadOnly) String NativeCodeOutputPath;

    /// <summary>
    /// The output path for binaries (C# code libraries).
    /// </summary>
    API_FIELD(ReadOnly) String ManagedCodeOutputPath;

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
        int32 TotalAssets = 0;

        /// <summary>
        /// The cooked assets (TotalAssets - CookedAssets is amount of reused cached assets).
        /// </summary>
        int32 CookedAssets = 0;

        /// <summary>
        /// The final output content size (in bytes).
        /// </summary>
        uint64 ContentSize = 0;

        /// <summary>
        /// The asset type stats. Key is the asset typename, value is the stats container.
        /// </summary>
        Dictionary<String, AssetTypeStatistics> AssetStats;
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

    /// <summary>
    /// The final files collection to include in build (valid only after CollectAssetsStep).
    /// </summary>
    HashSet<String> Files;

    struct BinaryModuleInfo
    {
        String Name;
        String NativePath;
        String ManagedPath;
    };

    /// <summary>
    /// The binary modules used in the build. Valid after scripts compilation step. This list includes game, all plugins modules and engine module.
    /// </summary>
    Array<BinaryModuleInfo, InlinedAllocation<64>> BinaryModules;

public:

    /// <summary>
    /// Gets the absolute path to the Platform Data folder that contains the binary files used by the current build configuration.
    /// </summary>
    String GetGameBinariesPath() const;

    /// <summary>
    /// Gets the absolute path to the platform folder that contains the dependency files used by the current build configuration.
    /// </summary>
    String GetPlatformBinariesRoot() const;

    /// <summary>
    /// Gets the name of the platform and architecture for the current BuildPlatform.
    /// </summary>
    void GetBuildPlatformName(const Char*& platform, const Char*& architecture) const;

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

    void Error(const StringView& msg);

    void Error(const String& msg)
    {
        Error(StringView(msg));
    }

    void Error(const Char* msg)
    {
        Error(StringView(msg));
    }
};
