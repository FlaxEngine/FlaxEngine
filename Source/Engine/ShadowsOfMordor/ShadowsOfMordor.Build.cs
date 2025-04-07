// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// Lightmaps baking module.
/// </summary>
public class ShadowsOfMordor : EngineModule
{
    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.PublicDefinitions.Add("COMPILE_WITH_GI_BAKING");

        options.PrivateDependencies.Add("ContentImporters");
    }

    /// <inheritdoc />
    public override void GetFilesToDeploy(List<string> files)
    {
    }
}
