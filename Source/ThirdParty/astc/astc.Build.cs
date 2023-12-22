// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// https://github.com/ARM-software/astc-encoder
/// </summary>
public class astc : ThirdPartyModule
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        LicenseType = LicenseTypes.Apache2;
        LicenseFilePath = "LICENSE.txt";

        // Merge third-party modules into engine binary
        BinaryModuleName = "FlaxEngine";
    }

    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.PublicDefinitions.Add("COMPILE_WITH_ASTC");
        var depsRoot = options.DepsFolder;
        switch (options.Platform.Target)
        {
        case TargetPlatform.Mac:
            options.OutputFiles.Add(Path.Combine(depsRoot, "libastcenc.a"));
            break;
        default: throw new InvalidPlatformException(options.Platform.Target);
        }
    }
}
