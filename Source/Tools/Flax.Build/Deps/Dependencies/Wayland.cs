// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;
using Flax.Build;

namespace Flax.Deps.Dependencies
{
    /// <summary>
    /// Wayland protocols.
    /// </summary>
    /// <seealso cref="Flax.Deps.Dependency" />
    class Wayland : Dependency
    {
        /// <inheritdoc />
        public override TargetPlatform[] Platforms
        {
            get
            {
                switch (BuildPlatform)
                {
                case TargetPlatform.Linux:
                    return new[]
                    {
                        TargetPlatform.Linux,
                    };
                default: return new TargetPlatform[0];
                }
            }
        }

        /// <inheritdoc />
        public override void Build(BuildOptions options)
        {
            var dstPath = Path.Combine(options.ThirdPartyFolder, "Wayland");
            var protocolsPath = Path.Combine(dstPath, "protocols");
            var includePath = Path.Combine(dstPath, "include");

            foreach (var platform in options.Platforms)
            {
                BuildStarted(platform);
                switch (platform)
                {
                case TargetPlatform.Linux:
                    if (!Directory.Exists(Path.Combine(includePath, "wayland")))
                        Directory.CreateDirectory(Path.Combine(includePath, "wayland"));

                    string[] protocolFiles = Directory.GetFiles(protocolsPath, "*.xml", SearchOption.TopDirectoryOnly);
                    foreach (var protocolPath in protocolFiles)
                    {
                        var headerFile = Path.ChangeExtension(Path.GetFileName(protocolPath), "h");
                        var glueFile = Path.ChangeExtension(Path.GetFileName(protocolPath), "c");
                        Utilities.Run("wayland-scanner", $"client-header {protocolPath} include/wayland/{headerFile}", null, dstPath, Utilities.RunOptions.DefaultTool);
                        Utilities.Run("wayland-scanner", $"private-code {protocolPath} {glueFile}", null, dstPath, Utilities.RunOptions.DefaultTool);
                    }
                    break;
                }
            }

            Utilities.FileCopy("/usr/share/licenses/wayland/COPYING", Path.Combine(dstPath, "LICENSE.txt"));
        }
    }
}
