// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

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
                TargetPlatform.Mac,
            };
        }
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="VulkanSdk"/> class.
    /// </summary>
    public VulkanSdk()
    {
        var platform = Flax.Build.Platform.BuildTargetPlatform;
        if (!Platforms.Contains(platform))
            return;

        var vulkanSdk = Environment.GetEnvironmentVariable("VULKAN_SDK");
        if (vulkanSdk == null || !Directory.Exists(vulkanSdk))
        {
            if (platform == TargetPlatform.Mac)
            {
                // Try to guess install location for the current user
                var path = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), "VulkanSDK");
                if (Directory.Exists(path))
                {
                    var subDirs = Directory.GetDirectories(path);
                    if (subDirs.Length != 0)
                    {
                        path = Path.Combine(subDirs[0], "macOS");
                        if (Directory.Exists(path))
                            vulkanSdk = path;
                    }
                }
            }
            else if (platform == TargetPlatform.Linux)
            {
                // Try to use system-installed Vulkan SDK
                if (File.Exists("/usr/include/vulkan/vulkan.h"))
                    vulkanSdk = "/usr/include";
            }
        }
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
    /// <param name="platform">Target platform hint.</param>
    /// <param name="includesFolderPath">The includes folder path.</param>
    /// <returns>True if got valid folder, otherwise false.</returns>
    public bool TryGetIncludePath(TargetPlatform platform, out string includesFolderPath)
    {
        includesFolderPath = string.Empty;
        if (IsValid)
        {
            var vulkanSdk = RootPath;

            // Use system-installed headers
            if (vulkanSdk.EndsWith("/include") && Directory.Exists(vulkanSdk))
            {
                if (platform != Flax.Build.Platform.BuildTargetPlatform)
                {
                    Log.Warning(string.Format("Cannot use system-installed VulkanSDK at {0} when building for platform {1}", vulkanSdk, platform));
                    return false;
                }
                includesFolderPath = vulkanSdk;
                return true;
            }

            // Check potential location with headers
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

        options.PrivateDependencies.Add("VulkanMemoryAllocator");

        switch (options.Platform.Target)
        {
        case TargetPlatform.Switch:
            options.SourcePaths.Add(Path.Combine(Globals.EngineRoot, "Source", "Platforms", "Switch", "Engine", "GraphicsDevice", "Vulkan"));
            break;
        default:
            options.PrivateDependencies.Add("volk");
            break;
        }
    }
}
