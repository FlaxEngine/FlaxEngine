// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using Flax.Build;

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

        Type = TargetType.DotNet;
        OutputType = TargetOutputType.Library;
        Platforms = new[]
        {
            TargetPlatform.Windows,
        };
        Configurations = new[]
        {
            TargetConfiguration.Debug,
            TargetConfiguration.Release,
        };
        CustomExternalProjectFilePath = System.IO.Path.Combine(FolderPath, "Flax.Build.Tests.csproj");
    }
}
