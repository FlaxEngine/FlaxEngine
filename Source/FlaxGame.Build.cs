// Copyright (c) Wojciech Figat. All rights reserved.

using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// The Flax Game target that builds a standalone application to run cooked game project.
/// </summary>
public class FlaxGame : EngineTarget
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        OutputName = "FlaxGame";
        ConfigurationName = "Game";
        IsPreBuilt = false;
        IsMonolithicExecutable = false;
    }

    /// <inheritdoc />
    public override TargetArchitecture[] GetArchitectures(TargetPlatform platform)
    {
        // No support for x86 on UWP
        if (platform == TargetPlatform.UWP)
            return new[] { TargetArchitecture.x64 };

        return base.GetArchitectures(platform);
    }

    /// <inheritdoc />
    public override string GetOutputFilePath(BuildOptions options, TargetOutputType? outputType)
    {
        // Override output name on UWP
        if (options.Platform.Target == TargetPlatform.UWP)
        {
            return Path.Combine(options.OutputFolder, "FlaxEngine.dll");
        }

        return base.GetOutputFilePath(options, outputType);
    }

    /// <inheritdoc />
    public override void SetupTargetEnvironment(BuildOptions options)
    {
        base.SetupTargetEnvironment(options);

        // Build engine as DLL to be linked in the game UWP project
        if (options.Platform.Target == TargetPlatform.UWP)
        {
            options.LinkEnv.Output = LinkerOutput.SharedLibrary;
            options.LinkEnv.GenerateWindowsMetadata = true;
            options.LinkEnv.GenerateDocumentation = true;
        }

        var platformName = options.Platform.Target.ToString();
        var architectureName = options.Architecture.ToString();
        var configurationName = options.Configuration.ToString();
        options.OutputFolder = Path.Combine(Globals.EngineRoot, "Source", "Platforms", platformName, "Binaries", "Game", architectureName, configurationName);
    }
}
