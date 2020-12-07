// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using FlaxEditor.Content;
using FlaxEditor.Content.Import;

namespace FlaxEditor.Progress.Handlers
{
    /// <summary>
    /// Importing assets progress reporting handler.
    /// </summary>
    /// <seealso cref="FlaxEditor.Progress.ProgressHandler" />
    public sealed class ImportAssetsProgress : ProgressHandler
    {
        private string _currentInfo;

        /// <summary>
        /// Initializes a new instance of the <see cref="ImportAssetsProgress"/> class.
        /// </summary>
        public ImportAssetsProgress()
        {
            var importing = Editor.Instance.ContentImporting;
            importing.ImportingQueueBegin += OnStart;
            importing.ImportingQueueEnd += OnEnd;
            importing.ImportFileBegin += OnImportFileBegin;
        }

        private void OnImportFileBegin(IFileEntryAction importFileEntry)
        {
            if (importFileEntry is ImportFileEntry)
                _currentInfo = string.Format("Importing \'{0}\'", System.IO.Path.GetFileName(importFileEntry.SourceUrl));
            else
                _currentInfo = string.Format("Creating \'{0}\'", importFileEntry.SourceUrl);
            UpdateProgress();
        }

        private void UpdateProgress()
        {
            var importing = Editor.Instance.ContentImporting;
            var info = string.Format("{0} ({1}/{2})...", _currentInfo, importing.ImportBatchDone, importing.ImportBatchSize);
            OnUpdate(importing.ImportingProgress, info);
        }
    }
}
