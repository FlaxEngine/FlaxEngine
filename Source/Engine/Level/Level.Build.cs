// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// Level module.
/// </summary>
public class Level : EngineModule
{
    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.PrivateDependencies.Add("Physics");
        options.PrivateDependencies.Add("Content");
        options.PrivateDependencies.Add("Graphics");
        options.PrivateDependencies.Add("Renderer");
        options.PrivateDependencies.Add("Particles");

        options.PublicDependencies.Add("Scripting");
        options.PublicDependencies.Add("Serialization");

        if (options.Target.IsEditor)
        {
            options.PrivateDependencies.Add("ContentImporters");

            options.PublicDependencies.Add("CSG");
        }
    }
}
