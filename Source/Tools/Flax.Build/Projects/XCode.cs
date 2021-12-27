// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;

namespace Flax.Build.Projects
{
    /// <summary>
    /// Project generator for XCode.
    /// </summary>
    public class XCodeProjectGenerator : ProjectGenerator
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="XCodeProjectGenerator"/> class.
        /// </summary>
        public XCodeProjectGenerator()
        {
        }

        /// <inheritdoc />
        public override string ProjectFileExtension => "xcodeproj";

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
            throw new NotImplementedException("TODO: XCode");
        }

        /// <inheritdoc />
        public override void GenerateSolution(Solution solution)
        {
            throw new NotImplementedException("TODO: XCode");
        }
    }
}
