// Copyright (c) Wojciech Figat. All rights reserved.

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

        /// <inheritdoc />
        public override bool HasDynamicCodeExecutionSupport => false;

        /// <summary>
        /// Initializes a new instance of the <see cref="UWPPlatform"/> class.
        /// </summary>
        public UWPPlatform()
        {
            // Skip if running on non-Windows system
            if (Platform.BuildTargetPlatform != TargetPlatform.Windows)
            {
                _hasRequiredSDKsInstalled = false;
                return;
            }

            // Visual Studio 2017+ supported only
            var visualStudio = VisualStudioInstance.GetInstances().FirstOrDefault(x => x.Version == VisualStudioVersion.VisualStudio2017 || x.Version == VisualStudioVersion.VisualStudio2019 || x.Version == VisualStudioVersion.VisualStudio2022);
            if (visualStudio == null)
                _hasRequiredSDKsInstalled = false;

            // Need Windows 10 SDK
            var sdks = GetSDKs();
            var sdk10 = sdks.FirstOrDefault(x => x.Key != WindowsPlatformSDK.v8_1).Value;
            if (sdk10 == null)
                _hasRequiredSDKsInstalled = false;

            // Need v141+ toolset
            var toolsets = GetToolsets();
            if (!toolsets.ContainsKey(WindowsPlatformToolset.v141) &&
                !toolsets.ContainsKey(WindowsPlatformToolset.v142) &&
                !toolsets.ContainsKey(WindowsPlatformToolset.v143) &&
                !toolsets.ContainsKey(WindowsPlatformToolset.v144))
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
