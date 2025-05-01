// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;

namespace Flax.Build.NativeCpp
{
    /// <summary>
    /// The linked output file types.
    /// </summary>
    public enum LinkerOutput
    {
        /// <summary>
        /// The executable file (aka .exe file).
        /// </summary>
        Executable,

        /// <summary>
        /// The shared library file (aka .dll file).
        /// </summary>
        SharedLibrary,

        /// <summary>
        /// The static library file (aka .lib file).
        /// </summary>
        StaticLibrary,

        /// <summary>
        /// The import library file (aka .lib file).
        /// </summary>
        ImportLibrary,
    }

    /// <summary>
    /// The C++ linking environment required to build source files in the native modules.
    /// </summary>
    public class LinkEnvironment : ICloneable
    {
        /// <summary>
        /// The output type.
        /// </summary>
        public LinkerOutput Output;

        /// <summary>
        /// Enables the code optimization.
        /// </summary>
        public bool Optimization = false;

        /// <summary>
        /// Enables debug information generation.
        /// </summary>
        public bool DebugInformation = false;

        /// <summary>
        /// Hints to use full debug information.
        /// </summary>
        public bool UseFullDebugInformation = false;

        /// <summary>
        /// Hints to use fast PDB linking.
        /// </summary>
        public bool UseFastPDBLinking = false;

        /// <summary>
        /// Enables the link time code generation (LTCG).
        /// </summary>
        public bool LinkTimeCodeGeneration = false;

        /// <summary>
        /// Hints to use incremental linking.
        /// </summary>
        public bool UseIncrementalLinking = false;

        /// <summary>
        /// Enables Windows Metadata generation.
        /// </summary>
        public bool GenerateWindowsMetadata = false;

        /// <summary>
        /// Use CONSOLE subsystem on Windows instead of the WINDOWS one.
        /// </summary>
        public bool LinkAsConsoleProgram = false;

        /// <summary>
        /// Enables documentation generation.
        /// </summary>
        public bool GenerateDocumentation = false;

        /// <summary>
        /// The collection of the object files to be linked.
        /// </summary>
        public readonly HashSet<string> InputFiles = new HashSet<string>();

        /// <summary>
        /// The collection of the documentation files to be used during linked file documentation generation. Used only if <see cref="GenerateDocumentation"/> is enabled.
        /// </summary>
        public readonly List<string> DocumentationFiles = new List<string>();

        /// <summary>
        /// The collection of dependent static or import libraries that need to be linked.
        /// </summary>
        public readonly List<string> InputLibraries = new List<string>();

        /// <summary>
        /// The collection of dependent static or import libraries paths.
        /// </summary>
        public readonly List<string> LibraryPaths = new List<string>();

        /// <summary>
        /// The collection of custom arguments to pass to the linker.
        /// </summary>
        public readonly HashSet<string> CustomArgs = new HashSet<string>();

        /// <inheritdoc />
        public object Clone()
        {
            var clone = new LinkEnvironment
            {
                Output = Output,
                Optimization = Optimization,
                DebugInformation = DebugInformation,
                UseFullDebugInformation = UseFullDebugInformation,
                UseFastPDBLinking = UseFastPDBLinking,
                LinkTimeCodeGeneration = LinkTimeCodeGeneration,
                UseIncrementalLinking = UseIncrementalLinking,
                GenerateWindowsMetadata = GenerateWindowsMetadata,
                LinkAsConsoleProgram = LinkAsConsoleProgram,
                GenerateDocumentation = GenerateDocumentation
            };
            clone.InputFiles.AddRange(InputFiles);
            clone.DocumentationFiles.AddRange(DocumentationFiles);
            clone.InputLibraries.AddRange(InputLibraries);
            clone.LibraryPaths.AddRange(LibraryPaths);
            clone.CustomArgs.AddRange(CustomArgs);
            return clone;
        }
    }
}
