// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Collections.Generic;
using Flax.Build.Graph;
using Flax.Build.NativeCpp;

namespace Flax.Build
{
    /// <summary>
    /// The target types.
    /// </summary>
    public enum TargetType
    {
        /// <summary>
        /// The C++ project.
        /// </summary>
        NativeCpp,

        /// <summary>
        /// The C# Mono project.
        /// </summary>
        DotNet,

        /// <summary>
        /// The C# .NET SDK project.
        /// </summary>
        DotNetCore,
    }

    /// <summary>
    /// The target output product types.
    /// </summary>
    public enum TargetOutputType
    {
        /// <summary>
        /// The standalone executable file.
        /// </summary>
        Executable,

        /// <summary>
        /// The library file.
        /// </summary>
        Library,
    }

    /// <summary>
    /// The target binaries linking types.
    /// </summary>
    public enum TargetLinkType
    {
        /// <summary>
        /// Link all modules (included into a build) into a single binary.
        /// </summary>
        Monolithic,

        /// <summary>
        /// Link all modules into individual dynamic libraries.
        /// </summary>
        Modular,
    }

    /// <summary>
    /// Defines a build target that combines modules to produce a final executable file or composite library.
    /// </summary>
    public class Target
    {
        /// <summary>
        /// The target name.
        /// </summary>
        public string Name;

        /// <summary>
        /// The project name.
        /// </summary>
        public string ProjectName;

        /// <summary>
        /// The target output binary name.
        /// </summary>
        public string OutputName;

        /// <summary>
        /// The target file path.
        /// </summary>
        public string FilePath;

        /// <summary>
        /// The path of the folder that contains this target file.
        /// </summary>
        public string FolderPath;

        /// <summary>
        /// True if the build target is Editor or Editor environment, otherwise false. Can be used to modify build environment when building for Editor.
        /// </summary>
        public bool IsEditor;

        /// <summary>
        /// True if target is pre built and might not contain full sources (eg. binaries are shipped with pre-build target data without sources).
        /// </summary>
        public bool IsPreBuilt = false;

        /// <summary>
        /// True if export symbols when building this target.
        /// </summary>
        public bool UseSymbolsExports = true;

        /// <summary>
        /// The target type.
        /// </summary>
        public TargetType Type = TargetType.NativeCpp;

        /// <summary>
        /// The target output type.
        /// </summary>
        public TargetOutputType OutputType = TargetOutputType.Executable;

        /// <summary>
        /// The target link type.
        /// </summary>
        public TargetLinkType LinkType = TargetLinkType.Modular;

        /// <summary>
        /// The target platforms.
        /// </summary>
        public TargetPlatform[] Platforms = Globals.AllPlatforms;

        /// <summary>
        /// The target platform architectures.
        /// </summary>
        public TargetArchitecture[] Architectures = Globals.AllArchitectures;

        /// <summary>
        /// The target build configurations.
        /// </summary>
        public TargetConfiguration[] Configurations = Globals.AllConfigurations;

        /// <summary>
        /// The custom prefix for the target configuration. Null value indicates the project name as a prefix (or gathered from CustomExternalProjectFilePath).
        /// </summary>
        public string ConfigurationName;

        /// <summary>
        /// The collection of macros to define globally across the whole target (for all of its modules).
        /// </summary>
        public List<string> GlobalDefinitions = new List<string>();

        /// <summary>
        /// The collection of the modules to be compiled into the target (module names).
        /// </summary>
        public List<string> Modules = new List<string>();

        /// <summary>
        /// The resource file for Win32 platforms to be included into the output executable file (can be used to customize app icon, description and file copyright note).
        /// </summary>
        public string Win32ResourceFile;

        /// <summary>
        /// the custom project file path (disables project file generation for this target).
        /// </summary>
        public string CustomExternalProjectFilePath;

        /// <summary>
        /// Initializes a new instance of the <see cref="Target"/> class.
        /// </summary>
        public Target()
        {
            var type = GetType();
            Name = type.Name;
            ProjectName = Name;
            OutputName = Name;
        }

        /// <summary>
        /// Initializes the target properties.
        /// </summary>
        public virtual void Init()
        {
            GlobalDefinitions.Add("UNICODE");
            GlobalDefinitions.Add("_UNICODE");
        }

        /// <summary>
        /// Gets the supported architectures for the given platform.
        /// </summary>
        /// <param name="platform">The platform.</param>
        /// <returns>The target architectures collection.</returns>
        public virtual TargetArchitecture[] GetArchitectures(TargetPlatform platform)
        {
            return Architectures;
        }

        /// <summary>
        /// Selects the referenced target to build from the list of possible targets (eg. editor targets pick editor targets).
        /// </summary>
        /// <param name="project">The project to pick the target from.</param>
        /// <param name="projectTargets">The targets declared within the project.</param>
        /// <returns>The target to reference.</returns>
        public virtual Target SelectReferencedTarget(ProjectInfo project, Target[] projectTargets)
        {
            for (int i = 0; i < projectTargets.Length; i++)
            {
                if (projectTargets[i].IsEditor == IsEditor)
                    return projectTargets[i];
            }
            return projectTargets.Length != 0 ? projectTargets[0] : null;
        }

        /// <summary>
        /// Gets the output file path.
        /// </summary>
        /// <param name="options">The build options.</param>
        /// <param name="outputType">The custom output type (for default one override).</param>
        /// <returns>The output file path.</returns>
        public virtual string GetOutputFilePath(BuildOptions options, TargetOutputType? outputType = null)
        {
            LinkerOutput linkerOutput;
            switch (outputType ?? OutputType)
            {
            case TargetOutputType.Executable:
                linkerOutput = LinkerOutput.Executable;
                break;
            case TargetOutputType.Library:
                linkerOutput = LinkerOutput.SharedLibrary;
                break;
            default: throw new ArgumentOutOfRangeException();
            }
            return Path.Combine(options.OutputFolder, options.Platform.GetLinkOutputFileName(OutputName + options.HotReloadPostfix, linkerOutput));
        }

        /// <summary>
        /// Setups the target building environment (native C++). Allows to modify compiler and linker options. Options applied here are used by all modules included into this target (can be overridden per module).
        /// </summary>
        /// <param name="options">The build options.</param>
        public virtual void SetupTargetEnvironment(BuildOptions options)
        {
            options.CompileEnv.PreprocessorDefinitions.AddRange(GlobalDefinitions);
            options.LinkEnv.Output = OutputType == TargetOutputType.Executable ? LinkerOutput.Executable : LinkerOutput.SharedLibrary;

            if (!options.Platform.HasModularBuildSupport)
            {
                // Building target into single executable (forced by platform)
                UseSymbolsExports = false;
                LinkType = TargetLinkType.Monolithic;
                OutputType = TargetOutputType.Executable;
                options.LinkEnv.Output = LinkerOutput.Executable;
                Modules.Add("Main");
            }

            options.CompileEnv.EnableExceptions = true; // TODO: try to disable this!
            options.CompileEnv.Sanitizers = Configuration.Sanitizers;
            switch (options.Configuration)
            {
            case TargetConfiguration.Debug:
                options.CompileEnv.PreprocessorDefinitions.Add("BUILD_DEBUG");
                options.CompileEnv.FunctionLevelLinking = false;
                options.CompileEnv.Optimization = false;
                options.CompileEnv.FavorSizeOrSpeed = FavorSizeOrSpeed.Neither;
                options.CompileEnv.DebugInformation = true;
                options.CompileEnv.RuntimeChecks = true;
                options.CompileEnv.StringPooling = false;
                options.CompileEnv.IntrinsicFunctions = true;
                options.CompileEnv.BufferSecurityCheck = true;
                options.CompileEnv.Inlining = false;
                options.CompileEnv.WholeProgramOptimization = false;

                options.LinkEnv.DebugInformation = true;
                options.LinkEnv.LinkTimeCodeGeneration = false;
                options.LinkEnv.UseIncrementalLinking = true;
                options.LinkEnv.Optimization = false;
                break;
            case TargetConfiguration.Development:
                options.CompileEnv.PreprocessorDefinitions.Add("BUILD_DEVELOPMENT");
                options.CompileEnv.FunctionLevelLinking = true;
                options.CompileEnv.Optimization = true;
                options.CompileEnv.FavorSizeOrSpeed = FavorSizeOrSpeed.FastCode;
                options.CompileEnv.DebugInformation = true;
                options.CompileEnv.RuntimeChecks = false;
                options.CompileEnv.StringPooling = true;
                options.CompileEnv.IntrinsicFunctions = true;
                options.CompileEnv.BufferSecurityCheck = true;
                options.CompileEnv.Inlining = true;
                options.CompileEnv.WholeProgramOptimization = false;

                options.LinkEnv.DebugInformation = true;
                options.LinkEnv.LinkTimeCodeGeneration = false;
                options.LinkEnv.UseIncrementalLinking = true;
                options.LinkEnv.Optimization = true;
                break;
            case TargetConfiguration.Release:
                options.CompileEnv.PreprocessorDefinitions.Add("BUILD_RELEASE");
                options.CompileEnv.FunctionLevelLinking = true;
                options.CompileEnv.Optimization = true;
                options.CompileEnv.FavorSizeOrSpeed = FavorSizeOrSpeed.FastCode;
                options.CompileEnv.DebugInformation = false;
                options.CompileEnv.RuntimeChecks = false;
                options.CompileEnv.StringPooling = true;
                options.CompileEnv.IntrinsicFunctions = true;
                options.CompileEnv.BufferSecurityCheck = false;
                options.CompileEnv.Inlining = true;
                options.CompileEnv.WholeProgramOptimization = true;

                options.LinkEnv.DebugInformation = false;
                options.LinkEnv.LinkTimeCodeGeneration = true;
                options.LinkEnv.UseIncrementalLinking = false;
                options.LinkEnv.Optimization = true;
                break;
            default: throw new ArgumentOutOfRangeException();
            }

            if (options.CompileEnv.UseDebugCRT)
                options.CompileEnv.PreprocessorDefinitions.Add("_DEBUG");
            else
                options.CompileEnv.PreprocessorDefinitions.Add("NDEBUG");
        }

        /// <summary>
        /// Called before building this target.
        /// </summary>
        public virtual void PreBuild()
        {
        }

        /// <summary>
        /// Called before building this target with a given build options. Can be used to inject custom commands into the task graph.
        /// </summary>
        /// <param name="graph">The task graph.</param>
        /// <param name="buildOptions">The current build options.</param>
        public virtual void PreBuild(TaskGraph graph, BuildOptions buildOptions)
        {
        }

        /// <summary>
        /// Called after building this target with a given build options. Can be used to inject custom commands into the task graph.
        /// </summary>
        /// <param name="graph">The task graph.</param>
        /// <param name="buildOptions">The current build options.</param>
        public virtual void PostBuild(TaskGraph graph, BuildOptions buildOptions)
        {
        }

        /// <summary>
        /// Called after building this target.
        /// </summary>
        public virtual void PostBuild()
        {
        }
    }
}
