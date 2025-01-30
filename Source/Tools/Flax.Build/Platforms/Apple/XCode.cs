// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Linq;

namespace Flax.Build.Platforms
{
    /// <summary>
    /// The XCode app.
    /// </summary>
    /// <seealso cref="Sdk" />
    public sealed class XCode : Sdk
    {
        /// <summary>
        /// The singleton instance.
        /// </summary>
        public static readonly XCode Instance = new XCode();

        /// <inheritdoc />
        public override TargetPlatform[] Platforms => new[]
        {
            TargetPlatform.Mac,
        };

        /// <summary>
        /// Initializes a new instance of the <see cref="XCode"/> class.
        /// </summary>
        public XCode()
        {
            if (!Platforms.Contains(Platform.BuildTargetPlatform))
                return;
            try
            {
                // Get path
                RootPath = Utilities.ReadProcessOutput("xcode-select", "--print-path");
                if (string.IsNullOrEmpty(RootPath) || !Directory.Exists(RootPath))
                    return;
                IsValid = true;
                Version = new Version(1, 0);

                // Get version (optional)
                var versionText = Utilities.ReadProcessOutput("xcodebuild", "-version");
                var versionLines = versionText?.Split('\n') ?? null;
                if (versionLines != null && versionLines.Length != 0)
                {
                    versionText = versionLines[0].Trim();
                    var versionParts = versionText.Split(' ');
                    if (versionParts != null && versionParts.Length > 1)
                    {
                        versionText = versionParts[1].Trim();
                        if (Version.TryParse(versionText, out var v))
                            Version = v;
                    }
                }
            }
            catch
            {
            }
            Log.Verbose(string.Format("Found XCode {1} at {0}", RootPath, Version));
        }
    }
}
