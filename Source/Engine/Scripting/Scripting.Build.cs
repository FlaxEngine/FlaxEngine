// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// Scripting module.
/// </summary>
public class Scripting : EngineModule
{
    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        if (EngineConfiguration.WithCSharp(options))
        {
            options.PublicDependencies.Add("mono");
        }

        options.PrivateDependencies.Add("Utilities");
    }
}
