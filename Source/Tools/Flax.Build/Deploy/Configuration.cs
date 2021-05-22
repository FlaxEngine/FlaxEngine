// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

namespace Flax.Build
{
    public static partial class Configuration
    {
        /// <summary>
        /// Builds and packages the editor.
        /// </summary>
        [CommandLine("deployEditor", "Builds and packages the editor.")]
        public static bool DeployEditor;

        /// <summary>
        /// Builds and packages the platforms data.
        /// </summary>
        [CommandLine("deployPlatforms", "Builds and packages the platforms data.")]
        public static bool DeployPlatforms;

        /// <summary>
        /// Certificate file path for binaries signing.
        /// </summary>
        [CommandLine("deployCert", "Certificate file path for binaries signing.")]
        public static string DeployCert;

        /// <summary>
        /// Certificate file password for binaries signing.
        /// </summary>
        [CommandLine("deployCertPass", "Certificate file password for binaries signing.")]
        public static string DeployCertPass;
    }
}
