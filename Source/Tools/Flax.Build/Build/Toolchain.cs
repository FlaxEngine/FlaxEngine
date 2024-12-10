// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;
using Flax.Build.Graph;
using Flax.Build.NativeCpp;

namespace Flax.Build
{
    /// <summary>
    /// The target platform compiler types.
    /// </summary>
    public enum TargetCompiler
    {
        /// <summary>
        /// Microsoft C++ (MSVC) C and C++ compiler and linker.
        /// </summary>
        MSVC,

        /// <summary>
        /// LLVM-based open source compiler.
        /// </summary>
        Clang,
    }

    /// <summary>
    /// The base class for all build toolchains.
    /// </summary>
    public abstract class Toolchain
    {
        /// <summary>
        /// Gets the platform target type.
        /// </summary>
        public Platform Platform { get; private set; }

        /// <summary>
        /// Gets the platform target architecture.
        /// </summary>
        public TargetArchitecture Architecture { get; private set; }

        /// <summary>
        /// The default system include paths (for native C++ compilation).
        /// </summary>
        public readonly List<string> SystemIncludePaths = new List<string>();

        /// <summary>
        /// The default system library paths (for native C++ linking).
        /// </summary>
        public readonly List<string> SystemLibraryPaths = new List<string>();

        /// <summary>
        /// True it toolset requires the import library (eg. .lib or .a) when linking the binary (shared library or executable file). Otherwise, linking will be performed again the shared library (eg. .dll or .so).
        /// </summary>
        public virtual bool UseImportLibraryWhenLinking => false;

        /// <summary>
        /// True it toolset generates the import library (eg. .lib or .a) file automatically when linking the binary (shared library or executable file).
        /// </summary>
        public virtual bool GeneratesImportLibraryWhenLinking => false;

        /// <summary>
        /// Gets the compiler attribute for symbols exported to shared library (dll file).
        /// </summary>
        public abstract string DllExport { get; }

        /// <summary>
        /// Gets the compiler attribute for symbols imported from shared library (dll file).
        /// </summary>
        public abstract string DllImport { get; }

        /// <summary>
        /// Gets the compiler type.
        /// </summary>
        public abstract TargetCompiler Compiler { get; }

        /// <summary>
        /// Gets the main native files compiler path.
        /// </summary>
        public virtual string NativeCompilerPath { get; }

        /// <summary>
        /// Initializes a new instance of the <see cref="Toolchain"/> class.
        /// </summary>
        /// <param name="platform">The platform.</param>
        /// <param name="architecture">The target architecture.</param>
        protected Toolchain(Platform platform, TargetArchitecture architecture)
        {
            Platform = platform;
            Architecture = architecture;
        }

        /// <summary>
        /// Prints the information about the toolchain to the log.
        /// </summary>
        public abstract void LogInfo();

        /// <summary>
        /// Setups the building environment (native C++). Allows to modify compiler and linker options.
        /// </summary>
        /// <param name="options">The build options.</param>
        public virtual void SetupEnvironment(BuildOptions options)
        {
            options.CompileEnv.IncludePaths.AddRange(SystemIncludePaths);
            options.LinkEnv.LibraryPaths.AddRange(SystemLibraryPaths);
        }

        /// <summary>
        /// Called before building a target with a given build options. Can be used to inject custom commands into the task graph.
        /// </summary>
        /// <param name="graph">The task graph.</param>
        /// <param name="options">The current build options.</param>
        public virtual void PreBuild(TaskGraph graph, BuildOptions options)
        {
        }

        /// <summary>
        /// Called after building a target with a given build options. Can be used to inject custom commands into the task graph.
        /// </summary>
        /// <param name="graph">The task graph.</param>
        /// <param name="options">The current build options.</param>
        public virtual void PostBuild(TaskGraph graph, BuildOptions options)
        {
        }

        /// <summary>
        /// Compiles the C++ source files.
        /// </summary>
        /// <param name="graph">The task graph.</param>
        /// <param name="options">The build options with compilation environment.</param>
        /// <param name="sourceFiles">The source files.</param>
        /// <param name="outputPath">The output directory path (for object files).</param>
        /// <returns>The output data.</returns>
        public abstract CompileOutput CompileCppFiles(TaskGraph graph, BuildOptions options, List<string> sourceFiles, string outputPath);

        /// <summary>
        /// Links the compiled object files.
        /// </summary>
        /// <param name="graph">The task graph.</param>
        /// <param name="options">The build options with linking environment.</param>
        /// <param name="outputFilePath">The output file path (result linked file).</param>
        public abstract void LinkFiles(TaskGraph graph, BuildOptions options, string outputFilePath);

        /// <summary>
        /// C# compilation options container.
        /// </summary>
        public struct CSharpOptions
        {
            public enum ActionTypes
            {
                MonoCompile,
                MonoLink,
                GetOutputFiles,
                GetPlatformTools,
            };

            public ActionTypes Action;
            public List<string> InputFiles;
            public List<string> OutputFiles;
            public string AssembliesPath;
            public string ClassLibraryPath;
            public string PlatformToolsPath;
            public bool EnableDebugSymbols;
            public bool EnableToolDebug;
        }

        /// <summary>
        /// Compiles the C# assembly with AOT cross-compiler.
        /// </summary>
        /// <param name="options">The options.</param>
        /// <returns>True if failed, or not supported.</returns>
        public virtual bool CompileCSharp(ref CSharpOptions options)
        {
            switch (options.Action)
            {
            case CSharpOptions.ActionTypes.GetOutputFiles:
                foreach (var inputFile in options.InputFiles)
                {
                    if (Configuration.AOTMode == DotNetAOTModes.MonoAOTDynamic)
                        options.OutputFiles.Add(inputFile + Platform.SharedLibraryFileExtension);
                    else
                        options.OutputFiles.Add(Path.Combine(Path.GetDirectoryName(inputFile), Platform.StaticLibraryFilePrefix + Path.GetFileName(inputFile) + Platform.StaticLibraryFileExtension));
                }
                return false;
            case CSharpOptions.ActionTypes.GetPlatformTools:
                options.PlatformToolsPath = Path.Combine(Globals.EngineRoot, "Source/Platforms", Platform.Target.ToString(), "Binaries/Tools");
                return false;
            }
            return true;
        }
    }
}
