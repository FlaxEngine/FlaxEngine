// Copyright (c) 2012-2019 Wojciech Figat. All rights reserved.

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
        public override TargetPlatform[] Platforms => new []
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
            if (string.IsNullOrEmpty(sdkPath))
            {
                // Look for ndk installed side-by-side with an sdk
                if (AndroidSdk.Instance.IsValid && Directory.Exists(Path.Combine(AndroidSdk.Instance.RootPath, "ndk")))
                {
                    var subdirs = Directory.GetDirectories(Path.Combine(AndroidSdk.Instance.RootPath, "ndk"));
                    if (subdirs.Length != 0)
                    {
                        Array.Sort(subdirs, (a, b) => Version.Parse(Path.GetFileName(a)).CompareTo(Version.Parse(Path.GetFileName(b))));
                        sdkPath = subdirs.Last();
                    }
                }
            }
            else if (!Directory.Exists(sdkPath))
            {
                Log.Warning(string.Format("Specified Android NDK folder in ANDROID_NDK env variable doesn't exist ({0})", sdkPath));
            }
            if (!string.IsNullOrEmpty(sdkPath))
            {
                var lines = File.ReadAllLines(Path.Combine(sdkPath, "source.properties"));
                if (lines.Length > 1)
                {
                    RootPath = sdkPath;
                    var ver = lines[1].Substring(lines[1].IndexOf(" = ", StringComparison.Ordinal) + 2);
                    Version = Version.Parse(ver);
                    IsValid = true;
                    Log.Info(string.Format("Found Android NDK {1} at {0}", RootPath, Version));
                }
            }
        }
    }
}
