// Copyright (c) Wojciech Figat. All rights reserved.

using System.Linq;
using Flax.Build.Projects.VisualStudio;

namespace Flax.Build.Platforms
{
    /// <summary>
    /// The GDK platform implementation.
    /// </summary>
    /// <seealso cref="Platform" />
    /// <seealso cref="Flax.Build.Platforms.WindowsPlatformBase" />
    public abstract class GDKPlatform : WindowsPlatformBase
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="GDKPlatform"/> class.
        /// </summary>
        protected GDKPlatform()
        {
            // Visual Studio 2017 or newer required
            var visualStudio = VisualStudioInstance.GetInstances().FirstOrDefault(x => x.Version >= VisualStudioVersion.VisualStudio2017);
            if (visualStudio == null)
                _hasRequiredSDKsInstalled = false;

            // Windows 10.0.19041.0 SDK or newer required
            var sdks = GetSDKs();
            if (sdks.All(x => x.Key < WindowsPlatformSDK.v10_0_19041_0))
                _hasRequiredSDKsInstalled = false;

            // v141 toolset or newer required
            var toolsets = GetToolsets();
            if (!toolsets.ContainsKey(WindowsPlatformToolset.v141) &&
                !toolsets.ContainsKey(WindowsPlatformToolset.v142) &&
                !toolsets.ContainsKey(WindowsPlatformToolset.v143) &&
                !toolsets.ContainsKey(WindowsPlatformToolset.v144))
                _hasRequiredSDKsInstalled = false;
        }
    }
}
