// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Linq;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// Target that builds standalone, native tests.
/// </summary>
public class FlaxTestsTarget : EngineTarget
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        // Initialize
        OutputName = "FlaxTests";
        ConfigurationName = "Tests";
        IsPreBuilt = false;
        UseSymbolsExports = false;
        Platforms = new[]
        {
            TargetPlatform.Windows,
            TargetPlatform.Linux,
            TargetPlatform.Mac,
        };
        Architectures = new[]
        {
            TargetArchitecture.x64,
        };
        Configurations = new[]
        {
            TargetConfiguration.Development,
        };

        Modules.Remove("Main");
        Modules.Add("Tests");
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
        var testTargetName = "FlaxNativeTests"; // Should this be added to .flaxproj, similarly as "GameTarget" and "EditorTarget"?
        var result = projectTargets.FirstOrDefault(x => x.Name == testTargetName);
        if (result == null)
            throw new Exception(string.Format("Invalid or missing test target {0} specified in project {1} (referenced by project {2}).", testTargetName, project.Name, Project.Name));
        return result;
    }
}
