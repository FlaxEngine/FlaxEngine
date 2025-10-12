// Copyright (c) Wojciech Figat. All rights reserved.

using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// https://github.com/NVIDIA/nvapi
/// </summary>
public class nvapi : DepsModule
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
        LicenseFilePath = "License.txt";

        // Merge third-party modules into engine binary
        BinaryModuleName = "FlaxEngine";
    }

    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        var depsRoot = options.DepsFolder;
        options.PublicDefinitions.Add("COMPILE_WITH_NVAPI");
        switch (options.Platform.Target)
        {
        case TargetPlatform.Windows:
            switch (options.Architecture)
            {
            case TargetArchitecture.x64:
                options.OutputFiles.Add(Path.Combine(depsRoot, "nvapi64.lib"));
                break;
            default: throw new InvalidArchitectureException(options.Architecture);
            }
            break;
        default: throw new InvalidPlatformException(options.Platform.Target);
        }
    }
}
