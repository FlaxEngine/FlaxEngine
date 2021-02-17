// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using Flax.Build.NativeCpp;

namespace Flax.Build
{
    partial class Builder
    {
        private static RulesAssembly _rules;
        private static Type[] _buildTypes;

        /// <summary>
        /// The build configuration files postfix.
        /// </summary>
        public static string BuildFilesPostfix = ".Build.cs";

        /// <summary>
        /// The cached list of types from Flax.Build assembly. Reused by other build tool utilities to improve performance.
        /// </summary>
        internal static Type[] BuildTypes
        {
            get
            {
                if (_buildTypes == null)
                {
                    using (new ProfileEventScope("CacheBuildTypes"))
                    {
                        _buildTypes = typeof(Program).Assembly.GetTypes();
                    }
                }
                return _buildTypes;
            }
        }

        /// <summary>
        /// The rules assembly data.
        /// </summary>
        public class RulesAssembly
        {
            private readonly Dictionary<string, Module> _modulesLookup;

            /// <summary>
            /// The rules assembly.
            /// </summary>
            public readonly Assembly Assembly;

            /// <summary>
            /// The targets objects.
            /// </summary>
            public readonly Target[] Targets;

            /// <summary>
            /// The modules objects.
            /// </summary>
            public readonly Module[] Modules;

            /// <summary>
            /// The plugin objects.
            /// </summary>
            public readonly Plugin[] Plugins;

            internal RulesAssembly(Assembly assembly, Target[] targets, Module[] modules, Plugin[] plugins)
            {
                Assembly = assembly;
                Targets = targets;
                Modules = modules;
                Plugins = plugins;
                _modulesLookup = new Dictionary<string, Module>(modules.Length);
                for (int i = 0; i < modules.Length; i++)
                {
                    var module = modules[i];
                    _modulesLookup.Add(module.Name, module);
                }
            }

            /// <summary>
            /// Gets the target of the given name.
            /// </summary>
            /// <param name="name">The name.</param>
            /// <returns>The target or null if not found.</returns>
            public Target GetTarget(string name)
            {
                return Targets.FirstOrDefault(x => string.Equals(x.Name, name, StringComparison.OrdinalIgnoreCase));
            }

            /// <summary>
            /// Gets the module of the given name.
            /// </summary>
            /// <param name="name">The name.</param>
            /// <returns>The module or null if not found.</returns>
            public Module GetModule(string name)
            {
                if (!_modulesLookup.TryGetValue(name, out var module))
                    module = Modules.FirstOrDefault(x => string.Equals(x.Name, name, StringComparison.OrdinalIgnoreCase));
                return module;
            }
        }

        /// <summary>
        /// Gets the build options for the given target and the configuration.
        /// </summary>
        /// <param name="target">The target.</param>
        /// <param name="platform">The platform.</param>
        /// <param name="toolchain">The toolchain.</param>
        /// <param name="architecture">The build architecture.</param>
        /// <param name="configuration">The build configuration.</param>
        /// <param name="workingDirectory">The build workspace root folder path.</param>
        /// <param name="hotReloadPostfix">The output binaries postfix added for hot-reload builds in Editor to prevent file names collisions.</param>
        /// <returns>The build options.</returns>
        public static BuildOptions GetBuildOptions(Target target, Platform platform, Toolchain toolchain, TargetArchitecture architecture, TargetConfiguration configuration, string workingDirectory, string hotReloadPostfix = "")
        {
            var platformName = platform.Target.ToString();
            var architectureName = architecture.ToString();
            var configurationName = configuration.ToString();
            var options = new BuildOptions
            {
                Target = target,
                Platform = platform,
                Toolchain = toolchain,
                Architecture = architecture,
                Configuration = configuration,
                CompileEnv = new CompileEnvironment(),
                LinkEnv = new LinkEnvironment(),
                IntermediateFolder = Path.Combine(workingDirectory, Configuration.IntermediateFolder, target.Name, platformName, architectureName, configurationName),
                OutputFolder = Path.Combine(workingDirectory, Configuration.BinariesFolder, target.Name, platformName, architectureName, configurationName),
                WorkingDirectory = workingDirectory,
                HotReloadPostfix = hotReloadPostfix,
            };
            toolchain?.SetupEnvironment(options);
            target.SetupTargetEnvironment(options);
            return options;
        }

        /// <summary>
        /// Generates the rules assembly (from Module and Target files in the workspace directory).
        /// </summary>
        /// <returns>The compiled rules assembly.</returns>
        public static RulesAssembly GenerateRulesAssembly()
        {
            if (_rules != null)
                return _rules;

            using (new ProfileEventScope("InitInBuildPlugins"))
            {
                foreach (var type in BuildTypes.Where(x => x.IsClass && !x.IsAbstract && x.IsSubclassOf(typeof(Plugin))))
                {
                    var plugin = (Plugin)Activator.CreateInstance(type);
                    plugin.Init();
                }
            }

            using (new ProfileEventScope("GenerateRulesAssembly"))
            {
                // Find source files
                var files = new List<string>();
                using (new ProfileEventScope("FindRules"))
                {
                    // Use rules from any of the included projects workspace
                    if (Globals.Project != null)
                    {
                        var projects = Globals.Project.GetAllProjects();
                        foreach (var project in projects)
                        {
                            var sourceFolder = Path.Combine(project.ProjectFolderPath, "Source");
                            if (Directory.Exists(sourceFolder))
                                FindRules(sourceFolder, files);
                        }
                    }
                }

                // Log info
                if (Configuration.Verbose)
                {
                    Log.Verbose("Build files:");
                    using (new LogIndentScope())
                    {
                        foreach (var e in files)
                            Log.Verbose(e);
                    }
                }

                // Compile code
                Assembly assembly;
                using (new ProfileEventScope("CompileRules"))
                {
                    var assembler = new Assembler();
                    assembler.SourceFiles.AddRange(files);
                    assembly = assembler.Build();
                }

                // Prepare targets and modules objects
                Type[] types;
                var targetObjects = new List<Target>(16);
                var moduleObjects = new List<Module>(256);
                var pluginObjects = new List<Plugin>();
                using (new ProfileEventScope("GetTypes"))
                {
                    types = assembly.GetTypes();
                    for (var i = 0; i < types.Length; i++)
                    {
                        var type = types[i];
                        if (!type.IsClass || type.IsAbstract)
                            continue;
                        if (type.IsSubclassOf(typeof(Target)))
                        {
                            var target = (Target)Activator.CreateInstance(type);

                            var targetFilename = target.Name + BuildFilesPostfix;
                            target.FilePath = files.FirstOrDefault(path => string.Equals(Path.GetFileName(path), targetFilename, StringComparison.OrdinalIgnoreCase));
                            if (target.FilePath == null)
                            {
                                targetFilename = target.Name + "Target" + BuildFilesPostfix;
                                target.FilePath = files.FirstOrDefault(path => string.Equals(Path.GetFileName(path), targetFilename, StringComparison.OrdinalIgnoreCase));
                                if (target.FilePath == null)
                                {
                                    if (target.Name.EndsWith("Target"))
                                    {
                                        targetFilename = target.Name.Substring(0, target.Name.Length - "Target".Length) + BuildFilesPostfix;
                                        target.FilePath = files.FirstOrDefault(path => string.Equals(Path.GetFileName(path), targetFilename, StringComparison.OrdinalIgnoreCase));
                                    }
                                    if (target.FilePath == null)
                                    {
                                        throw new Exception(string.Format("Failed to find source file path for {0}", target));
                                    }
                                }
                            }
                            target.FolderPath = Path.GetDirectoryName(target.FilePath);
                            target.Init();
                            targetObjects.Add(target);
                        }
                        else if (type.IsSubclassOf(typeof(Module)))
                        {
                            var module = (Module)Activator.CreateInstance(type);

                            var moduleFilename = module.Name + BuildFilesPostfix;
                            module.FilePath = files.FirstOrDefault(path => string.Equals(Path.GetFileName(path), moduleFilename, StringComparison.OrdinalIgnoreCase));
                            if (module.FilePath == null)
                            {
                                moduleFilename = module.Name + "Module" + BuildFilesPostfix;
                                module.FilePath = files.FirstOrDefault(path => string.Equals(Path.GetFileName(path), moduleFilename, StringComparison.OrdinalIgnoreCase));
                                if (module.FilePath == null)
                                {
                                    throw new Exception(string.Format("Failed to find source file path for {0}", module));
                                }
                            }
                            module.FolderPath = Path.GetDirectoryName(module.FilePath);
                            module.Init();
                            moduleObjects.Add(module);
                        }
                        else if (type.IsSubclassOf(typeof(Plugin)))
                        {
                            var plugin = (Plugin)Activator.CreateInstance(type);

                            plugin.Init();
                            pluginObjects.Add(plugin);
                        }
                    }
                }

                _rules = new RulesAssembly(assembly, targetObjects.ToArray(), moduleObjects.ToArray(), pluginObjects.ToArray());
            }

            return _rules;
        }

        private static void FindRules(string directory, List<string> result)
        {
            // Optional way:
            //result.AddRange(Directory.GetFiles(directory, '*' + BuildFilesPostfix, SearchOption.AllDirectories));

            var files = Directory.GetFiles(directory);
            for (int i = 0; i < files.Length; i++)
            {
                var file = files[i];
                if (file.EndsWith(BuildFilesPostfix, StringComparison.OrdinalIgnoreCase))
                {
                    result.Add(file);
                }
            }
            var directories = Directory.GetDirectories(directory);
            for (int i = 0; i < directories.Length; i++)
            {
                FindRules(directories[i], result);
            }
        }
    }
}
