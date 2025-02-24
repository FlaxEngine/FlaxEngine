// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Xml;
using Flax.Build.Platforms;

namespace Flax.Build.Projects.VisualStudio
{
    /// <summary>
    /// Project generator for Visual Studio IDE.
    /// </summary>
    public abstract class VisualStudioProjectGenerator : ProjectGenerator
    {
        private sealed class AndroidProject : VisualStudioProject
        {
            /// <inheritdoc />
            public override Guid ProjectTypeGuid => ProjectTypeGuids.Android;

            /// <inheritdoc />
            public override void Generate(string solutionPath, bool isMainProject)
            {
                // Try to reuse the existing project guid from existing files
                ProjectGuid = GetProjectGuid(Path, Name);
                if (ProjectGuid == Guid.Empty)
                    ProjectGuid = GetProjectGuid(solutionPath, Name);
                if (ProjectGuid == Guid.Empty)
                    ProjectGuid = Guid.NewGuid();

                var gen = (VisualStudioProjectGenerator)Generator;
                var projectFileToolVersion = gen.ProjectFileToolVersion;
                var vcProjectFileContent = new StringBuilder();
                vcProjectFileContent.AppendLine("<?xml version=\"1.0\" encoding=\"utf-8\"?>");
                vcProjectFileContent.AppendLine(string.Format("<Project DefaultTargets=\"Build\" ToolsVersion=\"{0}\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">", projectFileToolVersion));
                vcProjectFileContent.AppendLine("  <ItemGroup Label=\"ProjectConfigurations\">");
                foreach (var configuration in Configurations)
                {
                    vcProjectFileContent.AppendLine(string.Format("    <ProjectConfiguration Include=\"{0}\">", configuration.Name));
                    vcProjectFileContent.AppendLine(string.Format("      <Configuration>{0}</Configuration>", configuration.Text));
                    vcProjectFileContent.AppendLine(string.Format("      <Platform>{0}</Platform>", configuration.ArchitectureName));
                    vcProjectFileContent.AppendLine("    </ProjectConfiguration>");
                }
                vcProjectFileContent.AppendLine("  </ItemGroup>");
                vcProjectFileContent.AppendLine("  <PropertyGroup Label=\"Globals\">");
                vcProjectFileContent.AppendLine(string.Format("    <ProjectGuid>{0}</ProjectGuid>", ProjectGuid.ToString("B").ToUpperInvariant()));
                vcProjectFileContent.AppendLine(string.Format("    <RootNamespace>{0}</RootNamespace>", BaseName));
                vcProjectFileContent.AppendLine(string.Format("    <MinimumVisualStudioVersion>{0}</MinimumVisualStudioVersion>", projectFileToolVersion));
                vcProjectFileContent.AppendLine(string.Format("    <AndroidAPILevel>{0}</AndroidAPILevel>", Configuration.AndroidPlatformApi));
                vcProjectFileContent.AppendLine(string.Format("    <AndroidSupportedAbis>{0}</AndroidSupportedAbis>", "arm64-v8a"));
                vcProjectFileContent.AppendLine("    <ConfigurationType>Application</ConfigurationType>");
                vcProjectFileContent.AppendLine("    <AntPackage>");
                vcProjectFileContent.AppendLine("      <AndroidAppLibName>$(RootNamespace)</AndroidAppLibName>");
                vcProjectFileContent.AppendLine("    </AntPackage>");
                vcProjectFileContent.AppendLine("  </PropertyGroup>");
                vcProjectFileContent.AppendLine("  <Import Project=\"$(AndroidTargetsPath)\\Android.Default.props\" />");
                vcProjectFileContent.AppendLine("  <Import Project=\"$(AndroidTargetsPath)\\Android.props\" />");
                vcProjectFileContent.AppendLine("  <ImportGroup Label=\"ExtensionSettings\" />");
                vcProjectFileContent.AppendLine("  <ImportGroup Label=\"Shared\" />");
                vcProjectFileContent.AppendLine("  <PropertyGroup Label=\"UserMacros\" />");
                vcProjectFileContent.AppendLine("  <PropertyGroup />");
                vcProjectFileContent.AppendLine("  <Import Project=\"$(AndroidTargetsPath)\\Android.targets\" />");
                vcProjectFileContent.AppendLine("  <ImportGroup Label=\"ExtensionTargets\" />");
                vcProjectFileContent.AppendLine("</Project>");
                Utilities.WriteFileIfChanged(Path, vcProjectFileContent.ToString());
            }
        }

        /// <summary>
        /// The Visual Studio solution project file types GUIDs. Based on http://www.codeproject.com/Reference/720512/List-of-Visual-Studio-Project-Type-GUIDs.
        /// </summary>
        public static class ProjectTypeGuids
        {
            /// <summary>
            /// The solution folder.
            /// </summary>
            public static Guid SolutionFolder = new Guid("{2150E333-8FDC-42A3-9474-1A3956D46DE8}");

            /// <summary>
            /// The Windows C#
            /// </summary>
            public static Guid WindowsCSharp = new Guid("{FAE04EC0-301F-11D3-BF4B-00C04F79EFBC}");

            /// <summary>
            /// The Windows Visual C++.
            /// </summary>
            public static Guid WindowsVisualCpp = new Guid("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}");

            /// <summary>
            /// The Android.
            /// </summary>
            public static Guid Android = new Guid("{39E2626F-3545-4960-A6E8-258AD8476CE5}");

            /// <summary>
            /// The Flax.VS.
            /// </summary>
            public static Guid FlaxVS = new Guid("{BE633490-FBA4-41EB-80D4-EFA312592B8E}");

            /// <summary>
            /// Converts the GUI to To the option.
            /// </summary>
            /// <param name="projectType">Type of the project.</param>
            /// <returns>The option string.</returns>
            public static string ToOption(Guid projectType)
            {
                return projectType.ToString("B").ToUpperInvariant();
            }
        }

        /// <summary>
        /// The target IDE version.
        /// </summary>
        public readonly VisualStudioVersion Version;

        /// <summary>
        /// Gets the project file tool version.
        /// </summary>
        public string ProjectFileToolVersion
        {
            get
            {
                switch (Version)
                {
                case VisualStudioVersion.VisualStudio2015: return "14.0";
                case VisualStudioVersion.VisualStudio2017: return "15.0";
                case VisualStudioVersion.VisualStudio2019: return "16.0";
                case VisualStudioVersion.VisualStudio2022: return "17.0";
                }

                return string.Empty;
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="VisualStudioProjectGenerator"/> class.
        /// </summary>
        /// <param name="version">The version.</param>
        protected VisualStudioProjectGenerator(VisualStudioVersion version)
        {
            Version = version;
        }

        /// <summary>
        /// Gets the project unique identifier from the existing project or generates a new one.
        /// </summary>
        /// <param name="path">The path.</param>
        /// <returns>The project ID.</returns>
        public static Guid GetProjectGuid(string path, string projectName)
        {
            // Look up for the guid in VC++-project file
            if (File.Exists(path) && Path.GetExtension(path).Equals(".vcxproj", StringComparison.OrdinalIgnoreCase))
            {
                try
                {
                    XmlDocument doc = new XmlDocument();
                    doc.Load(path);

                    XmlNodeList elements = doc.GetElementsByTagName("ProjectGuid");
                    foreach (XmlElement element in elements)
                    {
                        return Guid.ParseExact(element.InnerText.Trim("{}".ToCharArray()), "D");
                    }
                }
                catch
                {
                    // Hide errors
                }
            }
            if (File.Exists(path) && Path.GetExtension(path).Equals(".sln", StringComparison.OrdinalIgnoreCase))
            {
                try
                {
                    Regex projectRegex = new Regex(@"Project\(""{(\S+)}""\) = \""(\S+)\"", \""(\S+)\"", \""{(\S+)}\""");
                    MatchCollection matches = projectRegex.Matches(File.ReadAllText(path));
                    for (int i = 0; i < matches.Count; i++)
                    {
                        if (matches[i].Groups[1].Value.Equals("2150E333-8FDC-42A3-9474-1A3956D46DE8", StringComparison.OrdinalIgnoreCase))
                            continue;
                        if (matches[i].Groups[2].Value == projectName)
                            return Guid.ParseExact(matches[i].Groups[4].Value, "D");
                    }
                }
                catch
                {
                    // Hide errors
                }
            }

            return Guid.Empty;
        }

        /// <inheritdoc />
        public override string SolutionFileExtension => "sln";

        /// <inheritdoc />
        public override Project CreateProject()
        {
            return new VisualStudioProject
            {
                Generator = this,
            };
        }

        private struct SolutionConfiguration : IEquatable<SolutionConfiguration>, IComparable
        {
            public string Name;
            public string OriginalName;
            public string Configuration;
            public string Platform;

            public SolutionConfiguration(Project.ConfigurationData configuration)
            {
                OriginalName = configuration.Name.Replace("AnyCPU", "Any CPU");
                var name = OriginalName.Remove(OriginalName.IndexOf('|'));
                var parts = name.Split('.');
                if (parts.Length == 3)
                    Configuration = parts[0] + '.' + parts[2];
                else if (parts.Length == 1)
                    Configuration = parts[0];
                else
                    throw new Exception($"Unknown project configuration {configuration.Name}.");
                Platform = configuration.PlatformName + "_" + configuration.ArchitectureName;
                if (configuration.Architecture == TargetArchitecture.AnyCPU)
                    Platform = "Any CPU";
                var platform = Build.Platform.GetPlatform(configuration.Platform);
                if (platform is IProjectCustomizer customizer)
                    customizer.GetSolutionArchitectureName(configuration.Architecture, ref Platform);
                Name = Configuration + '|' + Platform;
            }

            public SolutionConfiguration(string configuration, string platform)
            {
                OriginalName = string.Empty;
                Name = configuration + '|' + platform;
                Configuration = configuration;
                Platform = platform;
            }

            /// <inheritdoc />
            public int CompareTo(object obj)
            {
                if (obj is SolutionConfiguration other)
                    return Name.CompareTo(other.Name);
                return 1;
            }

            /// <inheritdoc />
            public bool Equals(SolutionConfiguration other)
            {
                return Name == other.Name;
            }

            /// <inheritdoc />
            public override string ToString()
            {
                return Name;
            }
        }

        /// <inheritdoc />
        public override void GenerateSolution(Solution solution)
        {
            // Ensure that the main project is the first one (initially selected by Visual Studio)
            if (solution.MainProject != null && solution.Projects.Length != 0 && solution.Projects[0] != solution.MainProject)
            {
                for (int i = 1; i < solution.Projects.Length; i++)
                {
                    if (solution.Projects[i] == solution.MainProject)
                    {
                        solution.Projects[i] = solution.Projects[0];
                        solution.Projects[0] = solution.MainProject;
                        break;
                    }
                }
            }

            // Try to extract solution folder info from the existing solution file to make random IDs stable
            var solutionId = Guid.NewGuid();
            var folderIds = new Dictionary<string, Guid>();
            if (File.Exists(solution.Path))
            {
                try
                {
                    var contents = File.ReadAllText(solution.Path);

                    var solutionIdMatch = Regex.Match(contents, "SolutionGuid = \\{(.*?)\\}");
                    if (solutionIdMatch.Success)
                    {
                        var value = solutionIdMatch.Value;
                        solutionId = Guid.ParseExact(value.Substring(15), "B");
                    }

                    var folderIdMatches = new Regex("Project\\(\"{2150E333-8FDC-42A3-9474-1A3956D46DE8}\"\\) = \"(.*?)\", \"(.*?)\", \"{(.*?)}\"").Matches(contents);
                    foreach (Match match in folderIdMatches)
                    {
                        var folder = match.Groups[2].Value;
                        var folderId = Guid.ParseExact(match.Groups[3].Value, "D");
                        folderIds[folder] = folderId;
                    }
                }
                catch (Exception ex)
                {
                    Log.Warning("Failed to restore solution and solution folders identifiers from existing file.");
                    Log.Exception(ex);
                }
            }

            StringBuilder vcSolutionFileContent = new StringBuilder();
            var solutionDirectory = Path.GetDirectoryName(solution.Path);
            var projects = solution.Projects.Cast<VisualStudioProject>().ToArray();

            // Header
            if (Version == VisualStudioVersion.VisualStudio2022)
            {
                vcSolutionFileContent.AppendLine("Microsoft Visual Studio Solution File, Format Version 12.00");
                vcSolutionFileContent.AppendLine("# Visual Studio Version 17");
                vcSolutionFileContent.AppendLine("VisualStudioVersion = 17.0.31314.256");
                vcSolutionFileContent.AppendLine("MinimumVisualStudioVersion = 10.0.40219.1");
            }
            else if (Version == VisualStudioVersion.VisualStudio2019)
            {
                vcSolutionFileContent.AppendLine("Microsoft Visual Studio Solution File, Format Version 12.00");
                vcSolutionFileContent.AppendLine("# Visual Studio Version 16");
                vcSolutionFileContent.AppendLine("VisualStudioVersion = 16.0.28315.86");
                vcSolutionFileContent.AppendLine("MinimumVisualStudioVersion = 10.0.40219.1");
            }
            else if (Version == VisualStudioVersion.VisualStudio2017)
            {
                vcSolutionFileContent.AppendLine("Microsoft Visual Studio Solution File, Format Version 12.00");
                vcSolutionFileContent.AppendLine("# Visual Studio 15");
                vcSolutionFileContent.AppendLine("VisualStudioVersion = 15.0.25807.0");
                vcSolutionFileContent.AppendLine("MinimumVisualStudioVersion = 10.0.40219.1");
            }
            else if (Version == VisualStudioVersion.VisualStudio2015)
            {
                vcSolutionFileContent.AppendLine("Microsoft Visual Studio Solution File, Format Version 12.00");
                vcSolutionFileContent.AppendLine("# Visual Studio 14");
                vcSolutionFileContent.AppendLine("VisualStudioVersion = 14.0.22310.1");
                vcSolutionFileContent.AppendLine("MinimumVisualStudioVersion = 10.0.40219.1");
            }
            else
            {
                throw new Exception("Unsupported solution file format.");
            }

            // Solution folders
            var folderNames = new HashSet<string>();
            {
                // Move projects to subfolders based on group names including subfolders to match the location of the source in the workspace
                foreach (var project in projects)
                {
                    var folder = project.GroupName;

                    if (project.SourceDirectories != null && project.SourceDirectories.Count == 1)
                    {
                        var subFolder = Utilities.NormalizePath(Utilities.MakePathRelativeTo(Path.GetDirectoryName(project.SourceDirectories[0]), project.WorkspaceRootPath));
                        if (subFolder.StartsWith("Source/"))
                            subFolder = subFolder.Substring("Source/".Length);
                        if (subFolder.Length != 0)
                        {
                            if (folder.Length != 0)
                                folder += '/';
                            folder += subFolder;
                        }
                    }

                    if (string.IsNullOrEmpty(folder))
                        continue;

                    var folderParents = folder.Split('/');
                    for (int i = 0; i < folderParents.Length; i++)
                    {
                        var folderPath = folderParents[0];
                        for (int j = 1; j <= i; j++)
                            folderPath += '/' + folderParents[j];

                        if (folderNames.Contains(folderPath))
                        {
                            project.FolderGuid = folderIds[folderPath];
                        }
                        else
                        {
                            if (!folderIds.TryGetValue(folderPath, out project.FolderGuid))
                            {
                                project.FolderGuid = Guid.NewGuid();
                                folderIds.Add(folderPath, project.FolderGuid);
                            }
                            folderNames.Add(folderPath);
                        }
                    }
                }

                foreach (var folder in folderNames)
                {
                    var folderGuid = folderIds[folder].ToString("B").ToUpperInvariant();
                    var typeGuid = ProjectTypeGuids.ToOption(ProjectTypeGuids.SolutionFolder);
                    var lastSplit = folder.LastIndexOf('/');
                    var name = lastSplit != -1 ? folder.Substring(lastSplit + 1) : folder;

                    vcSolutionFileContent.AppendLine(string.Format("Project(\"{0}\") = \"{1}\", \"{2}\", \"{3}\"", typeGuid, name, folder, folderGuid));
                    vcSolutionFileContent.AppendLine("EndProject");
                }
            }

            // Solution projects
            foreach (var project in projects)
            {
                var projectId = project.ProjectGuid.ToString("B").ToUpperInvariant();
                var typeGuid = ProjectTypeGuids.ToOption(project.ProjectTypeGuid);

                vcSolutionFileContent.AppendLine(string.Format("Project(\"{0}\") = \"{1}\", \"{2}\", \"{3}\"", typeGuid, project.Name, Utilities.MakePathRelativeTo(project.Path, solutionDirectory), projectId));

                if (project.Dependencies.Count > 0)
                {
                    vcSolutionFileContent.AppendLine("\tProjectSection(ProjectDependencies) = postProject");
                    foreach (var dependency in project.Dependencies.Cast<VisualStudioProject>())
                    {
                        string dependencyId = dependency.ProjectGuid.ToString("B").ToUpperInvariant();
                        vcSolutionFileContent.AppendLine("\t\t" + dependencyId + " = " + dependencyId);
                    }

                    vcSolutionFileContent.AppendLine("\tEndProjectSection");
                }

                vcSolutionFileContent.AppendLine("EndProject");
            }

            // Global configuration
            {
                vcSolutionFileContent.AppendLine("Global");

                // Collect all unique configurations
                var configurations = new HashSet<SolutionConfiguration>();
                var mainArchitectures = solution.MainProject?.Targets?.SelectMany(x => x.Architectures).Distinct().ToArray();
                foreach (var project in projects)
                {
                    if (project.Configurations == null || project.Configurations.Count == 0)
                        throw new Exception("Missing configurations for project " + project.Name);

                    // Prevent generating default Debug|AnyCPU and Release|AnyCPU configurations from Flax projects
                    if (project.Name == "BuildScripts" || project.Name == "Flax.Build" || project.Name == "Flax.Build.Tests")
                        continue;

                    foreach (var configuration in project.Configurations)
                    {
                        // Skip architectures which are not included in the game project
                        if (mainArchitectures != null && !mainArchitectures.Contains(configuration.Architecture))
                            continue;

                        configurations.Add(new SolutionConfiguration(configuration));
                    }
                }

                // Add missing configurations (Visual Studio needs all permutations of configuration/platform pair)
                var configurationNames = configurations.Select(x => x.Configuration).Distinct().ToArray();
                var platformNames = configurations.Select(x => x.Platform).Distinct().ToArray();
                foreach (var configurationName in configurationNames)
                {
                    foreach (var platformName in platformNames)
                    {
                        configurations.Add(new SolutionConfiguration(configurationName, platformName));
                    }
                }

                // Sort configurations
                var configurationsSorted = new List<SolutionConfiguration>(configurations);
                configurationsSorted.Sort();

                // Global configurations
                {
                    vcSolutionFileContent.AppendLine("	GlobalSection(SolutionConfigurationPlatforms) = preSolution");

                    foreach (var configuration in configurationsSorted)
                    {
                        vcSolutionFileContent.AppendLine("		" + configuration.Name + " = " + configuration.Name);
                    }

                    vcSolutionFileContent.AppendLine("	EndGlobalSection");
                }

                // Per-project configurations mapping
                {
                    vcSolutionFileContent.AppendLine("	GlobalSection(ProjectConfigurationPlatforms) = postSolution");

                    foreach (var project in projects)
                    {
                        string projectId = project.ProjectGuid.ToString("B").ToUpperInvariant();

                        foreach (var configuration in configurationsSorted)
                        {
                            SolutionConfiguration projectConfiguration;
                            bool build = false;
                            int firstFullMatch = -1, firstPlatformMatch = -1, firstEditorMatch = -1;
                            for (int i = 0; i < project.Configurations.Count; i++)
                            {
                                var e = new SolutionConfiguration(project.Configurations[i]);
                                if (e.Name == configuration.Name)
                                {
                                    firstFullMatch = i;
                                    break;
                                }
                                if (firstPlatformMatch == -1 && e.Platform == configuration.Platform)
                                {
                                    firstPlatformMatch = i;
                                }
                                if (firstEditorMatch == -1 && e.Configuration == configuration.Configuration)
                                {
                                    firstEditorMatch = i;
                                }
                            }
                            if (project is AndroidProject)
                            {
                                // Utility Android deploy project only for exact match
                                if (firstFullMatch != -1)
                                    projectConfiguration = configuration;
                                else
                                    projectConfiguration = new SolutionConfiguration(project.Configurations[0]);
                            }
                            else if (firstFullMatch != -1)
                            {
                                projectConfiguration = configuration;

                                // Always build the main project
                                build = solution.MainProject == project;

                                // Build C# projects (needed for Rider solution wide analysis)
                                build |= project.Type == TargetType.DotNetCore;

                                // Always build the project named after solution if main project was not set
                                build |= solution.MainProject == null && project.Name == solution.Name;
                            }
                            else if (firstPlatformMatch != -1 && !configuration.Name.StartsWith("Editor."))
                            {
                                // No exact match, pick the first configuration for matching platform
                                projectConfiguration = new SolutionConfiguration(project.Configurations[firstPlatformMatch]);
                            }
                            else if (firstEditorMatch != -1 && configuration.Name.StartsWith("Editor."))
                            {
                                // No exact match, pick the matching editor configuration for different platform.
                                // As an example, Editor configuration for Android projects should be remapped
                                // to desktop platform in order to provide working Intellisense information.
                                projectConfiguration = new SolutionConfiguration(project.Configurations[firstEditorMatch]);
                            }
                            else
                            {
                                // No match
                                projectConfiguration = new SolutionConfiguration(project.Configurations[0]);
                            }

                            vcSolutionFileContent.AppendLine(string.Format("		{0}.{1}.ActiveCfg = {2}", projectId, configuration.Name, projectConfiguration.OriginalName));
                            if (build)
                                vcSolutionFileContent.AppendLine(string.Format("		{0}.{1}.Build.0 = {2}", projectId, configuration.Name, projectConfiguration.OriginalName));
                        }
                    }

                    vcSolutionFileContent.AppendLine("	EndGlobalSection");
                }

                // Always show solution root node
                {
                    vcSolutionFileContent.AppendLine("	GlobalSection(SolutionProperties) = preSolution");
                    vcSolutionFileContent.AppendLine("		HideSolutionNode = FALSE");
                    vcSolutionFileContent.AppendLine("	EndGlobalSection");
                }

                // Solution directory hierarchy
                {
                    vcSolutionFileContent.AppendLine("	GlobalSection(NestedProjects) = preSolution");

                    // Write nested folders hierarchy
                    foreach (var folder in folderNames)
                    {
                        var lastSplit = folder.LastIndexOf('/');
                        if (lastSplit != -1)
                        {
                            var folderGuid = folderIds[folder].ToString("B").ToUpperInvariant();
                            var parentFolder = folder.Substring(0, lastSplit);
                            var parentFolderGuid = folderIds[parentFolder].ToString("B").ToUpperInvariant();
                            vcSolutionFileContent.AppendLine(string.Format("		{0} = {1}", folderGuid, parentFolderGuid));
                        }
                    }

                    // Write mapping for projectId - folderId
                    foreach (var project in projects)
                    {
                        if (project.FolderGuid != Guid.Empty)
                        {
                            var projectGuidString = project.ProjectGuid.ToString("B").ToUpperInvariant();
                            var folderGuidString = project.FolderGuid.ToString("B").ToUpperInvariant();
                            vcSolutionFileContent.AppendLine(string.Format("		{0} = {1}", projectGuidString, folderGuidString));
                        }
                    }

                    vcSolutionFileContent.AppendLine("	EndGlobalSection");
                }

                // Solution identifier
                {
                    vcSolutionFileContent.AppendLine("	GlobalSection(ExtensibilityGlobals) = postSolution");
                    vcSolutionFileContent.AppendLine(string.Format("		SolutionGuid = {0}", solutionId.ToString("B").ToUpperInvariant()));
                    vcSolutionFileContent.AppendLine("	EndGlobalSection");
                }

                vcSolutionFileContent.AppendLine("EndGlobal");
            }

            // Save the file
            Utilities.WriteFileIfChanged(solution.Path, vcSolutionFileContent.ToString());

            // Generate launch profiles for C# projects
            if (Version >= VisualStudioVersion.VisualStudio2022)
            {
                var profiles = new Dictionary<string, string>();
                var profile = new StringBuilder();
                var configuration = "Development";
                var editorPath = Utilities.NormalizePath(Path.Combine(Globals.EngineRoot, Platform.GetEditorBinaryDirectory(), configuration, $"FlaxEditor{Utilities.GetPlatformExecutableExt()}")).Replace('\\', '/');
                var workspacePath = Utilities.NormalizePath(solutionDirectory).Replace('\\', '/');
                foreach (var project in projects)
                {
                    if (project.Type == TargetType.DotNetCore)
                    {
                        var path = Path.Combine(Path.GetDirectoryName(project.Path), "Properties/launchSettings.json");
                        path = Utilities.NormalizePath(path);
                        if (profiles.ContainsKey(path))
                            profile.AppendLine(",");
                        profile.AppendLine($"    \"{project.BaseName}\": {{");
                        profile.AppendLine("      \"commandName\": \"Executable\",");
                        profile.AppendLine($"      \"workingDirectory\": \"{workspacePath}\",");
                        profile.AppendLine($"      \"executablePath\": \"{editorPath}\",");
                        profile.AppendLine($"      \"commandLineArgs\": \"-project \\\"{workspacePath}\\\"\",");
                        profile.AppendLine("      \"nativeDebugging\": false");
                        profile.Append("    }");
                        if (profiles.ContainsKey(path))
                            profiles[path] += profile.ToString();
                        else
                            profiles.Add(path, profile.ToString());
                        profile.Clear();
                    }
                }
                foreach (var e in profiles)
                {
                    var folder = Path.GetDirectoryName(e.Key);
                    if (!Directory.Exists(folder))
                        Directory.CreateDirectory(folder);
                    profile.Clear();
                    profile.AppendLine("{");
                    profile.AppendLine("  \"profiles\": {");
                    profile.AppendLine(e.Value);
                    profile.AppendLine("  }");
                    profile.Append("}");
                    File.WriteAllText(e.Key, profile.ToString(), Encoding.UTF8);
                }
            }

            // Generate Rider-specific configuration files
            {
                StringBuilder dotSettingsFileContent = new StringBuilder();
                string dotSettingsUserFilePath = solution.Path + ".DotSettings.user";

                // Solution settings (user layer)
                bool useResharperBuild = false; // This needs to be disabled for custom build steps to run properly

                if (File.Exists(dotSettingsUserFilePath))
                {
                    foreach (var line in File.ReadAllLines(dotSettingsUserFilePath))
                    {
                        if (line.Contains(@"/UseMsbuildSolutionBuilder/@EntryValue"))
                        {
                            if (!useResharperBuild)
                            {
                                dotSettingsFileContent.Append("\t").Append(@"<s:String x:Key=""/Default/Environment/Hierarchy/Build/SolBuilderDuo/UseMsbuildSolutionBuilder/@EntryValue"">No</s:String>");
                                if (line.Contains("</wpf:ResourceDictionary>"))
                                    dotSettingsFileContent.Append("</wpf:ResourceDictionary>\n");
                                else
                                    dotSettingsFileContent.Append("\n");
                            }
                            continue;
                        }
                        dotSettingsFileContent.Append(line).Append("\n");
                    }
                }
                else
                {
                    dotSettingsFileContent.Append(@"<wpf:ResourceDictionary xml:space=""preserve"" xmlns:x=""http://schemas.microsoft.com/winfx/2006/xaml"" xmlns:s=""clr-namespace:System;assembly=mscorlib"" xmlns:ss=""urn:shemas-jetbrains-com:settings-storage-xaml"" xmlns:wpf=""http://schemas.microsoft.com/winfx/2006/xaml/presentation"">").Append("\n");
                    if (!useResharperBuild)
                        dotSettingsFileContent.Append("\t").Append(@"<s:String x:Key=""/Default/Environment/Hierarchy/Build/SolBuilderDuo/UseMsbuildSolutionBuilder/@EntryValue"">No</s:String>");
                    dotSettingsFileContent.Append("</wpf:ResourceDictionary>\n");
                }

                Utilities.WriteFileIfChanged(dotSettingsUserFilePath, dotSettingsFileContent.ToString());
            }

            // Custom MSBuild .targets file to prevent building Flax C#-projects directly with MSBuild
            {
                var targetsFileContent = new StringBuilder();
                targetsFileContent.AppendLine("<Project>");
                targetsFileContent.AppendLine("  <!-- Prevent building projects with MSBuild, let Flax.Build handle the building process -->");
                targetsFileContent.AppendLine("  <Target Name=\"Build\" Condition=\"'false' == 'true'\" />");
                targetsFileContent.AppendLine("</Project>");

                Utilities.WriteFileIfChanged(Path.Combine(Globals.Root, "Cache", "Projects", "Flax.Build.CSharp.SkipBuild.targets"), targetsFileContent.ToString());
            }

            // Override MSBuild build tasks to run Flax.Build in C#-only projects
            {
                // Build command for the build tool
                var buildToolPath = Path.ChangeExtension(typeof(Builder).Assembly.Location, null);

                var targetsFileContent = new StringBuilder();
                targetsFileContent.AppendLine("<Project>");
                targetsFileContent.AppendLine("  <!-- Custom Flax.Build scripts for C# projects. -->");
                targetsFileContent.AppendLine("  <Target Name=\"Build\">");
                AppendBuildToolCommands(targetsFileContent, "-build");
                targetsFileContent.AppendLine("  </Target>");
                targetsFileContent.AppendLine("  <Target Name=\"Rebuild\">");
                AppendBuildToolCommands(targetsFileContent, "-rebuild");
                targetsFileContent.AppendLine("  </Target>");
                targetsFileContent.AppendLine("  <Target Name=\"Clean\">");
                AppendBuildToolCommands(targetsFileContent, "-clean");
                targetsFileContent.AppendLine("  </Target>");
                targetsFileContent.AppendLine("</Project>");

                Utilities.WriteFileIfChanged(Path.Combine(Globals.Root, "Cache", "Projects", "Flax.Build.CSharp.targets"), targetsFileContent.ToString());

                void AppendBuildToolCommands(StringBuilder str, string extraArgs)
                {
                    if (solution.MainProject == null)
                        return;
                    foreach (var configuration in solution.MainProject.Configurations)
                    {
                        var cmdLine = string.Format("\"{0}\" -log -mutex -workspace=\"{1}\" -arch={2} -configuration={3} -platform={4} -buildTargets={5}",
                                                buildToolPath,
                                                solution.MainProject.WorkspaceRootPath,
                                                configuration.Architecture,
                                                configuration.Configuration,
                                                configuration.Platform,
                                                configuration.Target);
                        Configuration.PassArgs(ref cmdLine);

                        str.AppendLine(string.Format("    <Exec Command='{0} {1}' Condition=\"'$(Configuration)|$(Platform)'=='{2}'\"/>", cmdLine, extraArgs, configuration.Name));
                    }
                }
            }
        }

        /// <inheritdoc />
        public override void GenerateCustomProjects(List<Project> projects)
        {
            base.GenerateCustomProjects(projects);

            // Generate custom project for Visual Studio project to debug native code on Android
            var androidProject = projects.FirstOrDefault(p => p.Configurations.Any(c => c.Platform == TargetPlatform.Android));
            if (androidProject != null && AndroidSdk.Instance.IsValid && AndroidNdk.Instance.IsValid)
            {
                var platform = Platform.GetPlatform(TargetPlatform.Android);
                var rootProject = Globals.Project;
                var workspaceRoot = rootProject.ProjectFolderPath;
                var project = new AndroidProject
                {
                    Generator = this,
                };
                var targetsSorted = new List<Target>(androidProject.Targets);
                targetsSorted.Sort((a, b) => b.Platforms.Length.CompareTo(a.Platforms.Length));
                var target = targetsSorted.First(x => x.Platforms.Contains(TargetPlatform.Android));
                project.Type = TargetType.NativeCpp;
                project.Name = project.BaseName = "Android";
                project.Targets = new Target[0];
                project.SearchPaths = new string[0];
                project.WorkspaceRootPath = rootProject.ProjectFolderPath;
                project.Path = Path.Combine(workspaceRoot, "Cache", "Projects", project.Name + ".androidproj");
                project.SourceDirectories = new List<string>();
                project.Configurations = new List<Project.ConfigurationData>
                {
                    new Project.ConfigurationData(target, androidProject, platform, TargetArchitecture.ARM64, TargetConfiguration.Debug),
                    new Project.ConfigurationData(target, androidProject, platform, TargetArchitecture.ARM64, TargetConfiguration.Development),
                    new Project.ConfigurationData(target, androidProject, platform, TargetArchitecture.ARM64, TargetConfiguration.Release),
                };
                projects.Add(project);
            }
        }
    }
}
