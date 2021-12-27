// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using Flax.Build.Projects;

namespace Flax.Build.Platforms
{
    /// <summary>
    /// The build platform for all Mac systems.
    /// </summary>
    /// <seealso cref="Platform" />
    public sealed class MacPlatform : Platform
    {
        /// <inheritdoc />
        public override TargetPlatform Target => TargetPlatform.Mac;

        /// <inheritdoc />
        public override bool HasRequiredSDKsInstalled { get; }

        /// <inheritdoc />
        public override bool HasSharedLibrarySupport => true;

        /// <inheritdoc />
        public override string ExecutableFileExtension => string.Empty;

        /// <inheritdoc />
        public override string SharedLibraryFileExtension => ".dylib";

        /// <inheritdoc />
        public override string StaticLibraryFileExtension => ".a";

        /// <inheritdoc />
        public override string ProgramDatabaseFileExtension => ".dSYM";

        /// <inheritdoc />
        public override string SharedLibraryFilePrefix => string.Empty;

        /// <inheritdoc />
        public override string StaticLibraryFilePrefix => "lib";

        /// <inheritdoc />
        public override ProjectFormat DefaultProjectFormat => ProjectFormat.XCode;

        /// <summary>
        /// Initializes a new instance of the <see cref="Flax.Build.Platforms.MacPlatform"/> class.
        /// </summary>
        public MacPlatform()
        {
            if (Platform.BuildTargetPlatform != TargetPlatform.Mac)
                return;

            throw new System.NotImplementedException("TODO: detect MacSDK installation");
        }

        /// <inheritdoc />
        protected override Toolchain CreateToolchain(TargetArchitecture architecture)
        {
            return new MacToolchain(this, architecture);
        }
    }
}
