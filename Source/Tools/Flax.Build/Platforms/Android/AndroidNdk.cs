// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Linq;

namespace Flax.Build.Platforms
{
    /// <summary>
    /// The Android NDK.
    /// </summary>
    /// <seealso cref="Sdk" />
    public sealed class AndroidNdk : Sdk
    {
        /// <summary>
        /// The singleton instance.
        /// </summary>
        public static readonly AndroidNdk Instance = new AndroidNdk();

        /// <inheritdoc />
        public override TargetPlatform[] Platforms => new[]
        {
            TargetPlatform.Windows,
            TargetPlatform.Linux,
        };

        /// <summary>
        /// Initializes a new instance of the <see cref="AndroidNdk"/> class.
        /// </summary>
        public AndroidNdk()
        {
            if (!Platforms.Contains(Platform.BuildTargetPlatform))
                return;

            // Find Android NDK folder path
            var sdkPath = Environment.GetEnvironmentVariable("ANDROID_NDK");
            FindNDKVersion(sdkPath);
            if (!IsValid)
            {
                if (!string.IsNullOrEmpty(sdkPath))
                {
                    Log.Warning(string.Format("Specified Android NDK folder in ANDROID_NDK env variable doesn't contain valid NDK ({0})", sdkPath));
                }

                // Look for ndk installed side-by-side with a sdk
                if (AndroidSdk.Instance.IsValid)
                {
                    sdkPath = Path.Combine(AndroidSdk.Instance.RootPath, "ndk");
                    FindNDKVersion(sdkPath);
                }
            }
        }

        private void FindNDKVersion(string folder)
        {
            // Skip if folder is invalid or missing
            if (string.IsNullOrEmpty(folder) || !Directory.Exists(folder))
                return;

            // Check that explicit folder
            FindNDK(folder);
            if (IsValid)
                return;

            // Check folders with versions
            var subDirs = Directory.GetDirectories(folder);
            if (subDirs.Length != 0)
            {
                Utilities.SortVersionDirectories(subDirs);
                FindNDK(subDirs.Last());
            }

            if (!IsValid)
            {
                Log.Warning(string.Format("Failed to detect Android NDK version ({0})", folder));
            }
        }

        private void FindNDK(string sdkPath)
        {
            if (!string.IsNullOrEmpty(sdkPath) && Directory.Exists(sdkPath))
            {
                var sourceProperties = Path.Combine(sdkPath, "source.properties");
                if (File.Exists(sourceProperties))
                {
                    var lines = File.ReadAllLines(sourceProperties);
                    if (lines.Length > 1)
                    {
                        var ver = lines[1].Substring(lines[1].IndexOf(" = ", StringComparison.Ordinal) + 2);
                        if (Version.TryParse(ver, out var v))
                        {
                            Version = v;
                            IsValid = true;
                        }
                    }
                }
                else if (Version.TryParse(Path.GetFileName(sdkPath), out var v))
                {
                    Version = v;
                    IsValid = true;
                }
            }
            if (IsValid)
            {
                RootPath = sdkPath;
                Log.Info(string.Format("Found Android NDK {1} at {0}", RootPath, Version));
            }
        }
    }
}
