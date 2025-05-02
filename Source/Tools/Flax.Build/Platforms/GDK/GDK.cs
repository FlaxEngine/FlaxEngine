// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Linq;

namespace Flax.Build.Platforms
{
    /// <summary>
    /// The Microsoft Game Development Kit.
    /// </summary>
    /// <seealso cref="Sdk" />
    public sealed class GDK : Sdk
    {
        /// <summary>
        /// The singleton instance.
        /// </summary>
        public static readonly GDK Instance = new GDK();

        /// <inheritdoc />
        public override TargetPlatform[] Platforms => new[]
        {
            TargetPlatform.Windows,
        };

        /// <summary>
        /// Initializes a new instance of the <see cref="GDK"/> class.
        /// </summary>
        public GDK()
        {
            if (!Platforms.Contains(Platform.BuildTargetPlatform))
                return;

            var sdkDir = Environment.GetEnvironmentVariable("GameDKLatest");
            if (!Directory.Exists(sdkDir))
                sdkDir = Environment.GetEnvironmentVariable("GRDKLatest");
            if (Directory.Exists(sdkDir))
            {
                if (sdkDir.EndsWith("GRDK\\"))
                    sdkDir = sdkDir.Remove(sdkDir.Length - 6);
                else if (sdkDir.EndsWith("GRDK"))
                    sdkDir = sdkDir.Remove(sdkDir.Length - 5);
                RootPath = sdkDir;

                // Read the SDK version number
                string sdkManifest = Path.Combine(RootPath, "GRDK", "grdk.ini");
                if (File.Exists(sdkManifest))
                {
                    var contents = File.ReadAllText(sdkManifest);
                    const string prefix = "_xbld_edition=";
                    var start = contents.IndexOf(prefix) + prefix.Length;
                    var end = contents.IndexOf("\r\n_xbld_full_productbuild");
                    var versionText = contents.Substring(start, end - start);
                    Version = new Version(int.Parse(versionText), 0);

                    var minEdition = 230305;
                    if (Version.Major < minEdition)
                    {
                        Log.Error(string.Format("Unsupported GDK version {0}. Minimum supported is edition {1}.", Version.Major, minEdition));
                        return;
                    }

                    Log.Info(string.Format("Found GDK {0} at {1}", Version.Major, RootPath));
                    IsValid = true;
                }
            }
        }
    }
}
