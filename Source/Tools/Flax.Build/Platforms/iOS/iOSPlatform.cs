// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

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
        public override bool HasDynamicCodeExecutionSupport => false;

        /// <summary>
        /// Initializes a new instance of the <see cref="Flax.Build.Platforms.iOSPlatform"/> class.
        /// </summary>
        public iOSPlatform()
        {
            if (Platform.BuildTargetPlatform != TargetPlatform.Mac)
                return;
            if (!HasRequiredSDKsInstalled)
            {
                Log.Warning("Missing XCode. Cannot build for iOS platform.");
                return;
            }
        }

        /// <inheritdoc />
        protected override Toolchain CreateToolchain(TargetArchitecture architecture)
        {
            return new iOSToolchain(this, architecture);
        }
    }
}
