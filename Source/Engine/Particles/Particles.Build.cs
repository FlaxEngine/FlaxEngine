// Copyright (c) Wojciech Figat. All rights reserved.

using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// Particles module.
/// </summary>
public class Particles : EngineModule
{
    /// <summary>
    /// Determinates whenever GPU particles simulation should be enabled for a given build.
    /// </summary>
    /// <param name="options">The options.</param>
    /// <returns>True if use GPU particles, otherwise false.</returns>
    public static bool UseGPU(BuildOptions options)
    {
        // TODO: disable GPU particles support in build for low-end platforms
        return true;
    }

    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.PrivateDependencies.Add("Utilities");
        options.PrivateDependencies.Add("Graphics");

        options.SourcePaths.Clear();
        options.SourceFiles.AddRange(Directory.GetFiles(FolderPath, "*.*", SearchOption.TopDirectoryOnly));
        options.SourcePaths.Add(Path.Combine(FolderPath, "Graph", "CPU"));

        if (options.Target.IsEditor)
        {
            options.PrivateDependencies.Add("ShadersCompilation");

            options.PublicDefinitions.Add("COMPILE_WITH_PARTICLE_GPU_GRAPH");
            options.SourcePaths.Add(Path.Combine(FolderPath, "Graph", "GPU"));

            if (UseGPU(options))
            {
                options.PublicDefinitions.Add("COMPILE_WITH_GPU_PARTICLES");
            }
        }
        else
        {
            if (UseGPU(options))
            {
                options.PublicDefinitions.Add("COMPILE_WITH_GPU_PARTICLES");
                options.SourceFiles.Add(Path.Combine(FolderPath, "Graph", "GPU", "GPUParticles.cpp"));
            }
        }
    }
}
