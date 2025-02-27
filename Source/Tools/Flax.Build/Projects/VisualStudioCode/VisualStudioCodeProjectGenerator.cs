// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace Flax.Build.Projects.VisualStudioCode
{
    /// <summary>
    /// Project generator for Visual Studio Code.
    /// </summary>
    public class VisualStudioCodeProjectGenerator : ProjectGenerator
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="VisualStudioCodeProjectGenerator"/> class.
        /// </summary>
        public VisualStudioCodeProjectGenerator()
        {
        }

        /// <inheritdoc />
        public override string ProjectFileExtension => string.Empty;

        /// <inheritdoc />
        public override string SolutionFileExtension => string.Empty;

        /// <inheritdoc />
        public override TargetType? Type => null;

        /// <inheritdoc />
        public override Project CreateProject()
        {
            return new Project
            {
                Generator = this,
            };
        }

        /// <inheritdoc />
        public override void GenerateProject(Project project, string solutionPath, bool isMainProject)
        {
            // Not used, solution contains all projects definitions
        }

        private class JsonWriter : IDisposable
        {
            private readonly List<string> _lines = new List<string>();
            private string _tabs = "";

            public void BeginRootObject()
            {
                BeginObject();
            }

            public void EndRootObject()
            {
                EndObject();
                if (_tabs.Length > 0)
                {
                    throw new Exception("Invalid EndRootObject call before all objects and arrays have been closed.");
                }
            }

            public void BeginObject(string name = null)
            {
                string prefix = name == null ? "" : Quoted(name) + ": ";
                _lines.Add(_tabs + prefix + "{");
                _tabs += "\t";
            }

            public void EndObject()
            {
                _lines[_lines.Count - 1] = _lines[_lines.Count - 1].TrimEnd(',');
                _tabs = _tabs.Remove(_tabs.Length - 1);
                _lines.Add(_tabs + "},");
            }

            public void BeginArray(string name = null)
            {
                string prefix = name == null ? "" : Quoted(name) + ": ";
                _lines.Add(_tabs + prefix + "[");
                _tabs += "\t";
            }

            public void EndArray()
            {
                _lines[_lines.Count - 1] = _lines[_lines.Count - 1].TrimEnd(',');
                _tabs = _tabs.Remove(_tabs.Length - 1);
                _lines.Add(_tabs + "],");
            }

            public void AddField(string name, bool value)
            {
                _lines.Add(_tabs + Quoted(name) + ": " + value.ToString().ToLower() + ",");
            }

            public void AddField(string name, int value)
            {
                _lines.Add(_tabs + Quoted(name) + ": " + value + ",");
            }

            public void AddField(string name, string value)
            {
                _lines.Add(_tabs + Quoted(name) + ": " + Quoted(value) + ",");
            }

            public void AddUnnamedField(string value)
            {
                _lines.Add(_tabs + Quoted(value) + ",");
            }

            private string Quoted(string value)
            {
                value = value.Replace("\\", "\\\\");
                value = value.Replace("\"", "\\\"");
                return "\"" + value + "\"";
            }

            public void Save(string path)
            {
                _lines[_lines.Count - 1] = _lines[_lines.Count - 1].TrimEnd(',');
                Utilities.WriteFileIfChanged(path, string.Join(Environment.NewLine, _lines.ToArray()));
            }

            /// <inheritdoc />
            public void Dispose()
            {
                _lines.Clear();
                _tabs = null;
            }
        }

        /// <inheritdoc />
        public override void GenerateSolution(Solution solution)
        {
            // Setup directory for Visual Studio Code files
            var vsCodeFolder = Path.Combine(solution.WorkspaceRootPath, ".vscode");
            if (!Directory.Exists(vsCodeFolder))
                Directory.CreateDirectory(vsCodeFolder);

            // Prepare
            var buildToolWorkspace = Environment.CurrentDirectory;
            var buildToolPath = Path.ChangeExtension(Utilities.MakePathRelativeTo(typeof(Builder).Assembly.Location, solution.WorkspaceRootPath), null);
            var rules = Builder.GenerateRulesAssembly();
            var mainProject = solution.MainProject ?? solution.Projects.FirstOrDefault(x => x.Name == Globals.Project.Name);

            // Create tasks file
            using (var json = new JsonWriter())
            {
                json.BeginRootObject();
                {
                    json.AddField("version", "2.0.0");

                    json.BeginArray("tasks");
                    {
                        foreach (var project in solution.Projects)
                        {
                            if (project.Name == "BuildScripts")
                                continue;

                            // Skip duplicate build tasks
                            if (project.Name == "FlaxEngine" || (mainProject != null && mainProject.Name != "Flax" && mainProject != project))
                                continue;

                            bool defaultTask = project == mainProject;
                            foreach (var configuration in project.Configurations)
                            {
                                var target = configuration.Target;
                                var name = solution.Name + '|' + configuration.Name;

                                json.BeginObject();

                                json.AddField("label", name);

                                bool isDefaultTask = defaultTask && configuration.Configuration == TargetConfiguration.Development && configuration.Platform == Platform.BuildPlatform.Target && configuration.Architecture == Platform.BuildTargetArchitecture;

                                json.BeginObject("group");
                                {
                                    json.AddField("kind", "build");
                                    json.AddField("isDefault", isDefaultTask);
                                }
                                json.EndObject();

                                if (isDefaultTask)
                                    defaultTask = false;

                                switch (Platform.BuildPlatform.Target)
                                {
                                case TargetPlatform.Windows:
                                {
                                    json.AddField("command", buildToolPath);
                                    json.BeginArray("args");
                                    {
                                        json.AddUnnamedField("-build");
                                        json.AddUnnamedField("-log");
                                        json.AddUnnamedField("-mutex");
                                        json.AddUnnamedField(string.Format("\\\"-workspace={0}\\\"", buildToolWorkspace));
                                        json.AddUnnamedField(string.Format("-arch={0}", configuration.ArchitectureName));
                                        json.AddUnnamedField(string.Format("-configuration={0}", configuration.ConfigurationName));
                                        json.AddUnnamedField(string.Format("-platform={0}", configuration.PlatformName));
                                        json.AddUnnamedField(string.Format("-buildTargets={0}", target.Name));
                                        if (!string.IsNullOrEmpty(Configuration.Compiler))
                                            json.AddUnnamedField(string.Format("-compiler={0}", Configuration.Compiler));
                                        if (!string.IsNullOrEmpty(Configuration.Dotnet))
                                            json.AddUnnamedField(string.Format("-dotnet={0}", Configuration.Dotnet));
                                    }
                                    json.EndArray();

                                    json.AddField("type", "shell");

                                    json.BeginObject("options");
                                    {
                                        json.AddField("cwd", buildToolWorkspace);
                                    }
                                    json.EndObject();
                                    break;
                                }
                                case TargetPlatform.Linux:
                                {
                                    json.AddField("command", buildToolPath);
                                    json.BeginArray("args");
                                    {
                                        json.AddUnnamedField("--build");
                                        json.AddUnnamedField("--log");
                                        json.AddUnnamedField("--mutex");
                                        json.AddUnnamedField(string.Format("--workspace=\\\"{0}\\\"", buildToolWorkspace));
                                        json.AddUnnamedField(string.Format("--arch={0}", configuration.Architecture));
                                        json.AddUnnamedField(string.Format("--configuration={0}", configuration.ConfigurationName));
                                        json.AddUnnamedField(string.Format("--platform={0}", configuration.PlatformName));
                                        json.AddUnnamedField(string.Format("--buildTargets={0}", target.Name));
                                        if (!string.IsNullOrEmpty(Configuration.Compiler))
                                            json.AddUnnamedField(string.Format("--compiler={0}", Configuration.Compiler));
                                        if (!string.IsNullOrEmpty(Configuration.Dotnet))
                                            json.AddUnnamedField(string.Format("-dotnet={0}", Configuration.Dotnet));
                                    }
                                    json.EndArray();

                                    json.AddField("type", "shell");

                                    json.BeginObject("options");
                                    {
                                        json.AddField("cwd", buildToolWorkspace);
                                    }
                                    json.EndObject();
                                    break;
                                }
                                case TargetPlatform.Mac:
                                {
                                    json.AddField("command", buildToolPath);
                                    json.BeginArray("args");
                                    {
                                        json.AddUnnamedField("--build");
                                        json.AddUnnamedField("--log");
                                        json.AddUnnamedField("--mutex");
                                        json.AddUnnamedField(string.Format("--workspace=\\\"{0}\\\"", buildToolWorkspace));
                                        json.AddUnnamedField(string.Format("--arch={0}", configuration.Architecture));
                                        json.AddUnnamedField(string.Format("--configuration={0}", configuration.ConfigurationName));
                                        json.AddUnnamedField(string.Format("--platform={0}", configuration.PlatformName));
                                        json.AddUnnamedField(string.Format("--buildTargets={0}", target.Name));
                                        if (!string.IsNullOrEmpty(Configuration.Compiler))
                                            json.AddUnnamedField(string.Format("--compiler={0}", Configuration.Compiler));
                                        if (!string.IsNullOrEmpty(Configuration.Dotnet))
                                            json.AddUnnamedField(string.Format("-dotnet={0}", Configuration.Dotnet));
                                    }
                                    json.EndArray();

                                    json.AddField("type", "shell");

                                    json.BeginObject("options");
                                    {
                                        json.AddField("cwd", buildToolWorkspace);
                                    }
                                    json.EndObject();
                                    break;
                                }
                                default: throw new Exception("Visual Code project generator does not support current platform.");
                                }

                                json.EndObject();
                            }
                        }
                    }
                    json.EndArray();
                }
                json.EndRootObject();

                json.Save(Path.Combine(vsCodeFolder, "tasks.json"));
            }

            bool hasNativeProjects = solution.Projects.Any(x => x.Type == TargetType.NativeCpp);
            bool hasMonoProjects = solution.Projects.Any(x => x.Type == TargetType.DotNet);
            bool hasDotnetProjects = solution.Projects.Any(x => x.Type == TargetType.DotNetCore);

            // Create launch file
            using (var json = new JsonWriter())
            {
                json.BeginRootObject();
                {
                    json.AddField("version", "0.2.0");

                    json.BeginArray("configurations");
                    {
                        var cppProject = solution.Projects.FirstOrDefault(x => x.BaseName == solution.Name || x.Name == solution.Name);
                        var mainProjectModule = mainProject != null && mainProject.Targets?.Length != 0 ? mainProject.Targets[0]?.Modules[0] : null;
                        var csharpProject = mainProjectModule != null ? solution.Projects.FirstOrDefault(x => x.BaseName == mainProjectModule || x.Name == mainProjectModule) : null;

                        if (cppProject != null)
                        {
                            // C++ project
                            foreach (var configuration in cppProject.Configurations)
                            {
                                var name = solution.Name + '|' + configuration.Name + " (C++)";
                                var target = configuration.Target;
                                var outputType = cppProject.OutputType ?? target.OutputType;
                                var outputTargetFilePath = target.GetOutputFilePath(configuration.TargetBuildOptions, TargetOutputType.Executable);

                                json.BeginObject();
                                {
                                    if (configuration.Platform == TargetPlatform.Windows)
                                        json.AddField("type", "cppvsdbg");
                                    else
                                        json.AddField("type", "cppdbg");
                                    json.AddField("name", name);
                                    json.AddField("request", "launch");
                                    json.AddField("preLaunchTask", solution.Name + '|' + configuration.Name);
                                    json.AddField("cwd", buildToolWorkspace);

                                    WriteNativePlatformLaunchSettings(json, configuration.Platform);

                                    if (outputType != TargetOutputType.Executable && configuration.Name.StartsWith("Editor."))
                                    {
                                        if (configuration.Platform == TargetPlatform.Windows)
                                        {
                                            var editorFolder = configuration.Architecture == TargetArchitecture.x64 ? "Win64" : "Win32";
                                            json.AddField("program", Path.Combine(Globals.EngineRoot, "Binaries", "Editor", editorFolder, configuration.ConfigurationName, "FlaxEditor.exe"));
                                        }
                                        else if (configuration.Platform == TargetPlatform.Linux)
                                            json.AddField("program", Path.Combine(Globals.EngineRoot, "Binaries", "Editor", "Linux", configuration.ConfigurationName, "FlaxEditor"));
                                        else if (configuration.Platform == TargetPlatform.Mac)
                                            json.AddField("program", Path.Combine(Globals.EngineRoot, "Binaries", "Editor", "Mac", configuration.ConfigurationName, "FlaxEditor"));

                                        json.BeginArray("args");
                                        {
                                            json.AddUnnamedField("-project");
                                            json.AddUnnamedField(buildToolWorkspace);
                                            json.AddUnnamedField("-skipCompile");
                                            if (hasMonoProjects)
                                            {
                                                json.AddUnnamedField("-debug");
                                                json.AddUnnamedField("127.0.0.1:55555");
                                            }
                                        }
                                        json.EndArray();
                                    }
                                    else
                                    {
                                        json.AddField("program", outputTargetFilePath);
                                        json.BeginArray("args");
                                        {
                                            if (configuration.Platform == TargetPlatform.Linux || configuration.Platform == TargetPlatform.Mac)
                                                json.AddUnnamedField("--std");
                                            if (hasMonoProjects)
                                            {
                                                json.AddUnnamedField("-debug");
                                                json.AddUnnamedField("127.0.0.1:55555");
                                            }
                                        }
                                        json.EndArray();
                                    }

                                    json.AddField("visualizerFile", Path.Combine(Globals.EngineRoot, "Source", "flax.natvis"));
                                }
                                json.EndObject();
                            }
                        }
                        if (csharpProject != null)
                        {
                            // C# project
                            foreach (var configuration in csharpProject.Configurations)
                            {
                                var name = solution.Name + '|' + configuration.Name + " (C#)";
                                var outputTargetFilePath = configuration.Target.GetOutputFilePath(configuration.TargetBuildOptions, TargetOutputType.Executable);

                                json.BeginObject();
                                {
                                    json.AddField("type", "coreclr");
                                    json.AddField("name", name);
                                    json.AddField("request", "launch");
                                    json.AddField("preLaunchTask", solution.Name + '|' + configuration.Name);
                                    json.AddField("cwd", buildToolWorkspace);
                                    if (configuration.Platform == Platform.BuildPlatform.Target)
                                    {
                                        var editorFolder = configuration.Platform == TargetPlatform.Windows ? (configuration.Architecture == TargetArchitecture.x64 ? "Win64" : "Win32") : configuration.Platform.ToString();
                                        json.AddField("program", Path.Combine(Globals.EngineRoot, "Binaries", "Editor", editorFolder, configuration.ConfigurationName, "FlaxEditor"));
                                        json.BeginArray("args");
                                        {
                                            json.AddUnnamedField("-project");
                                            json.AddUnnamedField(buildToolWorkspace);
                                            json.AddUnnamedField("-skipCompile");
                                        }
                                        json.EndArray();
                                    }
                                    else
                                    {
                                        json.AddField("program", configuration.Target.GetOutputFilePath(configuration.TargetBuildOptions, TargetOutputType.Executable));
                                    }

                                    switch (configuration.Platform)
                                    {
                                    case TargetPlatform.Windows:
                                        json.AddField("stopAtEntry", false);
                                        json.AddField("externalConsole", true);
                                        break;
                                    case TargetPlatform.Linux: break;
                                    }
                                }
                                json.EndObject();
                            }
                        }
                    }

                    if (hasNativeProjects)
                    {
                        foreach (var platform in solution.Projects.SelectMany(x => x.Configurations).Select(x => x.Platform).Distinct())
                        {
                            if (platform != TargetPlatform.Windows && platform != TargetPlatform.Linux && platform != TargetPlatform.Mac)
                                continue;

                            json.BeginObject();
                            {
                                if (platform == TargetPlatform.Windows)
                                    json.AddField("type", "cppvsdbg");
                                else
                                    json.AddField("type", "cppdbg");
                                json.AddField("name", solution.Name + " (Attach Editor)");
                                json.AddField("request", "attach");
                                json.AddField("processId", "${command:pickProcess}"); // Does not seem to be possible to attach by process name?

                                WriteNativePlatformLaunchSettings(json, platform);

                                json.AddField("visualizerFile", Path.Combine(Globals.EngineRoot, "Source", "flax.natvis"));
                            }
                            json.EndObject();
                        }
                    }
                    if (hasDotnetProjects)
                    {
                        json.BeginObject();
                        {
                            json.AddField("type", "coreclr");
                            json.AddField("name", solution.Name + " (C# Attach Editor)");
                            json.AddField("request", "attach");
                            json.AddField("processName", "FlaxEditor");
                        }
                        json.EndObject();
                    }
                    if (hasMonoProjects)
                    {
                        json.BeginObject();
                        {
                            json.AddField("type", "mono");
                            json.AddField("name", solution.Name + " (C# Attach)");
                            json.AddField("request", "attach");
                            json.AddField("address", "localhost");
                            json.AddField("port", 55555);
                        }
                        json.EndObject();
                    }

                    json.EndArray();
                }
                json.EndRootObject();

                json.Save(Path.Combine(vsCodeFolder, "launch.json"));
            }

            static void WriteNativePlatformLaunchSettings(JsonWriter json, TargetPlatform platform)
            {
                switch (Platform.BuildPlatform.Target)
                {
                case TargetPlatform.Linux:
                    if (platform == TargetPlatform.Linux)
                    {
                        json.AddField("MIMode", "gdb");
                        json.BeginArray("setupCommands");
                        {
                            json.BeginObject();
                            json.AddField("description", "Enable pretty-printing for gdb");
                            json.AddField("text", "-enable-pretty-printing");
                            json.AddField("ignoreFailures", true);
                            json.EndObject();

                            // Ignore signals used by C# runtime
                            json.BeginObject();
                            json.AddField("description", "ignore SIG34 signal");
                            json.AddField("text", "handle SIG34 nostop noprint pass");
                            json.EndObject();
                            json.BeginObject();
                            json.AddField("description", "ignore SIG35 signal");
                            json.AddField("text", "handle SIG35 nostop noprint pass");
                            json.EndObject();
                            json.BeginObject();
                            json.AddField("description", "ignore SIG36 signal");
                            json.AddField("text", "handle SIG36 nostop noprint pass");
                            json.EndObject();
                            json.BeginObject();
                            json.AddField("description", "ignore SIG357 signal");
                            json.AddField("text", "handle SIG37 nostop noprint pass");
                            json.EndObject();
                        }
                        json.EndArray();
                    }
                    break;
                case TargetPlatform.Mac:
                    if (platform == TargetPlatform.Mac)
                    {
                        json.AddField("MIMode", "lldb");
                    }
                    break;
                }
                switch (platform)
                {
                case TargetPlatform.Windows:
                    json.AddField("stopAtEntry", false);
                    json.AddField("externalConsole", true);
                    break;
                }
            }

            // Create C++ properties file
            using (var json = new JsonWriter())
            {
                json.BeginRootObject();
                json.BeginArray("configurations");
                json.BeginObject();
                if (mainProject != null)
                {
                    json.AddField("name", mainProject.Name);

                    var targetPlatform = Platform.BuildPlatform.Target;
                    var configuration = TargetConfiguration.Development;
                    var architecture = TargetArchitecture.x64;

                    var compilerPath = string.Empty;
                    var cppVersion = NativeCpp.CppVersion.Cpp14;
                    var includePaths = new HashSet<string>();
                    var preprocessorDefinitions = new HashSet<string>();
                    foreach (var e in mainProject.Defines)
                        preprocessorDefinitions.Add(e);

                    foreach (var target in mainProject.Targets)
                    {
                        var platform = Platform.GetPlatform(targetPlatform);
                        if (platform.HasRequiredSDKsInstalled && target.Platforms.Contains(targetPlatform))
                        {
                            var toolchain = platform.GetToolchain(architecture);
                            var targetBuildOptions = Builder.GetBuildOptions(target, platform, toolchain, architecture, configuration, Globals.Root);
                            targetBuildOptions.Flags |= NativeCpp.BuildFlags.GenerateProject;
                            var modules = Builder.CollectModules(rules, platform, target, targetBuildOptions, toolchain, architecture, configuration);
                            foreach (var module in modules)
                            {
                                // This merges private module build options into global target - not the best option but helps with syntax highlighting and references collecting
                                module.Key.Setup(targetBuildOptions);
                                module.Key.SetupEnvironment(targetBuildOptions);
                            }

                            cppVersion = targetBuildOptions.CompileEnv.CppVersion;
                            compilerPath = toolchain.NativeCompilerPath;
                            foreach (var e in targetBuildOptions.CompileEnv.PreprocessorDefinitions)
                                preprocessorDefinitions.Add(e);
                            foreach (var e in targetBuildOptions.CompileEnv.IncludePaths)
                                includePaths.Add(e);
                        }
                    }

                    if (compilerPath.Length != 0)
                        json.AddField("compilerPath", compilerPath);

                    switch (cppVersion)
                    {
                    case NativeCpp.CppVersion.Cpp14:
                        json.AddField("cStandard", "c11");
                        json.AddField("cppStandard", "c++14");
                        break;
                    case NativeCpp.CppVersion.Cpp17:
                    case NativeCpp.CppVersion.Latest:
                        json.AddField("cStandard", "c17");
                        json.AddField("cppStandard", "c++17");
                        break;
                    case NativeCpp.CppVersion.Cpp20:
                        json.AddField("cStandard", "c17");
                        json.AddField("cppStandard", "c++20");
                        break;
                    default:
                        throw new Exception($"Visual Code project generator does not support C++ standard {cppVersion}.");
                    }

                    json.BeginArray("includePath");
                    {
                        foreach (var path in includePaths)
                        {
                            json.AddUnnamedField(path.Replace('\\', '/'));
                        }
                    }
                    json.EndArray();

                    if (targetPlatform == TargetPlatform.Windows)
                    {
                        json.AddField("intelliSenseMode", "msvc-x64");
                    }
                    else
                    {
                        json.AddField("intelliSenseMode", "clang-x64");
                    }

                    json.BeginArray("defines");
                    {
                        foreach (string definition in preprocessorDefinitions)
                        {
                            json.AddUnnamedField(definition);
                        }
                    }
                    json.EndArray();
                }
                json.EndObject();
                json.EndArray();
                json.EndRootObject();

                json.Save(Path.Combine(vsCodeFolder, "c_cpp_properties.json"));
            }

            // Create settings file
            using (var json = new JsonWriter())
            {
                json.BeginRootObject();

                // File and folders excludes
                json.BeginObject("files.exclude");
                json.AddField("**/.git", true);
                json.AddField("**/.svn", true);
                json.AddField("**/.hg", true);
                json.AddField("**/.vs", true);
                json.AddField("**/Binaries", true);
                json.AddField("**/Cache", true);
                json.AddField("**/packages", true);
                json.AddField("**/Logs", true);
                json.AddField("**/Screenshots", true);
                json.AddField("**/Output", true);
                json.AddField("**/*.flax", true);
                json.EndObject();

                // Extension settings
                json.AddField("omnisharp.useModernNet", true);

                json.EndRootObject();
                json.Save(Path.Combine(vsCodeFolder, "settings.json"));
            }

            // Create workspace file
            using (var json = new JsonWriter())
            {
                json.BeginRootObject();

                // Settings
                json.BeginObject("settings");
                json.AddField("typescript.tsc.autoDetect", "off");
                json.AddField("npm.autoDetect", "off");
                json.AddField("gulp.autoDetect", "off");
                json.AddField("jake.autoDetect", "off");
                json.AddField("grunt.autoDetect", "off");
                json.AddField("omnisharp.defaultLaunchSolution", solution.Name + ".sln");
                json.AddField("omnisharp.useModernNet", true);
                json.EndObject();

                // Folders
                json.BeginArray("folders");
                {
                    // This project
                    json.BeginObject();
                    json.AddField("name", solution.Name);
                    json.AddField("path", ".");
                    json.EndObject();

                    // Referenced projects outside the current project (including engine too)
                    foreach (var project in Globals.Project.GetAllProjects())
                    {
                        if (!project.ProjectFolderPath.Contains(Globals.Project.ProjectFolderPath))
                        {
                            json.BeginObject();
                            {
                                json.AddField("name", project.Name);
                                json.AddField("path", project.ProjectFolderPath);
                            }
                            json.EndObject();
                        }
                    }
                }
                json.EndArray();

                json.EndRootObject();
                json.Save(Path.Combine(solution.WorkspaceRootPath, solution.Name + ".code-workspace"));
            }

            // Create extensions recommendations file
            using (var json = new JsonWriter())
            {
                json.BeginRootObject();

                json.BeginArray("recommendations");
                {
                    json.AddUnnamedField("ms-dotnettools.csharp");

                    if (!Globals.Project.IsCSharpOnlyProject)
                    {
                        json.AddUnnamedField("ms-vscode.cpptools");
                    }
                }
                json.EndArray();

                json.EndRootObject();
                json.Save(Path.Combine(vsCodeFolder, "extensions.json"));
            }

            // Create OmniSharp configuration file
            using (var json = new JsonWriter())
            {
                json.BeginRootObject();

                json.BeginObject("msbuild");
                {
                    json.AddField("enabled", true);
                    json.AddField("Configuration", "Editor.Debug");
                }
                json.EndObject();

                json.EndRootObject();
                json.Save(Path.Combine(solution.WorkspaceRootPath, "omnisharp.json"));
            }
        }
    }
}
