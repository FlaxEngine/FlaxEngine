// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;
using Flax.Build.NativeCpp;

/// <summary>
/// DirectX shaders compiler module using https://github.com/microsoft/DirectXShaderCompiler.
/// </summary>
public class ShaderCompilerDX : ShaderCompiler
{
    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.SourceFiles.Clear();
        options.SourcePaths.Clear();
        options.SourceFiles.Add(Path.Combine(FolderPath, "ShaderCompilerDX.h"));
        options.SourceFiles.Add(Path.Combine(FolderPath, "ShaderCompilerDX.cpp"));

        options.PublicDefinitions.Add("COMPILE_WITH_DX_SHADER_COMPILER");

        var depsRoot = options.DepsFolder;
        options.OutputFiles.Add(Path.Combine(depsRoot, "dxcompiler.lib"));
        options.DependencyFiles.Add(Path.Combine(depsRoot, "dxcompiler.dll"));
        options.DependencyFiles.Add(Path.Combine(depsRoot, "dxil.dll"));
        options.DelayLoadLibraries.Add("dxcompiler.dll");
    }

    /// <inheritdoc />
    public override void GetFilesToDeploy(List<string> files)
    {
        base.GetFilesToDeploy(files);

        files.Add(Path.Combine(FolderPath, "ShaderCompilerDX.h"));
    }
}
