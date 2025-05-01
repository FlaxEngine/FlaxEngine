// Copyright (c) Wojciech Figat. All rights reserved.

using System.IO;

namespace FlaxEditor.Content
{
    /// <summary>
    /// Content tree node used for main directories.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.ContentTreeNode" />
    public class MainContentTreeNode : ContentTreeNode
    {
        private FileSystemWatcher _watcher;

        /// <inheritdoc />
        public override bool CanDelete => false;

        /// <inheritdoc />
        public override bool CanDuplicate => false;

        /// <summary>
        /// Initializes a new instance of the <see cref="MainContentTreeNode"/> class.
        /// </summary>
        /// <param name="parent">The parent project.</param>
        /// <param name="type">The folder type.</param>
        /// <param name="path">The folder path.</param>
        public MainContentTreeNode(ProjectTreeNode parent, ContentFolderType type, string path)
        : base(parent, type, path)
        {
            _watcher = new FileSystemWatcher(path)
            {
                IncludeSubdirectories = true,
                EnableRaisingEvents = true
            };
            //_watcher.Changed += OnEvent;
            _watcher.Created += OnEvent;
            _watcher.Deleted += OnEvent;
            _watcher.Renamed += OnEvent;
        }

        private void OnEvent(object sender, FileSystemEventArgs e)
        {
            Editor.Instance.ContentDatabase.OnDirectoryEvent(this, e);
        }

        /// <inheritdoc />
        protected override void DoDragDrop()
        {
            // No drag for root nodes
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _watcher.EnableRaisingEvents = false;
            _watcher.Dispose();
            _watcher = null;

            base.OnDestroy();
        }
    }
}
