// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// Physics module.
/// </summary>
public class Physics : EngineModule
{
    /// <summary>
    /// Enables using collisions cooking.
    /// </summary>
    public static bool WithCooking = true;

    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.PrivateDependencies.Add("PhysX");

        if (WithCooking)
        {
            options.PublicDefinitions.Add("COMPILE_WITH_PHYSICS_COOKING");
        }
    }
}
