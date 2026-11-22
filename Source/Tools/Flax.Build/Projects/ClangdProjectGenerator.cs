// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using Flax.Build.NativeCpp;

namespace Flax.Build.Projects
{
    /// <summary>
    /// Project generator for the clangd language server.
    /// </summary>
    public class ClangdProjectGenerator : ProjectGenerator
    {
        private struct CompileCommand
        {
            public string File;
            public string WorkingDirectory;
            public List<string> Arguments;
        }

        /// <inheritdoc />
        public override string ProjectFileExtension => "json";

        /// <inheritdoc />
        public override string SolutionFileExtension => "json";

        /// <inheritdoc />
        public override TargetType? Type => null;

        /// <inheritdoc />
        public override void GenerateProject(Project project, string solutionPath, bool isMainProject)
        {
        }

        /// <inheritdoc />
        public override void GenerateSolution(Solution solution)
        {
            var commands = GetCompileCommands(solution);
            if (!commands.Any())
                return;

            var path = Path.Combine(solution.WorkspaceRootPath, "compile_commands.json");

            var contents = new StringBuilder();
            contents.AppendLine("[");
            for (var i = 0; i < commands.Length; i++)
            {
                var command = commands[i];
                contents.AppendLine("  {");
                contents.AppendLine($"    \"file\": {Utilities.ToJsonString(command.File)},");
                contents.AppendLine($"    \"directory\": {Utilities.ToJsonString(command.WorkingDirectory)},");
                contents.AppendLine($"    \"arguments\": [ {string.Join(',',  command.Arguments.Select(Utilities.ToJsonString))} ]");
                contents.AppendLine(i == commands.Length - 1 ? "  }" : "  },");
            }
            contents.AppendLine("]");

            Utilities.WriteFileIfChanged(path, contents.ToString());
        }

        private CompileCommand[] GetCompileCommands(Solution solution)
        {
            var rules = Builder.GenerateRulesAssembly();
            var platform = Platform.BuildPlatform;
            var architecture = Platform.BuildTargetArchitecture;
            var configuration = TargetConfiguration.Development; // No way to control build configuration, use Development as default
            var result = new Dictionary<string, CompileCommand>(StringComparer.OrdinalIgnoreCase);
            var mainProject = solution.MainProject ?? solution.Projects.FirstOrDefault(x => x.Name == Globals.Project.Name);

            if (!platform.HasRequiredSDKsInstalled)
                return [];

            var projects = solution.Projects.Where(x => x.Type == TargetType.NativeCpp).OrderBy(x => x == mainProject ? 0 : 1).ToArray();
            foreach (var project in projects)
            {
                foreach (var target in project.Targets)
                {
                    if (!target.Platforms.Contains(platform.Target) ||
                        !target.GetArchitectures(platform.Target).Contains(architecture) ||
                        !target.Configurations.Contains(configuration))
                    {
                        continue;
                    }

                    var toolchain = platform.TryGetToolchain(architecture);
                    if (toolchain == null)
                        continue;

                    var targetBuildOptions = Builder.GetBuildOptions(target, platform, toolchain, architecture, configuration, project.WorkspaceRootPath);
                    targetBuildOptions.Flags |= BuildFlags.GenerateProject;
                    var modules = Builder.CollectModules(rules, platform, target, targetBuildOptions, toolchain, architecture, configuration);
                    var targetDefinitions = new HashSet<string>(targetBuildOptions.CompileEnv.PreprocessorDefinitions);
                    targetDefinitions.AddRange(project.Defines);

                    // Generate compile commands for every single source file in the solution
                    foreach (var module in modules)
                    {
                        module.Key.SetupEnvironment(module.Value);
                        var definitions = new HashSet<string>(module.Value.CompileEnv.PreprocessorDefinitions);
                        definitions.AddRange(project.Defines);
                        AddCompileCommands(result, solution.WorkspaceRootPath, module.Value, module.Value.SourceFiles, definitions);
                    }
                    AddCompileCommands(result, solution.WorkspaceRootPath, targetBuildOptions, targetBuildOptions.SourceFiles, targetDefinitions);
                }
            }

            return result.Values.OrderBy(x => x.File).ToArray();
        }

        private void AddCompileCommands(Dictionary<string, CompileCommand> result, string workspaceRoot, BuildOptions options, List<string> sourceFiles, HashSet<string> definitions)
        {
            if (sourceFiles == null)
                return;

            foreach (var sourceFile in sourceFiles)
            {
                var file = Utilities.NormalizePath(sourceFile);
                if (result.ContainsKey(file))
                    continue;
                var extension = Path.GetExtension(file).ToLowerInvariant();
                if (extension is not (".c" or ".cc" or ".cpp" or ".cxx"))
                    continue;

                var arguments = new List<string>();
                var isC = extension == ".c";
                arguments.Add(isC ? "clang" : "clang++");
                switch (options.CompileEnv.CppVersion)
                {
                case CppVersion.Cpp14:
                    arguments.Add(isC ? "-std=c11" : "-std=c++14");
                    break;
                case CppVersion.Cpp17:
                    arguments.Add(isC ? "-std=c17" : "-std=c++17");
                    break;
                case CppVersion.Cpp20:
                default:
                    arguments.Add(isC ? "-std=c17" : "-std=c++20");
                    break;
                }

                // Use stable ordering to not needlessly rewrite the file again
                foreach (var definition in definitions.OrderBy(x => x))
                {
                    if (!string.IsNullOrEmpty(definition))
                        arguments.Add("-D" + definition);
                }
                foreach (var includePath in options.CompileEnv.IncludePaths.Select(Utilities.NormalizePath).Distinct())
                    arguments.Add("-I" + includePath);

                result.Add(file, new CompileCommand
                {
                    File = file,
                    WorkingDirectory = Utilities.NormalizePath(workspaceRoot),
                    Arguments = arguments,
                });
            }
        }
    }
}
