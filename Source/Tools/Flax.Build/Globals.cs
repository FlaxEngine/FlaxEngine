// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

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
    }
}
