// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// https://www.libsdl.org/
/// </summary>
public class SDL : DepsModule
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        LicenseType = LicenseTypes.Custom;
        LicenseFilePath = "LICENSE.txt";

        // Merge third-party modules into engine binary
        BinaryModuleName = "FlaxEngine";
    }

    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        var depsRoot = options.DepsFolder;
        switch (options.Platform.Target)
        {
        case TargetPlatform.Windows:
            options.OutputFiles.Add(Path.Combine(depsRoot, "SDL3.lib"));
            options.OptionalDependencyFiles.Add(Path.Combine(depsRoot, "SDL3.pdb"));
            options.OptionalDependencyFiles.Add(Path.Combine(depsRoot, "SDL3.dll"));

            // For static linkage
            options.OutputFiles.Add("Setupapi.lib");
            options.OutputFiles.Add("Version.lib");
            options.OutputFiles.Add("Imm32.lib");
            options.OutputFiles.Add("Gdi32.lib");

            break;
        case TargetPlatform.Linux:
        case TargetPlatform.Mac:
            options.OutputFiles.Add(Path.Combine(depsRoot, "libSDL3.a"));
            break;
        default: throw new InvalidPlatformException(options.Platform.Target);
        }
		
		options.PublicIncludePaths.Add(Path.Combine(Globals.EngineRoot, @"Source\ThirdParty\SDL"));
    }
}
