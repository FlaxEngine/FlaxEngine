// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using Flax.Build.NativeCpp;

namespace Flax.Build
{
    partial class Configuration
    {
        /// <summary>
        /// Specifies the minimum iOS version to use (eg. 14).
        /// </summary>
        [CommandLine("iOSMinVer", "<version>", "Specifies the minimum iOS version to use (eg. 14).")]
        public static string iOSMinVer = "14";
    }
}

namespace Flax.Build.Platforms
{
    /// <summary>
    /// The build toolchain for all iOS systems.
    /// </summary>
    /// <seealso cref="UnixToolchain" />
    public sealed class iOSToolchain : AppleToolchain
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="iOSToolchain"/> class.
        /// </summary>
        /// <param name="platform">The platform.</param>
        /// <param name="architecture">The target architecture.</param>
        public iOSToolchain(iOSPlatform platform, TargetArchitecture architecture)
        : base(platform, architecture, "iPhoneOS")
        {
        }

        /// <inheritdoc />
        public override void SetupEnvironment(BuildOptions options)
        {
            base.SetupEnvironment(options);

            options.CompileEnv.PreprocessorDefinitions.Add("PLATFORM_IOS");

            // TODO: move this to the specific module configs (eg. Platform.Build.cs)
            options.LinkEnv.InputLibraries.Add("z");
            options.LinkEnv.InputLibraries.Add("bz2");
            options.LinkEnv.InputLibraries.Add("Foundation.framework");
            options.LinkEnv.InputLibraries.Add("CoreFoundation.framework");
            options.LinkEnv.InputLibraries.Add("CoreGraphics.framework");
            options.LinkEnv.InputLibraries.Add("SystemConfiguration.framework");
            options.LinkEnv.InputLibraries.Add("IOKit.framework");
            options.LinkEnv.InputLibraries.Add("UIKit.framework");
            options.LinkEnv.InputLibraries.Add("QuartzCore.framework");
            //options.LinkEnv.InputLibraries.Add("QuartzCore.framework");
            //options.LinkEnv.InputLibraries.Add("AudioToolbox.framework");
            //options.LinkEnv.InputLibraries.Add("AudioUnit.framework");
        }

        protected override void AddArgsCommon(BuildOptions options, List<string> args)
        {
            base.AddArgsCommon(options, args);

            args.Add("-miphoneos-version-min=" + Configuration.iOSMinVer);
        }
    }
}
