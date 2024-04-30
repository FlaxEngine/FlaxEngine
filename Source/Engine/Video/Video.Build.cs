// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// Video module.
/// </summary>
public class Video : EngineModule
{
    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.SourcePaths.Clear();
        options.SourceFiles.AddRange(Directory.GetFiles(FolderPath, "*.*", SearchOption.TopDirectoryOnly));

        switch (options.Platform.Target)
        {
        case TargetPlatform.Windows:
        case TargetPlatform.UWP:
        case TargetPlatform.XboxOne:
        case TargetPlatform.XboxScarlett:
            // Media Foundation
            options.SourcePaths.Add(Path.Combine(FolderPath, "MF"));
            options.CompileEnv.PreprocessorDefinitions.Add("VIDEO_API_MF");
            options.OutputFiles.Add("mf.lib");
            options.OutputFiles.Add("mfcore.lib");
            options.OutputFiles.Add("mfplat.lib");
            options.OutputFiles.Add("mfplay.lib");
            options.OutputFiles.Add("mfreadwrite.lib");
            options.OutputFiles.Add("mfuuid.lib");
            break;
        }
    }

    /// <inheritdoc />
    public override void GetFilesToDeploy(List<string> files)
    {
        files.AddRange(Directory.GetFiles(FolderPath, "*.h", SearchOption.TopDirectoryOnly));
    }
}
