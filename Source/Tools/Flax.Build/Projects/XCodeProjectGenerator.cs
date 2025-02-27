// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace Flax.Build.Projects
{
    /// <summary>
    /// Project generator for XCode.
    /// </summary>
    public class XCodeProjectGenerator : ProjectGenerator
    {
        private Random _rand = new Random(1995);
        private byte[] _randBytes = new byte[12];

        /// <summary>
        /// Initializes a new instance of the <see cref="XCodeProjectGenerator"/> class.
        /// </summary>
        public XCodeProjectGenerator()
        {
        }

        /// <inheritdoc />
        public override string ProjectFileExtension => string.Empty;

        /// <inheritdoc />
        public override string SolutionFileExtension => "xcodeproj";

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
        }

        /// <inheritdoc />
        public override void GenerateSolution(Solution solution)
        {
            // Prepare
            // TOOD: use random IDs for project and configurations
            var groupId = "04FD6344277A0F15000EA5CA";
            var projectId = "04FD6345277A0F15000EA5CA";
            var targetId = "04FD633E277A01BD000EA5CA";
            var projectName = solution.Name;
            var targetName = projectName;
            var projectWorkspacePath = Path.Combine(solution.Path, projectName + ".xcworkspace");
            var buildToolWorkspace = Environment.CurrentDirectory;
            var buildToolPath = Utilities.MakePathRelativeTo(typeof(Builder).Assembly.Location, solution.WorkspaceRootPath);
            var buildToolCallerPath = Path.Combine(Globals.EngineRoot, "Development/Scripts/Mac/XCodeBuild.sh");
            var rules = Builder.GenerateRulesAssembly();

            // Generate folders
            Directory.CreateDirectory(solution.Path);
            Directory.CreateDirectory(projectWorkspacePath);

            // Generate XCode project
            {
                var contents = new StringBuilder();

                contents.AppendLine("// !$*UTF8*$!");
                contents.AppendLine("{");
                contents.AppendLine("\tarchiveVersion = 1;");
                contents.AppendLine("\tclasses = {");
                contents.AppendLine("\t};");
                contents.AppendLine("\tobjectVersion = 55;");
                contents.AppendLine("\tobjects = {");

                contents.AppendLine("");
                var sourceGroupId = "04FD634B277A16FE000EA5CA";
                contents.AppendLine("/* Begin PBXFileReference section */");
                contents.AppendLine("\t\t" + sourceGroupId + " /* Source */ = {isa = PBXFileReference; lastKnownFileType = folder; path = Source; sourceTree = \"<group>\"; };");
                contents.AppendLine("/* End PBXFileReference section */");

                contents.AppendLine("");
                contents.AppendLine("/* Begin PBXGroup section */");
                contents.AppendLine("\t\t" + groupId + " = {");
                contents.AppendLine("\t\t\tisa = PBXGroup;");
                contents.AppendLine("\t\t\tchildren = (");
                contents.AppendLine("\t\t\t\t" + sourceGroupId + ",");
                contents.AppendLine("\t\t\t);");
                contents.AppendLine("\t\t\tsourceTree = \"<group>\";");
                contents.AppendLine("\t\t};");
                contents.AppendLine("/* End PBXGroup section */");

                contents.AppendLine("");
                contents.AppendLine("/* Begin PBXLegacyTarget section */");
                contents.AppendLine("\t\t" + targetId + " /* " + targetName + " */ = {");
                contents.AppendLine("\t\t\tisa = PBXLegacyTarget;");
                contents.AppendLine("\t\t\tbuildArgumentsString = \"$(ACTION) --build --log --mutex --arch=$(BUILD_ARCH) --configuration=$(BUILD_CONFIGURATION) --platform=$(BUILD_PLATFORM) --buildTargets=$(BUILD_TARGET)\";");
                contents.AppendLine("\t\t\tbuildConfigurationList = 04FD6341277A01BD000EA5CA /* Build configuration list for PBXLegacyTarget \"" + projectName + "\" */;");
                contents.AppendLine("\t\t\tbuildPhases = (");
                contents.AppendLine("\t\t\t);");
                contents.AppendLine("\t\t\tbuildToolPath = \"" + buildToolCallerPath + "\";");
                contents.AppendLine("\t\t\tbuildWorkingDirectory = \"" + buildToolWorkspace + "\";");
                contents.AppendLine("\t\t\tdependencies = (");
                contents.AppendLine("\t\t\t);");
                contents.AppendLine("\t\t\tname = \"" + targetName + "\";");
                contents.AppendLine("\t\t\tpassBuildSettingsInEnvironment = 1;");
                contents.AppendLine("\t\t\tproductName = \"" + targetName + "\";");
                contents.AppendLine("\t\t};");
                contents.AppendLine("/* End PBXLegacyTarget section */");

                contents.AppendLine("");
                contents.AppendLine("/* Begin PBXProject section */");
                contents.AppendLine("\t\t" + projectId + " /* Project object */ = {");
                contents.AppendLine("\t\t\tisa = PBXProject;");
                contents.AppendLine("\t\t\tattributes = {");
                contents.AppendLine("\t\t\t\tBuildIndependentTargetsInParallel = 1;");
                contents.AppendLine("\t\t\t\tLastUpgradeCheck = 1320;");
                contents.AppendLine("\t\t\t\tTargetAttributes = {");
                contents.AppendLine("\t\t\t\t\t" + targetId + " = {");
                contents.AppendLine("\t\t\t\t\t\tCreatedOnToolsVersion = 13.2.1;");
                contents.AppendLine("\t\t\t\t\t};");
                contents.AppendLine("\t\t\t\t};");
                contents.AppendLine("\t\t\t};");
                contents.AppendLine("\t\t\tbuildConfigurationList = 04FD6348277A0F15000EA5CA /* Build configuration list for PBXProject \"" + projectName + "\" */;");
                contents.AppendLine("\t\t\tcompatibilityVersion = \"Xcode 13.0\";");
                contents.AppendLine("\t\t\tdevelopmentRegion = en;");
                contents.AppendLine("\t\t\thasScannedForEncodings = 0;");
                contents.AppendLine("\t\t\tknownRegions = (");
                contents.AppendLine("\t\t\t\ten,");
                contents.AppendLine("\t\t\t\tBase,");
                contents.AppendLine("\t\t\t);");
                contents.AppendLine("\t\t\tmainGroup = " + groupId + ";");
                contents.AppendLine("\t\t\tprojectDirPath = \"\";");
                contents.AppendLine("\t\t\tprojectRoot = \"\";");
                contents.AppendLine("\t\t\ttargets = (");
                contents.AppendLine("\t\t\t\t" + targetId + " /* " + targetName + " */,");
                contents.AppendLine("\t\t\t);");
                contents.AppendLine("\t\t};");
                contents.AppendLine("/* End PBXProject section */");

                contents.AppendLine("");
                contents.AppendLine("/* Begin XCBuildConfiguration section */");
                var configurations = new Dictionary<string, string>();
                var defaultConfiguration = string.Empty;
                foreach (var project in solution.Projects)
                {
                    if (project.Type == TargetType.NativeCpp)
                    {
                        foreach (var configuration in project.Configurations)
                        {
                            var target = configuration.Target;
                            var name = project.Name + '.' + configuration.Name.Replace('|', '.');
                            var id = GetRandomGuid();
                            configurations.Add(id, name);
                            if (project == solution.MainProject && configuration.Configuration == TargetConfiguration.Development)
                                defaultConfiguration = name;

                            contents.AppendLine("\t\t" + id + " /* " + name + " */ = {");
                            contents.AppendLine("\t\t\tisa = XCBuildConfiguration;");
                            contents.AppendLine("\t\t\tbuildSettings = {");
                            contents.AppendLine("\t\t\t\tBUILD_ARCH = \"" + configuration.ArchitectureName + "\";");
                            contents.AppendLine("\t\t\t\tBUILD_CONFIGURATION = \"" + configuration.ConfigurationName + "\";");
                            contents.AppendLine("\t\t\t\tBUILD_PLATFORM = \"" + configuration.PlatformName + "\";");
                            contents.AppendLine("\t\t\t\tBUILD_TARGET = \"" + configuration.Target + "\";");
                            contents.AppendLine("\t\t\t};");
                            contents.AppendLine("\t\t\tname = " + name + ";");
                            contents.AppendLine("\t\t};");
                        }
                    }
                }
                contents.AppendLine("/* End XCBuildConfiguration section */");

                contents.AppendLine("");
                contents.AppendLine("/* Begin XCConfigurationList section */");
                contents.AppendLine("\t\t04FD6348277A0F15000EA5CA /* Build configuration list for PBXProject \"" + projectName + "\" */ = {");
                contents.AppendLine("\t\t\tisa = XCConfigurationList;");
                contents.AppendLine("\t\t\tbuildConfigurations = (");
                foreach (var e in configurations)
                    contents.AppendLine("\t\t\t\t" + e.Key + " /* " + e.Value + " */,");
                contents.AppendLine("\t\t\t);");
                contents.AppendLine("\t\t\tdefaultConfigurationIsVisible = 0;");
                contents.AppendLine("\t\t\tdefaultConfigurationName = " + defaultConfiguration + ";");
                contents.AppendLine("\t\t};");
                contents.AppendLine("\t\t04FD6341277A01BD000EA5CA /* Build configuration list for PBXLegacyTarget \"" + targetName + "\" */ = {");
                contents.AppendLine("\t\t\tisa = XCConfigurationList;");
                contents.AppendLine("\t\t\tbuildConfigurations = (");
                foreach (var e in configurations)
                    contents.AppendLine("\t\t\t\t" + e.Key + " /* " + e.Value + " */,");
                contents.AppendLine("\t\t\t);");
                contents.AppendLine("\t\t\tdefaultConfigurationIsVisible = 0;");
                contents.AppendLine("\t\t\tdefaultConfigurationName = " + defaultConfiguration + ";");
                contents.AppendLine("\t\t};");
                contents.AppendLine("/* End XCConfigurationList section */");

                contents.AppendLine("\t};");
                contents.AppendLine("\trootObject = " + projectId + " /* Project object */;");
                contents.AppendLine("}");

                Utilities.WriteFileIfChanged(Path.Combine(solution.Path, "project.pbxproj"), contents.ToString());
            }

            // Generate XCode workspace data
            {
                var contents = new StringBuilder();

                contents.AppendLine("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
                contents.AppendLine("<Workspace");
                contents.AppendLine("   version = \"1.0\">");
                contents.AppendLine("   <FileRef");
                contents.AppendLine("      location = \"self:\">");
                contents.AppendLine("   </FileRef>");
                contents.AppendLine("</Workspace>");

                Utilities.WriteFileIfChanged(Path.Combine(projectWorkspacePath, "contents.xcworkspacedata"), contents.ToString());
            }
        }

        private string GetRandomGuid()
        {
            _rand.NextBytes(_randBytes);
            string result = string.Empty;
            for (int i = 0; i < 12; i++)
            {
                result += _randBytes[i].ToString("X2");
            }
            return result;
        }
    }
}
