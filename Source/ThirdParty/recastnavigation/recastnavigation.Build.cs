// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using Flax.Build;

/// <summary>
/// https://github.com/recastnavigation/recastnavigation
/// </summary>
public class recastnavigation : ThirdPartyModule
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        LicenseType = LicenseTypes.zLib;
        LicenseFilePath = "License.txt";

        // Merge third-party modules into engine binary
        BinaryModuleName = "FlaxEngine";
    }
}
