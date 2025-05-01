// Copyright (c) Wojciech Figat. All rights reserved.

using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// Terrain module.
/// </summary>
public class Terrain : EngineModule
{
    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.PrivateDependencies.Add("Physics");

        if (options.Target.IsEditor)
        {
            options.PrivateDependencies.Add("ContentImporters");
        }
    }
}
