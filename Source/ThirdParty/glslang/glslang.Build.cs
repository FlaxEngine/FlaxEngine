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

        // Merge third-party modules into engine binary
        BinaryModuleName = "FlaxEngine";
    }

    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        var depsRoot = options.DepsFolder;
        switch (options.Platform.Target)
        {
        case TargetPlatform.Windows:
            options.OutputFiles.Add(Path.Combine(depsRoot, "GenericCodeGen.lib"));
            options.OutputFiles.Add(Path.Combine(depsRoot, "glslang.lib"));
            options.OutputFiles.Add(Path.Combine(depsRoot, "HLSL.lib"));
            options.OutputFiles.Add(Path.Combine(depsRoot, "MachineIndependent.lib"));
            options.OutputFiles.Add(Path.Combine(depsRoot, "OSDependent.lib"));
            options.OutputFiles.Add(Path.Combine(depsRoot, "OGLCompiler.lib"));
            options.OutputFiles.Add(Path.Combine(depsRoot, "SPIRV.lib"));
            break;
        case TargetPlatform.Linux:
            options.OutputFiles.Add(Path.Combine(depsRoot, "libGenericCodeGen.a"));
            options.OutputFiles.Add(Path.Combine(depsRoot, "libglslang.a"));
            options.OutputFiles.Add(Path.Combine(depsRoot, "libHLSL.a"));
            options.OutputFiles.Add(Path.Combine(depsRoot, "libMachineIndependent.a"));
            options.OutputFiles.Add(Path.Combine(depsRoot, "libOSDependent.a"));
            options.OutputFiles.Add(Path.Combine(depsRoot, "libOGLCompiler.a"));
            options.OutputFiles.Add(Path.Combine(depsRoot, "libSPIRV.a"));
            break;
        default: throw new InvalidPlatformException(options.Platform.Target);
        }
    }
}
