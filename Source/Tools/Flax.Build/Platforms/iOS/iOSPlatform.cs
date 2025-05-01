// Copyright (c) Wojciech Figat. All rights reserved.

using System.IO;

namespace Flax.Build.Platforms
{
    /// <summary>
    /// The build platform for all iOS systems.
    /// </summary>
    /// <seealso cref="UnixPlatform" />
    public sealed class iOSPlatform : ApplePlatform
    {
        /// <inheritdoc />
        public override TargetPlatform Target => TargetPlatform.iOS;

        /// <inheritdoc />
        public override bool HasRequiredSDKsInstalled { get; }

        /// <inheritdoc />
        public override bool HasDynamicCodeExecutionSupport => false;

        /// <summary>
        /// Initializes a new instance of the <see cref="Flax.Build.Platforms.iOSPlatform"/> class.
        /// </summary>
        public iOSPlatform()
        {
            if (Platform.BuildTargetPlatform != TargetPlatform.Mac)
                return;
            if (!XCode.Instance.IsValid)
            {
                Log.Warning("Missing XCode. Cannot build for iOS platform.");
                return;
            }

            // We should check and see if the actual iphoneSDK is installed
            string iphoneSDKPath = Utilities.ReadProcessOutput("/usr/bin/xcrun", "--sdk iphoneos --show-sdk-path");
            if (string.IsNullOrEmpty(iphoneSDKPath) || !Directory.Exists(iphoneSDKPath))
            {
                Log.Warning("Missing iPhoneSDK. Cannot build for iOS platform.");
                HasRequiredSDKsInstalled = false;
            }
            else
                HasRequiredSDKsInstalled = true;
        }

        /// <inheritdoc />
        protected override Toolchain CreateToolchain(TargetArchitecture architecture)
        {
            return new iOSToolchain(this, architecture);
        }
    }
}
