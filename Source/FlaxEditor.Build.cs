// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.IO;
using Flax.Build;
using Flax.Build.Graph;
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

        // Initialize
        IsEditor = true;
        OutputName = "FlaxEditor";
        ConfigurationName = "Editor";
        IsPreBuilt = false;
        Platforms = new[]
        {
            TargetPlatform.Windows,
        };
        Architectures = new[]
        {
            TargetArchitecture.x64,
        };
        GlobalDefinitions.Add("USE_EDITOR");
        Win32ResourceFile = Path.Combine(Globals.EngineRoot, "Source", "FlaxEditor.rc");

        Modules.Add("Editor");
        Modules.Add("CSG");
        Modules.Add("ShadowsOfMordor");
        Modules.Add("ShadersCompilation");
        Modules.Add("ContentExporters");
        Modules.Add("ContentImporters");
    }

    /// <inheritdoc />
    public override void PreBuild(TaskGraph graph, BuildOptions buildOptions)
    {
        base.PreBuild(graph, buildOptions);

        // Register localization dependency
        string locDir = Path.Combine(Globals.EngineRoot, "Source", "Editor", "Localization", "Languages");
        buildOptions.AdvancedDependencies.Add(new BuildOptions.DependencyFileEntry()
        {
            IsFolder = true,
            DestinationNames = new[]
            {
                Path.Combine("Localization")
            },
            SourcePaths = new[]
            {
                locDir
            }
        });
    }

    /// <inheritdoc />
    public override void SetupTargetEnvironment(BuildOptions options)
    {
        base.SetupTargetEnvironment(options);

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
                    default: throw new InvalidArchitectureException(options.Architecture, "Not supported Editor architecture.");
                }
                break;
            default: throw new InvalidPlatformException(options.Platform.Target, "Not supported Editor platform.");
        }
    }
}
