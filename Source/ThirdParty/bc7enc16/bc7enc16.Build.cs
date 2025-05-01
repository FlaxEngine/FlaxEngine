// Copyright (c) Wojciech Figat. All rights reserved.

using Flax.Build;

/// <summary>
/// https://github.com/richgel999/bc7enc16
/// </summary>
public class bc7enc16 : ThirdPartyModule
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
