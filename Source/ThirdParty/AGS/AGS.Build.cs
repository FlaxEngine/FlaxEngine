// Copyright (c) Wojciech Figat. All rights reserved.

using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// https://github.com/GPUOpen-LibrariesAndSDKs/AGS_SDK
/// </summary>
public class AGS : DepsModule
{
    public static bool Use(BuildOptions options)
    {
        return options.Platform.Target == TargetPlatform.Windows && options.Architecture == TargetArchitecture.x64;
    }

    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        LicenseType = LicenseTypes.MIT;
        LicenseFilePath = "LICENSE.txt";

        // Merge third-party modules into engine binary
        BinaryModuleName = "FlaxEngine";
    }

    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        var depsRoot = options.DepsFolder;
        options.PublicDefinitions.Add("COMPILE_WITH_AGS");
        switch (options.Platform.Target)
        {
        case TargetPlatform.Windows:
            switch (options.Architecture)
            {
            case TargetArchitecture.x64:
                options.OutputFiles.Add(Path.Combine(depsRoot, "amd_ags_x64.lib"));
                options.OptionalDependencyFiles.Add(Path.Combine(depsRoot, "amd_ags_x64.dll"));
                break;
            default: throw new InvalidArchitectureException(options.Architecture);
            }
            break;
        default: throw new InvalidPlatformException(options.Platform.Target);
        }
    }
}
