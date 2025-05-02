// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// Debugging module.
/// </summary>
public class Debug : EngineModule
{
    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        if (options.Target.IsEditor)
        {
            options.PublicDefinitions.Add("COMPILE_WITH_DEBUG_DRAW");
        }
    }

    /// <inheritdoc />
    public override void GetFilesToDeploy(List<string> files)
    {
        files.AddRange(Directory.GetFiles(FolderPath, "*.h", SearchOption.TopDirectoryOnly));
    }
}
