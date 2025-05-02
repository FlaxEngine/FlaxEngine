// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;
using Flax.Build.NativeCpp;

/// <summary>
/// DirectX shaders compiler module using D3DCompiler.
/// </summary>
public class ShaderCompilerD3D : ShaderCompiler
{
    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.SourceFiles.Clear();
        options.SourcePaths.Clear();
        options.SourceFiles.Add(Path.Combine(FolderPath, "ShaderCompilerD3D.h"));
        options.SourceFiles.Add(Path.Combine(FolderPath, "ShaderCompilerD3D.cpp"));

        options.PublicDefinitions.Add("COMPILE_WITH_D3D_SHADER_COMPILER");

        var depsRoot = options.DepsFolder;
        options.OutputFiles.Add("dxguid.lib");
        options.OutputFiles.Add("d3dcompiler.lib");
        options.DependencyFiles.Add(Path.Combine(depsRoot, "d3dcompiler_47.dll"));
        options.DelayLoadLibraries.Add("d3dcompiler_47.dll");
    }

    /// <inheritdoc />
    public override void GetFilesToDeploy(List<string> files)
    {
        base.GetFilesToDeploy(files);

        files.Add(Path.Combine(FolderPath, "ShaderCompilerD3D.h"));
    }
}
