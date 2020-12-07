// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

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
    }
}
