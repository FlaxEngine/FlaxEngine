// Copyright (c) Wojciech Figat. All rights reserved.

using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// https://github.com/Chlumsky/msdfgen
/// </summary>
public class msdfgen : EngineDepsModule
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        LicenseType = LicenseTypes.MIT;
        LicenseFilePath = "LICENSE.txt";
    }

    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        var depsRoot = options.DepsFolder;
        switch (options.Platform.Target)
        {
        case TargetPlatform.Windows:
        case TargetPlatform.XboxOne:
        case TargetPlatform.XboxScarlett:
            options.OutputFiles.Add(Path.Combine(depsRoot, "msdfgen-core.lib"));
            break;
        case TargetPlatform.Linux:
        case TargetPlatform.Mac:
        case TargetPlatform.iOS:
        case TargetPlatform.Android:
        case TargetPlatform.Switch:
        case TargetPlatform.PS4:
        case TargetPlatform.PS5:
        case TargetPlatform.Web:
            options.OutputFiles.Add(Path.Combine(depsRoot, "libmsdfgen-core.a"));
            break;
        default: throw new InvalidPlatformException(options.Platform.Target);
        }

        options.PublicIncludePaths.Add(Path.Combine(Globals.EngineRoot, "Source/ThirdParty/msdfgen"));
    }
}
