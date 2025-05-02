// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// Engine tests module.
/// </summary>
public class Tests : EngineModule
{
    /// <inheritdoc />
    public Tests()
    {
        Deploy = false;
    }

    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.PrivateDependencies.Add("ModelTool");
    }

    /// <inheritdoc />
    public override void GetFilesToDeploy(List<string> files)
    {
    }
}
