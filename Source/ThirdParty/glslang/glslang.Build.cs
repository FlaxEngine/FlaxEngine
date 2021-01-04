// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// https://github.com/KhronosGroup/glslang
/// </summary>
public class glslang : DepsModule
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        LicenseType = LicenseTypes.Custom;
        LicenseFilePath = "LICENSE.txt";
    }

    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        var depsRoot = options.DepsFolder;
        options.OutputFiles.Add(Path.Combine(depsRoot, "GenericCodeGen.lib"));
        options.OutputFiles.Add(Path.Combine(depsRoot, "glslang.lib"));
        options.OutputFiles.Add(Path.Combine(depsRoot, "HLSL.lib"));
        options.OutputFiles.Add(Path.Combine(depsRoot, "MachineIndependent.lib"));
        options.OutputFiles.Add(Path.Combine(depsRoot, "OSDependent.lib"));
        options.OutputFiles.Add(Path.Combine(depsRoot, "OGLCompiler.lib"));
        options.OutputFiles.Add(Path.Combine(depsRoot, "SPIRV.lib"));
    }
}
