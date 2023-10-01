// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System.Runtime.InteropServices;

namespace Flax.Build.Platforms
{
    /// <summary>
    /// The build platform for all Mac systems.
    /// </summary>
    /// <seealso cref="UnixPlatform" />
    public sealed class MacPlatform : ApplePlatform
    {
        /// <inheritdoc />
        public override TargetPlatform Target => TargetPlatform.Mac;

        /// <summary>
        /// Initializes a new instance of the <see cref="Flax.Build.Platforms.MacPlatform"/> class.
        /// </summary>
        public MacPlatform()
        {
            if (Platform.BuildTargetPlatform != TargetPlatform.Mac)
                return;
            if (!HasRequiredSDKsInstalled)
            {
                Log.Warning("Missing XCode. Cannot build for Mac platform.");
                return;
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
            case TargetPlatform.iOS:
            case TargetPlatform.Mac: return HasRequiredSDKsInstalled;
            default: return false;
            }
        }

        /// <summary>
        /// Returns true if running an x64 binary an arm64 host machine.
        /// </summary>
        public unsafe static bool GetProcessIsTranslated()
		{
			int ret = 0;
			ulong size = sizeof(int);
			if (sysctlbyname("sysctl.proc_translated", &ret, &size, null, 0) == -1)
				return false;
			return ret != 0;
		}

        [DllImport("c")]
		private static unsafe extern int sysctlbyname(string name, void* oldp, ulong* oldlenp, void* newp, ulong newlen);
    }
}
