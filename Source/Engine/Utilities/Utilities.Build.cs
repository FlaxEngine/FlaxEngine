// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// Common utilities module.
/// </summary>
public class Utilities : EngineModule
{
    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.PrivateDependencies.Add("TextureTool");
        options.PrivateDependencies.Add("Graphics");
    }
}
