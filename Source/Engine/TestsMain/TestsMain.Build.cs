// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// Application startup module.
/// </summary>
public class TestsMain : EngineModule
{
    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        // Use source folder per platform group
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
        case TargetPlatform.Linux:
            options.SourcePaths.Add(Path.Combine(FolderPath, "Linux"));
            break;
        default: throw new InvalidPlatformException(options.Platform.Target);
        }
    }

    /// <inheritdoc />
    public override void GetFilesToDeploy(List<string> files)
    {
    }
}
