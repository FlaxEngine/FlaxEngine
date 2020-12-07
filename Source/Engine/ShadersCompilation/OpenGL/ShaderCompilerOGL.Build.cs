// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using Flax.Build.NativeCpp;

/// <summary>
/// OpenGL shaders compiler module.
/// </summary>
public class ShaderCompilerOGL : ShaderCompiler
{
    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.PublicDefinitions.Add("COMPILE_WITH_OGL_SHADER_COMPILER");
    }
}
