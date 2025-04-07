// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "CookingData.h"
#include "Engine/Scripting/ScriptingType.h"

/// <summary>
/// Game building service. Processes project files and outputs build game for a target platform.
/// </summary>
API_CLASS(Static, Namespace="FlaxEditor") class FLAXENGINE_API GameCooker
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(GameCooker);
    friend CookingData;
public:
    /// <summary>
    /// Single build step.
    /// </summary>
    class FLAXENGINE_API BuildStep
    {
    public:
        /// <summary>
        /// Finalizes an instance of the <see cref="BuildStep"/> class.
        /// </summary>
        virtual ~BuildStep() = default;

        /// <summary>
        /// Called when building starts.
        /// </summary>
        /// <param name="data">The data.</param>
        virtual void OnBuildStarted(CookingData& data)
        {
        }

        /// <summary>
        /// Performs this step.
        /// </summary>
        /// <param name="data">The data.</param>
        /// <returns>True if failed, otherwise false.</returns>
        virtual bool Perform(CookingData& data) = 0;

        /// <summary>
        /// Called when building ends.
        /// </summary>
        /// <param name="data">The cooking data.</param>
        /// <param name="failed">True if build failed, otherwise false.</param>
        virtual void OnBuildEnded(CookingData& data, bool failed)
        {
        }
    };

public:
    /// <summary>
    /// Gets the current build data. Valid only during active build process.
    /// </summary>
    API_PROPERTY() static CookingData* GetCurrentData();

    /// <summary>
    /// Determines whether game building is running.
    /// </summary>
    API_PROPERTY() static bool IsRunning();

    /// <summary>
    /// Determines whether building cancel has been requested.
    /// </summary>
    API_PROPERTY() static bool IsCancelRequested();

    /// <summary>
    /// Gets the tools for the given platform.
    /// </summary>
    /// <param name="platform">The platform.</param>
    /// <returns>The platform tools implementation or null if not supported.</returns>
    static PlatformTools* GetTools(BuildPlatform platform);

    /// <summary>
    /// Starts building game for the specified platform.
    /// </summary>
    /// <param name="platform">The target platform.</param>
    /// <param name="configuration">The build configuration.</param>
    /// <param name="outputPath">The output path (output directory).</param>
    /// <param name="options">The build options.</param>
    /// <param name="customDefines">The list of custom defines passed to the build tool when compiling project scripts. Can be used in build scripts for configuration (Configuration.CustomDefines).</param>
    /// <param name="preset">The name of build preset used for cooking (can be used by editor and game plugins).</param>
    /// <param name="presetTarget">The name of build preset target used for cooking (can be used by editor and game plugins).</param>
    /// <returns>True if failed to start the build, otherwise false.</returns>
    API_FUNCTION() static bool Build(BuildPlatform platform, BuildConfiguration configuration, const StringView& outputPath, BuildOptions options, const Array<String>& customDefines, const StringView& preset = StringView::Empty, const StringView& presetTarget = StringView::Empty);

    /// <summary>
    /// Sends a cancel event to the game building service.
    /// </summary>
    /// <param name="waitForEnd">If set to <c>true</c> wait for the stopped building end.</param>
    API_FUNCTION() static void Cancel(bool waitForEnd = false);

    /// <summary>
    /// Gets the current Editor build info (platform, configuration, etc).
    /// </summary>
    API_FUNCTION() static void GetCurrentPlatform(API_PARAM(Out) PlatformType& platform, API_PARAM(Out) BuildPlatform& buildPlatform, API_PARAM(Out) BuildConfiguration& buildConfiguration);

    /// <summary>
    /// Building event type.
    /// </summary>
    enum class EventType
    {
        /// <summary>
        /// The build started.
        /// </summary>
        BuildStarted = 0,

        /// <summary>
        /// The build failed.
        /// </summary>
        BuildFailed = 1,

        /// <summary>
        /// The build done.
        /// </summary>
        BuildDone = 2,
    };

    /// <summary>
    /// Occurs when building event rises.
    /// </summary>
    static Delegate<EventType> OnEvent;

    /// <summary>
    /// Occurs when building game progress fires.
    /// </summary>
    static Delegate<const String&, float> OnProgress;

    /// <summary>
    /// Occurs when game files and data is deployed.
    /// </summary>
    API_EVENT() static Action DeployFiles;

    /// <summary>
    /// Occurs when game files and data are deployed and can be post-processed.
    /// </summary>
    API_EVENT() static Action PostProcessFiles;

    /// <summary>
    /// Occurs when game files and data are ready to be packaged. Called only if game is about to packaged, otherwise this step is skipped.
    /// </summary>
    API_EVENT() static Action PackageFiles;

    /// <summary>
    /// Occurs when building collects assets to cook.
    /// </summary>
    static Delegate<HashSet<Guid>&> OnCollectAssets;
};
