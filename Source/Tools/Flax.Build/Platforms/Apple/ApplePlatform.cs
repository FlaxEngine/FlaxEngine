// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.IO;
using Flax.Build.Projects;

namespace Flax.Build.Platforms
{
    /// <summary>
    /// The build platform for all Apple systems.
    /// </summary>
    /// <seealso cref="UnixPlatform" />
    public abstract class ApplePlatform : UnixPlatform
    {
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
        /// Initializes a new instance of the <see cref="Flax.Build.Platforms.ApplePlatform"/> class.
        /// </summary>
        public ApplePlatform()
        {
            if (Platform.BuildTargetPlatform != TargetPlatform.Mac)
                return;
            HasRequiredSDKsInstalled = XCode.Instance.IsValid;
        }

        public static void FixInstallNameId(string dylibPath)
        {
            Utilities.Run("install_name_tool", string.Format(" -id \"@rpath/{0}\" \"{1}\"", Path.GetFileName(dylibPath), dylibPath), null, null, Utilities.RunOptions.ThrowExceptionOnError);
        }
    }
}
