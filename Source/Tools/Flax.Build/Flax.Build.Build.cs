// Copyright (c) Wojciech Figat. All rights reserved.

using Flax.Build;

/// <summary>
/// Flax.Build tool build target configuration.
/// </summary>
public class FlaxBuildTarget : Target
{
    /// <inheritdoc />
    public FlaxBuildTarget()
    {
        Name = ProjectName = OutputName = "Flax.Build";
    }

    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        IsPreBuilt = false;
        Type = TargetType.DotNetCore;
        OutputType = TargetOutputType.Library;
        Platforms = new[]
        {
            Flax.Build.Platform.BuildPlatform.Target,
        };
        Configurations = new[]
        {
            TargetConfiguration.Debug,
            TargetConfiguration.Release,
        };
        CustomExternalProjectFilePath = System.IO.Path.Combine(FolderPath, "Flax.Build.csproj");
    }
}
