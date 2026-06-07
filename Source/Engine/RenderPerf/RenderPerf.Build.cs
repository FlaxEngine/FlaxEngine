using System;
using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// Optional NVIDIA Nsight Perf SDK integration for GPU region profiling.
/// </summary>
public class RenderPerf : EngineModule
{
    /// <summary>
    /// Determinates whenever the RenderPerf module should be built for a given target.
    /// </summary>
    public static bool Use(BuildOptions options)
    {
        return options.Platform.Target == TargetPlatform.Windows;
    }

    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.PublicDefinitions.Add("COMPILE_WITH_RENDER_PERF");

        string perfSdk = Environment.GetEnvironmentVariable("NVPERF_SDK_PATH");
        if (string.IsNullOrEmpty(perfSdk))
            perfSdk = @"C:\Users\nproc\Downloads\PerfSDK_2025_5";

        string perfInclude = Path.Combine(perfSdk, "redist", "include");
        if (!Directory.Exists(perfInclude))
            return;

        string perfIncludeOs = Path.Combine(perfInclude, "windows-desktop-x64");
        string perfUtility = Path.Combine(perfSdk, "redist", "NvPerfUtility", "include");
        string perfRyml = Path.Combine(perfSdk, "Samples", "NvPerfUtility", "imports", "rapidyaml-0.4.0");

        options.PublicDefinitions.Add("COMPILE_WITH_RENDER_PERF_NVPERF=1");
        options.PublicIncludePaths.Add(perfInclude);
        options.PublicIncludePaths.Add(perfIncludeOs);
        options.PublicIncludePaths.Add(perfUtility);
        options.PrivateIncludePaths.Add(perfRyml);
        options.PublicDefinitions.Add("NOMINMAX");

        options.PrivateDependencies.Add("volk");

        string perfBin = Path.Combine(perfSdk, "NvPerf", "bin", "x64");
        if (Directory.Exists(perfBin))
        {
            foreach (string dll in Directory.GetFiles(perfBin, "nvperf*.dll"))
                options.DependencyFiles.Add(dll);
        }
    }
}
