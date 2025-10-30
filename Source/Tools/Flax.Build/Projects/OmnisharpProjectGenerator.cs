// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace Flax.Build.Projects.VisualStudioCode
{
    /// <summary>
    /// Project generator for Omnisharp.
    /// </summary>
    public class OmnisharpProjectGenerator : ProjectGenerator
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="OmnisharpProjectGenerator"/> class.
        /// </summary>
        public OmnisharpProjectGenerator()
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

        /// <inheritdoc />
        public override void GenerateSolution(Solution solution)
        {
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
