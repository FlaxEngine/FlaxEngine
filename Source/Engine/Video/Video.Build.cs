// Copyright (c) Wojciech Figat. All rights reserved.

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
        case TargetPlatform.Mac:
        case TargetPlatform.iOS:
            // AVFoundation
            options.SourcePaths.Add(Path.Combine(FolderPath, "AV"));
            options.CompileEnv.PreprocessorDefinitions.Add("VIDEO_API_AV");
            break;
        case TargetPlatform.PS4:
            options.SourcePaths.Add(Path.Combine(Globals.EngineRoot, "Source", "Platforms", "PS4", "Engine", "Video"));
            options.CompileEnv.PreprocessorDefinitions.Add("VIDEO_API_PS4");
            break;
        case TargetPlatform.PS5:
            options.SourcePaths.Add(Path.Combine(Globals.EngineRoot, "Source", "Platforms", "PS5", "Engine", "Video"));
            options.CompileEnv.PreprocessorDefinitions.Add("VIDEO_API_PS5");
            break;
        case TargetPlatform.Switch:
            options.SourcePaths.Add(Path.Combine(Globals.EngineRoot, "Source", "Platforms", "Switch", "Engine", "Video"));
            options.CompileEnv.PreprocessorDefinitions.Add("VIDEO_API_SWITCH");
            break;
        case TargetPlatform.Android:
            options.SourcePaths.Add(Path.Combine(FolderPath, "Android"));
            options.CompileEnv.PreprocessorDefinitions.Add("VIDEO_API_ANDROID");
            break;
        }
    }

    /// <inheritdoc />
    public override void GetFilesToDeploy(List<string> files)
    {
        files.AddRange(Directory.GetFiles(FolderPath, "*.h", SearchOption.TopDirectoryOnly));
    }
}
