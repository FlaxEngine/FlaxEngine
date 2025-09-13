// Copyright (c) Wojciech Figat. All rights reserved.

using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// https://gitlab.com/libtiff/libtiff
/// </summary>
public class libtiff : ThirdPartyModule
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        LicenseType = LicenseTypes.Custom;
        LicenseFilePath = "LICENSE.md";
    }

    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        var depsRoot = options.DepsFolder;
        switch (options.Platform.Target)
        {
            case TargetPlatform.Linux:
            // TODO: Compile libtiff library for Mac
            //case TargetPlatform.Mac:
                options.OutputFiles.Add(Path.Combine(depsRoot, "libtiff.a"));
                break;
            default:
                throw new InvalidPlatformException(options.Platform.Target);
        }
    }
}