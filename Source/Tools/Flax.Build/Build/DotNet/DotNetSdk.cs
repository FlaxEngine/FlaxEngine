// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Linq;
using Microsoft.Win32;

namespace Flax.Build
{
    /// <summary>
    /// The DotNet SDK.
    /// </summary>
    public sealed class DotNetSdk : Sdk
    {
        /// <summary>
        /// The singleton instance.
        /// </summary>
        public static readonly DotNetSdk Instance = new DotNetSdk();

        /// <summary>
        /// The minimum SDK version.
        /// </summary>
        public static Version MinimumVersion => new Version(7, 0);

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
        /// Dotnet SDK version (eg. 7.0).
        /// </summary>
        public readonly string VersionName;

        /// <summary>
        /// Dotnet Shared FX library version (eg. 7.0).
        /// </summary>
        public readonly string RuntimeVersionName;

        /// <summary>
        /// Dotnet hosting library files path (eg. <RootPath>/packs/Microsoft.NETCore.App.Host.<os>/<VersionName>/runtimes/<os>-<arcg>/native).
        /// </summary>
        public readonly string HostRootPath;

        /// <summary>
        /// Initializes a new instance of the <see cref="DotNetSdk"/> class.
        /// </summary>
        public DotNetSdk()
        {
            var platform = Platform.BuildTargetPlatform;
            var architecture = Platform.BuildTargetArchitecture;
            if (!Platforms.Contains(platform))
                return;

            // Find system-installed SDK
            string dotnetPath;
            string rid, ridFallback, arch;
            string[] dotnetSdkVersions, dotnetRuntimeVersions;
            switch (architecture)
            {
            case TargetArchitecture.x86:
                arch = "x86";
                break;
            case TargetArchitecture.x64:
                arch = "x64";
                break;
            case TargetArchitecture.ARM:
                arch = "arm";
                break;
            case TargetArchitecture.ARM64:
                arch = "arm64";
                break;
            default:
                throw new InvalidArchitectureException(architecture);
            }
            switch (platform)
            {
            case TargetPlatform.Windows:
            {
                rid = $"win-{arch}";
                ridFallback = "";
#pragma warning disable CA1416
                using RegistryKey baseKey = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, RegistryView.Registry64);
                using RegistryKey sdkVersionsKey = baseKey.OpenSubKey($@"SOFTWARE\WOW6432Node\dotnet\Setup\InstalledVersions\{arch}\sdk");
                using RegistryKey runtimeKey = baseKey.OpenSubKey(@$"SOFTWARE\WOW6432Node\dotnet\Setup\InstalledVersions\{arch}\sharedfx\Microsoft.NETCore.App");
                using RegistryKey hostKey = baseKey.OpenSubKey(@$"SOFTWARE\dotnet\Setup\InstalledVersions\{arch}\sharedhost");
                dotnetPath = (string)hostKey.GetValue("Path");
                dotnetSdkVersions = sdkVersionsKey.GetValueNames();
                dotnetRuntimeVersions = runtimeKey.GetValueNames();
#pragma warning restore CA1416
                break;
            }
            case TargetPlatform.Linux:
            {
                // TODO: Support /etc/dotnet/install_location

                // Detect custom RID in some distros
                rid = Utilities.ReadProcessOutput("dotnet", "--info").Split('\n').FirstOrDefault(x => x.StartsWith(" RID:"), "").Replace("RID:", "").Trim();
                ridFallback = $"linux-{arch}";
                if (rid == ridFallback)
                    ridFallback = "";
                dotnetPath = "/usr/share/dotnet/";
                dotnetSdkVersions = Directory.GetDirectories($"{dotnetPath}sdk/").Select(Path.GetFileName).ToArray();
                dotnetRuntimeVersions = Directory.GetDirectories($"{dotnetPath}shared/Microsoft.NETCore.App/").Select(Path.GetFileName).ToArray();
                break;
            }
            default:
                throw new InvalidPlatformException(platform);
            }

            // Pick SDK version
            string dotnetSdkVersion = dotnetSdkVersions.OrderByDescending(ParseVersion).FirstOrDefault();
            string dotnetRuntimeVersion = dotnetRuntimeVersions.OrderByDescending(ParseVersion).FirstOrDefault();
            if (string.IsNullOrEmpty(dotnetSdkVersion))
                dotnetSdkVersion = Environment.GetEnvironmentVariable("DOTNET_ROOT");
            if (string.IsNullOrEmpty(dotnetPath) || string.IsNullOrEmpty(dotnetSdkVersion) || string.IsNullOrEmpty(dotnetRuntimeVersion))
            {
                Log.Warning("Missing .NET SDK");
                return;
            }
            int majorVersion = int.Parse(dotnetSdkVersion.Substring(0, dotnetSdkVersion.IndexOf(".")));
            if (majorVersion < MinimumVersion.Major)
            {
                Log.Warning($"Unsupported .NET SDK {dotnetSdkVersion} version found. Minimum version required is .NET {MinimumVersion}.");
                return;
            }
            HostRootPath = Path.Combine(dotnetPath, $"packs/Microsoft.NETCore.App.Host.{rid}/{dotnetRuntimeVersion}/runtimes/{rid}/native");
            if (!string.IsNullOrEmpty(ridFallback) && !Directory.Exists(HostRootPath))
                HostRootPath = Path.Combine(dotnetPath, $"packs/Microsoft.NETCore.App.Host.{ridFallback}/{dotnetRuntimeVersion}/runtimes/{ridFallback}/native");
            if (!Directory.Exists(HostRootPath))
            {
                Log.Warning($"Missing .NET SDK host runtime path in {HostRootPath}.");
                return;
            }

            // Found
            RootPath = dotnetPath;
            IsValid = true;
            Version = ParseVersion(dotnetSdkVersion);
            VersionName = dotnetSdkVersion;
            RuntimeVersionName = dotnetRuntimeVersion;
            Log.Verbose($"Found .NET SDK {VersionName} (runtime {RuntimeVersionName}) at {RootPath}");
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
    }
}
