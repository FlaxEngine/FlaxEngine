// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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

        /// <summary>
        /// Exception when .NET SDK is missing.
        /// </summary>
        public sealed class MissingException : Exception
        {
            /// <summary>
            /// Init with a proper message.
            /// </summary>
            public MissingException()
            : base(string.IsNullOrEmpty(Configuration.Dotnet) ? $"Missing .NET SDK {MinimumVersion} (or higher)." : $"Missing .NET SDK {Configuration.Dotnet}.")
            {
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
        public static Version MinimumVersion => new Version(8, 0);

        /// <summary>
        /// The maximum SDK version.
        /// </summary>
        public static Version MaximumVersion => new Version(9, 0);

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
        /// Maximum supported C#-language version for the SDK.
        /// </summary>
        public string CSharpLanguageVersion => Version.Major switch
        {
            _ when Version.Major >= 9 => "13.0",
            _ when Version.Major >= 8 => "12.0",
            _ when Version.Major >= 7 => "11.0",
            _ when Version.Major >= 6 => "10.0",
            _ when Version.Major >= 5 => "9.0",
            _ => "7.3",
        };

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
                ridFallback = Utilities.ReadProcessOutput("dotnet", "--info").Split('\n').FirstOrDefault(x => x.StartsWith(" RID:"), "").Replace("RID:", "").Trim();
                if (string.IsNullOrEmpty(dotnetPath))
                    dotnetPath ??= SearchForDotnetLocationLinux();
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
                if (Flax.Build.Platforms.MacPlatform.BuildingForx64)
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

            dotnetSdkVersions = MergeVersions(dotnetSdkVersions, GetVersions(Path.Combine(dotnetPath, "sdk")));
            dotnetRuntimeVersions = MergeVersions(dotnetRuntimeVersions, GetVersions(Path.Combine(dotnetPath, "shared", "Microsoft.NETCore.App")));

            dotnetSdkVersions = dotnetSdkVersions.Where(x => File.Exists(Path.Combine(dotnetPath, "sdk", x, ".version")));
            dotnetRuntimeVersions = dotnetRuntimeVersions.Where(x => File.Exists(Path.Combine(dotnetPath, "shared", "Microsoft.NETCore.App", x, ".version")));
            dotnetRuntimeVersions = dotnetRuntimeVersions.Where(x => Directory.Exists(Path.Combine(dotnetPath, "packs", "Microsoft.NETCore.App.Ref", x)));

            dotnetSdkVersions = dotnetSdkVersions.OrderByDescending(ParseVersion);
            dotnetRuntimeVersions = dotnetRuntimeVersions.OrderByDescending(ParseVersion);

            Log.Verbose($"Found the following .NET SDK versions: {string.Join(", ", dotnetSdkVersions)}");
            Log.Verbose($"Found the following .NET runtime versions: {string.Join(", ", dotnetRuntimeVersions)}");

            var dotnetSdkVersion = GetVersion(dotnetSdkVersions);
            var dotnetRuntimeVersion = GetVersion(dotnetRuntimeVersions);
            if (!string.IsNullOrEmpty(dotnetRuntimeVersion) && ParseVersion(dotnetRuntimeVersion).Major > ParseVersion(dotnetSdkVersion).Major)
            {
                // Make sure the reference assemblies are not newer than the SDK itself
                var dotnetRuntimeVersionsRemaining = dotnetRuntimeVersions;
                do
                {
                    dotnetRuntimeVersionsRemaining = dotnetRuntimeVersionsRemaining.Skip(1);
                    dotnetRuntimeVersion = GetVersion(dotnetRuntimeVersionsRemaining);
                } while (!string.IsNullOrEmpty(dotnetRuntimeVersion) && ParseVersion(dotnetRuntimeVersion).Major > ParseVersion(dotnetSdkVersion).Major);
            }
            
            var minVer = string.IsNullOrEmpty(Configuration.Dotnet) ? MinimumVersion.ToString() : Configuration.Dotnet;
            if (string.IsNullOrEmpty(dotnetSdkVersion))
            {
                if (dotnetSdkVersions.Any())
                    Log.Warning($"Unsupported .NET SDK versions found: {string.Join(", ", dotnetSdkVersions)}. Minimum version required is .NET {minVer}.");
                else
                    Log.Warning($"Missing .NET SDK. Minimum version required is .NET {minVer}.");
                return;
            }
            if (string.IsNullOrEmpty(dotnetRuntimeVersion))
            {
                if (dotnetRuntimeVersions.Any())
                    Log.Warning($"Unsupported .NET runtime versions found: {string.Join(", ", dotnetRuntimeVersions)}. Minimum version required is .NET {minVer}.");
                else
                    Log.Warning($"Missing .NET runtime. Minimum version required is .NET {minVer}.");
                return;
            }
            RootPath = dotnetPath;
            Version = ParseVersion(dotnetSdkVersion);
            VersionName = dotnetSdkVersion;
            RuntimeVersionName = dotnetRuntimeVersion;

            // Pick SDK runtime
            if (!TryAddHostRuntime(platform, architecture, rid) && !TryAddHostRuntime(platform, architecture, ridFallback))
            {
                var path = Path.Combine(RootPath, "packs", $"Microsoft.NETCore.App.Host.{rid}", RuntimeVersionName, "runtimes", rid, "native");
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
                _hostRuntimes[new KeyValuePair<TargetPlatform, TargetArchitecture>(platform, arch)] = new HostRuntime(platform, Utilities.NormalizePath(path));
            return exists;
        }

        internal static string SelectVersionFolder(string root)
        {
            var versions = GetVersions(root);
            var version = GetVersion(versions);
            if (version == null)
                throw new Exception($"Failed to select dotnet version from '{root}' ({string.Join(", ", versions)})");
            return Path.Combine(root, version);
        }

        private static IEnumerable<string> MergeVersions(IEnumerable<string> a, IEnumerable<string> b)
        {
            if (a == null || !a.Any())
                return b;
            if (b == null || !b.Any())
                return a;
            var result = new HashSet<string>();
            result.AddRange(a);
            result.AddRange(b);
            return result;
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
            if (ver.Build == -1)
                return new Version(ver.Major, ver.Minor);
            return new Version(ver.Major, ver.Minor, ver.Build, rev);
        }

        private static IEnumerable<string> GetVersions(string folder)
        {
            return Directory.GetDirectories(folder).Select(Path.GetFileName);
        }

        private static string GetVersion(IEnumerable<string> versions)
        {
            Version dotnetVer = null;
            int dotnetVerNum = -1;
            if (!string.IsNullOrEmpty(Configuration.Dotnet))
            {
                dotnetVer = ParseVersion(Configuration.Dotnet);
                if (int.TryParse(Configuration.Dotnet, out var tmp) && tmp >= MinimumVersion.Major)
                    dotnetVerNum = tmp;
            }
            var sorted = versions.OrderByDescending(ParseVersion);
            foreach (var version in sorted)
            {
                var v = ParseVersion(version);

                // Filter by version specified in command line
                if (dotnetVer != null)
                {
                    if (dotnetVer.Major != v.Major)
                        continue;
                    if (dotnetVer.Minor != v.Minor)
                        continue;
                    if (dotnetVer.Revision != -1 && dotnetVer.Revision != v.Revision)
                        continue;
                    if (dotnetVer.Build != -1 && dotnetVer.Build != v.Build)
                        continue;
                }
                else if (dotnetVerNum != -1)
                {
                    if (dotnetVerNum != v.Major)
                        continue;
                }

                // Filter by min/max versions supported by Flax.Build
                if (v.Major >= MinimumVersion.Major && v.Major <= MaximumVersion.Major)
                    return version;
            }
            return null;
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
