// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.IO;
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
        /// Runs codesign tool on macOS to sign the code with a given identity from local keychain.
        /// </summary>
        /// <param name="file">Path to file to codesign.</param>
        /// <param name="signIdentity">App code signing identity name (from local Mac keychain). Use 'security find-identity -v -p codesigning' to list possible options.</param>
        public static void CodeSign(string file, string signIdentity)
        {
            var isDirectory = Directory.Exists(file);
            if (!isDirectory && !File.Exists(file))
                throw new FileNotFoundException("Missing file to sign.", file);
            string cmdLine = string.Format("--force --timestamp -s \"{0}\" \"{1}\"", signIdentity, file);
            if (isDirectory)
            {
                // Automatically sign contents
                cmdLine += " --deep";
            }
            {
                // Enable the hardened runtime
                cmdLine += " --options=runtime";
            }
            {
                // Add entitlements file with some settings for the app execution
                cmdLine += string.Format(" --entitlements \"{0}\"", Path.Combine(Globals.EngineRoot, "Source/Platforms/Mac/Default.entitlements"));
            }
            Utilities.Run("codesign", cmdLine, null, null, Utilities.RunOptions.Default | Utilities.RunOptions.ThrowExceptionOnError);
        }

        internal static bool BuildingForx64
        {
            get
            {
                // We need to support two paths here:
                // 1. We are running an x64 binary and we are running on an arm64 host machine
                // 2. We are running an Arm64 binary and we are targeting an x64 host machine
                var architecture = Platform.BuildTargetArchitecture;
                bool isRunningOnArm64Targetx64 = architecture == TargetArchitecture.ARM64 && (Configuration.BuildArchitectures != null && Configuration.BuildArchitectures[0] == TargetArchitecture.x64);
                return GetProcessIsTranslated() || isRunningOnArm64Targetx64;
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
