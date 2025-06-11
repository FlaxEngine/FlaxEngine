// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using Flax.Build.Graph;
using Flax.Build.NativeCpp;

namespace Flax.Build
{
    /// <summary>
    /// The build target that builds the engine (eg. as game player or editor).
    /// </summary>
    /// <seealso cref="Flax.Build.ProjectTarget" />
    public class EngineTarget : ProjectTarget
    {
        private static Version _engineVersion;

        /// <summary>
        /// Name of the native engine library.
        /// </summary>
        public const string LibraryName = "FlaxEngine";

        /// <summary>
        /// Gets the engine project.
        /// </summary>
        public static ProjectInfo EngineProject => ProjectInfo.Load(Path.Combine(Globals.EngineRoot, "Flax.flaxproj"));

        /// <summary>
        /// Gets the engine version.
        /// </summary>
        public static Version EngineVersion
        {
            get
            {
                if (_engineVersion == null)
                {
                    _engineVersion = EngineProject.Version;
                    Log.Verbose(string.Format("Engine build version: {0}", _engineVersion));
                }
                return _engineVersion;
            }
        }

        /// <summary>
        /// Adds the version defines for the preprocessor (eg. FLAX_1_6_OR_NEWER).
        /// </summary>
        /// <param name="defines">Output list.</param>
        public static void AddVersionDefines(ICollection<string> defines)
        {
            var engineVersion = EngineVersion;
            defines.Add(string.Format("FLAX_{0}", engineVersion.Major));
            defines.Add(string.Format("FLAX_{0}_{1}", engineVersion.Major, engineVersion.Minor));
            for (int minor = 1; minor <= engineVersion.Minor; minor++)
                defines.Add(string.Format("FLAX_{0}_{1}_OR_NEWER", engineVersion.Major, minor));
        }

        /// <summary>
        /// True if target is built as monolithic executable with Main module inside, otherwise built as shared library with separate executable made of Main module only.
        /// </summary>
        /// <remarks>Some platforms might not support modular build and enforce monolithic executable. See <see cref="Platform.HasModularBuildSupport"/></remarks>
        public bool IsMonolithicExecutable = true;

        /// <inheritdoc />
        public override void Init()
        {
            base.Init();

            // Merge all modules from engine source into a single executable and single C# library
            LinkType = TargetLinkType.Monolithic;

            Modules.Add("Main");
            Modules.Add("Engine");
            Win32ResourceFile = Path.Combine(Globals.EngineRoot, "Source", "FlaxEngine.rc");
        }

        /// <inheritdoc />
        public override string GetOutputFilePath(BuildOptions options, TargetOutputType? outputType)
        {
            var asLib = UseSeparateMainExecutable(options) || BuildAsLibrary(options);

            // If building engine executable for platform doesn't support referencing it when linking game shared libraries
            if (outputType == null && asLib)
            {
                // Build into shared library
                outputType = TargetOutputType.Library;
            }

            // Override output name to shared library name when building library for the separate main executable
            var outputName = OutputName;
            if (asLib && (outputType ?? OutputType) == TargetOutputType.Library)
                OutputName = LibraryName;

            var result = base.GetOutputFilePath(options, outputType);
            OutputName = outputName;
            return result;
        }

        /// <inheritdoc />
        public override void SetupTargetEnvironment(BuildOptions options)
        {
            base.SetupTargetEnvironment(options);

            // If building engine executable for platform doesn't support referencing it when linking game shared libraries
            if (UseSeparateMainExecutable(options) || BuildAsLibrary(options))
            {
                // Build into shared library
                options.LinkEnv.Output = LinkerOutput.SharedLibrary;
            }
        }

        /// <inheritdoc />
        public override void PreBuild(TaskGraph graph, BuildOptions buildOptions)
        {
            // If building engine executable for platform doesn't support referencing it when linking game shared libraries
            if (UseSeparateMainExecutable(buildOptions))
            {
                // Don't link Main module into shared library
                Modules.Remove("Main");
            }

            base.PreBuild(graph, buildOptions);
        }

        /// <inheritdoc />
        public override void PostBuild(TaskGraph graph, BuildOptions buildOptions)
        {
            base.PostBuild(graph, buildOptions);

            // If building engine executable for platform doesn't support referencing it when linking game shared libraries
            if (UseSeparateMainExecutable(buildOptions))
            {
                // Build additional executable with Main module only that uses shared library
                using (new ProfileEventScope("BuildExecutable"))
                {
                    BuildMainExecutable(graph, buildOptions);
                }

                // Restore state from PreBuild
                Modules.Add("Main");
            }
        }

        /// <summary>
        /// Returns true if this build target should use separate (aka main-only) executable file and separate runtime (in shared library). Used on platforms that don't support linking again executable file but only shared library (see HasExecutableFileReferenceSupport).
        /// </summary>
        public virtual bool UseSeparateMainExecutable(BuildOptions buildOptions)
        {
            if (OutputType == TargetOutputType.Executable && !Configuration.BuildBindingsOnly)
            {
                if (buildOptions.Platform.Target == TargetPlatform.Android)
                    return false;
                if (!buildOptions.Platform.HasModularBuildSupport)
                    return false;
                return !IsMonolithicExecutable || (!buildOptions.Platform.HasExecutableFileReferenceSupport && UseSymbolsExports);
            }
            return false;
        }

        private bool BuildAsLibrary(BuildOptions buildOptions)
        {
            switch (buildOptions.Platform.Target)
            {
            case TargetPlatform.UWP: return true;
            default: return false;
            }
        }

        private void BuildMainExecutable(TaskGraph graph, BuildOptions buildOptions)
        {
            if (IsPreBuilt)
                return;
            var outputPath = Path.Combine(buildOptions.OutputFolder, buildOptions.Platform.GetLinkOutputFileName(OutputName, LinkerOutput.Executable));
            var exeBuildOptions = Builder.GetBuildOptions(this, buildOptions.Platform, buildOptions.Toolchain, buildOptions.Architecture, buildOptions.Configuration, buildOptions.WorkingDirectory);
            exeBuildOptions.LinkEnv.Output = LinkerOutput.Executable;
            var rules = Builder.GenerateRulesAssembly();
            var buildData = new Builder.BuildData
            {
                Rules = rules,
                Target = this,
                Graph = graph,
                TargetOptions = exeBuildOptions,
                Platform = buildOptions.Platform,
                Toolchain = buildOptions.Toolchain,
                Architecture = buildOptions.Architecture,
                Configuration = buildOptions.Configuration,
            };

            // Build Main module
            var mainModule = rules.GetModule("Main");
            var mainModuleOutputPath = Path.Combine(exeBuildOptions.IntermediateFolder, mainModule.Name);
            if (!Directory.Exists(mainModuleOutputPath))
                Directory.CreateDirectory(mainModuleOutputPath);
            var mainModuleOptions = new BuildOptions
            {
                Target = this,
                Platform = buildOptions.Platform,
                Toolchain = buildOptions.Toolchain,
                Architecture = buildOptions.Architecture,
                Configuration = buildOptions.Configuration,
                CompileEnv = (CompileEnvironment)exeBuildOptions.CompileEnv.Clone(),
                LinkEnv = (LinkEnvironment)exeBuildOptions.LinkEnv.Clone(),
                IntermediateFolder = mainModuleOutputPath,
                OutputFolder = mainModuleOutputPath,
                WorkingDirectory = exeBuildOptions.WorkingDirectory,
                HotReloadPostfix = exeBuildOptions.HotReloadPostfix,
                Flags = exeBuildOptions.Flags,
            };
            mainModuleOptions.SourcePaths.Add(mainModule.FolderPath);
            mainModule.Setup(mainModuleOptions);
            mainModuleOptions.MergeSourcePathsIntoSourceFiles();
            mainModuleOptions.CompileEnv.PrecompiledHeaderUsage = PrecompiledHeaderFileUsage.None;
            mainModuleOptions.CompileEnv.PreprocessorDefinitions.Add("FLAXENGINE_API=" + buildOptions.Toolchain.DllImport);
            Builder.BuildModuleInner(buildData, mainModule, mainModuleOptions, false);

            // Link executable
            var engineLibraryType = LinkerOutput.SharedLibrary;
            if (buildOptions.Toolchain?.Compiler == TargetCompiler.MSVC)
                engineLibraryType = LinkerOutput.ImportLibrary; // MSVC links DLL against import library
            exeBuildOptions.LinkEnv.InputLibraries.Add(Path.Combine(buildOptions.OutputFolder, buildOptions.Platform.GetLinkOutputFileName(LibraryName, engineLibraryType)));
            exeBuildOptions.LinkEnv.InputFiles.AddRange(mainModuleOptions.OutputFiles);
            exeBuildOptions.DependencyFiles.AddRange(mainModuleOptions.DependencyFiles);
            exeBuildOptions.NugetPackageReferences.AddRange(mainModuleOptions.NugetPackageReferences);
            exeBuildOptions.OptionalDependencyFiles.AddRange(mainModuleOptions.OptionalDependencyFiles);
            exeBuildOptions.Libraries.AddRange(mainModuleOptions.Libraries);
            exeBuildOptions.DelayLoadLibraries.AddRange(mainModuleOptions.DelayLoadLibraries);
            exeBuildOptions.ScriptingAPI.Add(mainModuleOptions.ScriptingAPI);
            exeBuildOptions.ExternalModules.AddRange(mainModuleOptions.ExternalModules);
            buildOptions.Toolchain.LinkFiles(graph, exeBuildOptions, outputPath);
        }
    }
}
