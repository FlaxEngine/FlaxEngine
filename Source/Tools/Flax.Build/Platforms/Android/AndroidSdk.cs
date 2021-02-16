// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Linq;

namespace Flax.Build.Platforms
{
    /// <summary>
    /// The Android SDK.
    /// </summary>
    /// <seealso cref="Sdk" />
    public sealed class AndroidSdk : Sdk
    {
        /// <summary>
        /// The singleton instance.
        /// </summary>
        public static readonly AndroidSdk Instance = new AndroidSdk();

        /// <inheritdoc />
        public override TargetPlatform[] Platforms => new []
        {
            TargetPlatform.Windows,
            TargetPlatform.Linux,
        };

        /// <summary>
        /// Initializes a new instance of the <see cref="AndroidSdk"/> class.
        /// </summary>
        public AndroidSdk()
        {
            if (!Platforms.Contains(Platform.BuildTargetPlatform))
                return;

            // Find Android SDK folder path
            var sdkPath = Environment.GetEnvironmentVariable("ANDROID_SDK");
            if (string.IsNullOrEmpty(sdkPath))
            {
                // Look for adb in Android folders of some common locations
                string uniqueFile = Path.Combine("platform-tools", "adb.exe");
                string[] searchDirs =
                {
                    // <user>/AppData/Local
                    Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                    // Program Files
                    Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles),
                    // Program Files (x86)
                    Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles) + " (x86)",
                    // <user>/AppData/Roaming
                    Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),
                };
                sdkPath = null;
                foreach (string searchDir in searchDirs)
                {
                    string androidDir = Path.Combine(searchDir, "Android");
                    if (Directory.Exists(androidDir))
                    {
                        string[] subDirs = Directory.GetDirectories(androidDir, "*sdk*", SearchOption.TopDirectoryOnly);
                        foreach (string subDir in subDirs)
                        {
                            string path = Path.Combine(subDir, uniqueFile);
                            if (File.Exists(path))
                            {
                                sdkPath = subDir;
                                break;
                            }
                        }
                    }
                    if (sdkPath != null)
                        break;
                }
            }
            else if (!Directory.Exists(sdkPath))
            {
                Log.Warning(string.Format("Specified Android SDK folder in ANDROID_SDK env variable doesn't exist ({0})", sdkPath));
            }
            if (string.IsNullOrEmpty(sdkPath))
            {
                Log.Warning("Missing Android SDK. Cannot build for Android platform.");
            }
            else
            {
                RootPath = sdkPath;
                Version = new Version(1, 0);
                IsValid = true;
                Log.Info(string.Format("Found Android SDK at {0}", RootPath));
            }
        }

        /// <summary>
        /// Gets the name of the host platform used in Android SDK toolset.
        /// </summary>
        /// <returns>The name.</returns>
        public static string GetHostName()
        {
            string hostName;
            switch (Platform.BuildPlatform.Target)
            {
            case TargetPlatform.Windows:
                hostName = Environment.Is64BitOperatingSystem ? "windows-x86_64" : "windows";
                break;
            case TargetPlatform.Linux:
                hostName = "linux-x86_64";
                break;
            default: throw new InvalidPlatformException(Platform.BuildPlatform.Target);
            }
            return hostName;
        }
    }
}
