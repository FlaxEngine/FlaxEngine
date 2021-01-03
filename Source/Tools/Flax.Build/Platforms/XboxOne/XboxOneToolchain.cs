// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using Flax.Build.NativeCpp;

namespace Flax.Build.Platforms
{
    /// <summary>
    /// The Xbox One toolchain implementation.
    /// </summary>
    /// <seealso cref="Toolchain" />
    /// <seealso cref="Flax.Build.Platforms.WindowsToolchainBase" />
    public sealed class XboxOneToolchain : UWPToolchain
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="XboxOneToolchain"/> class.
        /// </summary>
        /// <param name="platform">The platform.</param>
        /// <param name="architecture">The target architecture.</param>
        public XboxOneToolchain(XboxOnePlatform platform, TargetArchitecture architecture)
        : base(platform, architecture)
        {
        }

        /// <inheritdoc />
        public override void SetupEnvironment(BuildOptions options)
        {
            base.SetupEnvironment(options);

            options.CompileEnv.PreprocessorDefinitions.Add("PLATFORM_XBOX_ONE");
            //options.CompileEnv.PreprocessorDefinitions.Add("_XBOX_ONE");
            options.CompileEnv.PreprocessorDefinitions.Add("PX_FOUNDATION_DLL=0"); // TODO: let Physics module decide about PhysX deploy mode
            options.CompileEnv.PreprocessorDefinitions.Add("WINAPI_FAMILY=WINAPI_FAMILY_PC_APP");
            options.CompileEnv.PreprocessorDefinitions.Add("_WINRT_DLL");
        }
    }
}
