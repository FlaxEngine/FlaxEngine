// Copyright (c) Wojciech Figat. All rights reserved.

using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// https://github.com/ARM-software/astc-encoder
/// </summary>
public class astc : DepsModule
{
    /// <summary>
    /// Returns true if can use astc lib for a given build setup.
    /// </summary>
    public static bool IsSupported(BuildOptions options)
    {
        switch (options.Platform.Target)
        {
        case TargetPlatform.Windows:
            return true;
        case TargetPlatform.Mac:
            return options.Architecture == TargetArchitecture.ARM64;
        default:
            return false;
        }
    }

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
        AddLib(options, options.DepsFolder, "astcenc");
    }
}
