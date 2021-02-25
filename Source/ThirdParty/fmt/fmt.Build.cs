// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;
using Flax.Build;

/// <summary>
/// https://fmt.dev/
/// </summary>
public class fmt : ThirdPartyModule
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        LicenseType = LicenseTypes.BSD2Clause;
        LicenseFilePath = "cppformat license.txt";

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
