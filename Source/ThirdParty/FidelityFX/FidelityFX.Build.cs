// Copyright (c) Wojciech Figat. All rights reserved.

using Flax.Build;
using System.Collections.Generic;
using System.IO;

/// <summary>
/// https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK
/// </summary>
public class FidelityFX : HeaderOnlyModule
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        LicenseType = LicenseTypes.MIT;
        LicenseFilePath = "license.txt";

        // Merge third-party modules into engine binary
        BinaryModuleName = "FlaxEngine";
    }

    /// <inheritdoc />
    public override void GetFilesToDeploy(List<string> files)
    {
        base.GetFilesToDeploy(files);

        files.AddRange(Directory.GetFiles(FolderPath, "*.h", SearchOption.AllDirectories));
    }
}
