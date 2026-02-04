// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using Flax.Build;

/// <summary>
/// https://github.com/lieff/minimp4
/// </summary>
public class minimp4 : HeaderOnlyModule
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        LicenseType = LicenseTypes.CreativeCommonsZero;
        LicenseFilePath = "LICENSE";

        // Merge third-party modules into engine binary
        BinaryModuleName = "FlaxEngine";
    }
}
