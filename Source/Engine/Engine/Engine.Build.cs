// Copyright (c) Wojciech Figat. All rights reserved.

using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// Engine module.
/// </summary>
public class Engine : EngineModule
{
    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.PublicDependencies.Add("AI");
        options.PublicDependencies.Add("Animations");
        options.PublicDependencies.Add("Audio");
        options.PublicDependencies.Add("Video");
        options.PublicDependencies.Add("Content");
        options.PublicDependencies.Add("Debug");
        options.PublicDependencies.Add("Foliage");
        options.PublicDependencies.Add("Graphics");
        options.PublicDependencies.Add("Renderer");
        options.PublicDependencies.Add("Render2D");
        options.PublicDependencies.Add("Input");
        options.PublicDependencies.Add("Level");
        options.PublicDependencies.Add("Navigation");
        options.PublicDependencies.Add("Networking");
        options.PublicDependencies.Add("Physics");
        options.PublicDependencies.Add("Particles");
        options.PublicDependencies.Add("Scripting");
        options.PublicDependencies.Add("Serialization");
        options.PublicDependencies.Add("Streaming");
        options.PublicDependencies.Add("Terrain");
        options.PublicDependencies.Add("Threading");
        options.PublicDependencies.Add("UI");
        options.PublicDependencies.Add("Utilities");
        options.PublicDependencies.Add("Visject");
        options.PublicDependencies.Add("Localization");
        options.PublicDependencies.Add("Online");

        // Use source folder per platform group
        switch (options.Platform.Target)
        {
        case TargetPlatform.PS4:
            options.SourcePaths.Add(Path.Combine(Globals.EngineRoot, "Source", "Platforms", "PS4", "Engine", "Engine"));
            break;
        case TargetPlatform.PS5:
            options.SourcePaths.Add(Path.Combine(Globals.EngineRoot, "Source", "Platforms", "PS5", "Engine", "Engine"));
            break;
        case TargetPlatform.Switch:
            options.SourcePaths.Add(Path.Combine(Globals.EngineRoot, "Source", "Platforms", "Switch", "Engine", "Engine"));
            break;
        }
    }
}
