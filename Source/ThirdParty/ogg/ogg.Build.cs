// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// https://github.com/xiph/ogg
/// </summary>
public class ogg : DepsModule
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        LicenseType = LicenseTypes.Custom;
        LicenseFilePath = "COPYING";

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
        case TargetPlatform.UWP:
        case TargetPlatform.XboxOne:
        case TargetPlatform.XboxScarlett:
            options.OutputFiles.Add(Path.Combine(depsRoot, "libogg_static.lib"));
            options.OptionalDependencyFiles.Add(Path.Combine(depsRoot, "libogg_static.pdb"));
            break;
        case TargetPlatform.Linux:
        case TargetPlatform.PS4:
        case TargetPlatform.Android:
            options.OutputFiles.Add(Path.Combine(depsRoot, "libogg.a"));
            break;
        default: throw new InvalidPlatformException(options.Platform.Target);
        }
    }
}
