// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using Flax.Build;

/// <summary>
/// https://github.com/nem0/OpenFBX
/// </summary>
public class OpenFBX : ThirdPartyModule
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
}
