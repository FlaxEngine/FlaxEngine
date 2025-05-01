// Copyright (c) Wojciech Figat. All rights reserved.

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
            importing.ImportingQueueBegin += () => FlaxEngine.Scripting.InvokeOnUpdate(OnStart);
            importing.ImportingQueueEnd += () => FlaxEngine.Scripting.InvokeOnUpdate(OnEnd);
            importing.ImportFileBegin += OnImportFileBegin;
        }

        private void OnImportFileBegin(IFileEntryAction importFileEntry)
        {
            string info;
            if (importFileEntry is ImportFileEntry)
                info = string.Format("Importing \'{0}\'", System.IO.Path.GetFileName(importFileEntry.SourceUrl));
            else
                info = string.Format("Creating \'{0}\'", importFileEntry.SourceUrl);
            FlaxEngine.Scripting.InvokeOnUpdate(() =>
            {
                _currentInfo = info;
                var importing = Editor.Instance.ContentImporting;
                var text = string.Format("{0} ({1}/{2})...", _currentInfo, importing.ImportBatchDone, importing.ImportBatchSize);
                OnUpdate(importing.ImportingProgress, text);
            });
        }
    }
}
