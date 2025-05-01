// Copyright (c) Wojciech Figat. All rights reserved.

using Flax.Build;

/// <summary>
/// https://github.com/hglm/detex
/// </summary>
public class detex : ThirdPartyModule
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
}
