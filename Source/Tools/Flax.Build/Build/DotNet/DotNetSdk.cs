// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Linq;
using System.Collections.Generic;
using Microsoft.Win32;

namespace Flax.Build
{
    /// <summary>
    /// The DotNet SDK.
    /// </summary>
    public sealed class DotNetSdk : Sdk
    {
        private Dictionary<KeyValuePair<TargetPlatform, TargetArchitecture>, string> _hostRuntimes = new();

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
                    TargetPlatform.Mac,
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
        /// Initializes a new instance of the <see cref="DotNetSdk"/> class.
        /// </summary>
        public DotNetSdk()
        {
            var platform = Platform.BuildTargetPlatform;
            var architecture = Platform.BuildTargetArchitecture;
            if (!Platforms.Contains(platform))
                return;

            // Find system-installed SDK
            string dotnetPath = Environment.GetEnvironmentVariable("DOTNET_ROOT");
            string rid, ridFallback, arch;
            string[] dotnetSdkVersions = null, dotnetRuntimeVersions = null;
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
            default: throw new InvalidArchitectureException(architecture);
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
                string osId = File.ReadAllLines("/etc/os-release").FirstOrDefault(x => x.StartsWith("ID="), "ID=linux").Substring("ID=".Length);

                rid = $"{osId}-{arch}";
                ridFallback = $"linux-{arch}";
                if (rid == ridFallback)
                    ridFallback = "";
                if (string.IsNullOrEmpty(dotnetPath))
                    dotnetPath = "/usr/share/dotnet/";
                break;
            }
            case TargetPlatform.Mac:
            {
                rid = $"osx-{arch}";
                ridFallback = "";
                if (string.IsNullOrEmpty(dotnetPath))
                    dotnetPath = "/usr/local/share/dotnet/";
                break;
            }
            default: throw new InvalidPlatformException(platform);
            }

            // Pick SDK version
            if (string.IsNullOrEmpty(dotnetPath))
            {
                Log.Warning("Missing .NET SDK");
                return;
            }
            if (!Directory.Exists(dotnetPath))
            {
                Log.Warning($"Missing .NET SDK ({dotnetPath})");
                return;
            }
            if (dotnetSdkVersions == null)
                dotnetSdkVersions = Directory.GetDirectories(Path.Combine(dotnetPath, "sdk")).Select(Path.GetFileName).ToArray();
            if (dotnetRuntimeVersions == null)
                dotnetRuntimeVersions = Directory.GetDirectories(Path.Combine(dotnetPath, "shared/Microsoft.NETCore.App/")).Select(Path.GetFileName).ToArray();
            string dotnetSdkVersion = dotnetSdkVersions.OrderByDescending(ParseVersion).FirstOrDefault();
            string dotnetRuntimeVersion = dotnetRuntimeVersions.OrderByDescending(ParseVersion).FirstOrDefault();
            if (string.IsNullOrEmpty(dotnetSdkVersion))
                dotnetSdkVersion = dotnetPath;
            if (string.IsNullOrEmpty(dotnetSdkVersion) || string.IsNullOrEmpty(dotnetRuntimeVersion))
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
            RootPath = dotnetPath;
            Version = ParseVersion(dotnetSdkVersion);
            VersionName = dotnetSdkVersion;
            RuntimeVersionName = dotnetRuntimeVersion;

            // Pick SDK runtime
            if (!TryAddHostRuntime(platform, architecture, rid) && !TryAddHostRuntime(platform, architecture, ridFallback))
            {
                var path = Path.Combine(RootPath, $"packs/Microsoft.NETCore.App.Host.{rid}/{RuntimeVersionName}/runtimes/{rid}/native");
                Log.Warning($"Missing .NET SDK host runtime for {platform} {architecture} ({path}).");
                return;
            }
            TryAddHostRuntime(TargetPlatform.Windows, TargetArchitecture.x86, "win-x86");
            TryAddHostRuntime(TargetPlatform.Windows, TargetArchitecture.x64, "win-x64");
            TryAddHostRuntime(TargetPlatform.Windows, TargetArchitecture.ARM64, "win-arm64");
            TryAddHostRuntime(TargetPlatform.Mac, TargetArchitecture.x64, "osx-x64");
            TryAddHostRuntime(TargetPlatform.Mac, TargetArchitecture.ARM64, "osx-arm64");

            // Found
            IsValid = true;
            Log.Verbose($"Found .NET SDK {VersionName} (runtime {RuntimeVersionName}) at {RootPath}");
            foreach (var e in _hostRuntimes)
                Log.Verbose($"  - Host Runtime for {e.Key.Key} {e.Key.Value}");
        }

        /// <summary>
        /// Gets the path to runtime host contents folder for a given target platform and architecture.
        /// In format: &lt;RootPath&gt;/packs/Microsoft.NETCore.App.Host.&lt;os&gt;/&lt;VersionName&gt;/runtimes/&lt;os&gt;-&lt;arch&gt;/native
        /// </summary>
        public bool GetHostRuntime(TargetPlatform platform, TargetArchitecture arch, out string path)
        {
            return _hostRuntimes.TryGetValue(new KeyValuePair<TargetPlatform, TargetArchitecture>(platform, arch), out path);
        }

        /// <summary>
        /// Adds an external hostfx location for a given platform.
        /// </summary>
        /// <param name="platform">Target platform.</param>
        /// <param name="arch">Target architecture.</param>
        /// <param name="path">Folder path with contents.</param>
        public void AddHostRuntime(TargetPlatform platform, TargetArchitecture arch, string path)
        {
            if (Directory.Exists(path))
                _hostRuntimes[new KeyValuePair<TargetPlatform, TargetArchitecture>(platform, arch)] = path;
        }

        private bool TryAddHostRuntime(TargetPlatform platform, TargetArchitecture arch, string rid)
        {
            if (string.IsNullOrEmpty(rid))
                return false;
            var path = Path.Combine(RootPath, $"packs/Microsoft.NETCore.App.Host.{rid}/{RuntimeVersionName}/runtimes/{rid}/native");
            var exists = Directory.Exists(path);
            if (exists)
                _hostRuntimes[new KeyValuePair<TargetPlatform, TargetArchitecture>(platform, arch)] = path;
            return exists;
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
