// Copyright (c) Wojciech Figat. All rights reserved.

using System;
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

    /// <summary>
    /// Enables using vehicles simulation.
    /// </summary>
    public static bool WithVehicle = true;

    /// <summary>
    /// Enables using cloth simulation.
    /// </summary>
    public static bool WithCloth = true;

    /// <summary>
    /// Enables using PhysX library. Can be overriden by SetupPhysicsBackend.
    /// </summary>
    public static bool WithPhysX = true;

    /// <summary>
    /// Physics system extension event to override for custom physics backend plugin.
    /// </summary>
    public static Action<Physics, BuildOptions> SetupPhysicsBackend = SetupPhysicsBackendPhysX;

    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        SetupPhysicsBackend(this, options);

        if (WithCooking)
        {
            options.PublicDefinitions.Add("COMPILE_WITH_PHYSICS_COOKING");
        }
    }

    private static void SetupPhysicsBackendPhysX(Physics physics, BuildOptions options)
    {
        if (WithPhysX)
        {
            options.PrivateDependencies.Add("PhysX");
            if (WithCloth && options.Platform.Target != TargetPlatform.PS4) // TODO: build nvcloth for ps4 with vs2017
                options.PrivateDependencies.Add("NvCloth");
        }
        else
        {
            options.PrivateDefinitions.Add("COMPILE_WITH_EMPTY_PHYSICS");
        }
    }
}
