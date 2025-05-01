// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// Texture utilities module.
/// </summary>
public class TextureTool : EngineModule
{
    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.SourcePaths.Clear();
        options.SourceFiles.Add(Path.Combine(FolderPath, "TextureTool.cpp"));
        options.SourceFiles.Add(Path.Combine(FolderPath, "TextureTool.h"));

        bool useDirectXTex = false;
        bool useStb = false;
        bool useExr = options.Target.IsEditor;

        switch (options.Platform.Target)
        {
        case TargetPlatform.Windows:
        case TargetPlatform.UWP:
        case TargetPlatform.XboxOne:
        case TargetPlatform.XboxScarlett:
            useDirectXTex = true;
            break;
        case TargetPlatform.Linux:
        case TargetPlatform.PS4:
        case TargetPlatform.PS5:
        case TargetPlatform.Android:
        case TargetPlatform.Switch:
        case TargetPlatform.Mac:
        case TargetPlatform.iOS:
            useStb = true;
            break;
        default: throw new InvalidPlatformException(options.Platform.Target);
        }

        if (useDirectXTex)
        {
            options.PrivateDependencies.Add("DirectXTex");
            options.SourceFiles.Add(Path.Combine(FolderPath, "TextureTool.DirectXTex.cpp"));
        }
        if (useStb)
        {
            options.PrivateDependencies.Add("stb");
            options.SourceFiles.Add(Path.Combine(FolderPath, "TextureTool.stb.cpp"));
            if (options.Target.IsEditor)
            {
                // Use helper lib for decompression
                options.PrivateDependencies.Add("detex");
                options.PrivateDependencies.Add("bc7enc16");
            }
        }
        if (useExr)
        {
            options.PrivateDependencies.Add("tinyexr");
        }
        if (options.Target.IsEditor && astc.IsSupported(options))
        {
            // ASTC for mobile (iOS and Android)
            options.SourceFiles.Add(Path.Combine(FolderPath, "TextureTool.astc.cpp"));
            options.PrivateDependencies.Add("astc");
        }

        options.PublicDefinitions.Add("COMPILE_WITH_TEXTURE_TOOL");
    }

    /// <inheritdoc />
    public override void GetFilesToDeploy(List<string> files)
    {
        files.Add(Path.Combine(FolderPath, "TextureTool.h"));
    }
}
