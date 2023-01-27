// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System.IO;
using System;
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

        var dotnetSdk = DotNetSdk.Instance;
        if (!dotnetSdk.IsValid)
            throw new Exception($"Missing NET SDK {DotNetSdk.MinimumVersion}.");

        // Setup build configuration
        switch (options.Platform.Target)
        {
        case TargetPlatform.Windows:
        case TargetPlatform.XboxOne:
        case TargetPlatform.XboxScarlett:
        case TargetPlatform.UWP:
            options.OutputFiles.Add(Path.Combine(dotnetSdk.HostRootPath, "nethost.lib"));
            options.DependencyFiles.Add(Path.Combine(dotnetSdk.HostRootPath, "nethost.dll"));
            break;
        case TargetPlatform.Linux:
        case TargetPlatform.Android:
        case TargetPlatform.Switch:
        case TargetPlatform.PS4:
        case TargetPlatform.PS5:
        case TargetPlatform.Mac:
            options.OutputFiles.Add(Path.Combine(dotnetSdk.HostRootPath, "libnethost.a"));
            options.DependencyFiles.Add(Path.Combine(dotnetSdk.HostRootPath, "libnethost.so"));
            break;
        default:
            throw new InvalidPlatformException(options.Platform.Target);
        }
        options.PublicIncludePaths.Add(dotnetSdk.HostRootPath);
        options.ScriptingAPI.Defines.Add("USE_NETCORE");
        options.DependencyFiles.Add(Path.Combine(FolderPath, "FlaxEngine.CSharp.runtimeconfig.json"));
    }
}
