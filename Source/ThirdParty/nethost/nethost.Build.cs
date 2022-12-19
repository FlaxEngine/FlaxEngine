// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

using System.IO;
using System;
using Flax.Build;
using Flax.Build.NativeCpp;
using Microsoft.Win32;
using System.Linq;

/// <summary>
/// Module for nethost (.NET runtime host library)
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

    private static Version ParseVersion(string version)
    {
        // Give precedence to final releases over release candidate / beta releases
        int rev = 9999;
        if (version.Contains("-")) // e.g. 7.0.0-rc.2.22472.3
        {
            version = version.Substring(0, version.IndexOf("-"));
            rev = 0;
        }
        Version ver = new Version(version);
        return new Version(ver.Major, ver.Minor, ver.Build, rev);
    }

    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.SourceFiles.Clear();

        string arch = "x64"; //options.Architecture == TargetArchitecture.x64 ? "x64" : "x86";

        string dotnetPath, dotnetVersion;
        string appHostRuntimePath;
        string os;
        string[] dotnetVersions;

        // Pick DotNet SDK
        if (options.Platform.Target == TargetPlatform.Windows)
        {
            os = $"win-{arch}";
#pragma warning disable CA1416
            using RegistryKey baseKey = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, RegistryView.Registry64);
            using RegistryKey hostKey = baseKey.OpenSubKey(@$"SOFTWARE\dotnet\Setup\InstalledVersions\{arch}\sharedhost");
            dotnetPath = (string)hostKey.GetValue("Path");
            using RegistryKey runtimeKey = baseKey.OpenSubKey(@$"SOFTWARE\WOW6432Node\dotnet\Setup\InstalledVersions\{arch}\sharedfx\Microsoft.NETCore.App");
            dotnetVersions = runtimeKey.GetValueNames();
#pragma warning restore CA1416
        }
        else if (options.Platform.Target == TargetPlatform.Linux)
        {
            // TODO: Support /etc/dotnet/install_location
            dotnetPath = "/usr/share/dotnet/";
            os = $"linux-{arch}";
            dotnetVersions = Directory.GetDirectories($"{dotnetPath}host/fxr/").Select(x => Path.GetFileName(x)).ToArray();
        }
        else
            throw new InvalidPlatformException(options.Platform.Target);

        // Pick SDK version
        dotnetVersion = dotnetVersions.OrderByDescending(ParseVersion).FirstOrDefault();
        if (string.IsNullOrEmpty(dotnetPath))
            dotnetPath = Environment.GetEnvironmentVariable("DOTNET_ROOT");
        if (string.IsNullOrEmpty(dotnetPath) || string.IsNullOrEmpty(dotnetVersion))
            throw new Exception("Failed to find dotnet installation");
        int majorVersion = int.Parse(dotnetVersion.Substring(0, dotnetVersion.IndexOf(".")));
        if (majorVersion < 7)
            throw new Exception($"Unsupported dotnet version found, minimum version required is .NET 7 (found {dotnetVersion})");

        // Setup build configuration
        switch (options.Platform.Target)
        {
        case TargetPlatform.Windows:
        case TargetPlatform.XboxOne:
        case TargetPlatform.XboxScarlett:
        case TargetPlatform.UWP:
            appHostRuntimePath = string.Format("{0}packs\\Microsoft.NETCore.App.Host.{1}\\{2}\\runtimes\\{1}\\native", dotnetPath, os, dotnetVersion);
            options.OutputFiles.Add(Path.Combine(appHostRuntimePath, "nethost.lib"));
            options.DependencyFiles.Add(Path.Combine(appHostRuntimePath, "nethost.dll"));
            break;
        case TargetPlatform.Linux:
        case TargetPlatform.Android:
        case TargetPlatform.Switch:
        case TargetPlatform.PS4:
        case TargetPlatform.PS5:
        case TargetPlatform.Mac:
            appHostRuntimePath = string.Format("{0}packs/Microsoft.NETCore.App.Host.{1}/{2}/runtimes/{1}/native", dotnetPath, os, dotnetVersion);
            options.OutputFiles.Add(Path.Combine(appHostRuntimePath, "libnethost.a"));
            options.DependencyFiles.Add(Path.Combine(appHostRuntimePath, "libnethost.so"));
            break;
        default:
            throw new InvalidPlatformException(options.Platform.Target);
        }
        options.PublicIncludePaths.Add(appHostRuntimePath);
        options.ScriptingAPI.Defines.Add("USE_NETCORE");
        options.DependencyFiles.Add(Path.Combine(FolderPath, "FlaxEngine.CSharp.runtimeconfig.json"));
    }
}
