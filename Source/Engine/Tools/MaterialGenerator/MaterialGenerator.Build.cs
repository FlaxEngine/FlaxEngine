// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// Materials shader code generation utilities module.
/// </summary>
public class MaterialGenerator : EngineModule
{
    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.PublicDefinitions.Add("COMPILE_WITH_MATERIAL_GRAPH");

        options.PublicDependencies.Add("Visject");
    }

    /// <inheritdoc />
    public override void GetFilesToDeploy(List<string> files)
    {
    }
}
