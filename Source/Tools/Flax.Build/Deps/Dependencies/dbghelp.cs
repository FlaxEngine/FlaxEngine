// Copyright (c) Wojciech Figat. All rights reserved.

using System.IO;
using System.Linq;
using Flax.Build;
using Flax.Build.Platforms;

namespace Flax.Deps.Dependencies
{
    /// <summary>
    /// Windows Debug Help Library.
    /// </summary>
    /// <seealso cref="Flax.Deps.Dependency" />
    class dbghelp : Dependency
    {
        /// <inheritdoc />
        public override TargetPlatform[] Platforms
        {
            get
            {
                switch (BuildPlatform)
                {
                case TargetPlatform.Windows:
                    return new[]
                    {
                        TargetPlatform.Windows,
                    };
                default: return new TargetPlatform[0];
                }
            }
        }

        /// <inheritdoc />
        public override void Build(BuildOptions options)
        {
            foreach (var platform in options.Platforms)
            {
                BuildStarted(platform);
                switch (platform)
                {
                case TargetPlatform.Windows:
                {
                    var sdk = WindowsPlatformBase.GetSDKs().Last();
                    foreach (var architecture in new[] { TargetArchitecture.x64, TargetArchitecture.ARM64 })
                    {
                        var depsFolder = GetThirdPartyFolder(options, platform, architecture);
                        var libLocation = @$"{sdk.Value}Debuggers\lib\{architecture}\dbghelp.lib";
                        var dllLocation = @$"{sdk.Value}Debuggers\{architecture}\dbghelp.dll";
                        Utilities.FileCopy(libLocation, Path.Combine(depsFolder, Path.GetFileName(libLocation)));
                        Utilities.FileCopy(dllLocation, Path.Combine(depsFolder, Path.GetFileName(dllLocation)));
                    }
                    break;
                }
                }
            }
        }
    }
}
