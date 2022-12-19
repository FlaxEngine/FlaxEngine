// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

using Flax.Build;

/// <summary>
/// FlaxEngine.Tests project build configuration.
/// </summary>
public class FlaxEngineTestsTarget : Target
{
    /// <inheritdoc />
    public FlaxEngineTestsTarget()
    {
        Name = ProjectName = OutputName = "FlaxEngine.Tests";
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
        CustomExternalProjectFilePath = System.IO.Path.Combine(FolderPath, "FlaxEngine.Tests.csproj");
    }
}
