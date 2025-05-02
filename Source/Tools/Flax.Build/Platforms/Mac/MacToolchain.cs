// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using Flax.Build.NativeCpp;

namespace Flax.Build
{
    partial class Configuration
    {
        /// <summary>
        /// Specifies the minimum Mac OSX version to use (eg. 10.14).
        /// </summary>
        [CommandLine("macOSXMinVer", "<version>", "Specifies the minimum Mac OSX version to use (eg. 10.14).")]
        public static string MacOSXMinVer = "10.15";
    }
}

namespace Flax.Build.Platforms
{
    /// <summary>
    /// The build toolchain for all Mac systems.
    /// </summary>
    /// <seealso cref="UnixToolchain" />
    public sealed class MacToolchain : AppleToolchain
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="MacToolchain"/> class.
        /// </summary>
        /// <param name="platform">The platform.</param>
        /// <param name="architecture">The target architecture.</param>
        public MacToolchain(MacPlatform platform, TargetArchitecture architecture)
        : base(platform, architecture, "MacOSX")
        {
        }

        /// <inheritdoc />
        public override void SetupEnvironment(BuildOptions options)
        {
            base.SetupEnvironment(options);

            options.CompileEnv.PreprocessorDefinitions.Add("PLATFORM_MAC");

            // TODO: move this to the specific module configs (eg. Platform.Build.cs)
            options.LinkEnv.InputLibraries.Add("z");
            options.LinkEnv.InputLibraries.Add("bz2");
            options.LinkEnv.InputLibraries.Add("CoreFoundation.framework");
            options.LinkEnv.InputLibraries.Add("CoreGraphics.framework");
            options.LinkEnv.InputLibraries.Add("CoreMedia.framework");
            options.LinkEnv.InputLibraries.Add("CoreVideo.framework");
            options.LinkEnv.InputLibraries.Add("SystemConfiguration.framework");
            options.LinkEnv.InputLibraries.Add("IOKit.framework");
            options.LinkEnv.InputLibraries.Add("Cocoa.framework");
            options.LinkEnv.InputLibraries.Add("QuartzCore.framework");
            options.LinkEnv.InputLibraries.Add("AVFoundation.framework");
        }

        protected override void AddArgsCommon(BuildOptions options, List<string> args)
        {
            base.AddArgsCommon(options, args);

            args.Add("-mmacosx-version-min=" + Configuration.MacOSXMinVer);
        }
    }
}
