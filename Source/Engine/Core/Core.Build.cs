// Copyright (c) Wojciech Figat. All rights reserved.

using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// Core module.
/// </summary>
public class Core : EngineModule
{
    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        // Don't reference anything from default modules
        options.PrivateDependencies.Clear();

        options.PublicDependencies.Add("Platform");
        options.PublicDependencies.Add("fmt");

        if (Profiler.Use(options))
            options.PublicDependencies.Add("Profiler");
    }
}
