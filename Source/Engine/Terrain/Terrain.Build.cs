// Copyright (c) Wojciech Figat. All rights reserved.

using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// Terrain module.
/// </summary>
public class Terrain : EngineModule
{
    /// <summary>
    /// Enables terrain editing and changing at runtime. If your game doesn't use procedural terrain in game then disable this option to reduce build size.
    /// </summary>
    public static bool WithEditing = true;

    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        if (!WithEditing)
        {
            options.PublicDefinitions.Add("TERRAIN_EDITING=0");
        }

        options.PrivateDependencies.Add("Physics");
        if (options.Target.IsEditor)
        {
            options.PrivateDependencies.Add("ContentImporters");
        }
    }
}
