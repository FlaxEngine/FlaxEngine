// Copyright (c) Wojciech Figat. All rights reserved.

using System.IO;

namespace FlaxEditor.Content.Import
{
    /// <summary>
    /// Folder import entry.
    /// </summary>
    public class FolderImportEntry : ImportFileEntry
    {
        /// <summary>
        /// Flag used to skip showing import settings dialog to used. Can be used for importing assets from code by plugins.
        /// </summary>
        public bool SkipSettingsDialog;

        /// <inheritdoc />
        public FolderImportEntry(ref Request request)
        : base(ref request)
        {
            SkipSettingsDialog = request.SkipSettingsDialog;
        }

        /// <inheritdoc />
        public override bool Import()
        {
            if (!Directory.Exists(ResultUrl))
            {
                Directory.CreateDirectory(ResultUrl);
                var parentPath = Path.GetDirectoryName(ResultUrl);
                var parent = Editor.Instance.ContentDatabase.Find(parentPath);
                if (parent == null)
                {
                    Editor.LogWarning("Failed to find the parent folder for the imported directory.");
                    return true;
                }
                Editor.Instance.ContentDatabase.RefreshFolder(parent, true);
            }
            var target = (ContentFolder)Editor.Instance.ContentDatabase.Find(ResultUrl);

            // Import all sub elements
            var files = Directory.GetFiles(SourceUrl);
            Editor.Instance.ContentImporting.Import(files, target, SkipSettingsDialog);

            // Import all sub dirs
            var folders = Directory.GetDirectories(SourceUrl);
            Editor.Instance.ContentImporting.Import(folders, target, SkipSettingsDialog);

            return false;
        }
    }
}
