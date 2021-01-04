// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// Navigation module.
/// </summary>
public class Navigation : EngineModule
{
    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.PublicDefinitions.Add("COMPILE_WITH_NAV_MESH_BUILDER");

        options.PrivateDependencies.Add("Level");
        options.PrivateDependencies.Add("recastnavigation");

        if (options.Target.IsEditor)
        {
            options.PrivateDependencies.Add("ContentImporters");
        }
    }
}
