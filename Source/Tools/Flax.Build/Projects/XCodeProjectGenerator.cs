// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
        public override string ProjectFileExtension => "pbxproj";

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
        public override void GenerateProject(Project project)
        {
            Console.WriteLine(project.Path);
            var contents = new StringBuilder();

            contents.AppendLine("// !$*UTF8*$!");
            contents.AppendLine("{");
            contents.AppendLine("\tarchiveVersion = 1;");
            contents.AppendLine("\tclasses = {");
            contents.AppendLine("\t};");
            contents.AppendLine("\tobjectVersion = 46;");
            contents.AppendLine("\tobjects = {");

            contents.AppendLine("\t};");
            contents.AppendLine("\trootObject = " + GetRandomGuid() + " /* Project object */;");
            contents.AppendLine("}");

            Utilities.WriteFileIfChanged(Path.Combine(project.Path), contents.ToString());
        }

        /// <inheritdoc />
        public override void GenerateSolution(Solution solution)
        {
            Directory.CreateDirectory(solution.Path);

            var contents = new StringBuilder();

            contents.AppendLine("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
            contents.AppendLine("<Workspace version=\"1.0\">");
            foreach (var project in solution.Projects)
            {
            }
            contents.AppendLine("</Workspace>");

            Utilities.WriteFileIfChanged(Path.Combine(solution.Path, solution.Name + ".xcworkspace", "contents.xcworkspacedata"), contents.ToString());
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
