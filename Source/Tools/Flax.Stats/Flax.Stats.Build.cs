// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

using Flax.Build;

/// <summary>
/// Flax.Stats tool project build configuration.
/// </summary>
public class FlaxStatsTarget : Target
{
    /// <inheritdoc />
    public FlaxStatsTarget()
    {
        Name = ProjectName = OutputName = "Flax.Stats";
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
            TargetPlatform.Linux,
            TargetPlatform.Mac,
        };
        Configurations = new[]
        {
            TargetConfiguration.Debug,
            TargetConfiguration.Release,
        };
        CustomExternalProjectFilePath = System.IO.Path.Combine(FolderPath, "Flax.Stats.csproj");
    }
}
