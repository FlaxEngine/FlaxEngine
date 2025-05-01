// Copyright (c) Wojciech Figat. All rights reserved.

using Flax.Build;

/// <summary>
/// https://github.com/zeux/meshoptimizer
/// </summary>
public class meshoptimizer : ThirdPartyModule
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        LicenseType = LicenseTypes.MIT;
        LicenseFilePath = "LICENSE.md";

        // Merge third-party modules into engine binary
        BinaryModuleName = "FlaxEngine";
    }
}
