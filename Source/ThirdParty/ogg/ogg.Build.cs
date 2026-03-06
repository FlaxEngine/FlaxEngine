// Copyright (c) Wojciech Figat. All rights reserved.

using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// https://github.com/xiph/ogg
/// </summary>
public class ogg : EngineDepsModule
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        LicenseType = LicenseTypes.Custom;
        LicenseFilePath = "COPYING";
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
        case TargetPlatform.PS5:
        case TargetPlatform.Android:
        case TargetPlatform.Switch:
        case TargetPlatform.Mac:
        case TargetPlatform.iOS:
            options.OutputFiles.Add(Path.Combine(depsRoot, "libogg.a"));
            break;
        case TargetPlatform.Web:
            options.OutputFiles.Add("--use-port=ogg");
            break;
        default: throw new InvalidPlatformException(options.Platform.Target);
        }
    }
}
