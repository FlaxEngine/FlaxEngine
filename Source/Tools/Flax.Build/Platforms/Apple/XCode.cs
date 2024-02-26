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
                RootPath = Utilities.ReadProcessOutput("xcode-select", "--print-path");
                if (string.IsNullOrEmpty(RootPath) || !Directory.Exists(RootPath))
                    return;
                IsValid = true;
                Version = new Version(1, 0);
                Log.Verbose(string.Format("Found XCode at {0}", RootPath));
            }
            catch
            {
            }
        }
    }
}
