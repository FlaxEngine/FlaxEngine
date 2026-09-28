// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// XR module.
/// </summary>
public class XR : EngineModule
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
            if (options.Architecture == TargetArchitecture.x64)
            {
                // OpenXR
                options.SourcePaths.Add(Path.Combine(FolderPath, "OpenXR"));
                options.CompileEnv.PreprocessorDefinitions.Add("XR_API_OPENXR");
                options.PrivateDependencies.Add("OpenXR");
                options.PrivateDependencies.Add("GraphicsDeviceDX11");
                var windowsToolchain = options.Toolchain as Flax.Build.Platforms.WindowsToolchain;
                if (windowsToolchain != null && windowsToolchain.SDK != Flax.Build.Platforms.WindowsPlatformSDK.v8_1)
                    options.PrivateDependencies.Add("GraphicsDeviceDX12");
            }
            break;
        default:
            // Not implemented
            break;
        }
    }

    /// <inheritdoc />
    public override void GetFilesToDeploy(List<string> files)
    {
        files.AddRange(Directory.GetFiles(FolderPath, "*.h", SearchOption.TopDirectoryOnly));
    }
}
