using System.Collections.Generic;
using System.IO;
using Flax.Build;

/// <summary>
/// https://github.com/redorav/ddspp
/// </summary>
public class ddspp : HeaderOnlyModule
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        LicenseType = LicenseTypes.MIT;
        LicenseFilePath = "LICENSE.txt";
    }
}