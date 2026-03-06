// Copyright (c) Wojciech Figat. All rights reserved.

using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// https://github.com/BinomialLLC/basis_universal
/// </summary>
public class basis_universal : EngineDepsModule
{
    /// <summary>
    /// Returns true if can use basis_universal lib for a given build setup.
    /// </summary>
    public static bool Use(BuildOptions options)
    {
        switch (options.Platform.Target)
        {
        case TargetPlatform.Windows:
        case TargetPlatform.Web:
        case TargetPlatform.Linux:
            return true;
        default:
            return false;
        }
    }

    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        LicenseType = LicenseTypes.Apache2;
        LicenseFilePath = "LICENSE";
    }

    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.PublicDefinitions.Add("COMPILE_WITH_BASISU");
        AddLib(options, options.DepsFolder, "basisu_encoder");
    }
}
