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
                void AddFrameworkDefines(string template, int major, int latestMinor)
                {
                    for (int minor = latestMinor; minor >= 0; minor--)
                    {
                        options.ScriptingAPI.Defines.Add(string.Format(template, major, minor));
                        options.ScriptingAPI.Defines.Add(string.Format($"{template}_OR_GREATER", major, minor));
                    }
                }

                // .NET
                options.PrivateDependencies.Add("nethost");
                options.ScriptingAPI.Defines.Add("USE_NETCORE");

                // .NET SDK
                AddFrameworkDefines("NET{0}_{1}", 7, 0); // "NET7_0" and "NET7_0_OR_GREATER"
                AddFrameworkDefines("NET{0}_{1}", 6, 0);
                AddFrameworkDefines("NET{0}_{1}", 5, 0);
                options.ScriptingAPI.Defines.Add("NET");
                AddFrameworkDefines("NETCOREAPP{0}_{1}", 3, 1); // "NETCOREAPP3_1" and "NETCOREAPP3_1_OR_GREATER"
                AddFrameworkDefines("NETCOREAPP{0}_{1}", 2, 2);
                AddFrameworkDefines("NETCOREAPP{0}_{1}", 1, 1);
                options.ScriptingAPI.Defines.Add("NETCOREAPP");

                if (options.Target is EngineTarget engineTarget && engineTarget.UseSeparateMainExecutable(options))
                {
                    // Build target doesn't support linking again main executable (eg. Linux) thus additional shared library is used for the engine (eg. libFlaxEditor.so)
                    var fileName = options.Platform.GetLinkOutputFileName(EngineTarget.LibraryName, LinkerOutput.SharedLibrary);
                    options.CompileEnv.PreprocessorDefinitions.Add("MCORE_MAIN_MODULE_NAME=" + fileName);
                }
            }
            else
            {
                // Mono
                options.PrivateDependencies.Add("mono");
                options.ScriptingAPI.Defines.Add("USE_MONO");
            }
        }

        options.PrivateDependencies.Add("Utilities");
    }
}
