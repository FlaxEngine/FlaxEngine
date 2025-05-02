// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// Application startup module.
/// </summary>
public class Main : EngineModule
{
    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        // Use source folder per platform group
        options.SourcePaths.Clear();
        switch (options.Platform.Target)
        {
        case TargetPlatform.Windows:
            options.SourcePaths.Add(Path.Combine(FolderPath, "Windows"));

            // Visual Leak Detector (https://kinddragon.github.io/vld/)
            /*{
                var vldPath = @"C:\Program Files (x86)\Visual Leak Detector";
                var vldBinaries = options.Toolchain.Architecture == TargetArchitecture.x64 ? "Win64" : "Win32";
                var vldBinariesPostfix = options.Toolchain.Architecture == TargetArchitecture.x64 ? "x64" : "x86";
                options.PrivateDefinitions.Add("USE_VLD_MEM_LEAKS_CHECK");
                options.PrivateDefinitions.Add("VLD_FORCE_ENABLE");
                options.PrivateIncludePaths.Add(Path.Combine(vldPath, "include"));
                options.OutputFiles.Add(Path.Combine(vldPath, "lib", vldBinaries, "vld.lib"));
                options.DependencyFiles.Add(Path.Combine(vldPath, "bin", vldBinaries, "vld_" + vldBinariesPostfix + ".dll"));
                options.DependencyFiles.Add(Path.Combine(vldPath, "bin", vldBinaries, "vld_" + vldBinariesPostfix + ".pdb"));
                options.DependencyFiles.Add(Path.Combine(vldPath, "bin", vldBinaries, "dbghelp.dll"));
                options.DependencyFiles.Add(Path.Combine(vldPath, "bin", vldBinaries, "Microsoft.DTfW.DHL.manifest"));
            }*/

            // Visual Studio memory leaks detection
            /*{
                options.PrivateDefinitions.Add("USE_VS_MEM_LEAKS_CHECK");
            }*/

            break;
        case TargetPlatform.UWP:
            options.SourcePaths.Add(Path.Combine(FolderPath, "UWP"));

            // Use Visual C++ component extensions C++/CX for UWP
            options.CompileEnv.WinRTComponentExtensions = true;
            options.CompileEnv.GenerateDocumentation = true;

            break;
        case TargetPlatform.PS4:
            options.SourcePaths.Add(Path.Combine(Globals.EngineRoot, "Source", "Platforms", "PS4", "Engine", "Main"));
            break;
        case TargetPlatform.PS5:
            options.SourcePaths.Add(Path.Combine(Globals.EngineRoot, "Source", "Platforms", "PS5", "Engine", "Main"));
            break;
        case TargetPlatform.XboxOne:
            options.SourcePaths.Add(Path.Combine(Globals.EngineRoot, "Source", "Platforms", "XboxOne", "Engine", "Main"));
            break;
        case TargetPlatform.XboxScarlett:
            options.SourcePaths.Add(Path.Combine(Globals.EngineRoot, "Source", "Platforms", "XboxScarlett", "Engine", "Main"));
            break;
        case TargetPlatform.Android:
            options.SourcePaths.Add(Path.Combine(FolderPath, "Android"));
            break;
        case TargetPlatform.Switch:
            options.SourcePaths.Add(Path.Combine(Globals.EngineRoot, "Source", "Platforms", "Switch", "Engine", "Main"));
            break;
        case TargetPlatform.Linux:
        case TargetPlatform.Mac:
        case TargetPlatform.iOS:
            options.SourcePaths.Add(Path.Combine(FolderPath, "Default"));
            break;
        default: throw new InvalidPlatformException(options.Platform.Target);
        }
    }

    /// <inheritdoc />
    public override void GetFilesToDeploy(List<string> files)
    {
        files.Add(Path.Combine(FolderPath, "Android/android_native_app_glue.h"));
    }
}
