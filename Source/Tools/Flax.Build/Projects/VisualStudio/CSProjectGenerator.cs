// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace Flax.Build.Projects.VisualStudio
{
    /// <summary>
    /// The Visual Studio project generator for C# projects (.csproj).
    /// </summary>
    /// <seealso cref="Flax.Build.Projects.VisualStudio.VisualStudioProjectGenerator" />
    public class CSProjectGenerator : VisualStudioProjectGenerator
    {
        /// <inheritdoc />
        public CSProjectGenerator(VisualStudioVersion version) : base(version)
        {
        }

        /// <inheritdoc />
        public override string ProjectFileExtension => "csproj";

        /// <inheritdoc />
        public override TargetType? Type => TargetType.DotNet;

        /// <inheritdoc />
        public override void GenerateProject(Project project, string solutionPath, bool isMainProject)
        {
            var csProjectFileContent = new StringBuilder();

            var vsProject = (VisualStudioProject)project;
            var projectFileToolVersion = ProjectFileToolVersion;
            var projectDirectory = Path.GetDirectoryName(project.Path);
            var defaultTarget = project.Targets[0];
            foreach (var target in project.Targets)
            {
                // Pick the Editor-related target
                if (target.IsEditor)
                {
                    defaultTarget = target;
                    break;
                }
            }
            var defaultConfiguration = TargetConfiguration.Debug;
            var defaultArchitecture = TargetArchitecture.AnyCPU;
            var projectTypes = ProjectTypeGuids.ToOption(ProjectTypeGuids.WindowsCSharp);
            if (vsProject.CSharp.UseFlaxVS && VisualStudioInstance.HasFlaxVS)
                projectTypes = ProjectTypeGuids.ToOption(ProjectTypeGuids.FlaxVS) + ';' + projectTypes;

            // Try to reuse the existing project guid from solution file
            vsProject.ProjectGuid = GetProjectGuid(solutionPath, vsProject.Name);
            if (vsProject.ProjectGuid == Guid.Empty)
                vsProject.ProjectGuid = Guid.NewGuid();

            // Header

            csProjectFileContent.AppendLine("<?xml version=\"1.0\" encoding=\"utf-8\"?>");
            csProjectFileContent.AppendLine(string.Format("<Project DefaultTargets=\"Build\" ToolsVersion=\"{0}\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">", projectFileToolVersion));
            csProjectFileContent.AppendLine("  <Import Project=\"$(MSBuildExtensionsPath)\\$(MSBuildToolsVersion)\\Microsoft.Common.props\" Condition=\"Exists('$(MSBuildExtensionsPath)\\$(MSBuildToolsVersion)\\Microsoft.Common.props')\" />");

            // Properties

            csProjectFileContent.AppendLine("  <PropertyGroup>");

            csProjectFileContent.AppendLine(string.Format("    <Configuration Condition=\" '$(Configuration)' == '' \">{0}</Configuration>", defaultConfiguration));
            csProjectFileContent.AppendLine(string.Format("    <Platform Condition=\" '$(Platform)' == '' \">{0}</Platform>", defaultArchitecture));
            csProjectFileContent.AppendLine(string.Format("    <ProjectTypeGuids>{0}</ProjectTypeGuids>", projectTypes));
            csProjectFileContent.AppendLine(string.Format("    <ProjectGuid>{0}</ProjectGuid>", vsProject.ProjectGuid.ToString("B").ToUpperInvariant()));

            switch (project.OutputType ?? defaultTarget.OutputType)
            {
            case TargetOutputType.Executable:
                csProjectFileContent.AppendLine("    <OutputType>Exe</OutputType>");
                break;
            case TargetOutputType.Library:
                csProjectFileContent.AppendLine("    <OutputType>Library</OutputType>");
                break;
            default: throw new ArgumentOutOfRangeException();
            }

            csProjectFileContent.AppendLine(string.Format("    <RootNamespace>{0}</RootNamespace>", project.BaseName));
            csProjectFileContent.AppendLine(string.Format("    <AssemblyName>{0}.CSharp</AssemblyName>", project.BaseName));
            csProjectFileContent.AppendLine(string.Format("    <TargetFrameworkVersion>{0}</TargetFrameworkVersion>", "v4.5.2"));
            csProjectFileContent.AppendLine("    <LangVersion>7.3</LangVersion>");
            csProjectFileContent.AppendLine("    <FileAlignment>512</FileAlignment>");
            csProjectFileContent.AppendLine("    <TargetFrameworkProfile />");
            if (Version >= VisualStudioVersion.VisualStudio2022)
                csProjectFileContent.AppendLine("    <ResolveNuGetPackages>false</ResolveNuGetPackages>");

            csProjectFileContent.AppendLine("  </PropertyGroup>");

            // Default configuration
            {
                var configuration = project.Configurations.First();
                foreach (var e in project.Configurations)
                {
                    if (e.Configuration == defaultConfiguration && e.Target == defaultTarget && e.Platform == Platform.BuildTargetPlatform)
                    {
                        configuration = e;
                        break;
                    }
                }
                var defines = string.Join(";", project.Defines);
                if (configuration.TargetBuildOptions.ScriptingAPI.Defines.Count != 0)
                {
                    if (defines.Length != 0)
                        defines += ";";
                    defines += string.Join(";", configuration.TargetBuildOptions.ScriptingAPI.Defines);
                }
                var outputPath = Utilities.MakePathRelativeTo(project.CSharp.OutputPath ?? configuration.TargetBuildOptions.OutputFolder, projectDirectory);
                var intermediateOutputPath = Utilities.MakePathRelativeTo(project.CSharp.IntermediateOutputPath ?? Path.Combine(configuration.TargetBuildOptions.IntermediateFolder, "CSharp"), projectDirectory);

                csProjectFileContent.AppendLine(string.Format("  <PropertyGroup Condition=\" '$(Configuration)|$(Platform)' == '{0}|{1}' \">", defaultConfiguration, defaultArchitecture));
                csProjectFileContent.AppendLine("    <DebugSymbols>true</DebugSymbols>");
                csProjectFileContent.AppendLine("    <DebugType>portable</DebugType>");
                csProjectFileContent.AppendLine(string.Format("    <Optimize>{0}</Optimize>", defaultConfiguration == TargetConfiguration.Debug ? "false" : "true"));
                csProjectFileContent.AppendLine(string.Format("    <OutputPath>{0}\\</OutputPath>", outputPath));
                csProjectFileContent.AppendLine(string.Format("    <BaseIntermediateOutputPath>{0}\\</BaseIntermediateOutputPath>", intermediateOutputPath));
                csProjectFileContent.AppendLine(string.Format("    <IntermediateOutputPath>{0}\\</IntermediateOutputPath>", intermediateOutputPath));
                csProjectFileContent.AppendLine(string.Format("    <DefineConstants>{0}</DefineConstants>", defines));
                csProjectFileContent.AppendLine("    <ErrorReport>prompt</ErrorReport>");
                csProjectFileContent.AppendLine("    <WarningLevel>4</WarningLevel>");
                csProjectFileContent.AppendLine("    <AllowUnsafeBlocks>true</AllowUnsafeBlocks>");
                if (configuration.TargetBuildOptions.ScriptingAPI.IgnoreMissingDocumentationWarnings)
                    csProjectFileContent.AppendLine("    <NoWarn>1591</NoWarn>");
                if (configuration.TargetBuildOptions.ScriptingAPI.IgnoreSpecificWarnings.Any())
                {
                    foreach (var warningString in configuration.TargetBuildOptions.ScriptingAPI.IgnoreSpecificWarnings)
                    {
                        csProjectFileContent.AppendLine($"    <NoWarn>{warningString}</NoWarn>");
                    }
                }
                csProjectFileContent.AppendLine(string.Format("    <DocumentationFile>{0}\\{1}.CSharp.xml</DocumentationFile>", outputPath, project.BaseName));
                csProjectFileContent.AppendLine("    <UseVSHostingProcess>true</UseVSHostingProcess>");
                csProjectFileContent.AppendLine("  </PropertyGroup>");
            }

            // Configurations
            foreach (var configuration in project.Configurations)
            {
                var defines = string.Join(";", project.Defines);
                if (configuration.TargetBuildOptions.ScriptingAPI.Defines.Count != 0)
                {
                    if (defines.Length != 0)
                        defines += ";";
                    defines += string.Join(";", configuration.TargetBuildOptions.ScriptingAPI.Defines);
                }
                var outputPath = Utilities.MakePathRelativeTo(project.CSharp.OutputPath ?? configuration.TargetBuildOptions.OutputFolder, projectDirectory);
                var intermediateOutputPath = Utilities.MakePathRelativeTo(project.CSharp.IntermediateOutputPath ?? Path.Combine(configuration.TargetBuildOptions.IntermediateFolder, "CSharp"), projectDirectory);

                csProjectFileContent.AppendLine(string.Format("  <PropertyGroup Condition=\" '$(Configuration)|$(Platform)' == '{0}' \">", configuration.Name));
                csProjectFileContent.AppendLine("    <DebugSymbols>true</DebugSymbols>");
                csProjectFileContent.AppendLine("    <DebugType>portable</DebugType>");
                csProjectFileContent.AppendLine(string.Format("    <Optimize>{0}</Optimize>", configuration.Configuration == TargetConfiguration.Debug ? "false" : "true"));
                csProjectFileContent.AppendLine(string.Format("    <OutputPath>{0}\\</OutputPath>", outputPath));
                csProjectFileContent.AppendLine(string.Format("    <BaseIntermediateOutputPath>{0}\\</BaseIntermediateOutputPath>", intermediateOutputPath));
                csProjectFileContent.AppendLine(string.Format("    <IntermediateOutputPath>{0}\\</IntermediateOutputPath>", intermediateOutputPath));
                csProjectFileContent.AppendLine(string.Format("    <DefineConstants>{0}</DefineConstants>", defines));
                csProjectFileContent.AppendLine("    <ErrorReport>prompt</ErrorReport>");
                csProjectFileContent.AppendLine("    <WarningLevel>4</WarningLevel>");
                csProjectFileContent.AppendLine("    <AllowUnsafeBlocks>true</AllowUnsafeBlocks>");
                if (configuration.TargetBuildOptions.ScriptingAPI.IgnoreMissingDocumentationWarnings)
                    csProjectFileContent.AppendLine("    <NoWarn>1591</NoWarn>");
                if (configuration.TargetBuildOptions.ScriptingAPI.IgnoreSpecificWarnings.Any())
                {
                    foreach (var warningString in configuration.TargetBuildOptions.ScriptingAPI.IgnoreSpecificWarnings)
                    {
                        csProjectFileContent.AppendLine($"    <NoWarn>{warningString}</NoWarn>");
                    }
                }
                csProjectFileContent.AppendLine(string.Format("    <DocumentationFile>{0}\\{1}.CSharp.xml</DocumentationFile>", outputPath, project.BaseName));
                csProjectFileContent.AppendLine("    <UseVSHostingProcess>true</UseVSHostingProcess>");
                csProjectFileContent.AppendLine("  </PropertyGroup>");
            }

            // References

            csProjectFileContent.AppendLine("  <ItemGroup>");

            foreach (var reference in project.CSharp.SystemReferences)
            {
                csProjectFileContent.AppendLine(string.Format("    <Reference Include=\"{0}\" />", reference));
            }

            foreach (var reference in project.CSharp.FileReferences)
            {
                csProjectFileContent.AppendLine(string.Format("    <Reference Include=\"{0}\">", Path.GetFileNameWithoutExtension(reference)));
                csProjectFileContent.AppendLine(string.Format("      <HintPath>{0}</HintPath>", Utilities.MakePathRelativeTo(reference, projectDirectory)));
                csProjectFileContent.AppendLine("    </Reference>");
            }

            foreach (var dependency in project.Dependencies)
            {
                csProjectFileContent.AppendLine(string.Format("    <ProjectReference Include=\"{0}\">", Utilities.MakePathRelativeTo(dependency.Path, projectDirectory)));
                csProjectFileContent.AppendLine(string.Format("      <Project>{0}</Project>", ((VisualStudioProject)dependency).ProjectGuid.ToString("B").ToUpperInvariant()));
                csProjectFileContent.AppendLine(string.Format("      <Name>{0}</Name>", dependency.BaseName));
                csProjectFileContent.AppendLine("    </ProjectReference>");
            }

            csProjectFileContent.AppendLine("  </ItemGroup>");

            // Files and folders

            csProjectFileContent.AppendLine("  <ItemGroup>");

            var files = new List<string>();
            if (project.SourceFiles != null)
                files.AddRange(project.SourceFiles);
            if (project.SourceDirectories != null)
            {
                foreach (var folder in project.SourceDirectories)
                {
                    files.AddRange(Directory.GetFiles(folder, "*", SearchOption.AllDirectories));
                }
            }

            foreach (var file in files)
            {
                string fileType;
                if (file.EndsWith(".cs", StringComparison.OrdinalIgnoreCase))
                    fileType = "Compile";
                else
                    fileType = "None";

                var projectPath = Utilities.MakePathRelativeTo(file, projectDirectory);
                csProjectFileContent.AppendLine(string.Format("    <{0} Include=\"{1}\" />", fileType, projectPath));
            }

            if (project.GeneratedSourceFiles != null)
            {
                foreach (var file in project.GeneratedSourceFiles)
                {
                    string fileType;
                    if (file.EndsWith(".cs", StringComparison.OrdinalIgnoreCase))
                        fileType = "Compile";
                    else
                        fileType = "None";

                    csProjectFileContent.AppendLine(string.Format("    <{0} Visible=\"false\" Include=\"{1}\" />", fileType, file));
                }
            }

            csProjectFileContent.AppendLine("  </ItemGroup>");

            // End

            csProjectFileContent.AppendLine("  <Import Project=\"$(MSBuildToolsPath)\\Microsoft.CSharp.targets\" />");
            csProjectFileContent.AppendLine("</Project>");

            if (defaultTarget.CustomExternalProjectFilePath == null)
            {
                // Save the files
                Utilities.WriteFileIfChanged(project.Path, csProjectFileContent.ToString());
            }
        }
    }
}
