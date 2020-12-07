// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using Flax.Build;

/// <summary>
/// https://github.com/lz4/lz4
/// </summary>
public class lz4 : ThirdPartyModule
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        LicenseType = LicenseTypes.BSD2Clause;
    }
}
