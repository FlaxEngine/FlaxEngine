// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;

namespace Flax.Build.NativeCpp
{
    /// <summary>
    /// The C++ compilation output data container.
    /// </summary>
    public class CompileOutput
    {
        /// <summary>
        /// The result object files.
        /// </summary>
        public readonly List<string> ObjectFiles = new List<string>();

        /// <summary>
        /// The result debug data files.
        /// </summary>
        public readonly List<string> DebugDataFiles = new List<string>();

        /// <summary>
        /// The result documentation files.
        /// </summary>
        public readonly List<string> DocumentationFiles = new List<string>();

        /// <summary>
        /// The result precompiled header file (PCH) created during compilation. Can be used in other compilations (as shared).
        /// </summary>
        public string PrecompiledHeaderFile;
    }
}
