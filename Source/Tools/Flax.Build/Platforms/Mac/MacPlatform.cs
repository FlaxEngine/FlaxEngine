// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.IO;
using Flax.Build.Projects;

namespace Flax.Build.Platforms
{
    /// <summary>
    /// The build platform for all Mac systems.
    /// </summary>
    /// <seealso cref="UnixPlatform" />
    public sealed class MacPlatform : UnixPlatform
    {
        /// <inheritdoc />
        public override TargetPlatform Target => TargetPlatform.Mac;

        /// <inheritdoc />
        public override bool HasRequiredSDKsInstalled { get; }

        /// <inheritdoc />
        public override bool HasSharedLibrarySupport => true;

        /// <inheritdoc />
        public override string SharedLibraryFileExtension => ".dylib";

        /// <inheritdoc />
        public override string ProgramDatabaseFileExtension => ".dSYM";

        /// <inheritdoc />
        public override string SharedLibraryFilePrefix => string.Empty;

        /// <inheritdoc />
        public override ProjectFormat DefaultProjectFormat => ProjectFormat.XCode;

        /// <summary>
        /// XCode Developer path returned by xcode-select.
        /// </summary>
        public string XCodePath;

        /// <summary>
        /// Initializes a new instance of the <see cref="Flax.Build.Platforms.MacPlatform"/> class.
        /// </summary>
        public MacPlatform()
        {
            if (Platform.BuildTargetPlatform != TargetPlatform.Mac)
                return;
            try
            {
                // Check if XCode is installed
                XCodePath = Utilities.ReadProcessOutput("xcode-select", "--print-path");
                if (string.IsNullOrEmpty(XCodePath) || !Directory.Exists(XCodePath))
                    throw new Exception(XCodePath);
                Log.Verbose(string.Format("Found XCode at {0}", XCodePath));
                HasRequiredSDKsInstalled = true;
            }
            catch
            {
                Log.Warning("Missing XCode. Cannot build for Mac platform.");
            }
        }

        /// <inheritdoc />
        protected override Toolchain CreateToolchain(TargetArchitecture architecture)
        {
            return new MacToolchain(this, architecture);
        }

        /// <inheritdoc />
        public override bool CanBuildPlatform(TargetPlatform platform)
        {
            switch (platform)
            {
            case TargetPlatform.Mac: return HasRequiredSDKsInstalled;
            default: return false;
            }
        }

        public static void FixInstallNameId(string dylibPath)
        {
            Utilities.Run("install_name_tool", string.Format(" -id \"@rpath/{0}\" \"{1}\"", Path.GetFileName(dylibPath), dylibPath), null, null, Utilities.RunOptions.ThrowExceptionOnError);
        }
    }
}
