// Copyright (c) Wojciech Figat. All rights reserved.

namespace Flax.Build
{
    public static partial class Configuration
    {
        /// <summary>
        /// The deps to build. Separated by ',' char. Empty if build all deps.
        /// </summary>
        [CommandLine("depsToBuild", "<package1,package2>", "The deps to build. Separated by ',' char. Empty if build all deps.")]
        public static string DepsToBuild = string.Empty;
    }
}
