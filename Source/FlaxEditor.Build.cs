// Copyright (c) Wojciech Figat. All rights reserved.

using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// The Flax Editor target that builds a standalone editor application.
/// </summary>
public class FlaxEditor : EngineTarget
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        IsEditor = true;
        OutputName = "FlaxEditor";
        ConfigurationName = "Editor";
        IsPreBuilt = false;
        Platforms = new[]
        {
            TargetPlatform.Windows,
            TargetPlatform.Linux,
            TargetPlatform.Mac,
        };
        Architectures = new[]
        {
            TargetArchitecture.x64,
            TargetArchitecture.ARM64,
        };
        GlobalDefinitions.Add("USE_EDITOR");

        Modules.Add("Editor");
        Modules.Add("CSG");
        Modules.Add("ShadowsOfMordor");
        Modules.Add("ShadersCompilation");
        Modules.Add("ContentExporters");
        Modules.Add("ContentImporters");
    }

    /// <inheritdoc />
    public override void SetupTargetEnvironment(BuildOptions options)
    {
        base.SetupTargetEnvironment(options);

        // Setup output folder for Editor binaries
        switch (options.Platform.Target)
        {
        case TargetPlatform.Windows:
            switch (options.Architecture)
            {
            case TargetArchitecture.x64:
                options.OutputFolder = Path.Combine(options.WorkingDirectory, "Binaries", "Editor", "Win64", options.Configuration.ToString());
                break;
            case TargetArchitecture.x86:
                options.OutputFolder = Path.Combine(options.WorkingDirectory, "Binaries", "Editor", "Win32", options.Configuration.ToString());
                break;
            case TargetArchitecture.ARM64:
                options.OutputFolder = Path.Combine(options.WorkingDirectory, "Binaries", "Editor", "ARM64", options.Configuration.ToString());
                break;
            default: throw new InvalidArchitectureException(options.Architecture, "Not supported Editor architecture.");
            }
            break;
        case TargetPlatform.Linux:
            options.OutputFolder = Path.Combine(options.WorkingDirectory, "Binaries", "Editor", "Linux", options.Configuration.ToString());
            options.DependencyFiles.Add(Path.Combine(Globals.EngineRoot, "Source", "Logo.png"));
            break;
        case TargetPlatform.Mac:
            options.OutputFolder = Path.Combine(options.WorkingDirectory, "Binaries", "Editor", "Mac", options.Configuration.ToString());
            break;
        default: throw new InvalidPlatformException(options.Platform.Target, "Not supported Editor platform.");
        }
    }
}
