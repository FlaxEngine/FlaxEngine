// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.IO;

namespace Flax.Build.Platforms
{
    /// <summary>
    /// The Android build platform implementation.
    /// </summary>
    /// <seealso cref="Platform" />
    /// <seealso cref="UnixPlatform" />
    public class AndroidPlatform : UnixPlatform
    {
        /// <inheritdoc />
        public override TargetPlatform Target => TargetPlatform.Android;

        /// <inheritdoc />
        public override bool HasRequiredSDKsInstalled { get; }

        /// <inheritdoc />
        public override bool HasSharedLibrarySupport => true;
    
        /// <inheritdoc />
        public override bool HasExecutableFileReferenceSupport => true;

        /// <inheritdoc />
        public override string ExecutableFileExtension => ".so";

        /// <inheritdoc />
        public override string ExecutableFilePrefix => "lib";

        /// <summary>
        /// Initializes a new instance of the <see cref="AndroidPlatform"/> class.
        /// </summary>
        public AndroidPlatform()
        {
            HasRequiredSDKsInstalled = AndroidSdk.Instance.IsValid && AndroidNdk.Instance.IsValid;
        }

        /// <inheritdoc />
        protected override Toolchain CreateToolchain(TargetArchitecture architecture)
        {
            var ndk = AndroidNdk.Instance.RootPath;
            var toolchainRoot = Path.Combine(ndk, "toolchains", "llvm", "prebuilt", AndroidSdk.GetHostName());
            return new AndroidToolchain(this, architecture, toolchainRoot);
        }
    }
}
