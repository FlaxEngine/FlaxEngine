// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;
using Microsoft.Win32;

/// <summary>
/// Editor module.
/// </summary>
public class Editor : EditorModule
{
    private void AddPlatformTools(BuildOptions options, string platformToolsRoot, string platformToolsRootExternal, string platform, params string[] macros)
    {
        if (Directory.Exists(Path.Combine(platformToolsRoot, platform)))
        {
            // Platform Tools bundled inside Editor module
            options.PrivateDefinitions.AddRange(macros);
        }
        else
        {
            string externalPath = Path.Combine(platformToolsRootExternal, platform, "Editor", "PlatformTools");
            if (Directory.Exists(externalPath))
            {
                // Platform Tools inside external platform implementation location
                options.PrivateDefinitions.AddRange(macros);
                options.SourcePaths.Add(externalPath);
                AddSourceFileIfExists(options, Path.Combine(Globals.EngineRoot, "Source", "Platforms", platform, "Engine", "Platform", platform + "PlatformSettings.cs"));
            }
        }
    }

    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.ScriptingAPI.SystemReferences.Add("System.Xml");
        options.ScriptingAPI.SystemReferences.Add("System.Xml.ReaderWriter");
        options.ScriptingAPI.SystemReferences.Add("System.Text.RegularExpressions");
        options.ScriptingAPI.SystemReferences.Add("System.IO.Compression.ZipFile");
        options.ScriptingAPI.SystemReferences.Add("System.Diagnostics.Process");
        if (Profiler.Use(options))
            options.ScriptingAPI.Defines.Add("USE_PROFILER");

        // Enable optimizations for Editor, disable this for debugging the editor
        if (options.Configuration == TargetConfiguration.Development)
            options.ScriptingAPI.Optimization = true;

        options.PublicDependencies.Add("Engine");
        options.PrivateDependencies.Add("pugixml");
        options.PrivateDependencies.Add("curl");
        options.PrivateDependencies.Add("ContentImporters");
        options.PrivateDependencies.Add("ContentExporters");
        options.PrivateDependencies.Add("ShadowsOfMordor");
        options.PrivateDependencies.Add("CSG");
        options.PrivateDependencies.Add("ShadersCompilation");
        options.PrivateDependencies.Add("MaterialGenerator");
        options.PrivateDependencies.Add("Renderer");
        options.PrivateDependencies.Add("TextureTool");
        options.PrivateDependencies.Add("Particles");

        var platformToolsRoot = Path.Combine(FolderPath, "Cooker", "Platform");
        var platformToolsRootExternal = Path.Combine(Globals.EngineRoot, "Source", "Platforms");
        if (options.Platform.Target == TargetPlatform.Windows)
        {
            AddPlatformTools(options, platformToolsRoot, platformToolsRootExternal, "Windows", "PLATFORM_TOOLS_WINDOWS");
            AddPlatformTools(options, platformToolsRoot, platformToolsRootExternal, "UWP", "PLATFORM_TOOLS_UWP");
            AddPlatformTools(options, platformToolsRoot, platformToolsRootExternal, "XboxOne", "PLATFORM_TOOLS_XBOX_ONE", "PLATFORM_TOOLS_GDK");
            AddPlatformTools(options, platformToolsRoot, platformToolsRootExternal, "PS4", "PLATFORM_TOOLS_PS4");
            AddPlatformTools(options, platformToolsRoot, platformToolsRootExternal, "PS5", "PLATFORM_TOOLS_PS5");
            AddPlatformTools(options, platformToolsRoot, platformToolsRootExternal, "XboxScarlett", "PLATFORM_TOOLS_XBOX_SCARLETT", "PLATFORM_TOOLS_GDK");
            AddPlatformTools(options, platformToolsRoot, platformToolsRootExternal, "Android", "PLATFORM_TOOLS_ANDROID");
            AddPlatformTools(options, platformToolsRoot, platformToolsRootExternal, "Switch", "PLATFORM_TOOLS_SWITCH");
            AddPlatformTools(options, platformToolsRoot, platformToolsRootExternal, "Linux", "PLATFORM_TOOLS_LINUX");
        }
        else if (options.Platform.Target == TargetPlatform.Linux)
        {
            AddPlatformTools(options, platformToolsRoot, platformToolsRootExternal, "Linux", "PLATFORM_TOOLS_LINUX");
            AddPlatformTools(options, platformToolsRoot, platformToolsRootExternal, "Android", "PLATFORM_TOOLS_ANDROID");
        }
        else if (options.Platform.Target == TargetPlatform.Mac)
        {
            AddPlatformTools(options, platformToolsRoot, platformToolsRootExternal, "Mac", "PLATFORM_TOOLS_MAC");
            AddPlatformTools(options, platformToolsRoot, platformToolsRootExternal, "Android", "PLATFORM_TOOLS_ANDROID");
            AddPlatformTools(options, platformToolsRoot, platformToolsRootExternal, "iOS", "PLATFORM_TOOLS_IOS");
        }

        // Visual Studio integration
        if (options.Platform.Target == TargetPlatform.Windows && Flax.Build.Platform.BuildTargetPlatform == TargetPlatform.Windows)
        {
#pragma warning disable CA1416
            var path = Registry.GetValue("HKEY_CLASSES_ROOT\\TypeLib\\{80CC9F66-E7D8-4DDD-85B6-D9E6CD0E93E2}\\8.0\\0\\win32", null, null) as string;
#pragma warning restore CA1416
            if (path != null && File.Exists(path))
                options.PrivateDefinitions.Add("USE_VISUAL_STUDIO_DTE");
        }

        // Setup .NET versions range valid for Game Cooker (minimal is the one used by the native runtime)
        var dotnetSdk = DotNetSdk.Instance;
        if (dotnetSdk.IsValid)
        {
            var sdkVer = dotnetSdk.Version.Major;
            var minVer = Math.Max(DotNetSdk.MinimumVersion.Major, sdkVer);
            var maxVer = DotNetSdk.MaximumVersion.Major;
            options.PrivateDefinitions.Add("GAME_BUILD_DOTNET_RUNTIME_MIN_VER=" + minVer);
            options.PrivateDefinitions.Add("GAME_BUILD_DOTNET_RUNTIME_MAX_VER=" + DotNetSdk.MaximumVersion.Major);
            Log.Verbose($"Using Dotnet runtime versions range {minVer}-{maxVer} for Game Cooker");
        }
    }

    /// <inheritdoc />
    public override void GetFilesToDeploy(List<string> files)
    {
        files.Add(Path.Combine(FolderPath, "Editor.h"));
        files.Add(Path.Combine(FolderPath, "ProjectInfo.h"));

        files.Add(Path.Combine(FolderPath, "Cooker/CookingData.h"));
        files.Add(Path.Combine(FolderPath, "Cooker/GameCooker.h"));
        files.Add(Path.Combine(FolderPath, "Cooker/PlatformTools.h"));
        files.Add(Path.Combine(FolderPath, "Cooker/Steps/CookAssetsStep.h"));
        files.Add(Path.Combine(FolderPath, "Utilities/ScreenUtilities.h"));
        files.Add(Path.Combine(FolderPath, "Utilities/ViewportIconsRenderer.h"));
    }
}
