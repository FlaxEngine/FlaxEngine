// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace Flax.Build.Projects.VisualStudio
{
    /// <summary>
    /// The Visual Studio project generator for C# projects (.NET SDK .csproj).
    /// </summary>
    /// <seealso cref="Flax.Build.Projects.VisualStudio.VisualStudioProjectGenerator" />
    public class CSSDKProjectGenerator : VisualStudioProjectGenerator
    {
        /// <inheritdoc />
        public CSSDKProjectGenerator(VisualStudioVersion version) : base(version)
        {
        }

        /// <inheritdoc />
        public override string ProjectFileExtension => "csproj";

        /// <inheritdoc />
        public override TargetType? Type => TargetType.DotNetCore;

        /// <inheritdoc />
        public override void GenerateProject(Project project)
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

            // Header
            csProjectFileContent.AppendLine("<Project Sdk=\"Microsoft.NET.Sdk\">");
            csProjectFileContent.AppendLine("");

            //csProjectFileContent.AppendLine(string.Format("<Project DefaultTargets=\"Build\" ToolsVersion=\"{0}\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">", projectFileToolVersion));
            //csProjectFileContent.AppendLine("  <Import Project=\"$(MSBuildExtensionsPath)\\$(MSBuildToolsVersion)\\Microsoft.Common.props\" Condition=\"Exists('$(MSBuildExtensionsPath)\\$(MSBuildToolsVersion)\\Microsoft.Common.props')\" />");

            // Properties

            csProjectFileContent.AppendLine("  <PropertyGroup>");

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

            var baseConfiguration = project.Configurations.First();
            var baseOutputDir = Utilities.MakePathRelativeTo(project.CSharp.OutputPath ?? baseConfiguration.TargetBuildOptions.OutputFolder, projectDirectory);
            var baseIntermediateOutputPath = Utilities.MakePathRelativeTo(project.CSharp.IntermediateOutputPath ?? Path.Combine(baseConfiguration.TargetBuildOptions.IntermediateFolder, "CSharp"), projectDirectory);
            var baseConfigurations = project.Configurations.Select(x => x.Name.Split('|')[0]).Distinct().ToArray();

            csProjectFileContent.AppendLine("    <TargetFramework>net7.0</TargetFramework>");
            csProjectFileContent.AppendLine("    <ImplicitUsings>enable</ImplicitUsings>");
            csProjectFileContent.AppendLine("    <Nullable>disable</Nullable>");
            csProjectFileContent.AppendLine(string.Format("    <Configurations>{0}</Configurations>", string.Join(";", baseConfigurations)));
            csProjectFileContent.AppendLine("    <EnableDefaultItems>false</EnableDefaultItems>");                                  // ?
            csProjectFileContent.AppendLine("    <GenerateRuntimeConfigurationFiles>true</GenerateRuntimeConfigurationFiles>");     // Needed for Hostfxr
            csProjectFileContent.AppendLine("    <EnableDynamicLoading>true</EnableDynamicLoading>");                               // ?
            csProjectFileContent.AppendLine("    <AppendTargetFrameworkToOutputPath>false</AppendTargetFrameworkToOutputPath>");    // Prevents outputting the file under net7.0 subdirectory
            csProjectFileContent.AppendLine("    <AssemblyName>$(MSBuildProjectName).CSharp</AssemblyName>");                       // For backwards compatibility, keep the filename same
            csProjectFileContent.AppendLine("    <GenerateAssemblyInfo>false</GenerateAssemblyInfo>");                              // Prevents AssemblyInfo.cs generation (causes duplicate attributes)
            csProjectFileContent.AppendLine("    <EnableBaseIntermediateOutputPathMismatchWarning>false</EnableBaseIntermediateOutputPathMismatchWarning>");
            csProjectFileContent.AppendLine(string.Format("    <OutDir>{0}</OutDir>", baseOutputDir));                             // This needs to be set here to fix errors in VS
            csProjectFileContent.AppendLine(string.Format("    <IntermediateOutputPath>{0}</IntermediateOutputPath>", baseIntermediateOutputPath)); // This needs to be set here to fix errors in VS

            csProjectFileContent.AppendLine("  </PropertyGroup>");
            csProjectFileContent.AppendLine("");

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
                csProjectFileContent.AppendLine(string.Format("    <DocumentationFile>{0}\\{1}.CSharp.xml</DocumentationFile>", outputPath, project.Name));
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

                csProjectFileContent.AppendLine(string.Format("  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)' == '{0}' or '$(Configuration)|$(Platform)' == '{1}'\">", configuration.Name, configuration.Name.Replace(configuration.ArchitectureName, "AnyCPU")));
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
                csProjectFileContent.AppendLine(string.Format("    <DocumentationFile>{0}\\{1}.CSharp.xml</DocumentationFile>", outputPath, project.Name));
                csProjectFileContent.AppendLine("    <UseVSHostingProcess>true</UseVSHostingProcess>");
                csProjectFileContent.AppendLine("  </PropertyGroup>");
            }

            // Nuget Package References
            csProjectFileContent.AppendLine("  <ItemGroup>");
            csProjectFileContent.AppendLine("    <PackageReference Include=\"DotNetZip\" Version=\"1.16\" />");
            csProjectFileContent.AppendLine("    <PackageReference Include=\"Microsoft.CodeAnalysis.CSharp\" Version=\"4.3\" />");
            csProjectFileContent.AppendLine("    <PackageReference Include=\"Microsoft.VisualStudio.Setup.Configuration.Interop\" Version=\"3.2\" />");
            csProjectFileContent.AppendLine("    <PackageReference Include=\"Microsoft.Windows.Compatibility\" Version=\"6.0\" />");
            csProjectFileContent.AppendLine("    <PackageReference Include=\"Newtonsoft.Json\" Version=\"13.0\" />");
            csProjectFileContent.AppendLine("    <PackageReference Include=\"System.CodeDom\" Version=\"6.0\" />");
            csProjectFileContent.AppendLine("  </ItemGroup>");

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
                csProjectFileContent.AppendLine(string.Format("      <Name>{0}</Name>", dependency.Name));
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

            //csProjectFileContent.AppendLine("  <Import Project=\"$(MSBuildToolsPath)\\Microsoft.CSharp.targets\" />");
            csProjectFileContent.AppendLine("</Project>");

            if (defaultTarget.CustomExternalProjectFilePath == null)
            {
                // Save the files
                Utilities.WriteFileIfChanged(project.Path, csProjectFileContent.ToString());
            }
        }
    }
}
