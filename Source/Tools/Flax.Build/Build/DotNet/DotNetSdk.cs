// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Linq;
using System.Collections.Generic;
using Microsoft.Win32;

namespace Flax.Build
{
    partial class Configuration
    {
        /// <summary>
        /// Prints all .NET Runtimes found on system (use plaform/arch switches to filter results).
        /// </summary>
        [CommandLine("printDotNetRuntime", "Prints all .NET Runtimes found on system (use plaform/arch switches to filter results).")]
        public static void PrintDotNetRuntime()
        {
            Log.Info("Printing .NET Runtimes...");
            DotNetSdk.Instance.PrintRuntimes();
        }
    }

    /// <summary>
    /// The DotNet SDK.
    /// </summary>
    public sealed class DotNetSdk : Sdk
    {
        /// <summary>
        /// Runtime host types.
        /// </summary>
        public enum HostType
        {
            /// <summary>
            /// Core CLR runtime.
            /// </summary>
            CoreCLR,

            /// <summary>
            /// Old-school Mono runtime.
            /// </summary>
            Mono,
        }

        /// <summary>
        /// Host runtime description.
        /// </summary>
        public struct HostRuntime : IEquatable<HostRuntime>
        {
            /// <summary>
            /// The path to runtime host contents folder for a given target platform and architecture.
            /// In format: &lt;RootPath&gt;/packs/Microsoft.NETCore.App.Host.&lt;os&gt;/&lt;VersionName&gt;/runtimes/&lt;os&gt;-&lt;arch&gt;/native
            /// </summary>
            public string Path;

            /// <summary>
            /// Type of the host.
            /// </summary>
            public HostType Type;

            /// <summary>
            /// Initializes a new instance of the <see cref="HostRuntime"/> class.
            /// </summary>
            /// <param name="platform">The target platform (used to guess the host type).</param>
            /// <param name="path">The host contents folder path.</param>
            public HostRuntime(TargetPlatform platform, string path)
            {
                Path = path;

                // Detect host type (this could check if monosgen-2.0 or hostfxr lib exists but platform-switch is easier)
                switch (platform)
                {
                case TargetPlatform.Windows:
                case TargetPlatform.Linux:
                case TargetPlatform.Mac:
                    Type = HostType.CoreCLR;
                    break;
                default:
                    Type = HostType.Mono;
                    break;
                }
            }

            /// <inheritdoc />
            public bool Equals(HostRuntime other)
            {
                return Path == other.Path && Type == other.Type;
            }

            /// <inheritdoc />
            public override bool Equals(object? obj)
            {
                return obj is HostRuntime other && Equals(other);
            }

            /// <inheritdoc />
            public override int GetHashCode()
            {
                return HashCode.Combine(Path, (int)Type);
            }

            /// <inheritdoc />
            public override string ToString()
            {
                return Path;
            }
        }

        private Dictionary<KeyValuePair<TargetPlatform, TargetArchitecture>, HostRuntime> _hostRuntimes = new();

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
            IEnumerable<string> dotnetSdkVersions = null, dotnetRuntimeVersions = null;
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
                rid = $"linux-{arch}";
                ridFallback = "";
                if (string.IsNullOrEmpty(dotnetPath))
                    dotnetPath ??= SearchForDotnetLocationLinux();
                if (dotnetPath == null)
                    ridFallback = Utilities.ReadProcessOutput("dotnet", "--info").Split('\n').FirstOrDefault(x => x.StartsWith(" RID:"), "").Replace("RID:", "").Trim();
                break;
            }
            case TargetPlatform.Mac:
            {
                rid = $"osx-{arch}";
                ridFallback = "";
                if (string.IsNullOrEmpty(dotnetPath))
                {
                    dotnetPath = "/usr/local/share/dotnet/";
                    if (!Directory.Exists(dotnetPath)) // Officially recommended dotnet location
                    {
                        if (Environment.GetEnvironmentVariable("PATH") is string globalBinPath)
                            dotnetPath = globalBinPath.Split(':').FirstOrDefault(x => File.Exists(Path.Combine(x, "dotnet")));
                        else
                            dotnetPath = string.Empty;
                    }
                }

                bool isRunningOnArm64Targetx64 = architecture == TargetArchitecture.ARM64 && (Configuration.BuildArchitectures != null && Configuration.BuildArchitectures[0] == TargetArchitecture.x64);

                // We need to support two paths here: 
                // 1. We are running an x64 binary and we are running on an arm64 host machine 
                // 2. We are running an Arm64 binary and we are targeting an x64 host machine
                if (Flax.Build.Platforms.MacPlatform.GetProcessIsTranslated() || isRunningOnArm64Targetx64) 
                {
                    rid = "osx-x64";
                    dotnetPath = Path.Combine(dotnetPath, "x64");
                    architecture = TargetArchitecture.x64;
                }

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
                dotnetSdkVersions = GetVersions(Path.Combine(dotnetPath, "sdk"));
            if (dotnetRuntimeVersions == null)
                dotnetRuntimeVersions = GetVersions(Path.Combine(dotnetPath, "shared/Microsoft.NETCore.App"));
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
            TryAddHostRuntime(TargetPlatform.Android, TargetArchitecture.ARM, "android-arm", "Runtime.Mono");
            TryAddHostRuntime(TargetPlatform.Android, TargetArchitecture.ARM64, "android-arm64", "Runtime.Mono");
            TryAddHostRuntime(TargetPlatform.iOS, TargetArchitecture.ARM, "ios-arm64", "Runtime.Mono");
            TryAddHostRuntime(TargetPlatform.iOS, TargetArchitecture.ARM64, "ios-arm64", "Runtime.Mono");

            // Found
            IsValid = true;
            Log.Info($"Using .NET SDK {VersionName}, runtime {RuntimeVersionName} ({RootPath})");
            foreach (var e in _hostRuntimes)
                Log.Verbose($"  - Host Runtime for {e.Key.Key} {e.Key.Value}");
        }

        /// <summary>
        /// Initializes platforms which can register custom host runtime location
        /// </summary>
        private static void InitPlatforms()
        {
            Platform.GetPlatform(TargetPlatform.Windows, true);
        }

        /// <summary>
        /// Gets the host runtime identifier for a given platform (eg. linux-arm64).
        /// </summary>
        /// <param name="platform">Target platform.</param>
        /// <param name="architecture">Target architecture.</param>
        /// <returns>Runtime identifier.</returns>
        public static string GetHostRuntimeIdentifier(TargetPlatform platform, TargetArchitecture architecture)
        {
            string result;
            switch (platform)
            {
            case TargetPlatform.Windows:
            case TargetPlatform.XboxOne:
            case TargetPlatform.XboxScarlett:
            case TargetPlatform.UWP:
                result = "win";
                break;
            case TargetPlatform.Linux:
                result = "linux";
                break;
            case TargetPlatform.PS4:
                result = "ps4";
                break;
            case TargetPlatform.PS5:
                result = "ps5";
                break;
            case TargetPlatform.Android:
                result = "android";
                break;
            case TargetPlatform.Switch:
                result = "switch";
                break;
            case TargetPlatform.Mac:
                result = "osx";
                break;
            case TargetPlatform.iOS:
                result = "ios";
                break;
            default: throw new InvalidPlatformException(platform);
            }
            switch (architecture)
            {
            case TargetArchitecture.x86:
                result += "-x86";
                break;
            case TargetArchitecture.x64:
                result += "-x64";
                break;
            case TargetArchitecture.ARM:
                result += "-arm";
                break;
            case TargetArchitecture.ARM64:
                result += "-arm64";
                break;
            default: throw new InvalidArchitectureException(architecture);
            }
            return result;
        }

        /// <summary>
        /// Prints the .NET runtimes hosts.
        /// </summary>
        public void PrintRuntimes()
        {
            InitPlatforms();
            foreach (var e in _hostRuntimes)
            {
                // Filter with input commandline
                TargetPlatform[] platforms = Configuration.BuildPlatforms;
                if (platforms != null && !platforms.Contains(e.Key.Key))
                    continue;
                TargetArchitecture[] architectures = Configuration.BuildArchitectures;
                if (architectures != null && !architectures.Contains(e.Key.Value))
                    continue;

                Log.Message($"{e.Key.Key}, {e.Key.Value}, {e.Value}");
            }
        }

        /// <summary>
        /// Gets the runtime host for a given target platform and architecture.
        /// </summary>
        public bool GetHostRuntime(TargetPlatform platform, TargetArchitecture arch, out HostRuntime hostRuntime)
        {
            InitPlatforms();
            return _hostRuntimes.TryGetValue(new KeyValuePair<TargetPlatform, TargetArchitecture>(platform, arch), out hostRuntime);
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
                _hostRuntimes[new KeyValuePair<TargetPlatform, TargetArchitecture>(platform, arch)] = new HostRuntime(platform, path);
        }

        private bool TryAddHostRuntime(TargetPlatform platform, TargetArchitecture arch, string rid, string runtimeName = null)
        {
            if (string.IsNullOrEmpty(rid))
                return false;

            // Pick pack folder
            var packFolder = Path.Combine(RootPath, $"packs/Microsoft.NETCore.App.Host.{rid}");
            var exists = Directory.Exists(packFolder);
            if (!exists && runtimeName != null)
            {
                packFolder = Path.Combine(RootPath, $"packs/Microsoft.NETCore.App.{runtimeName}.{rid}");
                exists = Directory.Exists(packFolder);
            }
            if (!exists)
                return false;

            // Pick version folder
            var versions = GetVersions(packFolder);
            var version = GetVersion(versions);
            var path = Path.Combine(packFolder, $"{version}/runtimes/{rid}/native");
            exists = Directory.Exists(path);

            if (exists)
                _hostRuntimes[new KeyValuePair<TargetPlatform, TargetArchitecture>(platform, arch)] = new HostRuntime(platform, path);
            return exists;
        }

        internal static string SelectVersionFolder(string root)
        {
            var versions = GetVersions(root);
            var version = GetVersion(versions);
            return Path.Combine(root, version);
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
            if (!Version.TryParse(version, out var ver))
                return null;
            return new Version(ver.Major, ver.Minor, ver.Build, rev);
        }

        private static IEnumerable<string> GetVersions(string folder)
        {
            return Directory.GetDirectories(folder).Select(Path.GetFileName);
        }

        private static string GetVersion(IEnumerable<string> versions)
        {
            // TODO: reject 'future' versions like .Net 8?
            return versions.OrderByDescending(ParseVersion).FirstOrDefault();
        }

        private static string SearchForDotnetLocationLinux()
        {
            if (File.Exists("/etc/dotnet/install_location")) // Officially recommended dotnet location file
                return File.ReadAllText("/etc/dotnet/install_location").Trim();
            if (File.Exists("/usr/share/dotnet/dotnet")) // Officially recommended dotnet location
                return "/usr/share/dotnet";
            if (File.Exists("/usr/lib/dotnet/dotnet")) // Deprecated recommended dotnet location
                return "/usr/lib/dotnet";
            if (Environment.GetEnvironmentVariable("PATH") is string globalBinPath) // Searching for dotnet binary
                return globalBinPath.Split(':').FirstOrDefault(x => File.Exists(Path.Combine(x, "dotnet")));
            return null;
        }
    }
}
