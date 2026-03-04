// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEngine.GUI;

namespace FlaxEditor.Content
{
    /// <summary>
    /// Root tree node for the project workspace.
    /// </summary>
    /// <seealso cref="ContentFolderTreeNode" />
    public sealed class ProjectFolderTreeNode : ContentFolderTreeNode
    {
        /// <summary>
        /// The project/
        /// </summary>
        public readonly ProjectInfo Project;

        /// <summary>
        /// The project content directory.
        /// </summary>
        public MainContentFolderTreeNode Content;

        /// <summary>
        /// The project source code directory.
        /// </summary>
        public MainContentFolderTreeNode Source;

        /// <summary>
        /// Initializes a new instance of the <see cref="ProjectFolderTreeNode"/> class.
        /// </summary>
        /// <param name="project">The project.</param>
        public ProjectFolderTreeNode(ProjectInfo project)
        : base(null, project.ProjectFolderPath)
        {
            Project = project;
            Folder.FileName = Folder.ShortName = Text = project.Name;
        }

        /// <inheritdoc />
        public override string NavButtonLabel => Project.Name;

        /// <inheritdoc />
        protected override void DoDragDrop()
        {
            // No drag for root nodes
        }

        /// <inheritdoc />
        public override int Compare(Control other)
        {
            if (other is ProjectFolderTreeNode otherProject)
            {
                var gameProject = Editor.Instance.GameProject;
                var engineProject = Editor.Instance.EngineProject;
                bool isGame = Project == gameProject;
                bool isEngine = Project == engineProject;
                bool otherIsGame = otherProject.Project == gameProject;
                bool otherIsEngine = otherProject.Project == engineProject;

                // Main game project at the top
                if (isGame && !otherIsGame)
                    return -1;
                if (!isGame && otherIsGame)
                    return 1;

                // Engine project at the bottom (when distinct)
                if (isEngine && !otherIsEngine)
                    return 1;
                if (!isEngine && otherIsEngine)
                    return -1;

                return string.CompareOrdinal(Project.Name, otherProject.Project.Name);
            }
            return base.Compare(other);
        }
    }
}
