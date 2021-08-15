// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Linq;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// The Flax Editor target that builds standalone tests.
/// </summary>
public class FlaxNativeTestsTarget : EngineTarget
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        // Initialize
        OutputName = "FlaxNativeTests";
        ConfigurationName = "Tests";
        Platforms = new[]
        {
            TargetPlatform.Windows,
            TargetPlatform.Linux,
        };
        Architectures = new[]
        {
            TargetArchitecture.x64,
            TargetArchitecture.x86,
        };

        Modules.Remove("Main");
        Modules.Add("TestsMain");
    }

    /// <inheritdoc />
    public override void SetupTargetEnvironment(BuildOptions options)
    {
        base.SetupTargetEnvironment(options);

        options.LinkEnv.LinkAsConsoleProgram = true;

        // Setup output folder for Test binaries
        var platformName = options.Platform.Target.ToString();
        var architectureName = options.Architecture.ToString();
        var configurationName = options.Configuration.ToString();
        options.OutputFolder = Path.Combine(options.WorkingDirectory, "Binaries", "Tests", platformName, architectureName, configurationName);
    }

    /// <inheritdoc />
    public override Target SelectReferencedTarget(ProjectInfo project, Target[] projectTargets)
    {
        var testTargetName = "FlaxNativeTests";
        var result = projectTargets.FirstOrDefault(x => x.Name == testTargetName);
        if (result == null)
            throw new Exception("Invalid or missing test target {testTargetName} specified in project {project.Name} (referenced by project {Project.Name}).");
        return result;
    }
}
