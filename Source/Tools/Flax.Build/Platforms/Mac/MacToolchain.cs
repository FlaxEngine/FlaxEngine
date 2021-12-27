// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using Flax.Build.Graph;
using Flax.Build.NativeCpp;

namespace Flax.Build.Platforms
{
    /// <summary>
    /// The build toolchain for all Mac systems.
    /// </summary>
    /// <seealso cref="Toolchain" />
    public sealed class MacToolchain : Toolchain
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="MacToolchain"/> class.
        /// </summary>
        /// <param name="platform">The platform.</param>
        /// <param name="architecture">The target architecture.</param>
        public MacToolchain(MacPlatform platform, TargetArchitecture architecture)
        : base(platform, architecture)
        {
        }

        /// <inheritdoc />
        public override string DllExport => "__attribute__((__visibility__(\\\"default\\\")))";

        /// <inheritdoc />
        public override string DllImport => "";

        /// <inheritdoc />
        public override void LogInfo()
        {
            throw new NotImplementedException("TODO: MacToolchain.LogInfo");
        }

        /// <inheritdoc />
        public override void SetupEnvironment(BuildOptions options)
        {
            base.SetupEnvironment(options);

            options.CompileEnv.PreprocessorDefinitions.Add("PLATFORM_MAC");
        }

        /// <inheritdoc />
        public override CompileOutput CompileCppFiles(TaskGraph graph, BuildOptions options, List<string> sourceFiles, string outputPath)
        {
            throw new NotImplementedException("TODO: MacToolchain.CompileCppFiles");
        }

        /// <inheritdoc />
        public override void LinkFiles(TaskGraph graph, BuildOptions options, string outputFilePath)
        {
            throw new NotImplementedException("TODO: MacToolchain.LinkFiles");
        }
    }
}
