// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// Content module.
/// </summary>
public class Content : EngineModule
{
    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.PrivateDependencies.Add("lz4");
        options.PrivateDependencies.Add("AudioTool");
        options.PrivateDependencies.Add("TextureTool");
        options.PrivateDependencies.Add("ModelTool");
        options.PrivateDependencies.Add("Particles");

        if (options.Target.IsEditor)
        {
            options.PrivateDependencies.Add("ShadersCompilation");
            options.PrivateDependencies.Add("MaterialGenerator");
            options.PrivateDependencies.Add("ContentImporters");
            options.PrivateDependencies.Add("ContentExporters");
            options.PrivateDependencies.Add("Graphics");
        }
    }

    /// <inheritdoc />
    public override void GetFilesToDeploy(List<string> files)
    {
        files.AddRange(Directory.GetFiles(FolderPath, "*.h", SearchOption.TopDirectoryOnly));
        files.AddRange(Directory.GetFiles(Path.Combine(FolderPath, "Assets"), "*.h", SearchOption.TopDirectoryOnly));
        files.AddRange(Directory.GetFiles(Path.Combine(FolderPath, "Cache"), "*.h", SearchOption.TopDirectoryOnly));
        files.AddRange(Directory.GetFiles(Path.Combine(FolderPath, "Factories"), "*.h", SearchOption.TopDirectoryOnly));
        files.AddRange(Directory.GetFiles(Path.Combine(FolderPath, "Storage"), "*.h", SearchOption.TopDirectoryOnly));
        files.Add(Path.Combine(FolderPath, "Upgraders/BinaryAssetUpgrader.h"));
        files.Add(Path.Combine(FolderPath, "Upgraders/IAssetUpgrader.h"));
    }
}
