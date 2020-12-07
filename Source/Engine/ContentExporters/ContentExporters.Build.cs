// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// Content exporting module.
/// </summary>
public class ContentExporters : EngineModule
{
    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.PrivateDependencies.Add("AudioTool");
        options.PrivateDependencies.Add("TextureTool");

        options.PublicDefinitions.Add("COMPILE_WITH_ASSETS_EXPORTER");
    }

    /// <inheritdoc />
    public override void GetFilesToDeploy(List<string> files)
    {
    }
}
