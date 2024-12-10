// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using Flax.Build;

/// <summary>
/// https://github.com/microsoft/PixEvents
/// </summary>
public class WinPixEventRuntime : HeaderOnlyModule
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
