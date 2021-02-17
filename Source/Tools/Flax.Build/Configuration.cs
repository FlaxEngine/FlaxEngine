// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.Collections.Generic;

namespace Flax.Build
{
    /// <summary>
    /// The build tool configuration options.
    /// </summary>
    public static partial class Configuration
    {
        /// <summary>
        /// The custom working directory.
        /// </summary>
        [CommandLine("workspace", "<path>", "The custom working directory.")]
        public static string CurrentDirectory = null;

        /// <summary>
        /// Generates the projects for the workspace.
        /// </summary>
        [CommandLine("genproject", "Generates the projects for the workspace.")]
        public static bool GenerateProject = false;

        /// <summary>
        /// Runs the deps building tool to fetch and compiles the 3rd party files to produce binaries.
        /// </summary>
        [CommandLine("BuildDeps", "Runs the deps building tool to fetch and compiles the 3rd party files to produce binaries (build missing ones).")]
        public static bool BuildDeps = false;

        /// <summary>
        /// Runs the deps building tool to fetch and compiles the 3rd party files to produce binaries.
        /// </summary>
        [CommandLine("ReBuildDeps", "Runs the deps building tool to fetch and compiles the 3rd party files to produce binaries (force rebuild).")]
        public static bool ReBuildDeps = false;

        /// <summary>
        /// Runs the deploy tool.
        /// </summary>
        [CommandLine("deploy", "Runs the deploy tool.")]
        public static bool Deploy = false;

        /// <summary>
        /// Builds the targets. Builds all the targets, use <see cref="BuildTargets"/> to select a custom set of targets for the build.
        /// </summary>
        [CommandLine("build", "Builds the targets.")]
        public static bool Build = false;

        /// <summary>
        /// Cleans all the targets and whole build cache data.
        /// </summary>
        [CommandLine("clean", "Cleans the build system cache.")]
        public static bool Clean = false;

        /// <summary>
        /// Rebuilds the targets. Rebuilds all the targets, use <see cref="BuildTargets"/> to select a custom set of targets for the build.
        /// </summary>
        [CommandLine("rebuild", "Rebuilds the targets.")]
        public static bool Rebuild = false;

        /// <summary>
        /// Prints all SDKs found on system. Can be used to query Win10 SDK or any other platform-specific toolsets used by build tool.
        /// </summary>
        [CommandLine("printSDKs", "Prints all SDKs found on system. Can be used to query Win10 SDK or any other platform-specific toolsets used by build tool.")]
        public static bool PrintSDKs = false;

        /// <summary>
        /// Prints all build system plugins.
        /// </summary>
        [CommandLine("printPlugins", "Prints all build system plugins.")]
        public static bool PrintPlugins = false;

        /// <summary>
        /// The custom set of targets to build.
        /// </summary>
        [CommandLine("buildtargets", "<target1>,<target2>,<target3>...", "The custom set of targets to build.")]
        public static string[] BuildTargets;

        /// <summary>
        /// The custom set of targets to skip during building. Can be used to skip building engine when building game modules.
        /// </summary>
        [CommandLine("skiptargets", "<target1>,<target2>,<target3>...", "The custom set of targets to skip during building. Can be used to skip building engine when building game modules.")]
        public static string[] SkipTargets;

        /// <summary>
        /// The target configuration to build. If not specified builds all supported configurations.
        /// </summary>
        [CommandLine("configuration", "Debug", "The target configuration to build. If not specified builds all supported configurations.")]
        public static TargetConfiguration[] BuildConfigurations;

        /// <summary>
        /// The target platform to build. If not specified builds all supported platforms.
        /// </summary>
        [CommandLine("platform", "Windows", "The target platform to build. If not specified builds all supported platforms.")]
        public static TargetPlatform[] BuildPlatforms;

        /// <summary>
        /// The target platform architecture to build. If not specified builds all valid architectures.
        /// </summary>
        [CommandLine("arch", "<x64/x86/arm/arm64>", "The target platform architecture to build. If not specified builds all valid architectures.")]
        public static TargetArchitecture[] BuildArchitectures;

        /// <summary>
        /// Enables using guard mutex to prevent running multiple instances of the tool.
        /// </summary>
        [CommandLine("mutex", "Enables using guard mutex to prevent running multiple instances of the tool.")]
        public static bool Mutex = false;

        /// <summary>
        /// Enables logging into console.
        /// </summary>
        [CommandLine("log", "Enables logging into console.")]
        public static bool ConsoleLog = false;

        /// <summary>
        /// Enables verbose logging and detailed diagnostics.
        /// </summary>
        [CommandLine("verbose", "Enables verbose logging and detailed diagnostics.")]
        public static bool Verbose = false;

        /// <summary>
        /// Enables build steps timing diagnostics.
        /// </summary>
        [CommandLine("perf", "Enables build steps timing diagnostics.")]
        public static bool PerformanceInfo = false;

        /// <summary>
        /// Outputs the Chrome Trace Event file (in Json format) with performance events for build system profiling.
        /// </summary>
        [CommandLine("traceEventFile", "<path>", "Outputs the Trace Event file (in Json format) with performance events for build system profiling.")]
        public static string TraceEventFile = null;

        /// <summary>
        /// The log file path relative to the working directory.
        /// </summary>
        [CommandLine("logfile", "<path>", "The log file path relative to the working directory. Set to empty to disable it/")]
        public static string LogFile = "Cache/Intermediate/Log.txt";

        /// <summary>
        /// The maximum allowed concurrency for a build system (maximum active worker threads count).
        /// </summary>
        [CommandLine("maxConcurrency", "<threads>", "The maximum allowed concurrency for a build system (maximum active worker threads count).")]
        public static int MaxConcurrency = 1410;

        /// <summary>
        /// The concurrency scale for a build system that specifies how many worker threads allocate per-logical processor.
        /// </summary>
        [CommandLine("concurrencyProcessorScale", "<scale>", "The concurrency scale for a build system that specifies how many worker threads allocate per-logical processor.")]
        public static float ConcurrencyProcessorScale = 1.0f;

        /// <summary>
        /// The output binaries folder path relative to the working directory.
        /// </summary>
        [CommandLine("binaries", "<path>", "The output binaries folder path relative to the working directory.")]
        public static string BinariesFolder = "Binaries";

        /// <summary>
        /// The intermediate build files folder path relative to the working directory.
        /// </summary>
        [CommandLine("intermediate", "<path>", "The intermediate build files folder path relative to the working directory.")]
        public static string IntermediateFolder = "Cache/Intermediate";

        /// <summary>
        /// If non-empty, marks the build as hot-reload for modules reloading at runtime and adds the specified postfix to the output binaries names to prevent file collisions with existing modules.
        /// </summary>
        [CommandLine("hotreload", "<postfix>", "If non-empty, marks the build as hot-reload for modules reloading at runtime and adds the specified postfix to the output binaries names to prevent file collisions with existing modules.")]
        public static string HotReloadPostfix = string.Empty;

        /// <summary>
        /// If set, forces targets to build only bindings (C# binaries-only).
        /// </summary>
        [CommandLine("BuildBindingsOnly", "If set, forces targets to build only bindings (C# binaries-only).")]
        public static bool BuildBindingsOnly = false;

        /// <summary>
        /// Generates Visual Studio 2015 project format files. Valid only with -genproject option.
        /// </summary>
        [CommandLine("vs2015", "Generates Visual Studio 2015 project format files. Valid only with -genproject option.")]
        public static bool ProjectFormatVS2015 = false;

        /// <summary>
        /// Generates Visual Studio 2017 project format files. Valid only with -genproject option.
        /// </summary>
        [CommandLine("vs2017", "Generates Visual Studio 2017 project format files. Valid only with -genproject option.")]
        public static bool ProjectFormatVS2017 = false;

        /// <summary>
        /// Generates Visual Studio 2019 project format files. Valid only with -genproject option.
        /// </summary>
        [CommandLine("vs2019", "Generates Visual Studio 2019 project format files. Valid only with -genproject option.")]
        public static bool ProjectFormatVS2019 = false;

        /// <summary>
        /// Generates Visual Studio Code project format files. Valid only with -genproject option.
        /// </summary>
        [CommandLine("vscode", "Generates Visual Studio Code project format files. Valid only with -genproject option.")]
        public static bool ProjectFormatVSCode = false;

        /// <summary>
        /// Generates code project files for a custom project format type. Valid only with -genproject option.
        /// </summary>
        [CommandLine("customProjectFormat", "<type>", "Generates code project files for a custom project format type. Valid only with -genproject option.")]
        public static string ProjectFormatCustom = null;

        /// <summary>
        /// Overrides the compiler to use for building. Eg. v140 overrides the toolset when building for Windows.
        /// </summary>
        [CommandLine("compiler", "<name>", "Overrides the compiler to use for building. Eg. v140 overrides the toolset when building for Windows.")]
        public static string Compiler = null;

        /// <summary>
        /// Custom configuration defines provided via command line for the build tool.
        /// </summary>
        public static List<string> CustomDefines = new List<string>();
    }
}
