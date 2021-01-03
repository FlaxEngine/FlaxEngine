// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// https://github.com/KhronosGroup/SPIRV-Tools
/// </summary>
public class spirv_tools : DepsModule
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        Name = "spirv-tools";
        LicenseType = LicenseTypes.Custom;
        LicenseFilePath = "LICENSE.txt";
    }

    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        var depsRoot = options.DepsFolder;
        options.OutputFiles.Add(Path.Combine(depsRoot, "SPIRV-Tools.lib"));
        options.OutputFiles.Add(Path.Combine(depsRoot, "SPIRV-Tools-opt.lib"));
    }
}
