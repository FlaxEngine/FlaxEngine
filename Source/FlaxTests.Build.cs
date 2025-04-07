// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Linq;
using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// Target that builds standalone, native tests.
/// </summary>
public class FlaxTestsTarget : FlaxEditor
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        // Initialize
        OutputName = "FlaxTests";
        ConfigurationName = "Tests";
        IsPreBuilt = false;
        UseSymbolsExports = true;
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
        GlobalDefinitions.Add("FLAX_TESTS");
        Win32ResourceFile = null;

        Modules.Add("Tests");
    }

    /// <inheritdoc />
    public override void SetupTargetEnvironment(BuildOptions options)
    {
        base.SetupTargetEnvironment(options);

        // Setup C# scripts environment
        options.ScriptingAPI.IgnoreMissingDocumentationWarnings = true;
        options.ScriptingAPI.Defines.Add("FLAX_TESTS");
        var nunitFramework = Path.Combine(Globals.EngineRoot, "Source/Platforms/DotNet/NUnit/framework/nunit.framework.dll");
        options.ScriptingAPI.FileReferences.Add(nunitFramework);

        // Produce console program
        options.LinkEnv.LinkAsConsoleProgram = true;
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
