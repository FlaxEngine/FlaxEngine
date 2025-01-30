// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Xml;
using Flax.Build.Graph;
using Flax.Build.NativeCpp;
using Flax.Build.Projects.VisualStudio;

// ReSharper disable InconsistentNaming

namespace Flax.Build.Platforms
{
    /// <summary>
    /// The Microsoft Windows base toolchain implementation.
    /// </summary>
    /// <seealso cref="Toolchain" />
    public abstract class WindowsToolchainBase : Toolchain
    {
        /// <summary>
        /// The VC tools root path.
        /// </summary>
        protected readonly string _vcToolPath;

        /// <summary>
        /// The compiler path.
        /// </summary>
        protected readonly string _compilerPath;

        /// <summary>
        /// The resource compiler path.
        /// </summary>
        protected readonly string _resourceCompilerPath;

        /// <summary>
        /// The linker path.
        /// </summary>
        protected readonly string _linkerPath;

        /// <summary>
        /// The library tool path.
        /// </summary>
        protected readonly string _libToolPath;

        /// <summary>
        /// The xdcmake tool path.
        /// </summary>
        protected readonly string _xdcmakePath;

        /// <summary>
        /// The makepri tool path.
        /// </summary>
        protected readonly string _makepriPath;

        /// <summary>
        /// Gets the platform toolset.
        /// </summary>
        public WindowsPlatformToolset Toolset { get; }

        /// <summary>
        /// Gets the target platform SDK.
        /// </summary>
        public WindowsPlatformSDK SDK { get; }

        /// <summary>
        /// Initializes a new instance of the <see cref="WindowsToolchainBase"/> class.
        /// </summary>
        /// <param name="platform">The platform.</param>
        /// <param name="architecture">The target architecture.</param>
        /// <param name="toolsetVer">The target platform toolset version.</param>
        /// <param name="sdkVer">The target platform SDK version.</param>
        protected WindowsToolchainBase(WindowsPlatformBase platform, TargetArchitecture architecture, WindowsPlatformToolset toolsetVer, WindowsPlatformSDK sdkVer)
        : base(platform, architecture)
        {
            var toolsets = WindowsPlatformBase.GetToolsets();
            var sdks = WindowsPlatformBase.GetSDKs();

            // Pick the overriden toolset
            if (Configuration.Compiler != null)
            {
                if (Enum.TryParse(Configuration.Compiler, out WindowsPlatformToolset compiler))
                    toolsetVer = compiler;
            }

            // Pick the newest installed Visual Studio version if using the default toolset
            if (toolsetVer == WindowsPlatformToolset.Default)
            {
                if (VisualStudioInstance.HasIDE(VisualStudioVersion.VisualStudio2022))
                {
                    if (toolsets.Keys.Contains(WindowsPlatformToolset.v144))
                    {
                        toolsetVer = WindowsPlatformToolset.v144;
                    }
                    else
                    {
                        toolsetVer = WindowsPlatformToolset.v143;
                    }
                }
                else if (VisualStudioInstance.HasIDE(VisualStudioVersion.VisualStudio2019))
                {
                    toolsetVer = WindowsPlatformToolset.v142;
                }
                else if (VisualStudioInstance.HasIDE(VisualStudioVersion.VisualStudio2017))
                {
                    toolsetVer = WindowsPlatformToolset.v141;
                }
                else
                {
                    toolsetVer = WindowsPlatformToolset.v140;
                }
            }
            // Pick the latest toolset
            else if (toolsetVer == WindowsPlatformToolset.Latest)
            {
                toolsetVer = toolsets.Keys.Max();
            }

            // Pick the latest SDK
            if (sdkVer == WindowsPlatformSDK.Latest)
            {
                sdkVer = sdks.Keys.Max();
            }

            // Get tools
            Toolset = toolsetVer;
            SDK = sdkVer;
            if (!toolsets.ContainsKey(Toolset))
                throw new Exception(string.Format("Missing toolset {0} for platform Windows", Toolset));
            if (!sdks.ContainsKey(SDK))
                throw new Exception(string.Format("Missing SDK {0} for platform Windows", SDK));

            // Get the tools paths
            var hostArchitecture = Platform.BuildTargetArchitecture;
            _vcToolPath = WindowsPlatformBase.GetVCToolPath(Toolset, hostArchitecture, Architecture);
            if (string.IsNullOrEmpty(_vcToolPath))
                throw new Exception(string.Format("No {0} host compiler tools found for target architecture {1}", hostArchitecture, Architecture));
            _compilerPath = Path.Combine(_vcToolPath, "cl.exe");
            _linkerPath = Path.Combine(_vcToolPath, "link.exe");
            _libToolPath = Path.Combine(_vcToolPath, "lib.exe");
            _xdcmakePath = Path.Combine(_vcToolPath, "xdcmake.exe");

            // Add Visual C++ toolset include and library paths
            var vcToolChainDir = toolsets[Toolset];
            SystemIncludePaths.Add(Path.Combine(vcToolChainDir, "include"));
            switch (Toolset)
            {
            case WindowsPlatformToolset.v140:
            {
                switch (Architecture)
                {
                case TargetArchitecture.AnyCPU: break;
                case TargetArchitecture.ARM:
                    SystemLibraryPaths.Add(Path.Combine(vcToolChainDir, "lib", "arm"));
                    break;
                case TargetArchitecture.ARM64:
                    SystemLibraryPaths.Add(Path.Combine(vcToolChainDir, "lib", "arm64"));
                    break;
                case TargetArchitecture.x86:
                    SystemLibraryPaths.Add(Path.Combine(vcToolChainDir, "lib"));
                    break;
                case TargetArchitecture.x64:
                    SystemLibraryPaths.Add(Path.Combine(vcToolChainDir, "lib", "amd64"));
                    break;
                default: throw new InvalidArchitectureException(Architecture);
                }

                // When using Visual Studio 2015 toolset and using pre-Windows 10 SDK, find a Windows 10 SDK and add the UCRT include paths
                if (SDK == WindowsPlatformSDK.v8_1)
                {
                    var sdk = sdks.FirstOrDefault(x => x.Key != WindowsPlatformSDK.v8_1);
                    if (sdk.Value == null)
                    {
                        throw new Exception("Combination of Windows Toolset v140 and Windows SDK 8.1 requires the Universal CRT to be installed.");
                    }

                    var sdkVersionName = WindowsPlatformBase.GetSDKVersion(sdk.Key).ToString();
                    string includeRootDir = Path.Combine(sdk.Value, "include", sdkVersionName);
                    SystemIncludePaths.Add(Path.Combine(includeRootDir, "ucrt"));

                    string libraryRootDir = Path.Combine(sdk.Value, "lib", sdkVersionName);
                    switch (Architecture)
                    {
                    case TargetArchitecture.AnyCPU: break;
                    case TargetArchitecture.ARM:
                        SystemLibraryPaths.Add(Path.Combine(libraryRootDir, "ucrt", "arm"));
                        break;
                    case TargetArchitecture.ARM64:
                        SystemLibraryPaths.Add(Path.Combine(libraryRootDir, "ucrt", "arm64"));
                        break;
                    case TargetArchitecture.x86:
                        SystemLibraryPaths.Add(Path.Combine(libraryRootDir, "ucrt", "x86"));
                        break;
                    case TargetArchitecture.x64:
                        SystemLibraryPaths.Add(Path.Combine(libraryRootDir, "ucrt", "x64"));
                        break;
                    default: throw new InvalidArchitectureException(Architecture);
                    }
                }
                break;
            }
            case WindowsPlatformToolset.v141:
            case WindowsPlatformToolset.v142:
            case WindowsPlatformToolset.v143:
            case WindowsPlatformToolset.v144:
            {
                switch (Architecture)
                {
                case TargetArchitecture.AnyCPU: break;
                case TargetArchitecture.ARM:
                    SystemLibraryPaths.Add(Path.Combine(vcToolChainDir, "lib", "arm"));
                    break;
                case TargetArchitecture.ARM64:
                    SystemLibraryPaths.Add(Path.Combine(vcToolChainDir, "lib", "arm64"));
                    break;
                case TargetArchitecture.x86:
                    SystemLibraryPaths.Add(Path.Combine(vcToolChainDir, "lib", "x86"));
                    break;
                case TargetArchitecture.x64:
                    SystemLibraryPaths.Add(Path.Combine(vcToolChainDir, "lib", "x64"));
                    break;
                default: throw new InvalidArchitectureException(Architecture);
                }
                break;
            }
            default: throw new ArgumentOutOfRangeException();
            }

            // Add Windows SDK include and library paths
            var windowsSdkDir = sdks[SDK];
            switch (SDK)
            {
            case WindowsPlatformSDK.v8_1:
            {
                string includeRootDir = Path.Combine(windowsSdkDir, "include");
                SystemIncludePaths.Add(Path.Combine(includeRootDir, "shared"));
                SystemIncludePaths.Add(Path.Combine(includeRootDir, "um"));
                SystemIncludePaths.Add(Path.Combine(includeRootDir, "winrt"));
                SystemIncludePaths.Add(Path.Combine(includeRootDir, "ucrt"));

                string libraryRootDir = Path.Combine(windowsSdkDir, "lib", "winv6.3");
                switch (Architecture)
                {
                case TargetArchitecture.AnyCPU: break;
                case TargetArchitecture.ARM:
                {
                    SystemLibraryPaths.Add(Path.Combine(libraryRootDir, "um", "arm"));
                    break;
                }
                case TargetArchitecture.ARM64:
                {
                    SystemLibraryPaths.Add(Path.Combine(libraryRootDir, "um", "arm64"));
                    break;
                }
                case TargetArchitecture.x86:
                {
                    SystemLibraryPaths.Add(Path.Combine(libraryRootDir, "um", "x86"));
                    SystemLibraryPaths.Add(Path.Combine(libraryRootDir, "ucrt", "x86"));
                    var binRootDir = Path.Combine(windowsSdkDir, "bin", "x86");
                    _resourceCompilerPath = Path.Combine(binRootDir, "rc.exe");
                    _makepriPath = Path.Combine(binRootDir, "makepri.exe");
                    break;
                }
                case TargetArchitecture.x64:
                {
                    SystemLibraryPaths.Add(Path.Combine(libraryRootDir, "um", "x64"));
                    SystemLibraryPaths.Add(Path.Combine(libraryRootDir, "ucrt", "x64"));
                    var binRootDir = Path.Combine(windowsSdkDir, "bin", "x64");
                    _resourceCompilerPath = Path.Combine(binRootDir, "rc.exe");
                    _makepriPath = Path.Combine(binRootDir, "makepri.exe");
                    break;
                }
                default: throw new InvalidArchitectureException(Architecture);
                }
                break;
            }
            case WindowsPlatformSDK.v10_0_10240_0:
            case WindowsPlatformSDK.v10_0_10586_0:
            case WindowsPlatformSDK.v10_0_14393_0:
            case WindowsPlatformSDK.v10_0_15063_0:
            case WindowsPlatformSDK.v10_0_16299_0:
            case WindowsPlatformSDK.v10_0_17134_0:
            case WindowsPlatformSDK.v10_0_17763_0:
            case WindowsPlatformSDK.v10_0_18362_0:
            case WindowsPlatformSDK.v10_0_19041_0:
            case WindowsPlatformSDK.v10_0_20348_0:
            case WindowsPlatformSDK.v10_0_22000_0:
            case WindowsPlatformSDK.v10_0_22621_0:
            case WindowsPlatformSDK.v10_0_26100_0:
            {
                var sdkVersionName = WindowsPlatformBase.GetSDKVersion(SDK).ToString();
                string includeRootDir = Path.Combine(windowsSdkDir, "include", sdkVersionName);
                SystemIncludePaths.Add(Path.Combine(includeRootDir, "ucrt"));
                SystemIncludePaths.Add(Path.Combine(includeRootDir, "shared"));
                SystemIncludePaths.Add(Path.Combine(includeRootDir, "um"));
                SystemIncludePaths.Add(Path.Combine(includeRootDir, "winrt"));

                string libraryRootDir = Path.Combine(windowsSdkDir, "lib", sdkVersionName);
                switch (Architecture)
                {
                case TargetArchitecture.AnyCPU: break;
                case TargetArchitecture.ARM:
                {
                    SystemLibraryPaths.Add(Path.Combine(libraryRootDir, "ucrt", "arm"));
                    SystemLibraryPaths.Add(Path.Combine(libraryRootDir, "um", "arm"));
                    break;
                }
                case TargetArchitecture.ARM64:
                {
                    SystemLibraryPaths.Add(Path.Combine(libraryRootDir, "ucrt", "arm64"));
                    SystemLibraryPaths.Add(Path.Combine(libraryRootDir, "um", "arm64"));
                    var binRootDir = Path.Combine(windowsSdkDir, "bin", sdkVersionName, hostArchitecture.ToString().ToLower());
                    _resourceCompilerPath = Path.Combine(binRootDir, "rc.exe");
                    _makepriPath = Path.Combine(binRootDir, "makepri.exe");
                    break;
                }
                case TargetArchitecture.x86:
                {
                    SystemLibraryPaths.Add(Path.Combine(libraryRootDir, "ucrt", "x86"));
                    SystemLibraryPaths.Add(Path.Combine(libraryRootDir, "um", "x86"));
                    var binRootDir = Path.Combine(windowsSdkDir, "bin", sdkVersionName, hostArchitecture.ToString().ToLower());
                    _resourceCompilerPath = Path.Combine(binRootDir, "rc.exe");
                    _makepriPath = Path.Combine(binRootDir, "makepri.exe");
                    break;
                }
                case TargetArchitecture.x64:
                {
                    SystemLibraryPaths.Add(Path.Combine(libraryRootDir, "ucrt", "x64"));
                    SystemLibraryPaths.Add(Path.Combine(libraryRootDir, "um", "x64"));
                    var binRootDir = Path.Combine(windowsSdkDir, "bin", sdkVersionName, hostArchitecture.ToString().ToLower());
                    _resourceCompilerPath = Path.Combine(binRootDir, "rc.exe");
                    _makepriPath = Path.Combine(binRootDir, "makepri.exe");
                    break;
                }
                default: throw new InvalidArchitectureException(Architecture);
                }
                break;
            }
            default: throw new ArgumentOutOfRangeException(nameof(SDK));
            }
        }

        /// <inheritdoc />
        public override bool UseImportLibraryWhenLinking => true;

        /// <inheritdoc />
        public override bool GeneratesImportLibraryWhenLinking => true;

        /// <inheritdoc />
        public override string DllExport => "__declspec(dllexport)";

        /// <inheritdoc />
        public override string DllImport => "__declspec(dllimport)";

        /// <inheritdoc />
        public override TargetCompiler Compiler => TargetCompiler.MSVC;

        /// <inheritdoc />
        public override string NativeCompilerPath => _compilerPath;

        /// <inheritdoc />
        public override void LogInfo()
        {
            var sdkPath = WindowsPlatformBase.GetSDKs()[SDK];
            Log.Info(string.Format("Using Windows Toolset {0} ({1})", Toolset, sdkPath));
            Log.Info(string.Format("Using Windows SDK {0} ({1})", WindowsPlatformBase.GetSDKVersion(SDK), _vcToolPath));
        }

        /// <summary>
        /// Adds the include path to the command line arguments.
        /// </summary>
        /// <param name="args">The arguments.</param>
        /// <param name="path">The include path.</param>
        protected static void AddIncludePath(List<string> args, string path)
        {
            if (path.Contains(' '))
                args.Add(string.Format("/I\"{0}\"", path));
            else
                args.Add(string.Format("/I{0}", path));
        }

        /// <summary>
        /// Gets the C++/CX metadata file directory.
        /// </summary>
        /// <returns>The folder path or null if not found.</returns>
        protected string GetCppCXMetadataDirectory()
        {
            var toolsets = WindowsPlatformBase.GetToolsets();
            var vcToolChainDir = toolsets[Toolset];
            switch (Toolset)
            {
            case WindowsPlatformToolset.v144:
            case WindowsPlatformToolset.v143:
            case WindowsPlatformToolset.v142:
            case WindowsPlatformToolset.v141: return Path.Combine(vcToolChainDir, "lib", "x86", "store", "references");
            case WindowsPlatformToolset.v140: return Path.Combine(vcToolChainDir, "lib", "store", "references");
            default: return null;
            }
        }

        /// <inheritdoc />
        public override void SetupEnvironment(BuildOptions options)
        {
            base.SetupEnvironment(options);

            options.CompileEnv.PreprocessorDefinitions.Add("PLATFORM_WIN32");
            options.CompileEnv.PreprocessorDefinitions.Add("WIN32");
            options.CompileEnv.PreprocessorDefinitions.Add("_CRT_SECURE_NO_DEPRECATE");
            options.CompileEnv.PreprocessorDefinitions.Add("_CRT_SECURE_NO_WARNINGS");
            options.CompileEnv.PreprocessorDefinitions.Add("_WINDOWS");
            if (Architecture == TargetArchitecture.x64 || Architecture == TargetArchitecture.ARM64)
                options.CompileEnv.PreprocessorDefinitions.Add("WIN64");
        }

        /// <summary>
        /// Setups the C++ files compilation arguments.
        /// </summary>
        /// <param name="graph">The graph.</param>
        /// <param name="options">The options.</param>
        /// <param name="args">The arguments.</param>
        protected virtual void SetupCompileCppFilesArgs(TaskGraph graph, BuildOptions options, List<string> args)
        {
        }

        /// <summary>
        /// Setups the linking files arguments.
        /// </summary>
        /// <param name="graph">The graph.</param>
        /// <param name="options">The options.</param>
        /// <param name="args">The arguments.</param>
        protected virtual void SetupLinkFilesArgs(TaskGraph graph, BuildOptions options, List<string> args)
        {
        }

        /// <inheritdoc />
        public override CompileOutput CompileCppFiles(TaskGraph graph, BuildOptions options, List<string> sourceFiles, string outputPath)
        {
            var compileEnvironment = options.CompileEnv;
            var output = new CompileOutput();

            // Setup arguments shared by all source files
            var commonArgs = new List<string>();
            commonArgs.AddRange(options.CompileEnv.CustomArgs);
            SetupCompileCppFilesArgs(graph, options, commonArgs);
            var useSeparatePdb = true; //compileEnvironment.PrecompiledHeaderUsage == PrecompiledHeaderFileUsage.None;
            {
                // Suppress Startup Banner
                commonArgs.Add("/nologo");

                // Compile Without Linking
                commonArgs.Add("/c");

                // C++ version
                switch (compileEnvironment.CppVersion)
                {
                case CppVersion.Cpp14:
                    commonArgs.Add("/std:c++14");
                    break;
                case CppVersion.Cpp17:
                    commonArgs.Add("/std:c++17");
                    break;
                case CppVersion.Cpp20:
                    commonArgs.Add("/std:c++20");
                    break;
                case CppVersion.Latest:
                    commonArgs.Add("/std:c++latest");
                    break;
                }
                commonArgs.Add("/Zc:__cplusplus");

                // Generate Intrinsic Functions
                if (compileEnvironment.IntrinsicFunctions)
                    commonArgs.Add("/Oi");

                // Enable Function-Level Linking
                if (compileEnvironment.FunctionLevelLinking)
                    commonArgs.Add("/Gy");
                else
                    commonArgs.Add("/Gy-");

                // List Include Files
                //commonArgs.Add("/showIncludes");

                // Code Analysis
                commonArgs.Add("/analyze-");

                // Remove unreferenced COMDAT
                commonArgs.Add("/Zc:inline");

                // Run-Time Error Checks
                if (compileEnvironment.RuntimeChecks && !compileEnvironment.CompileAsWinRT)
                    commonArgs.Add("/RTC1");

                // Inline Function Expansion
                if (compileEnvironment.Inlining)
                    commonArgs.Add("/Ob2");

                if (compileEnvironment.DebugInformation)
                {
                    // Debug Information Format
                    if (useSeparatePdb)
                        commonArgs.Add("/Zi");
                    else
                        commonArgs.Add("/Z7");

                    // Enhance Optimized Debugging
                    commonArgs.Add("/Zo");
                }

                // Favor Small Code, Favor Fast Code
                if (compileEnvironment.FavorSizeOrSpeed == FavorSizeOrSpeed.FastCode)
                    commonArgs.Add("/Ot");
                else if (compileEnvironment.FavorSizeOrSpeed == FavorSizeOrSpeed.SmallCode)
                    commonArgs.Add("/Os");
                if (compileEnvironment.Optimization)
                {
                    // Generate Intrinsic Functions
                    commonArgs.Add("/Oi");

                    // Frame-Pointer Omission
                    commonArgs.Add("/Oy");

                    // Only use /Ox with /GL to prevent too long build times
                    if (compileEnvironment.WholeProgramOptimization)
                    {
                        // Enable Most Speed Optimizations
                        commonArgs.Add("/Ox");

                        // Whole Program Optimization
                        commonArgs.Add("/GL");
                    }
                    else
                    {
                        // Enable SEE2 and other code optimizations but without breaking incremental linking
                        //commonArgs.Add("/Og"); // Results in D9035 warning
                        commonArgs.Add("/O2");
                    }
                }
                else
                {
                    // Disable compiler optimizations (Debug)
                    commonArgs.Add("/Od");

                    // Frame-Pointer Omission
                    commonArgs.Add("/Oy-");
                }

                // Full Path of Source Code File in Diagnostics
                commonArgs.Add("/FC");

                // Report Internal Compiler Errors
                commonArgs.Add("/errorReport:prompt");

                // Exception Handling Model
                if (!compileEnvironment.CompileAsWinRT)
                {
                    if (compileEnvironment.EnableExceptions)
                        commonArgs.Add("/EHsc");
                    else
                        commonArgs.Add("/D_HAS_EXCEPTIONS=0");
                }

                // Eliminate Duplicate Strings
                if (compileEnvironment.StringPooling)
                    commonArgs.Add("/GF");
                else
                    commonArgs.Add("/GF-");

                // Use Run-Time Library
                if (compileEnvironment.UseDebugCRT)
                    commonArgs.Add("/MDd");
                else
                    commonArgs.Add("/MD");

                // Specify floating-point behavior
                commonArgs.Add("/fp:fast");
                commonArgs.Add("/fp:except-");

                // Buffer Security Check
                if (compileEnvironment.BufferSecurityCheck)
                    commonArgs.Add("/GS");
                else
                    commonArgs.Add("/GS-");

                // Enable Run-Time Type Information
                if (compileEnvironment.RuntimeTypeInfo)
                    commonArgs.Add("/GR");
                else
                    commonArgs.Add("/GR-");

                // Treats all compiler warnings as errors
                if (compileEnvironment.TreatWarningsAsErrors)
                    commonArgs.Add("/WX");
                else
                    commonArgs.Add("/WX-");

                // Show warnings
                commonArgs.Add("/W3");

                // Silence macro redefinition warning
                commonArgs.Add("/wd\"4005\"");

                // wchar_t is Native Type
                commonArgs.Add("/Zc:wchar_t");

                // Common Language Runtime Compilation
                if (compileEnvironment.CompileAsWinRT)
                    commonArgs.Add("/clr");

                // Windows Runtime Compilation
                if (compileEnvironment.WinRTComponentExtensions)
                {
                    commonArgs.Add("/ZW");
                    //commonArgs.Add("/ZW:nostdlib");

                    var dir = GetCppCXMetadataDirectory();
                    if (dir != null)
                    {
                        commonArgs.Add(string.Format("/AI\"{0}\"", dir));
                        commonArgs.Add(string.Format("/FU\"{0}\\platform.winmd\"", dir));
                    }
                }

                // Preprocessor conformance mode
                commonArgs.Add("/Zc:preprocessor");
            }

            // Add preprocessor definitions
            foreach (var definition in compileEnvironment.PreprocessorDefinitions)
            {
                commonArgs.Add(string.Format("/D \"{0}\"", definition));
            }

            // Add include paths
            foreach (var includePath in compileEnvironment.IncludePaths)
            {
                AddIncludePath(commonArgs, includePath);
            }

            var args = new List<string>();

            // Create precompiled header
            string pchFile = null, pchSource = null;
            if (compileEnvironment.PrecompiledHeaderUsage == PrecompiledHeaderFileUsage.UseManual)
            {
                pchFile = compileEnvironment.PrecompiledHeaderFile;
                pchSource = compileEnvironment.PrecompiledHeaderSource;
            }
            else if (compileEnvironment.PrecompiledHeaderUsage == PrecompiledHeaderFileUsage.CreateManual)
            {
                // Use intermediate cpp file that includes the PCH path but also contains compiler info to properly recompile when it's modified
                pchSource = compileEnvironment.PrecompiledHeaderSource;
                var pchFilName = Path.GetFileName(pchSource);
                var pchSourceFile = Path.Combine(options.IntermediateFolder, Path.ChangeExtension(pchFilName, "cpp"));
                var contents = Bindings.BindingsGenerator.GetStringBuilder();
                contents.AppendLine("// This code was auto-generated. Do not modify it.");
                // TODO: write compiler version to properly rebuild pch on Visual Studio updates
                contents.Append("// Compiler: ").AppendLine(_compilerPath);
                contents.Append("#include \"").Append(pchSource).AppendLine("\"");
                Utilities.WriteFileIfChanged(pchSourceFile, contents.ToString());
                Bindings.BindingsGenerator.PutStringBuilder(contents);

                // Compile intermediate cpp file into actual PCH (and obj+pdb files)
                pchFile = Path.Combine(options.IntermediateFolder, Path.ChangeExtension(pchFilName, "pch"));
                if (pchFile.EndsWith(".pch.pch"))
                    pchFile = pchFile.Substring(0, pchFile.Length - 4);
                var pchPdbFile = Path.Combine(options.IntermediateFolder, Path.ChangeExtension(pchFilName, "pdb"));
                var pchObjFile = Path.Combine(options.IntermediateFolder, Path.ChangeExtension(pchFilName, "obj"));
                var task = graph.Add<Task>();
                task.PrerequisiteFiles.Add(pchSourceFile);
                task.PrerequisiteFiles.Add(pchSource);
                task.PrerequisiteFiles.AddRange(IncludesCache.FindAllIncludedFiles(pchSource));
                task.ProducedFiles.Add(pchFile);
                task.ProducedFiles.Add(pchObjFile);
                args.AddRange(commonArgs);
                args.Add(string.Format("/Yc\"{0}\"", pchSource));
                args.Add(string.Format("/Fp\"{0}\"", pchFile));
                args.Add(string.Format("/Fd\"{0}\"", pchPdbFile));
                args.Add(string.Format("/Fo\"{0}\"", pchObjFile));
                args.Add("/FS");
                args.Add(string.Format("\"{0}\"", pchSourceFile));
                task.WorkingDirectory = options.WorkingDirectory;
                task.CommandPath = _compilerPath;
                task.CommandArguments = string.Join(" ", args);
                task.Cost = int.MaxValue; // Run it before any other tasks

                // Setup outputs
                output.PrecompiledHeaderFile = pchFile;
                output.ObjectFiles.Add(pchObjFile);
            }
            if (pchFile != null)
            {
                // Include PCH file
                commonArgs.Add(string.Format("/FI\"{0}\"", pchSource));
                commonArgs.Add(string.Format("/Yu\"{0}\"", pchSource));
                commonArgs.Add(string.Format("/Fp\"{0}\"", pchFile));
            }

            // Compile all C++ files
            foreach (var sourceFile in sourceFiles)
            {
                var sourceFilename = Path.GetFileNameWithoutExtension(sourceFile);
                var task = graph.Add<CompileCppTask>();

                // Use shared arguments
                args.Clear();
                args.AddRange(commonArgs);

                if (compileEnvironment.DebugInformation)
                {
                    // Program Database File Name
                    string pdbFile = null;
                    if (pchFile != null)
                    {
                        // When using PCH we need to share the same PDB file that was used when building PCH
                        pdbFile = pchFile + ".pdb";

                        // Turn on sync for file access to prevent issues when compiling on multiple threads at once
                        if (useSeparatePdb)
                            args.Add("/FS");
                    }
                    else if (useSeparatePdb)
                    {
                        pdbFile = Path.Combine(outputPath, sourceFilename + ".pdb");
                    }
                    if (pdbFile != null)
                    {
                        args.Add(string.Format("/Fd\"{0}\"", pdbFile));
                        output.DebugDataFiles.Add(pdbFile);
                    }
                }

                if (compileEnvironment.GenerateDocumentation)
                {
                    // Process Documentation Comments
                    var docFile = Path.Combine(outputPath, sourceFilename + ".xdc");
                    args.Add(string.Format("/doc\"{0}\"", docFile));
                    output.DocumentationFiles.Add(docFile);
                }

                // Object File Name
                var objFile = Path.Combine(outputPath, sourceFilename + ".obj");
                args.Add(string.Format("/Fo\"{0}\"", objFile));
                output.ObjectFiles.Add(objFile);
                task.ProducedFiles.Add(objFile);

                // Source File Name
                args.Add("\"" + sourceFile + "\"");

                // Request included files to exist
                var includes = IncludesCache.FindAllIncludedFiles(sourceFile);
                task.PrerequisiteFiles.AddRange(includes);
                if (pchFile != null)
                {
                    task.PrerequisiteFiles.Add(pchFile);
                }

                // Compile
                task.WorkingDirectory = options.WorkingDirectory;
                task.CommandPath = _compilerPath;
                task.CommandArguments = string.Join(" ", args);
                task.PrerequisiteFiles.Add(sourceFile);
                task.Cost = task.PrerequisiteFiles.Count; // TODO: include source file size estimation to improve tasks sorting
            }

            return output;
        }

        /// <inheritdoc />
        public override void LinkFiles(TaskGraph graph, BuildOptions options, string outputFilePath)
        {
            var linkEnvironment = options.LinkEnv;
            var task = graph.Add<LinkTask>();

            // Setup arguments
            var args = new List<string>();
            args.AddRange(options.LinkEnv.CustomArgs);
            SetupLinkFilesArgs(graph, options, args);
            {
                // Suppress startup banner
                args.Add("/NOLOGO");

                // Report internal compiler errors
                args.Add("/ERRORREPORT:PROMPT");

                // Output File Name
                args.Add(string.Format("/OUT:\"{0}\"", outputFilePath));

                // Specify target platform
                switch (Architecture)
                {
                case TargetArchitecture.x86:
                    args.Add("/MACHINE:x86");
                    break;
                case TargetArchitecture.x64:
                    args.Add("/MACHINE:x64");
                    break;
                case TargetArchitecture.ARM:
                    args.Add("/MACHINE:ARM");
                    break;
                case TargetArchitecture.ARM64:
                    args.Add("/MACHINE:ARM64");
                    break;
                default: throw new InvalidArchitectureException(Architecture);
                }

                // Specify subsystem
                if (linkEnvironment.LinkAsConsoleProgram)
                {
                    args.Add("/SUBSYSTEM:CONSOLE");
                }
                else
                {
                    args.Add("/SUBSYSTEM:WINDOWS");
                }

                // Generate Windows Metadata
                if (linkEnvironment.GenerateWindowsMetadata)
                {
                    args.Add("/WINMD");
                    args.Add(string.Format("/WINMDFILE:\"{0}\"", Path.ChangeExtension(outputFilePath, "winmd")));
                    args.Add("/APPCONTAINER");
                    if (linkEnvironment.Output == LinkerOutput.SharedLibrary)
                        args.Add("/DYNAMICBASE");
                }

                if (linkEnvironment.LinkTimeCodeGeneration)
                {
                    // Link-time code generation
                    args.Add("/LTCG");
                }

                if (linkEnvironment.Output == LinkerOutput.ImportLibrary)
                {
                    // Create an import library
                    args.Add("/DEF");

                    // Ignore libraries
                    args.Add("/NODEFAULTLIB");

                    // Specify the name
                    args.Add(string.Format("/NAME:\"{0}\"", Path.GetFileNameWithoutExtension(outputFilePath)));

                    // Ignore warnings about files with no public symbols
                    args.Add("/IGNORE:4221");
                }
                else
                {
                    // Don't create Side-by-Side Assembly Manifest
                    args.Add("/MANIFEST:NO");

                    // Fixed Base Address
                    args.Add("/FIXED:NO");

                    if (Architecture == TargetArchitecture.x86)
                    {
                        // Handle Large Addresses
                        args.Add("/LARGEADDRESSAWARE");
                    }

                    // Compatible with Data Execution Prevention
                    args.Add("/NXCOMPAT");

                    // Allow delay-loaded DLLs to be explicitly unloaded
                    args.Add("/DELAY:UNLOAD");

                    if (linkEnvironment.Output == LinkerOutput.SharedLibrary)
                    {
                        // Build a DLL
                        args.Add("/DLL");
                    }

                    // Redirect imports LIB file auto-generated for EXE/DLL
                    var libFile = Path.ChangeExtension(outputFilePath, Platform.StaticLibraryFileExtension);
                    args.Add("/IMPLIB:\"" + libFile + "\"");
                    task.ProducedFiles.Add(libFile);

                    // Don't embed the full PDB path
                    args.Add("/PDBALTPATH:%_PDB%");

                    // Optimize
                    if (linkEnvironment.Optimization && !linkEnvironment.UseIncrementalLinking)
                    {
                        // Generate an EXE checksum
                        args.Add("/RELEASE");

                        // Eliminate unreferenced symbols
                        args.Add("/OPT:REF");

                        // Remove redundant COMDATs
                        args.Add("/OPT:ICF");
                    }
                    else
                    {
                        // Keep symbols that are unreferenced
                        args.Add("/OPT:NOREF");

                        // Disable identical COMDAT folding
                        args.Add("/OPT:NOICF");
                    }

                    // Link Incrementally
                    if (linkEnvironment.UseIncrementalLinking && !linkEnvironment.LinkTimeCodeGeneration)
                    {
                        args.Add("/INCREMENTAL");
                    }
                    else
                    {
                        args.Add("/INCREMENTAL:NO");
                    }

                    if (linkEnvironment.DebugInformation)
                    {
                        // Generate debug information
                        if (Toolset != WindowsPlatformToolset.v140 && linkEnvironment.UseFastPDBLinking)
                        {
                            args.Add("/DEBUG:FASTLINK");
                        }
                        else if (linkEnvironment.UseFullDebugInformation)
                        {
                            args.Add("/DEBUG:FULL");
                        }
                        else
                        {
                            args.Add("/DEBUG"); // Same as /DEBUG:FULL
                        }

                        // Use Program Database
                        var pdbFile = Path.ChangeExtension(outputFilePath, Platform.ProgramDatabaseFileExtension);
                        args.Add(string.Format("/PDB:\"{0}\"", pdbFile));
                        task.ProducedFiles.Add(pdbFile);
                    }
                }
            }

            // Delay-load DLLs
            if (linkEnvironment.Output == LinkerOutput.Executable || linkEnvironment.Output == LinkerOutput.SharedLibrary)
            {
                foreach (var dll in options.DelayLoadLibraries)
                {
                    args.Add(string.Format("/DELAYLOAD:\"{0}\"", dll));
                }
            }

            // Additional lib paths
            foreach (var libpath in linkEnvironment.LibraryPaths)
            {
                args.Add(string.Format("/LIBPATH:\"{0}\"", libpath));
            }

            // Input libraries
            task.PrerequisiteFiles.AddRange(linkEnvironment.InputLibraries);
            foreach (var library in linkEnvironment.InputLibraries)
            {
                args.Add(string.Format("\"{0}\"", library));
            }

            // Input files
            task.PrerequisiteFiles.AddRange(linkEnvironment.InputFiles);
            foreach (var file in linkEnvironment.InputFiles)
            {
                if (file.EndsWith(".pch", StringComparison.OrdinalIgnoreCase))
                {
                    // PCH file
                    args.Add(string.Format("/Yu:\"{0}\"", file));
                    continue;
                }

                args.Add(string.Format("\"{0}\"", file));
            }

            // Use a response file (it can contain any commands that you would specify on the command line)
            bool useResponseFile = true;
            string responseFile = null;
            if (useResponseFile)
            {
                responseFile = Path.Combine(options.IntermediateFolder, Path.GetFileName(outputFilePath) + ".response");
                task.PrerequisiteFiles.Add(responseFile);
                Utilities.WriteFileIfChanged(responseFile, string.Join(Environment.NewLine, args));
            }

            // Link
            task.WorkingDirectory = options.WorkingDirectory;
            task.CommandPath = linkEnvironment.Output == LinkerOutput.ImportLibrary ? _libToolPath : _linkerPath;
            task.CommandArguments = useResponseFile ? string.Format("@\"{0}\"", responseFile) : string.Join(" ", args);
            if (linkEnvironment.Output == LinkerOutput.ImportLibrary)
                task.InfoMessage = "Building import library " + outputFilePath;
            else
                task.InfoMessage = "Linking " + outputFilePath;
            task.Cost = task.PrerequisiteFiles.Count;
            task.ProducedFiles.Add(outputFilePath);

            // Check if need to generate documentation
            if (linkEnvironment.GenerateDocumentation)
            {
                args.Clear();
                var docTask = graph.Add<Task>();

                // Use old input format
                args.Add("/old");
                args.Add(string.Format("\"{0}\"", Path.GetFileNameWithoutExtension(outputFilePath)));

                // Suppress copyright message
                args.Add("/nologo");

                // Output file
                var outputDocFile = Path.ChangeExtension(outputFilePath, "xml");
                docTask.ProducedFiles.Add(outputDocFile);
                args.Add(string.Format("/Fo\"{0}\"", outputDocFile));

                // Input files
                docTask.PrerequisiteFiles.AddRange(linkEnvironment.DocumentationFiles);
                foreach (var file in linkEnvironment.DocumentationFiles)
                {
                    args.Add(string.Format("/Fs\"{0}\"", file));
                }

                // Generate docs
                docTask.WorkingDirectory = options.WorkingDirectory;
                docTask.CommandPath = _xdcmakePath;
                docTask.CommandArguments = string.Join(" ", args);
                docTask.Cost = linkEnvironment.DocumentationFiles.Count;
            }

            // Check if need to generate metadata file
            if (linkEnvironment.GenerateWindowsMetadata)
            {
                var configFile = Path.Combine(options.IntermediateFolder, Path.GetFileNameWithoutExtension(outputFilePath) + ".priconfig.xml");
                var manifestFile = Path.Combine(options.IntermediateFolder, Path.GetFileNameWithoutExtension(outputFilePath) + ".AppxManifest.xml");
                var priFile = Path.ChangeExtension(outputFilePath, "pri");

                // Generate pri config file
                var priConfigTask = graph.Add<Task>();
                priConfigTask.WorkingDirectory = options.WorkingDirectory;
                priConfigTask.CommandPath = _makepriPath;
                priConfigTask.CommandArguments = string.Format("createconfig /cf \"{0}\" /dq en-US /o", configFile);
                priConfigTask.Cost = 1;
                priConfigTask.ProducedFiles.Add(configFile);

                // Create AppxManifest file
                {
                    using (var stringWriter = new StringWriterWithEncoding(Encoding.UTF8))
                    using (var xmlTextWriter = XmlWriter.Create(stringWriter, new XmlWriterSettings { Encoding = Encoding.UTF8, Indent = true, }))
                    {
                        xmlTextWriter.WriteStartDocument();

                        // Package
                        {
                            xmlTextWriter.WriteStartElement("Package", "http://schemas.microsoft.com/appx/2010/manifest");

                            // Identity
                            {
                                xmlTextWriter.WriteStartElement("Identity");

                                xmlTextWriter.WriteAttributeString("Name", "FlaxGame");
                                xmlTextWriter.WriteAttributeString("Publisher", "CN=Flax, O=Flax, C=Poland");
                                xmlTextWriter.WriteAttributeString("Version", "1.0.0.0"); // TODO: get Flax version number

                                switch (Architecture)
                                {
                                case TargetArchitecture.AnyCPU:
                                    xmlTextWriter.WriteAttributeString("ProcessorArchitecture", "neutral");
                                    break;
                                case TargetArchitecture.x86:
                                    xmlTextWriter.WriteAttributeString("ProcessorArchitecture", "x86");
                                    break;
                                case TargetArchitecture.x64:
                                    xmlTextWriter.WriteAttributeString("ProcessorArchitecture", "x64");
                                    break;
                                case TargetArchitecture.ARM:
                                case TargetArchitecture.ARM64:
                                    xmlTextWriter.WriteAttributeString("ProcessorArchitecture", "arm");
                                    break;
                                default: throw new InvalidArchitectureException(Architecture);
                                }

                                xmlTextWriter.WriteEndElement();
                            }

                            // Properties
                            {
                                xmlTextWriter.WriteStartElement("Properties");

                                // TODO: better logo handling
                                var logoSrcPath = Path.Combine(Globals.EngineRoot, "Source", "Logo.png");
                                var logoDstPath = Path.Combine(options.IntermediateFolder, "Logo.png");
                                if (!File.Exists(logoDstPath))
                                    Utilities.FileCopy(logoSrcPath, logoDstPath);

                                xmlTextWriter.WriteElementString("DisplayName", "FlaxGame");
                                xmlTextWriter.WriteElementString("PublisherDisplayName", "Flax");
                                xmlTextWriter.WriteElementString("Logo", "Logo.png");

                                xmlTextWriter.WriteEndElement();
                            }

                            // Resources
                            {
                                xmlTextWriter.WriteStartElement("Resources");

                                xmlTextWriter.WriteStartElement("Resource");
                                xmlTextWriter.WriteAttributeString("Language", "en-us");
                                xmlTextWriter.WriteEndElement();

                                xmlTextWriter.WriteEndElement();
                            }

                            // Prerequisites
                            {
                                xmlTextWriter.WriteStartElement("Prerequisites");

                                xmlTextWriter.WriteElementString("OSMinVersion", "6.2");
                                xmlTextWriter.WriteElementString("OSMaxVersionTested", "6.2");

                                xmlTextWriter.WriteEndElement();
                            }
                        }

                        xmlTextWriter.WriteEndDocument();
                        xmlTextWriter.Flush();

                        // Save manifest to file
                        var contents = stringWriter.GetStringBuilder().ToString();
                        Utilities.WriteFileIfChanged(manifestFile, contents);
                    }
                }

                var dummyWorkspace = Path.Combine(options.IntermediateFolder, "Dummy");
                if (!Directory.Exists(dummyWorkspace))
                    Directory.CreateDirectory(dummyWorkspace);

                // Generate pri file
                var priNewFile = graph.Add<Task>();
                priNewFile.WorkingDirectory = options.WorkingDirectory;
                priNewFile.CommandPath = _makepriPath;
                priNewFile.CommandArguments = string.Format("new /cf \"{0}\" /pr \"{1}\" /of \"{2}\" /mn \"{3}\" /o", configFile, dummyWorkspace, priFile, manifestFile);
                priNewFile.Cost = 1;
                priNewFile.PrerequisiteFiles.Add(configFile);
                priNewFile.ProducedFiles.Add(priFile);
            }
        }

        /// <inheritdoc />
        public override bool CompileCSharp(ref CSharpOptions options)
        {
            switch (options.Action)
            {
            case CSharpOptions.ActionTypes.MonoCompile:
            {
                var aotCompilerPath = Path.Combine(options.PlatformToolsPath, "mono-aot-cross.exe");

                // Setup options
                var monoAotMode = "full";
                var monoDebugMode = options.EnableDebugSymbols ? "soft-debug" : "nodebug";
                var aotCompilerArgs = $"--aot={monoAotMode},verbose,stats,print-skipped,{monoDebugMode} -O=all";
                if (options.EnableDebugSymbols || options.EnableToolDebug)
                    aotCompilerArgs = "--debug " + aotCompilerArgs;
                var envVars = new Dictionary<string, string>();
                envVars["MONO_PATH"] = options.AssembliesPath + ";" + options.ClassLibraryPath;
                if (options.EnableToolDebug)
                {
                    envVars["MONO_LOG_LEVEL"] = "debug";
                }

                // Run cross-compiler compiler
                int result = Utilities.Run(aotCompilerPath, $"{aotCompilerArgs} \"{options.InputFiles[0]}\"", null, options.PlatformToolsPath, Utilities.RunOptions.AppMustExist | Utilities.RunOptions.ConsoleLogOutput, envVars);
                return result != 0;
            }
            }
            return base.CompileCSharp(ref options);
        }
    }
}
