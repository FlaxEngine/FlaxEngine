// Copyright (c) Wojciech Figat. All rights reserved.

// ReSharper disable InconsistentNaming

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Flax.Build.Projects;
using Flax.Build.Projects.VisualStudio;
using Flax.Build.Projects.VisualStudioCode;
using Microsoft.Win32;

#pragma warning disable CA1416

namespace Flax.Build.Platforms
{
    /// <summary>
    /// The Windows platform toolset versions.
    /// </summary>
    public enum WindowsPlatformToolset
    {
        /// <summary>
        /// The same as Visual Studio version (for the project files generation).
        /// </summary>
        Default = 0,

        /// <summary>
        /// Use the latest toolset.
        /// </summary>
        Latest = 1,

        /// <summary>
        /// Visual Studio 2015
        /// </summary>
        v140 = 140,

        /// <summary>
        /// Visual Studio 2017
        /// </summary>
        v141 = 141,

        /// <summary>
        /// Visual Studio 2019
        /// </summary>
        v142 = 142,

        /// <summary>
        /// Visual Studio 2022
        /// </summary>
        v143 = 143,

        /// <summary>
        /// Visual Studio 2022 (v17.10 and later)
        /// </summary>
        v144 = 144,
    }

    /// <summary>
    /// The Windows platform SDK versions.
    /// </summary>
    public enum WindowsPlatformSDK
    {
        /// <summary>
        /// Use the latest SDK.
        /// </summary>
        Latest,

        /// <summary>
        /// Windows 8.1 SDK
        /// </summary>
        v8_1,

        /// <summary>
        /// Windows 10 SDK (10.0.10240.0) RTM (even if never named like that officially)
        /// </summary>
        v10_0_10240_0,

        /// <summary>
        /// Windows 10 SDK (10.0.10586.0) November 2015 Update
        /// </summary>
        v10_0_10586_0,

        /// <summary>
        /// Windows 10 SDK (10.0.14393.0) 2016 Anniversary Update
        /// </summary>
        v10_0_14393_0,

        /// <summary>
        /// Windows 10 SDK (10.0.15063.0) 2017 Creators Update
        /// </summary>
        v10_0_15063_0,

        /// <summary>
        /// Windows 10 SDK (10.0.16299.0) 2017 Fall Creators Update
        /// </summary>
        v10_0_16299_0,

        /// <summary>
        /// Windows 10 SDK (10.0.17134.0) April 2018 Update
        /// </summary>
        v10_0_17134_0,

        /// <summary>
        /// Windows 10 SDK (10.0.17763.0) October 2018 Update
        /// </summary>
        v10_0_17763_0,

        /// <summary>
        /// Windows 10 SDK (10.0.18362.0) May 2019 Update
        /// </summary>
        v10_0_18362_0,

        /// <summary>
        /// Windows 10 SDK (10.0.19041.0)
        /// </summary>
        v10_0_19041_0,

        /// <summary>
        /// Windows 10 SDK (10.0.20348.0) 21H1
        /// </summary>
        v10_0_20348_0,

        /// <summary>
        /// Windows 11 SDK (10.0.22000.0)
        /// </summary>
        v10_0_22000_0,

        /// <summary>
        /// Windows 11 SDK (10.0.22621.0) 22H2
        /// </summary>
        v10_0_22621_0,

        /// <summary>
        /// Windows 11 SDK (10.0.26100.0) 24H2
        /// </summary>
        v10_0_26100_0,
    }

    /// <summary>
    /// The Microsoft Windows base platform implementation.
    /// </summary>
    /// <seealso cref="Platform" />
    public abstract class WindowsPlatformBase : Platform, IProjectCustomizer
    {
        private static Dictionary<WindowsPlatformToolset, string> _toolsets;
        private static Dictionary<WindowsPlatformSDK, string> _sdks;

        /// <summary>
        /// The flag used for <see cref="HasRequiredSDKsInstalled"/>.
        /// </summary>
        protected bool _hasRequiredSDKsInstalled;

        /// <inheritdoc />
        public override bool HasRequiredSDKsInstalled => _hasRequiredSDKsInstalled;

        /// <inheritdoc />
        public override bool HasSharedLibrarySupport => true;

        /// <inheritdoc />
        public override bool HasExecutableFileReferenceSupport => true;

        /// <inheritdoc />
        public override string ExecutableFileExtension => ".exe";

        /// <inheritdoc />
        public override string SharedLibraryFileExtension => ".dll";

        /// <inheritdoc />
        public override string StaticLibraryFileExtension => ".lib";

        /// <inheritdoc />
        public override string ProgramDatabaseFileExtension => ".pdb";

        /// <inheritdoc />
        public override ProjectFormat DefaultProjectFormat
        {
            get
            {
                if (VisualStudioInstance.HasIDE())
                    return ProjectFormat.VisualStudio;
                if (VisualStudioCodeInstance.HasIDE())
                    return ProjectFormat.VisualStudioCode;
                return ProjectFormat.VisualStudio;
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="WindowsPlatformBase"/> class.
        /// </summary>
        protected WindowsPlatformBase()
        {
            var sdsk = GetSDKs();
            var toolsets = GetToolsets();
            _hasRequiredSDKsInstalled = sdsk.Count > 0 && toolsets.Count > 0;
        }

        /// <summary>
        /// Tries to reads a directory name stored in a registry key.
        /// </summary>
        /// <param name="keyName">The key to read from.</param>
        /// <param name="valueName">The value within the key to read.</param>
        /// <param name="value">The directory read from the registry key.</param>
        /// <returns>True if the key was read, false if it was missing or empty.</returns>
        public static bool TryReadDirRegistryKey(string keyName, string valueName, out string value)
        {
            value = Registry.GetValue(keyName, valueName, null) as string;
            if (string.IsNullOrEmpty(value))
            {
                value = null;
                return false;
            }
            return true;
        }

        /// <summary>
        /// Tries to reads an install directory for a 32-bit program from a registry key. It checks for per-user and machine wide settings, and under the Wow64 virtual keys.
        /// </summary>
        /// <param name="keySuffix">The path to the key to read, under one of the roots listed above.</param>
        /// <param name="valueName">The value to be read.</param>
        /// <param name="dir">When this method completes with success it contains a directory corresponding to the value read.</param>
        /// <returns>True if the key was read, false otherwise.</returns>
        public static bool TryReadInstallDirRegistryKey32(string keySuffix, string valueName, out string dir)
        {
            if (TryReadDirRegistryKey("HKEY_CURRENT_USER\\SOFTWARE\\" + keySuffix, valueName, out dir))
                return true;
            if (TryReadDirRegistryKey("HKEY_LOCAL_MACHINE\\SOFTWARE\\" + keySuffix, valueName, out dir))
                return true;
            if (TryReadDirRegistryKey("HKEY_CURRENT_USER\\SOFTWARE\\Wow6432Node\\" + keySuffix, valueName, out dir))
                return true;
            if (TryReadDirRegistryKey("HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\" + keySuffix, valueName, out dir))
                return true;
            return false;
        }

        private static void FindMsvcToolsets(string rootDir)
        {
            if (!Directory.Exists(rootDir))
                return;
            var toolsets = Directory.GetDirectories(rootDir);
            foreach (var toolset in toolsets)
            {
                if (Version.TryParse(Path.GetFileName(toolset), out var version) &&
                    (File.Exists(Path.Combine(toolset, "bin", "Hostx64", "x64", "cl.exe")) ||
                     File.Exists(Path.Combine(toolset, "bin", "Hostx86", "x64", "cl.exe"))))
                {
                    if (version.Major == 14 && version.Minor / 10 == 1)
                        _toolsets[WindowsPlatformToolset.v141] = toolset;
                    else if (version.Major == 14 && version.Minor / 10 == 2)
                        _toolsets[WindowsPlatformToolset.v142] = toolset;
                    else if (version.Major == 14 && version.Minor / 10 == 3)
                        _toolsets[WindowsPlatformToolset.v143] = toolset;
                    else if (version.Major == 14 && version.Minor / 10 == 4)
                        _toolsets[WindowsPlatformToolset.v144] = toolset;
                    else
                        Log.Warning("Found Unsupported MSVC toolset version: " + version);
                }
            }
        }

        /// <summary>
        /// Finds all the directories containing the Windows platform toolset.
        /// </summary>
        /// <returns>The collection of installed toolsets.</returns>
        public static Dictionary<WindowsPlatformToolset, string> GetToolsets()
        {
            if (_toolsets != null)
                return _toolsets;
            _toolsets = new Dictionary<WindowsPlatformToolset, string>();

            // Skip if running on non-Windows system
            if (BuildTargetPlatform != TargetPlatform.Windows)
                return _toolsets;

            var vsInstances = VisualStudioInstance.GetInstances();

            // Visual Studio 2015 - single instance
            var vs2015 = vsInstances.FirstOrDefault(x => x.Version == VisualStudioVersion.VisualStudio2015);
            if (vs2015 != null)
            {
                string rootDir = Path.Combine(vs2015.Path, "VC");
                if (Directory.Exists(rootDir) &&
                    (File.Exists(Path.Combine(rootDir, "bin", "amd64", "cl.exe")) ||
                     File.Exists(Path.Combine(rootDir, "bin", "x86_amd64", "cl.exe"))))
                {
                    _toolsets[WindowsPlatformToolset.v140] = rootDir;
                }
            }

            // Visual Studio 2017-2022 - multiple instances
            foreach (var vs in vsInstances.Where(x =>
                                                 x.Version == VisualStudioVersion.VisualStudio2017 ||
                                                 x.Version == VisualStudioVersion.VisualStudio2019 ||
                                                 x.Version == VisualStudioVersion.VisualStudio2022
                                                ))
            {
                FindMsvcToolsets(Path.Combine(vs.Path, "VC", "Tools", "MSVC"));
            }

            foreach (var e in _toolsets)
                Log.Verbose(string.Format("Found Windows toolset {0} at {1}", e.Key, e.Value));
            return _toolsets;
        }

        /// <summary>
        /// Gets the SDK version.
        /// </summary>
        /// <param name="sdk">The SDK.</param>
        /// <returns>The version of the SDK enum.</returns>
        public static Version GetSDKVersion(WindowsPlatformSDK sdk)
        {
            switch (sdk)
            {
            case WindowsPlatformSDK.v8_1: return new Version(8, 1);
            case WindowsPlatformSDK.v10_0_10240_0: return new Version(10, 0, 10240, 0);
            case WindowsPlatformSDK.v10_0_10586_0: return new Version(10, 0, 10586, 0);
            case WindowsPlatformSDK.v10_0_14393_0: return new Version(10, 0, 14393, 0);
            case WindowsPlatformSDK.v10_0_15063_0: return new Version(10, 0, 15063, 0);
            case WindowsPlatformSDK.v10_0_16299_0: return new Version(10, 0, 16299, 0);
            case WindowsPlatformSDK.v10_0_17134_0: return new Version(10, 0, 17134, 0);
            case WindowsPlatformSDK.v10_0_17763_0: return new Version(10, 0, 17763, 0);
            case WindowsPlatformSDK.v10_0_18362_0: return new Version(10, 0, 18362, 0);
            case WindowsPlatformSDK.v10_0_19041_0: return new Version(10, 0, 19041, 0);
            case WindowsPlatformSDK.v10_0_20348_0: return new Version(10, 0, 20348, 0);
            case WindowsPlatformSDK.v10_0_22000_0: return new Version(10, 0, 22000, 0);
            case WindowsPlatformSDK.v10_0_22621_0: return new Version(10, 0, 22621, 0);
            case WindowsPlatformSDK.v10_0_26100_0: return new Version(10, 0, 26100, 0);
            default: throw new ArgumentOutOfRangeException(nameof(sdk), sdk, null);
            }
        }

        /// <summary>
        /// Finds all the directories containing the Windows SDKs.
        /// </summary>
        /// <returns>The collection of installed SDks.</returns>
        public static Dictionary<WindowsPlatformSDK, string> GetSDKs()
        {
            if (_sdks != null)
                return _sdks;
            _sdks = new Dictionary<WindowsPlatformSDK, string>();

            // Skip if running on non-Windows system
            if (BuildTargetPlatform != TargetPlatform.Windows)
                return _sdks;

            // Check Windows 8.1 SDK
            if (TryReadInstallDirRegistryKey32("Microsoft\\Microsoft SDKs\\Windows\\v8.1", "InstallationFolder", out var sdk81))
            {
                if (File.Exists(Path.Combine(sdk81, "Include", "um", "windows.h")))
                {
                    _sdks.Add(WindowsPlatformSDK.v8_1, sdk81);
                }
            }

            // Check Windows 10 SDKs
            var sdk10Roots = new HashSet<string>();
            {
                string rootDir;
                if (TryReadInstallDirRegistryKey32("Microsoft\\Windows Kits\\Installed Roots", "KitsRoot10", out rootDir))
                {
                    sdk10Roots.Add(rootDir);
                }
                if (TryReadInstallDirRegistryKey32("Microsoft\\Microsoft SDKs\\Windows\\v10.0", "InstallationFolder", out rootDir))
                {
                    sdk10Roots.Add(rootDir);
                }
            }
            var win10Sdks = new[]
            {
                WindowsPlatformSDK.v10_0_10240_0,
                WindowsPlatformSDK.v10_0_10586_0,
                WindowsPlatformSDK.v10_0_14393_0,
                WindowsPlatformSDK.v10_0_15063_0,
                WindowsPlatformSDK.v10_0_16299_0,
                WindowsPlatformSDK.v10_0_17134_0,
                WindowsPlatformSDK.v10_0_17763_0,
                WindowsPlatformSDK.v10_0_18362_0,
                WindowsPlatformSDK.v10_0_19041_0,
                WindowsPlatformSDK.v10_0_20348_0,
                WindowsPlatformSDK.v10_0_22000_0,
                WindowsPlatformSDK.v10_0_22621_0,
                WindowsPlatformSDK.v10_0_26100_0,
            };
            foreach (var sdk10 in sdk10Roots)
            {
                var includeRootDir = Path.Combine(sdk10, "Include");
                if (Directory.Exists(includeRootDir))
                {
                    foreach (var includeDir in Directory.GetDirectories(includeRootDir))
                    {
                        if (Version.TryParse(Path.GetFileName(includeDir), out var version) && File.Exists(Path.Combine(includeDir, "um", "windows.h")))
                        {
                            bool unknown = true;
                            foreach (var win10Sdk in win10Sdks)
                            {
                                if (version == GetSDKVersion(win10Sdk))
                                {
                                    _sdks.Add(win10Sdk, sdk10);
                                    unknown = false;
                                    break;
                                }
                            }
                            if (unknown)
                                Log.Warning(string.Format("Unknown Windows 10 SDK version {0} at {1}", version, sdk10));
                        }
                    }
                }
            }

            foreach (var e in _sdks)
            {
                Log.Verbose(string.Format("Found Windows SDK {0} at {1}", e.Key, e.Value));
            }

            return _sdks;
        }

        /// <summary>
        /// Gets the path to the VC++ tool binaries for specified host and target architectures.
        /// </summary>
        /// <param name="toolset">The version of the toolset to use.</param>
        /// <param name="hostArchitecture">The host architecture for native binaries.</param>
        /// <param name="architecture">The target architecture to build for.</param>
        /// <returns>The directory containing the toolchain binaries.</returns>
        public static string GetVCToolPath(WindowsPlatformToolset toolset, TargetArchitecture hostArchitecture, TargetArchitecture architecture)
        {
            if (architecture == TargetArchitecture.AnyCPU)
                architecture = hostArchitecture;
            var toolsets = GetToolsets();
            var vcToolChainDir = toolsets[toolset];

            switch (toolset)
            {
            case WindowsPlatformToolset.v140:
            {
                if (architecture == TargetArchitecture.ARM64)
                {
                    Log.Verbose(string.Format("Unsupported {0} architecture for v140 toolset", hostArchitecture.ToString()));
                    return null;
                }
                else if (hostArchitecture == TargetArchitecture.x64)
                {
                    string nativeCompilerPath = Path.Combine(vcToolChainDir, "bin", "amd64", "cl.exe");
                    if (File.Exists(nativeCompilerPath))
                        return Path.GetDirectoryName(nativeCompilerPath);

                    string crossCompilerPath = Path.Combine(vcToolChainDir, "bin", "x86_amd64", "cl.exe");
                    if (File.Exists(crossCompilerPath))
                        return Path.GetDirectoryName(crossCompilerPath);

                    Log.Verbose(string.Format("No {0} host compiler toolchain found in {1} or {2}", hostArchitecture.ToString(), nativeCompilerPath, crossCompilerPath));
                    return null;
                }
                else if (hostArchitecture == TargetArchitecture.x86)
                {
                    string compilerPath = Path.Combine(vcToolChainDir, "bin", "cl.exe");
                    if (File.Exists(compilerPath))
                        return Path.GetDirectoryName(compilerPath);
                    Log.Verbose(string.Format("No {0} host compiler toolchain found in {1}", hostArchitecture.ToString(), compilerPath));
                    return null;
                }
                else
                {
                    Log.Verbose(string.Format("Unsupported {0} host compiler for v140 toolset", hostArchitecture.ToString()));
                    return null;
                }
            }
            case WindowsPlatformToolset.v141:
            case WindowsPlatformToolset.v142:
            case WindowsPlatformToolset.v143:
            case WindowsPlatformToolset.v144:
            {
                string hostFolder = hostArchitecture == TargetArchitecture.x86 ? "HostX86" : $"Host{hostArchitecture.ToString().ToLower()}";
                string nativeCompilerPath = Path.Combine(vcToolChainDir, "bin", hostFolder, architecture.ToString().ToLower(), "cl.exe");
                if (File.Exists(nativeCompilerPath))
                    return Path.GetDirectoryName(nativeCompilerPath);

                string crossCompilerPath = Path.Combine(vcToolChainDir, "bin", hostFolder, architecture.ToString().ToLower(), "cl.exe");
                if (File.Exists(crossCompilerPath))
                    return Path.GetDirectoryName(crossCompilerPath);

                Log.Verbose(string.Format("No {0} host compiler toolchain found in {1} or {2}", hostArchitecture.ToString(), nativeCompilerPath, crossCompilerPath));
                return null;
            }
            default: throw new ArgumentOutOfRangeException(nameof(toolset), toolset, null);
            }
        }

        /// <inheritdoc />
        void IProjectCustomizer.GetSolutionArchitectureName(TargetArchitecture architecture, ref string name)
        {
        }

        /// <inheritdoc />
        void IProjectCustomizer.GetProjectArchitectureName(Project project, Platform platform, TargetArchitecture architecture, ref string name)
        {
            if (architecture == TargetArchitecture.x86)
            {
                name = "Win32";
            }
        }
    }
}

#pragma warning restore CA1416
