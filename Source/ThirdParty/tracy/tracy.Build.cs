// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// https://github.com/wolfpld/tracy
/// </summary>
public class tracy : ThirdPartyModule
{
    /// <summary>
    /// Enables on-demand profiling.
    /// </summary>
    public static bool OnDemand = true;

    /// <summary>
    /// Enables GPU profiling.
    /// </summary>
    public static bool GPU = true;

    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        LicenseType = LicenseTypes.BSD3Clause;
        LicenseFilePath = "LICENSE";

        // Merge third-party modules into engine binary
        BinaryModuleName = "FlaxEngine";
    }

    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.SourcePaths.Clear();
        options.SourceFiles.Clear();
        options.SourceFiles.Add(Path.Combine(FolderPath, "tracy", "Tracy.hpp"));
        options.SourceFiles.Add(Path.Combine(FolderPath, "TracyClient.cpp"));

        options.PublicDefinitions.Add("TRACY_ENABLE");
        options.PrivateDefinitions.Add("TRACY_NO_INVARIANT_CHECK");
        options.PrivateDefinitions.Add("TRACY_NO_FRAME_IMAGE");
        if (OnDemand)
        {
            options.PublicDefinitions.Add("TRACY_ON_DEMAND");
        }
        switch (options.Platform.Target)
        {
        case TargetPlatform.Windows:
        case TargetPlatform.XboxOne:
        case TargetPlatform.XboxScarlett:
            options.PrivateDefinitions.Add("TRACY_DBGHELP_LOCK=FlaxDbgHelp");
            options.PrivateDefinitions.Add("TRACY_NO_PIPE");
            break;
        case TargetPlatform.Switch:
            options.PrivateDefinitions.Add("TRACY_USE_MALLOC");
            options.PrivateDefinitions.Add("TRACY_ONLY_IPV4");
            options.PrivateDefinitions.Add("TRACY_NO_PIPE");
            options.PrivateDefinitions.Add("TRACY_NO_CODE_TRANSFER");
            break;
        }

        if (GPU)
        {
            // Ask Graphics module which graphics backends are active
            var graphics = new Graphics();
            graphics.FilePath = FilePath;
            graphics.FolderPath = FolderPath;
            var graphicsOptions = (BuildOptions)options.Clone();
            graphics.Setup(graphicsOptions);
            if (graphicsOptions.PrivateDependencies.Contains("GraphicsDeviceDX11"))
                options.PrivateDefinitions.Add("TRACY_GPU_D3D11");
            if (graphicsOptions.PrivateDependencies.Contains("GraphicsDeviceDX12"))
                options.PrivateDefinitions.Add("TRACY_GPU_D3D12");
            if (graphicsOptions.PrivateDependencies.Contains("GraphicsDeviceVulkan"))
            {
                options.PrivateDefinitions.Add("TRACY_GPU_VULKAN");
                if (VulkanSdk.Instance.TryGetIncludePath(options.Platform.Target, out var includesFolderPath))
                    options.PrivateIncludePaths.Add(includesFolderPath);
            }
        }
    }

    /// <inheritdoc />
    public override void GetFilesToDeploy(List<string> files)
    {
        base.GetFilesToDeploy(files);

        files.Add(Path.Combine(FolderPath, "tracy", "Tracy.hpp"));
        files.Add(Path.Combine(FolderPath, "common", "TracySystem.hpp"));
        files.Add(Path.Combine(FolderPath, "common", "TracyQueue.hpp"));
        files.Add(Path.Combine(FolderPath, "client", "TracyCallstack.h"));
    }
}
