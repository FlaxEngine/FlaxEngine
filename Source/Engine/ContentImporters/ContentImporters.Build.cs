// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// Content importing module.
/// </summary>
public class ContentImporters : EngineModule
{
    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.PublicDependencies.Add("AudioTool");
        options.PublicDependencies.Add("ModelTool");
        options.PublicDependencies.Add("TextureTool");

        // TODO: convert into private deps and fix CreateCollisionData to not include physics?
        options.PublicDependencies.Add("Physics");

        options.PrivateDependencies.Add("Graphics");
        options.PrivateDependencies.Add("Particles");

        options.PublicDefinitions.Add("COMPILE_WITH_ASSETS_IMPORTER");
        options.PublicDefinitions.Add("COMPILE_WITH_MATERIAL_GRAPH");
    }

    /// <inheritdoc />
    public override void GetFilesToDeploy(List<string> files)
    {
        files.Add(Path.Combine(FolderPath, "AssetsImportingManager.h"));
        files.Add(Path.Combine(FolderPath, "Types.h"));
    }
}
