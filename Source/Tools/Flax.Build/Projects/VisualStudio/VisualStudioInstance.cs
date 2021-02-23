// Copyright (c) 2012-2020 Flax Engine. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Flax.Build.Platforms;
using Microsoft.VisualStudio.Setup.Configuration;

namespace Flax.Build.Projects.VisualStudio
{
    /// <summary>
    /// The Visual Studio instance utility.
    /// </summary>
    public sealed class VisualStudioInstance
    {
        private static List<VisualStudioInstance> _installDirs;
        private static int _hasFlaxVS;

        /// <summary>
        /// The version.
        /// </summary>
        public VisualStudioVersion Version;

        /// <summary>
        /// The install directory path.
        /// </summary>
        public string Path;

        /// <summary>
        /// Determinates whenever Visual Studio has Flax.VS extension installed (for C# scripts debugging).
        /// </summary>
        public static bool HasFlaxVS
        {
            get
            {
                if (_hasFlaxVS == 0)
                {
                    var p = Profiling.Begin("HasFlaxVS");
                    // This doesn't do the best job but it can detect if user has installed Flax.VS to any Visual Studio and should be enough
                    var appData = System.IO.Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Microsoft", "VisualStudio");
                    _hasFlaxVS = FindFlaxVS(appData) ? 1 : 2;
                    Profiling.End(p);
                }
                return _hasFlaxVS == 1;
            }
        }

        private static bool FindFlaxVS(string path)
        {
            if (!Directory.Exists(path))
                return false;

            var files = Directory.GetFiles(path);
            for (int i = 0; i < files.Length; i++)
            {
                if (files[i].EndsWith("Flax.VS.dll"))
                    return true;
            }

            var folders = Directory.GetDirectories(path);
            for (int i = 0; i < folders.Length; i++)
            {
                if (FindFlaxVS(folders[i]))
                    return true;
            }

            return false;
        }

        /// <summary>
        /// Determines whether any version of the IDE is installed.
        /// </summary>
        /// <returns><c>true</c> if any version is installed; otherwise, <c>false</c>.</returns>
        public static bool HasIDE()
        {
            return GetInstances().Count != 0;
        }

        /// <summary>
        /// Determines whether the specified version of the IDE is installed.
        /// </summary>
        /// <param name="version">The version to check.</param>
        /// <returns><c>true</c> if the specified version is installed; otherwise, <c>false</c>.</returns>
        public static bool HasIDE(VisualStudioVersion version)
        {
            return GetInstances().Any(x => x.Version == version);
        }

        /// <summary>
        /// Gets the installed Visual Studio instances (the first item is the latest version).
        /// </summary>
        /// <returns>The install locations.</returns>
        public static IReadOnlyList<VisualStudioInstance> GetInstances()
        {
            if (_installDirs == null)
            {
                _installDirs = new List<VisualStudioInstance>();

                if (Environment.OSVersion.Platform == PlatformID.Win32NT)
                {
                    // Visual Studio 2017-2020
                    List<VisualStudioInstance> preReleaseInstallDirs = null;
                    try
                    {
                        SetupConfiguration setup = new SetupConfiguration();
                        IEnumSetupInstances enumerator = setup.EnumAllInstances();

                        ISetupInstance[] instances = new ISetupInstance[1];
                        while (true)
                        {
                            enumerator.Next(1, instances, out int fetchedCount);
                            if (fetchedCount == 0)
                                break;

                            ISetupInstance2 instance = (ISetupInstance2)instances[0];
                            if ((instance.GetState() & InstanceState.Local) == InstanceState.Local)
                            {
                                VisualStudioVersion version;
                                string displayName = instance.GetDisplayName();
                                if (displayName.Contains("2019"))
                                    version = VisualStudioVersion.VisualStudio2019;
                                else if (displayName.Contains("2017"))
                                    version = VisualStudioVersion.VisualStudio2017;
                                else
                                    throw new Exception(string.Format("Unknown Visual Studio installation. Display name: {0}", displayName));

                                var vsInstance = new VisualStudioInstance
                                {
                                    Version = version,
                                    Path = instance.GetInstallationPath(),
                                };

                                if (instance is ISetupInstanceCatalog catalog && catalog.IsPrerelease())
                                {
                                    if (preReleaseInstallDirs == null)
                                        preReleaseInstallDirs = new List<VisualStudioInstance>();
                                    preReleaseInstallDirs.Add(vsInstance);
                                }
                                else
                                {
                                    _installDirs.Add(vsInstance);
                                }
                            }
                        }
                    }
                    catch
                    {
                        // Ignore errors
                    }

                    // Add pre-release locations after the normal installations
                    if (preReleaseInstallDirs != null)
                        _installDirs.AddRange(preReleaseInstallDirs);

                    // Visual Studio 2015
                    if (WindowsPlatform.TryReadInstallDirRegistryKey32("Microsoft\\VisualStudio\\SxS\\VS7", "14.0", out var dir))
                    {
                        _installDirs.Add(new VisualStudioInstance
                        {
                            Version = VisualStudioVersion.VisualStudio2015,
                            Path = dir,
                        });
                    }
                }

                foreach (var e in _installDirs)
                {
                    Log.Verbose($"Found {e.Version} at {e.Path}");
                }
            }

            return _installDirs;
        }
    }
}
