// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace Flax.Build.Projects.VisualStudio
{
    /// <summary>
    /// The Visual Studio project generator for C++ projects (.vcxproj).
    /// </summary>
    /// <seealso cref="Flax.Build.Projects.VisualStudio.VisualStudioProjectGenerator" />
    public class VCProjectGenerator : VisualStudioProjectGenerator
    {
        /// <summary>
        /// Gets the project file platform toolset version.
        /// </summary>
        public string ProjectFilePlatformToolsetVersion
        {
            get
            {
                switch (Version)
                {
                case VisualStudioVersion.VisualStudio2015: return "v140";
                case VisualStudioVersion.VisualStudio2017: return "v141";
                case VisualStudioVersion.VisualStudio2019: return "v142";
                }
                return string.Empty;
            }
        }

        /// <inheritdoc />
        public VCProjectGenerator(VisualStudioVersion version)
        : base(version)
        {
        }

        /// <inheritdoc />
        public override string ProjectFileExtension => "vcxproj";

        /// <inheritdoc />
        public override TargetType? Type => TargetType.NativeCpp;

        private static string FixPath(string path)
        {
            if (path.Contains(' '))
            {
                path = "\"" + path + "\"";
            }
            return path;
        }

        /// <inheritdoc />
        public override void GenerateProject(Project project)
        {
            var vcProjectFileContent = new StringBuilder();
            var vcFiltersFileContent = new StringBuilder();
            var vcUserFileContent = new StringBuilder();

            var vsProject = (VisualStudioProject)project;
            var projectFileToolVersion = ProjectFileToolVersion;
            var projectFilePlatformToolsetVersion = ProjectFilePlatformToolsetVersion;
            var projectDirectory = Path.GetDirectoryName(project.Path);
            var filtersDirectory = project.SourceFolderPath;

            // Header
            vcProjectFileContent.AppendLine("<?xml version=\"1.0\" encoding=\"utf-8\"?>");
            vcProjectFileContent.AppendLine(string.Format("<Project DefaultTargets=\"Build\" ToolsVersion=\"{0}\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">", projectFileToolVersion));

            vcFiltersFileContent.AppendLine("<?xml version=\"1.0\" encoding=\"utf-8\"?>");
            vcFiltersFileContent.AppendLine(string.Format("<Project ToolsVersion=\"{0}\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">", projectFileToolVersion));

            vcUserFileContent.AppendLine("<?xml version=\"1.0\" encoding=\"utf-8\"?>");
            vcUserFileContent.AppendLine(string.Format("<Project ToolsVersion=\"{0}\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">", projectFileToolVersion));

            // Project configurations
            vcProjectFileContent.AppendLine("  <ItemGroup Label=\"ProjectConfigurations\">");
            foreach (var configuration in project.Configurations)
            {
                vcProjectFileContent.AppendLine(string.Format("    <ProjectConfiguration Include=\"{0}\">", configuration.Name));
                vcProjectFileContent.AppendLine(string.Format("      <Configuration>{0}</Configuration>", configuration.Text));
                vcProjectFileContent.AppendLine(string.Format("      <Platform>{0}</Platform>", configuration.ArchitectureName));
                vcProjectFileContent.AppendLine("    </ProjectConfiguration>");
            }
            vcProjectFileContent.AppendLine("  </ItemGroup>");

            var platforms = vsProject.Configurations.Select(x => Platform.GetPlatform(x.Platform)).Distinct();
            foreach (var platform in platforms)
            {
                if (platform is IVisualStudioProjectCustomizer customizer)
                    customizer.WriteVisualStudioBegin(vsProject, platform, vcProjectFileContent, vcFiltersFileContent, vcUserFileContent);
            }

            // Globals
            vcProjectFileContent.AppendLine("  <PropertyGroup Label=\"Globals\">");
            vcProjectFileContent.AppendLine(string.Format("    <ProjectGuid>{0}</ProjectGuid>", vsProject.ProjectGuid.ToString("B").ToUpperInvariant()));
            vcProjectFileContent.AppendLine(string.Format("    <RootNamespace>{0}</RootNamespace>", project.Name));
            vcProjectFileContent.AppendLine(string.Format("    <PlatformToolset>{0}</PlatformToolset>", projectFilePlatformToolsetVersion));
            vcProjectFileContent.AppendLine(string.Format("    <MinimumVisualStudioVersion>{0}</MinimumVisualStudioVersion>", projectFileToolVersion));
            vcProjectFileContent.AppendLine("    <TargetRuntime>Native</TargetRuntime>");
            vcProjectFileContent.AppendLine("    <CharacterSet>Unicode</CharacterSet>");
            vcProjectFileContent.AppendLine("    <Keyword>MakeFileProj</Keyword>");
            vcProjectFileContent.AppendLine("  </PropertyGroup>");

            // Default properties
            vcProjectFileContent.AppendLine("  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.Default.props\" />");

            // Per configuration options
            foreach (var configuration in project.Configurations)
            {
                vcProjectFileContent.AppendLine(string.Format("  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='{0}'\" Label=\"Configuration\">", configuration.Name));
                vcProjectFileContent.AppendLine("    <ConfigurationType>Makefile</ConfigurationType>");
                vcProjectFileContent.AppendLine(string.Format("    <PlatformToolset>{0}</PlatformToolset>", projectFilePlatformToolsetVersion));
                vcProjectFileContent.AppendLine("  </PropertyGroup>");
            }

            // Other properties
            vcProjectFileContent.AppendLine("  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.props\" />");
            vcProjectFileContent.AppendLine("  <ImportGroup Label=\"ExtensionSettings\" />");

            // Per configuration property sheets
            foreach (var configuration in project.Configurations)
            {
                vcProjectFileContent.AppendLine(string.Format("  <ImportGroup Condition=\"'$(Configuration)|$(Platform)'=='{0}'\" Label=\"PropertySheets\">", configuration.Name));
                vcProjectFileContent.AppendLine("    <Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" Label=\"LocalAppDataPlatform\" />");
                vcProjectFileContent.AppendLine("  </ImportGroup>");
            }

            // User macros
            vcProjectFileContent.AppendLine("  <PropertyGroup Label=\"UserMacros\" />");

            // Per configuration options
            var buildToolPath = Utilities.MakePathRelativeTo(typeof(Builder).Assembly.Location, projectDirectory);
            var preprocessorDefinitions = new HashSet<string>();
            var includePaths = new HashSet<string>();
            foreach (var configuration in project.Configurations)
            {
                var platform = Platform.GetPlatform(configuration.Platform);
                var toolchain = platform.GetToolchain(configuration.Architecture);
                var target = configuration.Target;
                var targetBuildOptions = configuration.TargetBuildOptions;

                preprocessorDefinitions.Clear();
                foreach (var e in targetBuildOptions.CompileEnv.PreprocessorDefinitions)
                    preprocessorDefinitions.Add(e);

                includePaths.Clear();
                foreach (var e in targetBuildOptions.CompileEnv.IncludePaths)
                    includePaths.Add(e);

                var outputTargetFilePath = target.GetOutputFilePath(targetBuildOptions, project.OutputType);

                // Build command for the build tool
                var cmdLine = string.Format("{0} -log -mutex -workspace={1} -arch={2} -configuration={3} -platform={4} -buildTargets={5}",
                                            FixPath(buildToolPath),
                                            FixPath(project.WorkspaceRootPath),
                                            configuration.Architecture,
                                            configuration.Configuration,
                                            configuration.Platform,
                                            target.Name);
                if (!string.IsNullOrEmpty(Configuration.Compiler))
                    cmdLine += " -compiler=" + Configuration.Compiler;

                vcProjectFileContent.AppendLine(string.Format("  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='{0}'\">", configuration.Name));
                if (platform is IVisualStudioProjectCustomizer customizer)
                    customizer.WriteVisualStudioBuildProperties(vsProject, platform, toolchain, configuration, vcProjectFileContent, vcFiltersFileContent, vcUserFileContent);
                vcProjectFileContent.AppendLine(string.Format("    <IntDir>{0}</IntDir>", targetBuildOptions.IntermediateFolder));
                vcProjectFileContent.AppendLine(string.Format("    <OutDir>{0}</OutDir>", targetBuildOptions.OutputFolder));
                vcProjectFileContent.AppendLine("    <IncludePath />");
                vcProjectFileContent.AppendLine("    <ReferencePath />");
                vcProjectFileContent.AppendLine("    <LibraryPath />");
                vcProjectFileContent.AppendLine("    <LibraryWPath />");
                vcProjectFileContent.AppendLine("    <SourcePath />");
                vcProjectFileContent.AppendLine("    <ExcludePath />");
                vcProjectFileContent.AppendLine(string.Format("    <NMakeBuildCommandLine>{0} -build</NMakeBuildCommandLine>", cmdLine));
                vcProjectFileContent.AppendLine(string.Format("    <NMakeReBuildCommandLine>{0} -rebuild</NMakeReBuildCommandLine>", cmdLine));
                vcProjectFileContent.AppendLine(string.Format("    <NMakeCleanCommandLine>{0} -clean</NMakeCleanCommandLine>", cmdLine));
                vcProjectFileContent.AppendLine(string.Format("    <NMakeOutput>{0}</NMakeOutput>", outputTargetFilePath));

                if (preprocessorDefinitions.Count != 0)
                    vcProjectFileContent.AppendLine(string.Format("    <NMakePreprocessorDefinitions>$(NMakePreprocessorDefinitions);{0}</NMakePreprocessorDefinitions>", string.Join(";", preprocessorDefinitions)));

                if (includePaths.Count != 0)
                    vcProjectFileContent.AppendLine(string.Format("    <NMakeIncludeSearchPath>$(NMakeIncludeSearchPath);{0}</NMakeIncludeSearchPath>", string.Join(";", includePaths)));

                vcProjectFileContent.AppendLine("  </PropertyGroup>");
            }

            // Files and folders
            vcProjectFileContent.AppendLine("  <ItemGroup>");
            vcFiltersFileContent.AppendLine("  <ItemGroup>");

            var files = new List<string>();
            if (project.SourceFiles != null)
                files.AddRange(project.SourceFiles);
            if (project.SourceDirectories != null)
            {
                foreach (var folder in project.SourceDirectories)
                {
                    // TODO: optimize it? make source files searching faster?
                    files.AddRange(Directory.GetFiles(folder, "*", SearchOption.AllDirectories));
                }
            }

            var filterDirectories = new HashSet<string>();
            foreach (var file in files)
            {
                string fileType;
                if (file.EndsWith(".h", StringComparison.OrdinalIgnoreCase) || file.EndsWith(".inl", StringComparison.OrdinalIgnoreCase))
                {
                    fileType = "ClInclude";
                }
                else if (file.EndsWith(".cpp", StringComparison.OrdinalIgnoreCase) || file.EndsWith(".cc", StringComparison.OrdinalIgnoreCase))
                {
                    fileType = "ClCompile";
                }
                else if (file.EndsWith(".rc", StringComparison.OrdinalIgnoreCase))
                {
                    fileType = "ResourceCompile";
                }
                else if (file.EndsWith(".manifest", StringComparison.OrdinalIgnoreCase))
                {
                    fileType = "Manifest";
                }
                else if (file.EndsWith(".cs", StringComparison.OrdinalIgnoreCase))
                {
                    // C# files are added to .csproj
                    continue;
                }
                else if (file.EndsWith(".csproj", StringComparison.OrdinalIgnoreCase))
                {
                    // Skip C# project files
                    continue;
                }
                else
                {
                    fileType = "None";
                }

                var projectPath = Utilities.MakePathRelativeTo(file, projectDirectory);
                vcProjectFileContent.AppendLine(string.Format("    <{0} Include=\"{1}\"/>", fileType, projectPath));

                string filterPath = Utilities.MakePathRelativeTo(file, filtersDirectory);
                int lastSeparatorIdx = filterPath.LastIndexOf(Path.DirectorySeparatorChar);
                filterPath = lastSeparatorIdx == -1 ? string.Empty : filterPath.Substring(0, lastSeparatorIdx);
                if (!string.IsNullOrWhiteSpace(filterPath))
                {
                    // Add directories to the filters for this file
                    string pathRemaining = filterPath;
                    if (!filterDirectories.Contains(pathRemaining))
                    {
                        List<string> allDirectoriesInPath = new List<string>();
                        string currentPath = string.Empty;
                        while (true)
                        {
                            if (pathRemaining.Length > 0)
                            {
                                int slashIndex = pathRemaining.IndexOf(Path.DirectorySeparatorChar);
                                string splitDirectory;
                                if (slashIndex != -1)
                                {
                                    splitDirectory = pathRemaining.Substring(0, slashIndex);
                                    pathRemaining = pathRemaining.Substring(splitDirectory.Length + 1);
                                }
                                else
                                {
                                    splitDirectory = pathRemaining;
                                    pathRemaining = string.Empty;
                                }

                                if (!string.IsNullOrEmpty(currentPath))
                                {
                                    currentPath += Path.DirectorySeparatorChar;
                                }

                                currentPath += splitDirectory;

                                allDirectoriesInPath.Add(currentPath);
                            }
                            else
                            {
                                break;
                            }
                        }

                        for (var i = 0; i < allDirectoriesInPath.Count; i++)
                        {
                            string leadingDirectory = allDirectoriesInPath[i];
                            if (!filterDirectories.Contains(leadingDirectory))
                            {
                                filterDirectories.Add(leadingDirectory);

                                string filterGuid = Guid.NewGuid().ToString("B").ToUpperInvariant();
                                vcFiltersFileContent.AppendLine(string.Format("    <Filter Include=\"{0}\">", leadingDirectory));
                                vcFiltersFileContent.AppendLine(string.Format("      <UniqueIdentifier>{0}</UniqueIdentifier>", filterGuid));
                                vcFiltersFileContent.AppendLine("    </Filter>");
                            }
                        }
                    }

                    vcFiltersFileContent.AppendLine(string.Format("    <{0} Include=\"{1}\">", fileType, projectPath));
                    vcFiltersFileContent.AppendLine(string.Format("      <Filter>{0}</Filter>", filterPath));
                    vcFiltersFileContent.AppendLine(string.Format("    </{0}>", fileType));
                }
                else
                {
                    vcFiltersFileContent.AppendLine(string.Format("    <{0} Include=\"{1}\" />", fileType, projectPath));
                }
            }

            vcProjectFileContent.AppendLine("  </ItemGroup>");
            vcFiltersFileContent.AppendLine("  </ItemGroup>");

            // IntelliSense information
            vcProjectFileContent.AppendLine("  <PropertyGroup>");
            vcProjectFileContent.AppendLine(string.Format("    <NMakePreprocessorDefinitions>$(NMakePreprocessorDefinitions){0}</NMakePreprocessorDefinitions>", (project.Defines.Count > 0 ? (";" + string.Join(";", project.Defines)) : "")));
            vcProjectFileContent.AppendLine(string.Format("    <NMakeIncludeSearchPath>$(NMakeIncludeSearchPath){0}</NMakeIncludeSearchPath>", (project.SearchPaths.Length > 0 ? (";" + string.Join(";", project.SearchPaths)) : "")));
            vcProjectFileContent.AppendLine("    <NMakeForcedIncludes>$(NMakeForcedIncludes)</NMakeForcedIncludes>");
            vcProjectFileContent.AppendLine("    <NMakeAssemblySearchPath>$(NMakeAssemblySearchPath)</NMakeAssemblySearchPath>");
            vcProjectFileContent.AppendLine("    <NMakeForcedUsingAssemblies>$(NMakeForcedUsingAssemblies)</NMakeForcedUsingAssemblies>");
            vcProjectFileContent.AppendLine("  </PropertyGroup>");

            foreach (var platform in platforms)
            {
                if (platform is IVisualStudioProjectCustomizer customizer)
                    customizer.WriteVisualStudioEnd(vsProject, platform, vcProjectFileContent, vcFiltersFileContent, vcUserFileContent);
            }

            // End
            vcProjectFileContent.AppendLine("  <ItemDefinitionGroup>");
            vcProjectFileContent.AppendLine("  </ItemDefinitionGroup>");
            vcProjectFileContent.AppendLine("  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />");
            vcProjectFileContent.AppendLine("  <ImportGroup Label=\"ExtensionTargets\">");
            vcProjectFileContent.AppendLine("  </ImportGroup>");
            vcProjectFileContent.AppendLine("</Project>");

            vcFiltersFileContent.AppendLine("</Project>");

            vcUserFileContent.AppendLine("</Project>");

            // Save the files
            Utilities.WriteFileIfChanged(project.Path, vcProjectFileContent.ToString());
            Utilities.WriteFileIfChanged(project.Path + ".filters", vcFiltersFileContent.ToString());
            Utilities.WriteFileIfChanged(project.Path + ".user", vcUserFileContent.ToString());
        }
    }
}
