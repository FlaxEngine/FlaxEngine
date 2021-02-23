// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// GPU device implementation base module.
/// </summary>
public abstract class GraphicsDeviceBaseModule : EngineModule
{
    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        if (options.Configuration == TargetConfiguration.Debug && true)
        {
            // Enables GPU diagnostic tools (debug layer etc.)
            options.PublicDefinitions.Add("GPU_ENABLE_DIAGNOSTICS");
        }
    }

    /// <inheritdoc />
    public override void GetFilesToDeploy(List<string> files)
    {
    }
}

/// <summary>
/// Graphics module.
/// </summary>
public class Graphics : EngineModule
{
    private static bool _lodMissingVulkanSDK;
    private static bool _lodMissingWindowsSDK;

    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        switch (options.Platform.Target)
        {
        case TargetPlatform.Windows:
            options.PrivateDependencies.Add("GraphicsDeviceNull");
            options.PrivateDependencies.Add("GraphicsDeviceDX11");
            if (VulkanSdk.Instance.IsValid)
                options.PrivateDependencies.Add("GraphicsDeviceVulkan");
            else
                Log.WarningOnce("Building for Windows without Vulkan rendering backend (Vulkan SDK is missing)", ref _lodMissingVulkanSDK);
            var windowsToolchain = options.Toolchain as Flax.Build.Platforms.WindowsToolchain;
            if (windowsToolchain != null && windowsToolchain.SDK != Flax.Build.Platforms.WindowsPlatformSDK.v8_1)
                options.PrivateDependencies.Add("GraphicsDeviceDX12");
            else
                Log.WarningOnce("Building for Windows without DirectX 12 rendering backend (Windows 10 SDK is required)", ref _lodMissingWindowsSDK);
            break;
        case TargetPlatform.XboxOne:
        case TargetPlatform.UWP:
            options.PrivateDependencies.Add("GraphicsDeviceDX11");
            break;
        case TargetPlatform.XboxScarlett:
            options.PrivateDependencies.Add("GraphicsDeviceDX12");
            break;
        case TargetPlatform.Linux:
            options.PrivateDependencies.Add("GraphicsDeviceNull");
            if (VulkanSdk.Instance.IsValid)
                options.PrivateDependencies.Add("GraphicsDeviceVulkan");
            else
                Log.WarningOnce("Building for Linux without Vulkan rendering backend (Vulkan SDK is missing)", ref _lodMissingVulkanSDK);
            break;
        case TargetPlatform.PS4:
            options.PrivateDependencies.Add("GraphicsDevicePS4");
            break;
        case TargetPlatform.Android:
            options.PrivateDependencies.Add("GraphicsDeviceVulkan");
            break;
        default: throw new InvalidPlatformException(options.Platform.Target);
        }

        if (options.Target.IsEditor)
        {
            options.PublicDependencies.Add("ModelTool");
            options.PrivateDependencies.Add("assimp");
            options.PrivateDefinitions.Add("USE_ASSIMP");
            options.PublicDependencies.Add("ShadersCompilation");

            options.PublicDefinitions.Add("COMPILE_WITH_SHADER_CACHE_MANAGER");
        }

        // This fixes issue with missing define for GPU particles in ParticleMaterialShader.cpp
        if (Particles.UseGPU(options))
        {
            options.PrivateDefinitions.Add("COMPILE_WITH_GPU_PARTICLES");
        }
    }
}
