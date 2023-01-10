// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

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
            if (EngineConfiguration.WithDotNet(options))
                options.PublicDependencies.Add("nethost");
            else
                options.PublicDependencies.Add("mono");
        }

        options.PrivateDependencies.Add("Utilities");
    }
}
