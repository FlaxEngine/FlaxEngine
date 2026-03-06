// Copyright (c) Wojciech Figat. All rights reserved.

using Flax.Build;

/// <summary>
/// https://www.nuget.org/packages/Microsoft.Direct3D.D3D12
/// </summary>
public class DirectX12Agility : EngineDepsModule
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        LicenseType = LicenseTypes.MIT;
        LicenseFilePath = "LICENSE.txt";
    }
}
