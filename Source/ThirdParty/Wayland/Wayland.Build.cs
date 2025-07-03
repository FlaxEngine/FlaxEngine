// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// https://gitlab.freedesktop.org/wayland/wayland-protocolsk
/// </summary>
public class Wayland : ThirdPartyModule
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        LicenseType = LicenseTypes.MIT;
        LicenseFilePath = "LICENSE.txt";

        // Merge third-party modules into engine binary
        BinaryModuleName = "FlaxEngine";
    }

    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        if (options.Platform.Target != TargetPlatform.Linux)
            throw new InvalidPlatformException(options.Platform.Target);

        // Include generated protocol files for dependency
        options.PublicIncludePaths.Add(Path.Combine(FolderPath, "include"));
        options.SourceFiles.AddRange(Directory.GetFiles(FolderPath, "*.c", SearchOption.TopDirectoryOnly));
    }
}
