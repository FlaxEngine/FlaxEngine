// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// https://www.assimp.org/
/// </summary>
public class assimp : DepsModule
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        LicenseType = LicenseTypes.Custom;
        LicenseFilePath = "LICENSE";

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
            options.OutputFiles.Add(Path.Combine(depsRoot, "assimp-vc140-md.lib"));
            options.DependencyFiles.Add(Path.Combine(depsRoot, "assimp-vc140-md.dll"));
            options.DelayLoadLibraries.Add("assimp-vc140-md.dll");
            break;
        case TargetPlatform.Linux:
        case TargetPlatform.Mac:
            options.OutputFiles.Add(Path.Combine(depsRoot, "libassimp.a"));
            break;
        default: throw new InvalidPlatformException(options.Platform.Target);
        }
    }
}
