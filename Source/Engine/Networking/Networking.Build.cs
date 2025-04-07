// Copyright (c) Wojciech Figat. All rights reserved.

using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// Networking module.
/// </summary>
public class Networking : EngineModule
{
    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        Tags["Network"] = string.Empty;
        options.PublicDefinitions.Add("COMPILE_WITH_NETWORKING");
    }
}
