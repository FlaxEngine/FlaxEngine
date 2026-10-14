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
    private static bool _logMissingLoader;

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
                // OpenXR (the loader has to be built for this platform, see OpenXR dependency)
                if (!HasOpenXRLoader(options, "openxr_loader.lib"))
                    break;
                options.SourcePaths.Add(Path.Combine(FolderPath, "OpenXR"));
                options.CompileEnv.PreprocessorDefinitions.Add("XR_API_OPENXR");
                options.PrivateDependencies.Add("OpenXR");
                options.PrivateDependencies.Add("GraphicsDeviceDX11");
                var windowsToolchain = options.Toolchain as Flax.Build.Platforms.WindowsToolchain;
                if (windowsToolchain != null && windowsToolchain.SDK != Flax.Build.Platforms.WindowsPlatformSDK.v8_1)
                    options.PrivateDependencies.Add("GraphicsDeviceDX12");
                if (VulkanSdk.Instance.IsValid)
                {
                    options.PrivateDependencies.Add("GraphicsDeviceVulkan");
                    options.PrivateDependencies.Add("volk");
                }
            }
            break;
        case TargetPlatform.Linux:
            if (VulkanSdk.Instance.IsValid)
            {
                // OpenXR (the loader has to be built for this platform, see OpenXR dependency)
                if (!HasOpenXRLoader(options, "libopenxr_loader.a"))
                    break;
                options.SourcePaths.Add(Path.Combine(FolderPath, "OpenXR"));
                options.CompileEnv.PreprocessorDefinitions.Add("XR_API_OPENXR");
                options.PrivateDependencies.Add("OpenXR");
                options.PrivateDependencies.Add("GraphicsDeviceVulkan");
                options.PrivateDependencies.Add("volk");
            }
            break;
        default:
            // Not implemented
            break;
        }
    }

    private static bool HasOpenXRLoader(BuildOptions options, string libName)
    {
        if (File.Exists(Path.Combine(options.DepsFolder, libName)))
            return true;
        Log.WarningOnce(string.Format("Building for {0} without OpenXR (loader library is missing, run Flax.Build -BuildDeps -depsToBuild=OpenXR)", options.Platform.Target), ref _logMissingLoader);
        return false;
    }

    /// <inheritdoc />
    public override void GetFilesToDeploy(List<string> files)
    {
        files.AddRange(Directory.GetFiles(FolderPath, "*.h", SearchOption.TopDirectoryOnly));
    }
}
