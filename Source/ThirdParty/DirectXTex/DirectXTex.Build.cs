// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// https://github.com/microsoft/DirectXTex
/// </summary>
public class DirectXTex : DepsModule
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        LicenseType = LicenseTypes.MIT;
        LicenseFilePath = "LICENSE";

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
            options.OutputFiles.Add(Path.Combine(depsRoot, "DirectXTex.lib"));
            options.OptionalDependencyFiles.Add(Path.Combine(depsRoot, "DirectXTex.pdb"));
            break;
        default: throw new InvalidPlatformException(options.Platform.Target);
        }

        options.PublicDefinitions.Add("COMPILE_WITH_DIRECTXTEX");
    }
}
