// Copyright (c) Wojciech Figat. All rights reserved.

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
                        options.ScriptingAPI.Defines.Add(string.Format(template, major, minor));
                }

                // .NET
                options.PrivateDependencies.Add("nethost");
                options.ScriptingAPI.Defines.Add("USE_NETCORE");

                // .NET SDK
                var dotnetSdk = DotNetSdk.Instance;
                options.ScriptingAPI.Defines.Add("NET");
                AddFrameworkDefines("NET{0}_{1}", dotnetSdk.Version.Major, 0); // "NET7_0"
                for (int i = 5; i <= dotnetSdk.Version.Major; i++)
                    AddFrameworkDefines("NET{0}_{1}_OR_GREATER", dotnetSdk.Version.Major, 0); // "NET7_0_OR_GREATER"
                options.ScriptingAPI.Defines.Add("NETCOREAPP");
                AddFrameworkDefines("NETCOREAPP{0}_{1}_OR_GREATER", 3, 1); // "NETCOREAPP3_1_OR_GREATER"
                AddFrameworkDefines("NETCOREAPP{0}_{1}_OR_GREATER", 2, 2);
                AddFrameworkDefines("NETCOREAPP{0}_{1}_OR_GREATER", 1, 1);

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
