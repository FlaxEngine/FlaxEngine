// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.Linq;
using Flax.Build.Projects.VisualStudio;

namespace Flax.Build.Platforms
{
    /// <summary>
    /// The Universal Windows Platform (UWP) implementation.
    /// </summary>
    /// <seealso cref="Platform" />
    /// <seealso cref="Flax.Build.Platforms.WindowsPlatformBase" />
    public class UWPPlatform : WindowsPlatformBase
    {
        /// <inheritdoc />
        public override TargetPlatform Target => TargetPlatform.UWP;

        /// <summary>
        /// Initializes a new instance of the <see cref="UWPPlatform"/> class.
        /// </summary>
        public UWPPlatform()
        {
            // Visual Studio 2017+ supported only
            var visualStudio = VisualStudioInstance.GetInstances().FirstOrDefault(x => x.Version == VisualStudioVersion.VisualStudio2017 || x.Version == VisualStudioVersion.VisualStudio2019);
            if (visualStudio == null)
                _hasRequiredSDKsInstalled = false;

            // Need Windows 10 SDK
            var sdks = GetSDKs();
            var sdk10 = sdks.FirstOrDefault(x => x.Key != WindowsPlatformSDK.v8_1).Value;
            if (sdk10 == null)
                _hasRequiredSDKsInstalled = false;

            // Need v141+ toolset
            if (!GetToolsets().ContainsKey(WindowsPlatformToolset.v141) &&
                !GetToolsets().ContainsKey(WindowsPlatformToolset.v142))
            {
                _hasRequiredSDKsInstalled = false;
            }
        }

        /// <inheritdoc />
        protected override Toolchain CreateToolchain(TargetArchitecture architecture)
        {
            return new UWPToolchain(this, architecture);
        }
    }
}
