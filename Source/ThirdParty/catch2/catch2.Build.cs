// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;
using Flax.Build;

/// <summary>
/// https://github.com/catchorg/Catch2
/// </summary>
public class catch2 : HeaderOnlyModule
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        LicenseType = LicenseTypes.BoostSoftwareLicense;
        LicenseFilePath = "LICENSE.txt";
    }
}
