// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// Audio data utilities module.
/// </summary>
public class AudioTool : EngineModule
{
    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        // TODO: convert into private deps
        options.PublicDependencies.Add("minimp3");
        options.PublicDependencies.Add("ogg");
        options.PublicDependencies.Add("vorbis");

        options.PublicDefinitions.Add("COMPILE_WITH_AUDIO_TOOL");
        options.PublicDefinitions.Add("COMPILE_WITH_OGG_VORBIS");
    }

    /// <inheritdoc />
    public override void GetFilesToDeploy(List<string> files)
    {
    }
}
