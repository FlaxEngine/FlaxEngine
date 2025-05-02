// Copyright (c) Wojciech Figat. All rights reserved.

using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// https://github.com/KhronosGroup/SPIRV-Tools
/// </summary>
public class spirv_tools : DepsModule
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        Name = "spirv-tools";
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
            options.OutputFiles.Add(Path.Combine(depsRoot, "SPIRV-Tools.lib"));
            options.OutputFiles.Add(Path.Combine(depsRoot, "SPIRV-Tools-opt.lib"));
            break;
        case TargetPlatform.Linux:
        case TargetPlatform.Mac:
            options.OutputFiles.Add(Path.Combine(depsRoot, "libSPIRV-Tools.a"));
            options.OutputFiles.Add(Path.Combine(depsRoot, "libSPIRV-Tools-opt.a"));
            break;
        default: throw new InvalidPlatformException(options.Platform.Target);
        }
    }
}
