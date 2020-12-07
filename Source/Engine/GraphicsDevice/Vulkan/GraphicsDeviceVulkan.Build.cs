// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Linq;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// The Vulkan SDK.
/// </summary>
public sealed class VulkanSdk : Sdk
{
    /// <summary>
    /// The singleton instance.
    /// </summary>
    public static readonly VulkanSdk Instance = new VulkanSdk();

    /// <inheritdoc />
    public override TargetPlatform[] Platforms
    {
        get
        {
            return new[]
            {
                TargetPlatform.Windows,
                TargetPlatform.Linux,
            };
        }
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="VulkanSdk"/> class.
    /// </summary>
    public VulkanSdk()
    {
        if (!Platforms.Contains(Flax.Build.Platform.BuildTargetPlatform))
            return;

        var vulkanSdk = Environment.GetEnvironmentVariable("VULKAN_SDK");
        if (vulkanSdk != null)
        {
            if (Directory.Exists(vulkanSdk))
            {
                // Found
                RootPath = vulkanSdk;
                IsValid = true;
                Version = new Version(1, 0); // TODO: detecting Vulkan SDK version
                Log.Verbose("Found VulkanSDK at: " + vulkanSdk);
            }
            else
            {
                Log.Warning(string.Format("Missing VulkanSDK at {0}.", vulkanSdk));
            }
        }
        else
        {
            Log.Verbose("Missing VULKAN_SDK environment variable. Cannot build Vulkan.");
        }
    }

    /// <summary>
    /// Tries the get includes folder path (header files). This handles uppercase and lowercase installations for all platforms.
    /// </summary>
    /// <param name="includesFolderPath">The includes folder path.</param>
    /// <returns>True if got valid folder, otherwise false.</returns>
    public bool TryGetIncludePath(out string includesFolderPath)
    {
        if (IsValid)
        {
            var vulkanSdk = RootPath;

            var includes = new[]
            {
                Path.Combine(vulkanSdk, "include"),
                Path.Combine(vulkanSdk, "Include"),
                Path.Combine(vulkanSdk, "x86_64", "include"),
            };

            foreach (var include in includes)
            {
                if (Directory.Exists(include))
                {
                    includesFolderPath = include;
                    return true;
                }
            }

            Log.Warning(string.Format("Invalid VulkanSDK at {0}. Missing header files include folder. Verify installation.", vulkanSdk));
            foreach (var include in includes)
                Log.Warning(string.Format("No Vulkan header files in {0}", include));
        }

        includesFolderPath = string.Empty;
        return false;
    }
}

/// <summary>
/// Vulkan graphics backend module.
/// </summary>
public class GraphicsDeviceVulkan : GraphicsDeviceBaseModule
{
    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.PublicDefinitions.Add("GRAPHICS_API_VULKAN");

        options.PrivateDependencies.Add("volk");
        options.PrivateDependencies.Add("VulkanMemoryAllocator");
    }
}
