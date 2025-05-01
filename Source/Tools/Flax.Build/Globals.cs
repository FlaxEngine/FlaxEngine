// Copyright (c) Wojciech Figat. All rights reserved.

namespace Flax.Build
{
    /// <summary>
    /// The build tool global variables.
    /// </summary>
    public static partial class Globals
    {
        /// <summary>
        /// The root directory of the current workspace (eg. game project workspace, or engine project workspace).
        /// </summary>
        public static string Root = null;

        /// <summary>
        /// The root directory of the current engine installation.
        /// </summary>
        public static string EngineRoot = null;

        /// <summary>
        /// The project loaded from the workspace directory.
        /// </summary>
        public static ProjectInfo Project;

        /// <summary>
        /// All platforms array.
        /// </summary>
        public static readonly TargetPlatform[] AllPlatforms =
        {
            TargetPlatform.Windows,
            TargetPlatform.XboxOne,
            TargetPlatform.Linux,
            TargetPlatform.PS4,
            TargetPlatform.PS5,
            TargetPlatform.XboxScarlett,
            TargetPlatform.Android,
            TargetPlatform.Switch,
            TargetPlatform.Mac,
            TargetPlatform.iOS,
        };

        /// <summary>
        /// All architectures array.
        /// </summary>
        public static readonly TargetArchitecture[] AllArchitectures =
        {
            TargetArchitecture.x64,
            TargetArchitecture.x86,
            TargetArchitecture.ARM,
            TargetArchitecture.ARM64,
        };

        /// <summary>
        /// All configurations array.
        /// </summary>
        public static readonly TargetConfiguration[] AllConfigurations =
        {
            TargetConfiguration.Debug,
            TargetConfiguration.Development,
            TargetConfiguration.Release,
        };
    }
}
