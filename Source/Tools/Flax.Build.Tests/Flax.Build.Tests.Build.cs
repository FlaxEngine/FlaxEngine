// Copyright (c) Wojciech Figat. All rights reserved.

using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// Flax.Build.Tests project build configuration.
/// </summary>
public class FlaxBuildTestsTarget : Target
{
    /// <inheritdoc />
    public FlaxBuildTestsTarget()
    {
        Name = ProjectName = OutputName = "Flax.Build.Tests";
    }

    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        Type = TargetType.DotNetCore;
        OutputType = TargetOutputType.Library;
        Platforms = new[]
        {
            TargetPlatform.Windows,
            TargetPlatform.Linux,
            TargetPlatform.Mac,
        };
        Configurations = new[]
        {
            TargetConfiguration.Debug,
            TargetConfiguration.Release,
        };
        CustomExternalProjectFilePath = System.IO.Path.Combine(FolderPath, "Flax.Build.Tests.csproj");
    }

    /// <inheritdoc />
    public override void SetupTargetEnvironment(BuildOptions options)
    {
        base.SetupTargetEnvironment(options);

        options.CompileEnv.EnableExceptions = true;
    }
}
