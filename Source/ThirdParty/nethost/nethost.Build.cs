// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// Module for nethost (.NET runtime host library).
/// </summary>
public class nethost : ThirdPartyModule
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        LicenseType = LicenseTypes.MIT;
        LicenseFilePath = "LICENSE.TXT";

        // Merge third-party modules into engine binary
        BinaryModuleName = "FlaxEngine";
    }

    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.SourceFiles.Clear();

        // Get .NET SDK runtime host
        var dotnetSdk = DotNetSdk.Instance;
        if (!dotnetSdk.IsValid)
            throw new Exception($"Missing NET SDK {DotNetSdk.MinimumVersion}.");
        if (!dotnetSdk.GetHostRuntime(options.Platform.Target, options.Architecture, out var hostRuntime))
        {
            if (options.Target.IsPreBuilt)
                return; // Ignore missing Host Runtime when engine is already prebuilt
            if (options.Flags.HasFlag(BuildFlags.GenerateProject))
                return; // Ignore missing Host Runtime at projects evaluation stage (not important)
            throw new Exception($"Missing NET SDK runtime for {options.Platform.Target} {options.Architecture}.");
        }

        // Setup build configuration
        switch (options.Platform.Target)
        {
        case TargetPlatform.Windows:
        case TargetPlatform.XboxOne:
        case TargetPlatform.XboxScarlett:
        case TargetPlatform.UWP:
            options.OutputFiles.Add(Path.Combine(hostRuntime.Path, "nethost.lib"));
            options.DependencyFiles.Add(Path.Combine(hostRuntime.Path, "nethost.dll"));
            break;
        case TargetPlatform.Linux:
            options.OutputFiles.Add(Path.Combine(hostRuntime.Path, "libnethost.a"));
            options.DependencyFiles.Add(Path.Combine(hostRuntime.Path, "libnethost.so"));
            break;
        case TargetPlatform.Mac:
            options.OutputFiles.Add(Path.Combine(hostRuntime.Path, "libnethost.a"));
            options.DependencyFiles.Add(Path.Combine(hostRuntime.Path, "libnethost.dylib"));
            break;
        case TargetPlatform.Switch:
        case TargetPlatform.PS4:
        case TargetPlatform.PS5:
            options.OutputFiles.Add(Path.Combine(hostRuntime.Path, "libnethost.a"));
            //options.OutputFiles.Add(Path.Combine(hostRuntime.Path, "libhostfxr.a"));
            break;
        case TargetPlatform.Android: break;
        default: throw new InvalidPlatformException(options.Platform.Target);
        }
        options.DependencyFiles.Add(Path.Combine(FolderPath, "FlaxEngine.CSharp.runtimeconfig.json"));
        options.PublicDefinitions.Add("DOTNET_HOST_RUNTIME_IDENTIFIER=" + DotNetSdk.GetHostRuntimeIdentifier(options.Platform.Target, options.Architecture));
        switch (hostRuntime.Type)
        {
        case DotNetSdk.HostType.CoreCRL:
        {
            // Use CoreCRL for runtime hosting
            options.PublicDefinitions.Add("DOTNET_HOST_CORECRL");
            options.PublicIncludePaths.Add(hostRuntime.Path);
            break;
        }
        case DotNetSdk.HostType.Mono:
        {
            // Use Mono for runtime hosting
            options.PublicDefinitions.Add("DOTNET_HOST_MONO");
            options.PublicIncludePaths.Add(Path.Combine(hostRuntime.Path, "include", "mono-2.0"));
            break;
        }
        default: throw new ArgumentOutOfRangeException();
        }
    }
}
