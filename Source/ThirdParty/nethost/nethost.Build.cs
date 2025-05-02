// Copyright (c) Wojciech Figat. All rights reserved.

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
            throw new DotNetSdk.MissingException();
        if (!dotnetSdk.GetHostRuntime(options.Platform.Target, options.Architecture, out var hostRuntime))
        {
            if (options.Target.IsPreBuilt)
                return; // Ignore missing Host Runtime when engine is already prebuilt
            if (options.Flags.HasFlag(BuildFlags.GenerateProject))
                return; // Ignore missing Host Runtime at projects evaluation stage (not important)
            if (Configuration.BuildBindingsOnly)
                return; // Ignore missing Host Runtime when just building C# bindings (without native code)
            throw new Exception($"Missing NET SDK runtime for {options.Platform.Target} {options.Architecture}.");
        }

        // Setup build configuration
        switch (options.Platform.Target)
        {
        case TargetPlatform.Windows:
        case TargetPlatform.XboxOne:
        case TargetPlatform.XboxScarlett:
        case TargetPlatform.UWP:
            if (hostRuntime.Type == DotNetSdk.HostType.CoreCLR)
            {
                options.OutputFiles.Add(Path.Combine(hostRuntime.Path, "nethost.lib"));
                options.DependencyFiles.Add(Path.Combine(hostRuntime.Path, "nethost.dll"));
            }
            else if (hostRuntime.Type == DotNetSdk.HostType.Mono)
            {
                options.PublicDefinitions.Add("USE_MONO_DYNAMIC_LIB");
                options.OutputFiles.Add(Path.Combine(hostRuntime.Path, "coreclr.import.lib"));
                options.DependencyFiles.Add(Path.Combine(hostRuntime.Path, "coreclr.dll"));
            }
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
        {
            var type = Flax.Build.Utilities.GetType($"Flax.Build.Platforms.{options.Platform.Target}.nethost");
            var onLink = type?.GetMethod("OnLink");
            if (onLink != null)
            {
                // Custom linking logic overriden by platform tools
                onLink.Invoke(null, new object[] { options, hostRuntime.Path });
            }
            else
            {
                options.OutputFiles.Add(Path.Combine(hostRuntime.Path, "libmonosgen-2.0.a"));
                options.OutputFiles.Add(Path.Combine(hostRuntime.Path, "libSystem.Native.a"));
                options.OutputFiles.Add(Path.Combine(hostRuntime.Path, "libSystem.IO.Ports.Native.a"));
                options.OutputFiles.Add(Path.Combine(hostRuntime.Path, "libSystem.IO.Compression.Native.a"));
                options.OutputFiles.Add(Path.Combine(hostRuntime.Path, "libSystem.Globalization.Native.a"));
            }
            break;
        }
        case TargetPlatform.Android:
            options.PublicDefinitions.Add("USE_MONO_DYNAMIC_LIB");
            options.DependencyFiles.Add(Path.Combine(hostRuntime.Path, "libmonosgen-2.0.so"));
            options.Libraries.Add(Path.Combine(hostRuntime.Path, "libmonosgen-2.0.so"));
            break;
        case TargetPlatform.iOS:
            options.PublicDefinitions.Add("USE_MONO_DYNAMIC_LIB");
            options.DependencyFiles.Add(Path.Combine(hostRuntime.Path, "libmonosgen-2.0.dylib"));
            options.Libraries.Add(Path.Combine(hostRuntime.Path, "libmonosgen-2.0.dylib"));
            break;
        default: throw new InvalidPlatformException(options.Platform.Target);
        }
        options.DependencyFiles.Add(Path.Combine(FolderPath, "FlaxEngine.CSharp.runtimeconfig.json"));
        options.PublicDefinitions.Add("DOTNET_HOST_RUNTIME_IDENTIFIER=" + DotNetSdk.GetHostRuntimeIdentifier(options.Platform.Target, options.Architecture));
        switch (hostRuntime.Type)
        {
        case DotNetSdk.HostType.CoreCLR:
        {
            // Use CoreCLR for runtime hosting
            options.PublicDefinitions.Add("DOTNET_HOST_CORECLR");
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
