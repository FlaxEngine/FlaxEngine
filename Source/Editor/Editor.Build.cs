// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
    private void AddPlatformTools(BuildOptions options, string platformToolsRoot, string platformToolsRootExternal, string platform, string macro)
    {
        if (Directory.Exists(Path.Combine(platformToolsRoot, platform)))
        {
            // Platform Tools bundled inside Editor module
            options.PrivateDefinitions.Add(macro);
        }
        else
        {
            string externalPath = Path.Combine(platformToolsRootExternal, platform, "Editor", "PlatformTools");
            if (Directory.Exists(externalPath))
            {
                // Platform Tools inside external platform implementation location
                options.PrivateDefinitions.Add(macro);
                options.SourcePaths.Add(externalPath);
                AddSourceFileIfExists(options, Path.Combine(Globals.EngineRoot, "Source", "Platforms", platform, "Engine", "Platform", platform + "PlatformSettings.cs"));
            }
        }
    }

    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.PublicDependencies.Add("Engine");
        options.PrivateDependencies.Add("pugixml");
        options.PrivateDependencies.Add("UniversalAnalytics");
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
        AddPlatformTools(options, platformToolsRoot, platformToolsRootExternal, "Windows", "PLATFORM_TOOLS_WINDOWS");
        AddPlatformTools(options, platformToolsRoot, platformToolsRootExternal, "UWP", "PLATFORM_TOOLS_UWP");
        AddPlatformTools(options, platformToolsRoot, platformToolsRootExternal, "UWP", "PLATFORM_TOOLS_XBOX_ONE");
        AddPlatformTools(options, platformToolsRoot, platformToolsRootExternal, "Linux", "PLATFORM_TOOLS_LINUX");
        AddPlatformTools(options, platformToolsRoot, platformToolsRootExternal, "PS4", "PLATFORM_TOOLS_PS4");
        AddPlatformTools(options, platformToolsRoot, platformToolsRootExternal, "XboxScarlett", "PLATFORM_TOOLS_XBOX_SCARLETT");
        AddPlatformTools(options, platformToolsRoot, platformToolsRootExternal, "Android", "PLATFORM_TOOLS_ANDROID");

        // Visual Studio integration
        if (options.Platform.Target == TargetPlatform.Windows)
        {
            var path = Registry.GetValue("HKEY_CLASSES_ROOT\\TypeLib\\{80CC9F66-E7D8-4DDD-85B6-D9E6CD0E93E2}\\8.0\\0\\win32", null, null) as string;
            if (path != null && File.Exists(path))
                options.PrivateDefinitions.Add("USE_VISUAL_STUDIO_DTE");
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
    }
}
