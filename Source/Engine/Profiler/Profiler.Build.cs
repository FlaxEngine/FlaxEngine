// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// Profiling tools module.
/// </summary>
public class Profiler : EngineModule
{
    /// <summary>
    /// Determinates whenever performance profiling tools should be enabled for a given build.
    /// </summary>
    /// <param name="options">The options.</param>
    /// <returns>True if use profiler, otherwise false.</returns>
    public static bool Use(BuildOptions options)
    {
        return options.Configuration != TargetConfiguration.Release || options.Target.IsEditor;
    }

    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        // Don't ref Core module
        options.PrivateDependencies.Clear();

        options.PublicDefinitions.Add("COMPILE_WITH_PROFILER");

        // Tracy profiling tools
        switch (options.Platform.Target)
        {
        case TargetPlatform.Android: // dlopen failed: cannot locate symbol `tracy::ScopedZone::~ScopedZone()` referenced by "libFlaxGame.so"
        case TargetPlatform.Linux:
        case TargetPlatform.Windows:
            options.PublicDependencies.Add("tracy");
            break;
        }
    }
}
