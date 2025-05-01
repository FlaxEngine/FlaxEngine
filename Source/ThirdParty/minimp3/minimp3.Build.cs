// Copyright (c) Wojciech Figat. All rights reserved.

using Flax.Build;

/// <summary>
/// https://github.com/lieff/minimp3
/// </summary>
public class minimp3 : HeaderOnlyModule
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
