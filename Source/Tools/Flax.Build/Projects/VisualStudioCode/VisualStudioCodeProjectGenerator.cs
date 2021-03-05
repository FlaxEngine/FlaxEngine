// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
        public override void GenerateProject(Project project)
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
            var buildToolPath = Utilities.MakePathRelativeTo(typeof(Builder).Assembly.Location, solution.WorkspaceRootPath);
            var rules = Builder.GenerateRulesAssembly();

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
                            // C++ project
                            if (project.Type == TargetType.NativeCpp)
                            {
                                bool defaultTask = project == solution.MainProject;
                                foreach (var configuration in project.Configurations)
                                {
                                    var target = configuration.Target;
                                    var name = project.Name + '|' + configuration.Name;

                                    json.BeginObject();

                                    json.AddField("label", name);

                                    if (defaultTask && configuration.Configuration == TargetConfiguration.Development && configuration.Platform == Platform.BuildPlatform.Target)
                                    {
                                        defaultTask = false;
                                        json.BeginObject("group");
                                        {
                                            json.AddField("kind", "build");
                                            json.AddField("isDefault", true);
                                        }
                                        json.EndObject();
                                    }
                                    else
                                    {
                                        json.AddField("group", "build");
                                    }

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
                                        json.AddField("command", Path.Combine(Globals.EngineRoot, "Source/Platforms/Editor/Linux/Mono/bin/mono"));
                                        json.BeginArray("args");
                                        {
                                            json.AddUnnamedField(buildToolPath);
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
                    }
                    json.EndArray();
                }
                json.EndRootObject();

                json.Save(Path.Combine(vsCodeFolder, "tasks.json"));
            }

            // Create launch file
            using (var json = new JsonWriter())
            {
                json.BeginRootObject();
                {
                    json.AddField("version", "0.2.0");

                    json.BeginArray("configurations");
                    {
                        foreach (var project in solution.Projects)
                        {
                            // C++ project
                            if (project.Type == TargetType.NativeCpp)
                            {
                                foreach (var configuration in project.Configurations)
                                {
                                    var name = project.Name + '|' + configuration.Name;
                                    var target = configuration.Target;

                                    var outputType = project.OutputType ?? target.OutputType;
                                    var outputTargetFilePath = target.GetOutputFilePath(configuration.TargetBuildOptions, project.OutputType);

                                    json.BeginObject();
                                    {
                                        if (configuration.Platform == TargetPlatform.Windows)
                                            json.AddField("type", "cppvsdbg");
                                        else
                                            json.AddField("type", "cppdbg");
                                        json.AddField("name", name);
                                        json.AddField("request", "launch");
                                        json.AddField("preLaunchTask", name);
                                        json.AddField("cwd", buildToolWorkspace);

                                        switch (Platform.BuildPlatform.Target)
                                        {
                                        case TargetPlatform.Windows:
                                            if (configuration.Platform == TargetPlatform.Windows && outputType != TargetOutputType.Executable && configuration.Name.StartsWith("Editor."))
                                            {
                                                var editorFolder = configuration.Architecture == TargetArchitecture.x64 ? "Win64" : "Win32";
                                                json.AddField("program", Path.Combine(Globals.EngineRoot, "Binaries", "Editor", editorFolder, configuration.ConfigurationName, "FlaxEditor.exe"));
                                                json.BeginArray("args");
                                                {
                                                    json.AddUnnamedField("-project");
                                                    json.AddUnnamedField(buildToolWorkspace);
                                                    json.AddUnnamedField("-skipCompile");
                                                    json.AddUnnamedField("-debug");
                                                    json.AddUnnamedField("127.0.0.1:55555");
                                                }
                                                json.EndArray();
                                            }
                                            else
                                            {
                                                json.AddField("program", outputTargetFilePath);
                                            }
                                            break;
                                        case TargetPlatform.Linux:
                                            if (configuration.Platform == TargetPlatform.Linux && (outputType != TargetOutputType.Executable || project.Name == "Flax") && configuration.Name.StartsWith("Editor."))
                                            {
                                                json.AddField("program", Path.Combine(Globals.EngineRoot, "Binaries", "Editor", "Linux", configuration.ConfigurationName, "FlaxEditor"));
                                            }
                                            else
                                            {
                                                json.AddField("program", outputTargetFilePath);
                                            }
                                            if (configuration.Platform == TargetPlatform.Linux)
                                            {
                                                json.AddField("MIMode", "gdb");
                                                json.BeginArray("setupCommands");
                                                {
                                                    json.BeginObject();
                                                    json.AddField("description", "Enable pretty-printing for gdb");
                                                    json.AddField("text", "-enable-pretty-printing");
                                                    json.AddField("ignoreFailures", true);
                                                    json.EndObject();

                                                    // Ignore signals used by Mono
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
                                                json.BeginArray("args");
                                                {
                                                    json.AddUnnamedField("--std");
                                                    if (outputType != TargetOutputType.Executable && configuration.Name.StartsWith("Editor."))
                                                    {
                                                        json.AddUnnamedField("--project");
                                                        json.AddUnnamedField(buildToolWorkspace);
                                                        json.AddUnnamedField("--skipCompile");
                                                    }
                                                }
                                                json.EndArray();
                                            }
                                            break;
                                        }
                                        switch (configuration.Platform)
                                        {
                                        case TargetPlatform.Windows:
                                            json.AddField("stopAtEntry", false);
                                            json.AddField("externalConsole", true);
                                            break;
                                        case TargetPlatform.Linux:
                                            break;
                                        }
                                        json.AddField("visualizerFile", Path.Combine(Globals.EngineRoot, "Source", "flax.natvis"));
                                    }
                                    json.EndObject();
                                }
                            }
                            // C# project
                            else if (project.Type == TargetType.DotNet)
                            {
                                foreach (var configuration in project.Configurations)
                                {
                                    json.BeginObject();
                                    {
                                        json.AddField("type", "mono");
                                        json.AddField("name", project.Name + " (C# attach)" + '|' + configuration.Name);
                                        json.AddField("request", "attach");
                                        json.AddField("address", "localhost");
                                        json.AddField("port", 55555);
                                    }
                                    json.EndObject();
                                }
                            }
                        }
                    }
                    json.EndArray();
                }
                json.EndRootObject();

                json.Save(Path.Combine(vsCodeFolder, "launch.json"));
            }

            // Create C++ properties file
            using (var json = new JsonWriter())
            {
                json.BeginRootObject();
                json.BeginArray("configurations");
                json.BeginObject();
                {
                    var project = solution.MainProject ?? solution.Projects.First(x => x.Name == Globals.Project.Name);
                    json.AddField("name", project.Name);

                    var targetPlatform = Platform.BuildPlatform.Target;
                    var configuration = TargetConfiguration.Development;
                    var architecture = TargetArchitecture.x64;

                    var includePaths = new HashSet<string>();
                    var preprocessorDefinitions = new HashSet<string>();
                    foreach (var e in project.Defines)
                        preprocessorDefinitions.Add(e);

                    foreach (var target in project.Targets)
                    {
                        var platform = Platform.GetPlatform(targetPlatform);
                        if (platform.HasRequiredSDKsInstalled && target.Platforms.Contains(targetPlatform))
                        {
                            var toolchain = platform.GetToolchain(architecture);
                            var targetBuildOptions = Builder.GetBuildOptions(target, platform, toolchain, architecture, configuration, Globals.Root);
                            var modules = Builder.CollectModules(rules, platform, target, targetBuildOptions, toolchain, architecture, configuration);
                            foreach (var module in modules)
                            {
                                // This merges private module build options into global target - not the best option but helps with syntax highlighting and references collecting
                                module.Key.Setup(targetBuildOptions);
                                module.Key.SetupEnvironment(targetBuildOptions);
                            }

                            foreach (var e in targetBuildOptions.CompileEnv.PreprocessorDefinitions)
                                preprocessorDefinitions.Add(e);
                            foreach (var e in targetBuildOptions.CompileEnv.IncludePaths)
                                includePaths.Add(e);
                        }
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
                    json.AddUnnamedField("ms-vscode.mono-debug");

                    if (!Globals.Project.IsCSharpOnlyProject)
                    {
                        json.AddUnnamedField("ms-vscode.cpptools");
                    }
                }
                json.EndArray();

                json.EndRootObject();
                json.Save(Path.Combine(vsCodeFolder, "extensions.json"));
            }
        }
    }
}
