// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Linq;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// Target that builds standalone, native tests.
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

        // TODO: All platforms would benefit from the tests.
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
        var testTargetName = "FlaxNativeTests"; // Should this be added to .flaxproj, similarly as "GameTarget" and "EditorTarget"?
        var result = projectTargets.FirstOrDefault(x => x.Name == testTargetName);
        if (result == null)
            // Apparently .NET compiler that is used for building .Build.cs files is different that one used for building Flax.Build itself. String interpolation ($) does't work here.
            throw new Exception(string.Format("Invalid or missing test target {0} specified in project {1} (referenced by project {2}).", testTargetName, project.Name, Project.Name));
        return result;
    }
}
