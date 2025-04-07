// Copyright (c) Wojciech Figat. All rights reserved.

using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// Platform implementation module.
/// </summary>
public class Platform : EngineModule
{
    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        // Don't ref Core module but only Profiler
        options.PrivateDependencies.Clear();
        if (Profiler.Use(options))
            options.PublicDependencies.Add("Profiler");

        // Use source folder per platform group
        options.SourcePaths.Clear();
        options.SourcePaths.Add(Path.Combine(FolderPath, "Base"));
        options.SourceFiles.AddRange(Directory.GetFiles(FolderPath, "*.*", SearchOption.TopDirectoryOnly));
        switch (options.Platform.Target)
        {
        case TargetPlatform.Windows:
            options.SourcePaths.Add(Path.Combine(FolderPath, "Win32"));
            options.SourcePaths.Add(Path.Combine(FolderPath, "Windows"));

            options.OutputFiles.Add("dinput8.lib");
            options.OutputFiles.Add("xinput9_1_0.lib");

            if (options.Configuration != TargetConfiguration.Release)
            {
                options.OutputFiles.Add("dbghelp.lib");
                options.DelayLoadLibraries.Add("dbghelp.dll");
                options.DependencyFiles.Add(Path.Combine(options.DepsFolder, "dbghelp.dll"));
            }
            if (options.Target.IsEditor)
            {
                //options.Libraries.Add("Gdi32.dll");
                options.Libraries.Add("Dwmapi.dll");
            }
            break;
        case TargetPlatform.UWP:
            options.SourcePaths.Add(Path.Combine(FolderPath, "Win32"));
            options.SourcePaths.Add(Path.Combine(FolderPath, "UWP"));
            break;
        case TargetPlatform.Linux:
            options.SourcePaths.Add(Path.Combine(FolderPath, "Unix"));
            options.SourcePaths.Add(Path.Combine(FolderPath, "Linux"));
            break;
        case TargetPlatform.PS4:
            options.SourcePaths.Add(Path.Combine(FolderPath, "Unix"));
            options.SourcePaths.Add(Path.Combine(Globals.EngineRoot, "Source", "Platforms", "PS4", "Engine", "Platform"));
            break;
        case TargetPlatform.PS5:
            options.SourcePaths.Add(Path.Combine(FolderPath, "Unix"));
            options.SourcePaths.Add(Path.Combine(Globals.EngineRoot, "Source", "Platforms", "PS5", "Engine", "Platform"));
            break;
        case TargetPlatform.XboxOne:
            options.SourcePaths.Add(Path.Combine(FolderPath, "Win32"));
            options.SourcePaths.Add(Path.Combine(FolderPath, "GDK"));
            options.SourcePaths.Add(Path.Combine(Globals.EngineRoot, "Source", "Platforms", "XboxOne", "Engine", "Platform"));
            break;
        case TargetPlatform.XboxScarlett:
            options.SourcePaths.Add(Path.Combine(FolderPath, "Win32"));
            options.SourcePaths.Add(Path.Combine(FolderPath, "GDK"));
            options.SourcePaths.Add(Path.Combine(Globals.EngineRoot, "Source", "Platforms", "XboxScarlett", "Engine", "Platform"));
            break;
        case TargetPlatform.Android:
            options.SourcePaths.Add(Path.Combine(FolderPath, "Unix"));
            options.SourcePaths.Add(Path.Combine(FolderPath, "Android"));
            break;
        case TargetPlatform.Switch:
            options.SourcePaths.Add(Path.Combine(Globals.EngineRoot, "Source", "Platforms", "Switch", "Engine", "Platform"));
            break;
        case TargetPlatform.Mac:
            options.SourcePaths.Add(Path.Combine(FolderPath, "Unix"));
            options.SourcePaths.Add(Path.Combine(FolderPath, "Apple"));
            options.SourcePaths.Add(Path.Combine(FolderPath, "Mac"));
            break;
        case TargetPlatform.iOS:
            options.SourcePaths.Add(Path.Combine(FolderPath, "Unix"));
            options.SourcePaths.Add(Path.Combine(FolderPath, "Apple"));
            options.SourcePaths.Add(Path.Combine(FolderPath, "iOS"));
            break;
        default: throw new InvalidPlatformException(options.Platform.Target);
        }
        if (options.Target.IsEditor)
        {
            // Include platform settings headers
            options.SourceFiles.Add(Path.Combine(FolderPath, "Windows", "WindowsPlatformSettings.h"));
            options.SourceFiles.Add(Path.Combine(FolderPath, "UWP", "UWPPlatformSettings.h"));
            options.SourceFiles.Add(Path.Combine(FolderPath, "Linux", "LinuxPlatformSettings.h"));
            options.SourceFiles.Add(Path.Combine(FolderPath, "Android", "AndroidPlatformSettings.h"));
            options.SourceFiles.Add(Path.Combine(FolderPath, "GDK", "GDKPlatformSettings.h"));
            options.SourceFiles.Add(Path.Combine(FolderPath, "Apple", "ApplePlatformSettings.h"));
            options.SourceFiles.Add(Path.Combine(FolderPath, "Mac", "MacPlatformSettings.h"));
            options.SourceFiles.Add(Path.Combine(FolderPath, "iOS", "iOSPlatformSettings.h"));
            AddSourceFileIfExists(options, Path.Combine(Globals.EngineRoot, "Source", "Platforms", "XboxOne", "Engine", "Platform", "XboxOnePlatformSettings.h"));
            AddSourceFileIfExists(options, Path.Combine(Globals.EngineRoot, "Source", "Platforms", "XboxScarlett", "Engine", "Platform", "XboxScarlettPlatformSettings.h"));
            AddSourceFileIfExists(options, Path.Combine(Globals.EngineRoot, "Source", "Platforms", "PS4", "Engine", "Platform", "PS4PlatformSettings.h"));
            AddSourceFileIfExists(options, Path.Combine(Globals.EngineRoot, "Source", "Platforms", "PS5", "Engine", "Platform", "PS5PlatformSettings.h"));
            AddSourceFileIfExists(options, Path.Combine(Globals.EngineRoot, "Source", "Platforms", "Switch", "Engine", "Platform", "SwitchPlatformSettings.h"));
        }
    }
}
