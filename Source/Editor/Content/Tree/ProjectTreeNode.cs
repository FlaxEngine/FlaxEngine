// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine.GUI;

namespace FlaxEditor.Content
{
    /// <summary>
    /// Root tree node for the project workspace.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.ContentTreeNode" />
    public sealed class ProjectTreeNode : ContentTreeNode
    {
        /// <summary>
        /// The project/
        /// </summary>
        public readonly ProjectInfo Project;

        /// <summary>
        /// The project content directory.
        /// </summary>
        public MainContentTreeNode Content;

        /// <summary>
        /// The project source code directory.
        /// </summary>
        public MainContentTreeNode Source;

        /// <summary>
        /// Initializes a new instance of the <see cref="ProjectTreeNode"/> class.
        /// </summary>
        /// <param name="project">The project.</param>
        public ProjectTreeNode(ProjectInfo project)
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
            // Move the main game project to the top
            if (Project.Name == Editor.Instance.GameProject.Name)
                return -1;
            return base.Compare(other);
        }
    }
}
