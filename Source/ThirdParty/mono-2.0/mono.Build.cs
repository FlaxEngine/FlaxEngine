// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// https://www.mono-project.com/
/// </summary>
public class mono : DepsModule
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        LicenseType = LicenseTypes.Custom;
        LicenseFilePath = "LICENSE";

        // Merge third-party modules into engine binary
        BinaryModuleName = "FlaxEngine";
    }

    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        var depsRoot = options.DepsFolder;

        switch (options.Platform.Target)
        {
        case TargetPlatform.Windows:
        case TargetPlatform.UWP:
        case TargetPlatform.XboxOne:
        case TargetPlatform.XboxScarlett:
        {
            bool useDynamicLinking = false;
            if (useDynamicLinking)
            {
                // Dynamic linking (requires mono-2.0-sgen.dll to deploy side-by-side with the Flax)
                options.PublicDefinitions.Add("USE_MONO_DYNAMIC_LIB");
                options.OutputFiles.Add(Path.Combine(depsRoot, "mono-2.0-sgen.lib"));
                options.DependencyFiles.Add(Path.Combine(depsRoot, "mono-2.0-sgen.dll"));
                options.OptionalDependencyFiles.Add(Path.Combine(depsRoot, "mono-2.0-sgen.pdb"));
            }
            else
            {
                // Static linking
                options.OutputFiles.Add(Path.Combine(depsRoot, "libmono-static.lib"));
                var debugSymbolsLibs = new[]
                {
                    "libmonoruntime",
                    "libmonoutils",
                    "libmini",
                    "eglib",
                };
                foreach (var debugSymbol in debugSymbolsLibs)
                    options.OptionalDependencyFiles.Add(Path.Combine(depsRoot, debugSymbol + ".pdb"));

                // Additional dependencies required by Mono on Windows-based platforms
                options.OutputFiles.Add("Mswsock.lib");
                options.OutputFiles.Add("ws2_32.lib");
                options.OutputFiles.Add("psapi.lib");
                options.OutputFiles.Add("version.lib");
                options.OutputFiles.Add("winmm.lib");
                options.OutputFiles.Add("Bcrypt.lib");
            }
            options.DependencyFiles.Add(Path.Combine(depsRoot, "MonoPosixHelper.dll"));
            break;
        }
        case TargetPlatform.Linux:
            options.PublicDefinitions.Add("USE_MONO_DYNAMIC_LIB");
            options.DependencyFiles.Add(Path.Combine(depsRoot, "libmonosgen-2.0.so"));
            options.Libraries.Add(Path.Combine(depsRoot, "libmonosgen-2.0.so"));
            break;
        case TargetPlatform.PS4:
            options.OutputFiles.Add(Path.Combine(depsRoot, "libmono.a"));
            options.OutputFiles.Add(Path.Combine(depsRoot, "libmonoruntime.a"));
            options.OutputFiles.Add(Path.Combine(depsRoot, "libmonoutils.a"));
            options.OutputFiles.Add(Path.Combine(depsRoot, "mono.a"));
            break;
        case TargetPlatform.Android:
            options.PublicDefinitions.Add("USE_MONO_DYNAMIC_LIB");
            options.DependencyFiles.Add(Path.Combine(depsRoot, "libmonosgen-2.0.so"));
            options.Libraries.Add(Path.Combine(depsRoot, "libmonosgen-2.0.so"));
            break;
        case TargetPlatform.Switch:
            options.OutputFiles.Add(Path.Combine(depsRoot, "libmonosgen-2.0.a"));
            break;
        default: throw new InvalidPlatformException(options.Platform.Target);
        }

        // TODO: remove hardcoded include path for all modules and use this public thing to pass include only to modules that use it
        //options.PublicIncludePaths.Add(Path.Combine(Globals.EngineRoot, @"Source\ThirdParty\mono-2.0"));
    }

    /// <inheritdoc />
    public override void GetFilesToDeploy(List<string> files)
    {
        base.GetFilesToDeploy(files);

        files.AddRange(Directory.GetFiles(FolderPath, "*.h", SearchOption.AllDirectories));
    }
}
