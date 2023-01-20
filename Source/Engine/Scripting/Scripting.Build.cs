// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// Scripting module.
/// </summary>
public class Scripting : EngineModule
{
    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        if (EngineConfiguration.WithCSharp(options))
        {
            if (EngineConfiguration.WithDotNet(options))
            {
                options.PublicDependencies.Add("nethost");

                if (options.Target is EngineTarget engineTarget && engineTarget.UseSeparateMainExecutable(options))
                {
                    // Build target doesn't support linking again main executable (eg. Linux) thus additional shared library is used for the engine (eg. libFlaxEditor.so)
                    var fileName = options.Platform.GetLinkOutputFileName(engineTarget.OutputName, LinkerOutput.SharedLibrary);
                    options.CompileEnv.PreprocessorDefinitions.Add("MCORE_MAIN_MODULE_NAME=" + fileName);
                }
            }
            else
            {
                options.PublicDependencies.Add("mono");
            }
        }

        options.PrivateDependencies.Add("Utilities");
    }
}
