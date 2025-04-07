// Copyright (c) Wojciech Figat. All rights reserved.

using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// DirectX 12 graphics backend module.
/// </summary>
public class GraphicsDeviceDX12 : GraphicsDeviceBaseModule
{
    /// <summary>
    /// Enables using PIX events to instrument game labeling regions of CPU or GPU work and marking important occurrences.
    /// </summary>
    public static bool EnableWinPixEventRuntime = true;

    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.PublicDefinitions.Add("GRAPHICS_API_DIRECTX12");
        switch (options.Platform.Target)
        {
            case TargetPlatform.Windows:
                options.OutputFiles.Add("d3d12.lib");
                options.DelayLoadLibraries.Add("d3d12.dll");
                break;
            case TargetPlatform.XboxOne:
                options.OutputFiles.Add("d3d12_x.lib");
                break;
            case TargetPlatform.XboxScarlett:
                options.OutputFiles.Add("d3d12_xs.lib");
                break;
        }

        if (EnableWinPixEventRuntime && options.Configuration != TargetConfiguration.Release && options.Platform.Target == TargetPlatform.Windows)
        {
            options.PrivateDefinitions.Add("USE_PIX");
            options.PrivateIncludePaths.Add(Path.Combine(Globals.EngineRoot, "Source/ThirdParty/WinPixEventRuntime"));
            options.OutputFiles.Add(Path.Combine(options.DepsFolder, "WinPixEventRuntime.lib"));
            options.DependencyFiles.Add(Path.Combine(options.DepsFolder, "WinPixEventRuntime.dll"));
            options.DelayLoadLibraries.Add("WinPixEventRuntime.dll");
        }
    }
}
