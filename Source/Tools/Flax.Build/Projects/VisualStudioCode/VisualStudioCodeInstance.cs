// Copyright (c) 2012-2024 Flax Engine. All rights reserved.

using System;
using System.IO;
using Flax.Build.Platforms;

namespace Flax.Build.Projects.VisualStudioCode
{
    /// <summary>
    /// The Visual Studio Code instance utility.
    /// </summary>
    public sealed class VisualStudioCodeInstance
    {
        private static VisualStudioCodeInstance _instance;

        /// <summary>
        /// The install directory path.
        /// </summary>
        public string Path;

        /// <summary>
        /// Determines whether any version of the IDE is installed.
        /// </summary>
        /// <returns><c>true</c> if any version is installed; otherwise, <c>false</c>.</returns>
        public static bool HasIDE()
        {
            return GetInstance() != null;
        }

        /// <summary>
        /// Gets the installed Visual Studio instance.
        /// </summary>
        /// <returns>The install location.</returns>
        public static VisualStudioCodeInstance GetInstance()
        {
            if (_instance == null)
            {
                switch (Platform.BuildPlatform.Target)
                {
                case TargetPlatform.Windows:
                {
                    if (!WindowsPlatformBase.TryReadDirRegistryKey("HKEY_CURRENT_USER\\SOFTWARE\\Classes\\Applications\\Code.exe\\shell\\open\\command", string.Empty, out var cmd))
                    {
                        if (!WindowsPlatformBase.TryReadDirRegistryKey("HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\Applications\\Code.exe\\shell\\open\\command", string.Empty, out cmd))
                        {
                            return null;
                        }
                    }
                    var path = cmd.Substring(1, cmd.Length - "\" \"%1\"".Length - 1);
                    if (File.Exists(path))
                    {
                        _instance = new VisualStudioCodeInstance
                        {
                            Path = path,
                        };
                    }
                    break;
                }
                case TargetPlatform.Linux:
                {
                    var path = "/usr/bin/code";
                    if (File.Exists(path))
                    {
                        _instance = new VisualStudioCodeInstance
                        {
                            Path = path,
                        };
                    }
                    break;
                }
                case TargetPlatform.Mac:
                {
                    var userFolder = Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments);
                    var paths = new string[]
                    {
                        "/Applications/Visual Studio Code.app",
                        userFolder + "/Visual Studio Code.app",
                        userFolder + "/Downloads/Visual Studio Code.app",
                    };
                    foreach (var path in paths)
                    {
                        if (Directory.Exists(path))
                        {
                            _instance = new VisualStudioCodeInstance
                            {
                                Path = path,
                            };
                            break;
                        }
                    }
                    break;
                }
                }

                if (_instance != null)
                    Log.Verbose($"Found VS Code at {_instance.Path}");
            }

            return _instance;
        }
    }
}
