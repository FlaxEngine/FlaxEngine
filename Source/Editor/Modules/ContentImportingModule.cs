// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Threading;
using FlaxEditor.Content;
using FlaxEditor.Content.Create;
using FlaxEditor.Content.Import;
using FlaxEngine;
using Newtonsoft.Json;

namespace FlaxEditor.Modules
{
    /// <summary>
    /// Imports assets and other resources to the project. Provides per asset import settings editing.
    /// </summary>
    /// <seealso cref="FlaxEditor.Modules.EditorModule" />
    public sealed class ContentImportingModule : EditorModule
    {
        // Amount of requests done/total used to calculate importing progress

        private int _importBatchDone;
        private int _importBatchSize;

        // Firstly service is collecting import requests and then performs actual importing in the background.

        private readonly Queue<IFileEntryAction> _importingQueue = new Queue<IFileEntryAction>();
        private readonly List<Request> _requests = new List<Request>();

        private long _workerEndFlag;
        private Thread _workerThread;

        /// <summary>
        /// Gets a value indicating whether this instance is importing assets.
        /// </summary>
        public bool IsImporting => _importBatchSize > 0;

        /// <summary>
        /// Gets the importing assets progress.
        /// </summary>
        public float ImportingProgress => _importBatchSize > 0 ? (float)_importBatchDone / _importBatchSize : 1.0f;

        /// <summary>
        /// Gets the amount of files done in the current import batch.
        /// </summary>
        public float ImportBatchDone => _importBatchDone;

        /// <summary>
        /// Gets the size of the current import batch (imported files + files to import left).
        /// </summary>
        public int ImportBatchSize => _importBatchSize;

        /// <summary>
        /// Occurs when assets importing starts.
        /// </summary>
        public event Action ImportingQueueBegin;

        /// <summary>
        /// Occurs when file is being imported. Can be called on non-main thread.
        /// </summary>
        public event Action<IFileEntryAction> ImportFileBegin;

        /// <summary>
        /// Import file end delegate.
        /// </summary>
        /// <param name="entry">The imported file entry.</param>
        /// <param name="failed">if set to <c>true</c> if importing failed, otherwise false.</param>
        public delegate void ImportFileEndDelegate(IFileEntryAction entry, bool failed);

        /// <summary>
        /// Occurs when file importing end. Can be called on non-main thread.
        /// </summary>
        public event ImportFileEndDelegate ImportFileEnd;

        /// <summary>
        /// Occurs when assets importing ends. Can be called on non-main thread.
        /// </summary>
        public event Action ImportingQueueEnd;

        /// <summary>
        /// On Asset Import delegate.
        /// </summary>
        /// <param name="importState">The current state of the asset's import settings to be used.</param>
        public delegate void OnAssetImportDelegate(ImportSettingsContext importState);

        /// <summary>
        /// Occurs when an asset is about to be imported (before dialog prompt opens) and allows you to modify the settings object.
        /// </summary>
        public event OnAssetImportDelegate OnAssetImport;

        /// <summary>
        /// On Asset Import Complete delegate.
        /// </summary>
        /// <param name="entry">The imported file entry.</param>
        public delegate void OnAssetImportCompleteDelegate(IFileEntryAction entry);

        /// <summary>
        /// Occurs when asset imports successfully.
        /// </summary>
        public event OnAssetImportCompleteDelegate OnAssetImportComplete;

        /// <summary>
        /// Holds the current state of the asset importing settings as they are changed.
        /// </summary>
        public class ImportSettingsContext
        {
            public ImportSettingsContext(bool skipDialog, object settings, ImportFileEntry fileEntry, string inputPath, string outputPath)
            {
                SkipDialog = skipDialog;
                Settings = settings;
                FileEntry = fileEntry;
                InputPath = inputPath;
                OutputPath = outputPath;
            }

            /// <summary>
            /// Specifies if the dialog should be skipped
            /// </summary>
            public bool SkipDialog;

            /// <summary>
            /// The current state of the the settings object for this import.
            /// </summary>
            public object Settings;

            /// <summary>
            /// The file entry for this import.
            /// </summary>
            public ImportFileEntry FileEntry;

            /// <summary>
            /// The import source path for the asset.
            /// </summary>
            public string InputPath { get; private set; }

            /// <summary>
            /// The import output path for the asset.
            /// </summary>
            public string OutputPath { get; private set; }
        }

        /// <inheritdoc />
        internal ContentImportingModule(Editor editor)
        : base(editor)
        {
        }

        /// <summary>
        /// Creates the specified file entry (can show create settings dialog if needed).
        /// </summary>
        /// <param name="entry">The entry.</param>
        public void Create(CreateFileEntry entry)
        {
            if (entry.HasSettings)
            {
                // Use settings dialog
                var dialog = new CreateFilesDialog(entry);
                dialog.Show(Editor.Windows.MainWindow);
            }
            else
            {
                // Use direct creation
                LetThemBeCreatedxD(entry);
            }
        }

        /// <summary>
        /// Shows the dialog for selecting files to import.
        /// </summary>
        /// <param name="targetLocation">The target location.</param>
        public void ShowImportFileDialog(ContentFolder targetLocation)
        {
            // Ask user to select files to import
            if (FileSystem.ShowOpenFileDialog(Editor.Windows.MainWindow, null, "All files (*.*)\0*.*\0", true, "Select files to import", out var files))
                return;
            if (files != null && files.Length > 0)
            {
                Import(files, targetLocation);
            }
        }

        /// <summary>
        /// Reimports the specified <see cref="BinaryAssetItem"/> item.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <param name="settings">The import settings to override.</param>
        /// <param name="skipSettingsDialog">True if skip any popup dialogs showing for import options adjusting. Can be used when importing files from code.</param>
        public void Reimport(BinaryAssetItem item, object settings = null, bool skipSettingsDialog = false)
        {
            if (item != null && !item.GetImportPath(out string importPath))
            {
                // Check if input file is missing
                if (!System.IO.File.Exists(importPath))
                {
                    Editor.LogWarning(string.Format("Cannot reimport asset \'{0}\'. File \'{1}\' does not exist.", item.Path, importPath));
                    if (skipSettingsDialog)
                        return;

                    // Ask user to select new file location
                    var title = string.Format("Please find missing \'{0}\' file for asset \'{1}\'", importPath, item.ShortName);
                    if (FileSystem.ShowOpenFileDialog(Editor.Windows.MainWindow, null, "All files (*.*)\0*.*\0", false, title, out var files))
                        return;
                    if (files != null && files.Length > 0)
                        importPath = files[0];

                    // Validate file path again
                    if (!System.IO.File.Exists(importPath))
                        return;
                }

                Import(importPath, item.Path, true, skipSettingsDialog, settings);
            }
        }

        /// <summary>
        /// Imports the specified files.
        /// </summary>
        /// <param name="files">The files.</param>
        /// <param name="targetLocation">The target location.</param>
        /// <param name="skipSettingsDialog">True if skip any popup dialogs showing for import options adjusting. Can be used when importing files from code.</param>
        public void Import(IEnumerable<string> files, ContentFolder targetLocation, bool skipSettingsDialog = false)
        {
            if (targetLocation == null)
                throw new ArgumentNullException();
            if (files == null)
                return;

            lock (_requests)
            {
                bool skipDialog = skipSettingsDialog;
                foreach (var file in files)
                {
                    Import(file, targetLocation, skipSettingsDialog, null, ref skipDialog);
                }
            }
        }

        /// <summary>
        /// Imports the specified file.
        /// </summary>
        /// <param name="file">The file.</param>
        /// <param name="targetLocation">The target location.</param>
        /// <param name="skipSettingsDialog">True if skip any popup dialogs showing for import options adjusting. Can be used when importing files from code.</param>
        /// <param name="settings">Import settings to override. Use null to skip this value.</param>
        public void Import(string file, ContentFolder targetLocation, bool skipSettingsDialog = false, object settings = null)
        {
            bool skipDialog = skipSettingsDialog;
            Import(file, targetLocation, skipSettingsDialog, settings, ref skipDialog);
        }

        private void Import(string inputPath, ContentFolder targetLocation, bool skipSettingsDialog, object settings, ref bool skipDialog)
        {
            if (targetLocation == null)
                throw new ArgumentNullException();

            var extension = System.IO.Path.GetExtension(inputPath) ?? string.Empty;

            // Check if given file extension is a binary asset (.flax files) and can be imported by the engine
            bool isBuilt = Editor.CanImport(extension, out var outputExtension);
            if (isBuilt)
            {
                outputExtension = '.' + outputExtension;

                if (!targetLocation.CanHaveAssets)
                {
                    // Error
                    Editor.LogWarning(string.Format("Cannot import \'{0}\' to \'{1}\'. The target directory cannot have assets.", inputPath, targetLocation.Node.Path));
                    if (!skipDialog)
                    {
                        skipDialog = true;
                        MessageBox.Show("Target location cannot have assets. Use Content folder for your game assets.", "Cannot import assets", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }
                    return;
                }
            }
            else
            {
                // Preserve file extension (will copy file to the import location)
                outputExtension = extension;

                // Check if can place source files here
                if (!targetLocation.CanHaveScripts && (extension == ".cs" || extension == ".cpp" || extension == ".h"))
                {
                    // Error
                    Editor.LogWarning(string.Format("Cannot import \'{0}\' to \'{1}\'. The target directory cannot have scripts.", inputPath, targetLocation.Node.Path));
                    if (!skipDialog)
                    {
                        skipDialog = true;
                        MessageBox.Show("Target location cannot have scripts. Use Source folder for your game source code.", "Cannot import assets", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }
                    return;
                }
            }

            var shortName = System.IO.Path.GetFileNameWithoutExtension(inputPath);
            var outputPath = System.IO.Path.Combine(targetLocation.Path, shortName + outputExtension);

            Import(inputPath, outputPath, isBuilt, skipSettingsDialog, settings);
        }

        /// <summary>
        /// Imports the specified file to the target destination.
        /// Actual importing is done later after gathering settings from the user via <see cref="ImportFilesDialog"/>.
        /// </summary>
        /// <param name="inputPath">The input path.</param>
        /// <param name="outputPath">The output path.</param>
        /// <param name="isInBuilt">True if use in-built importer (engine backend).</param>
        /// <param name="skipSettingsDialog">True if skip any popup dialogs showing for import options adjusting. Can be used when importing files from code.</param>
        /// <param name="settings">Import settings to override. Use null to skip this value.</param>
        private void Import(string inputPath, string outputPath, bool isInBuilt, bool skipSettingsDialog = false, object settings = null)
        {
            lock (_requests)
            {
                _requests.Add(new Request
                {
                    InputPath = inputPath,
                    OutputPath = outputPath,
                    IsInBuilt = isInBuilt,
                    SkipSettingsDialog = skipSettingsDialog,
                    Settings = settings,
                });
            }
        }

        private void WorkerMain()
        {
            IFileEntryAction entry;
            bool wasLastTickWorking = false;

            while (Interlocked.Read(ref _workerEndFlag) == 0)
            {
                // Try to get entry to process
                lock (_requests)
                {
                    if (_importingQueue.Count > 0)
                        entry = _importingQueue.Dequeue();
                    else
                        entry = null;
                }

                // Check if has any no job
                bool inThisTickWork = entry != null;
                if (inThisTickWork)
                {
                    // Check if begin importing
                    if (!wasLastTickWorking)
                    {
                        _importBatchDone = 0;
                        ImportingQueueBegin?.Invoke();
                    }

                    // Import file
                    bool failed = true;
                    try
                    {
                        ImportFileBegin?.Invoke(entry);
                        failed = entry.Execute();
                    }
                    catch (Exception ex)
                    {
                        Editor.LogWarning(ex);
                    }
                    finally
                    {
                        if (failed)
                        {
                            Editor.LogWarning("Failed to import " + entry.SourceUrl + " to " + entry.ResultUrl);
                        }

                        _importBatchDone++;
                        ImportFileEnd?.Invoke(entry, failed);
                        if (!failed)
                        {
                            try
                            {
                                OnAssetImportComplete?.Invoke(entry);
                            }
                            catch (Exception ex)
                            {
                                Editor.LogWarning("Failed to execution workflow for OnAssetImportComplete:");
                                Editor.LogWarning(ex);
                            }
                        }
                    }
                }
                else
                {
                    // Check if end importing
                    if (wasLastTickWorking)
                    {
                        _importBatchDone = _importBatchSize = 0;
                        ImportingQueueEnd?.Invoke();
                    }

                    // Wait some time
                    Thread.Sleep(100);
                }

                wasLastTickWorking = inThisTickWork;
            }
        }

        internal void LetThemBeImportedxD(List<ImportFileEntry> entries)
        {
            int count = entries.Count;
            if (count > 0)
            {
                lock (_requests)
                {
                    _importBatchSize += count;
                    for (int i = 0; i < count; i++)
                        _importingQueue.Enqueue(entries[i]);
                }

                StartWorker();
            }
        }

        internal void LetThemBeCreatedxD(CreateFileEntry entry)
        {
            lock (_requests)
            {
                _importBatchSize += 1;
                _importingQueue.Enqueue(entry);
            }

            StartWorker();
        }

        private void StartWorker()
        {
            if (_workerThread != null)
                return;

            _workerEndFlag = 0;
            _workerThread = new Thread(WorkerMain)
            {
                Name = "Content Importer",
                Priority = ThreadPriority.Highest
            };
            _workerThread.Start();
        }

        private void EndWorker()
        {
            if (_workerThread == null)
                return;

            Interlocked.Increment(ref _workerEndFlag);
            Thread.Sleep(0);

            _workerThread.Join(1000);
#if !USE_NETCORE
            _workerThread.Abort(); // Deprecated in .NET 7
#endif
            _workerThread = null;
        }

        /// <inheritdoc />
        public override void OnInit()
        {
            ImportFileEntry.RegisterDefaultTypes();
        }

        /// <inheritdoc />
        public override void OnUpdate()
        {
            // Check if has no requests to process
            if (_requests.Count == 0)
                return;

            lock (_requests)
            {
                try
                {
                    // Get entries
                    List<ImportFileEntry> entries = new List<ImportFileEntry>(_requests.Count);
                    bool needSettingsDialog = false;
                    for (int i = 0; i < _requests.Count; i++)
                    {
                        var request = _requests[i];
                        var entry = ImportFileEntry.CreateEntry(ref request);
                        if (entry != null)
                        {
                            // Editor automation hook.
                            ImportSettingsContext importState = new ImportSettingsContext(needSettingsDialog, request.Settings, entry,
                                request.InputPath, request.OutputPath);
                            try
                            {
                                OnAssetImport?.Invoke(importState);
                            }
                            catch (Exception ex)
                            {
                                Editor.LogWarning("Automation workflow failed to run:");
                                Editor.LogWarning(ex);
                            }

                            request.Settings = importState.Settings != null ? importState.Settings : request.Settings;
                            if (request.Settings != null)
                            {
                                // Use overridden settings
                                bool success = entry.TryOverrideSettings(request.Settings);
                                if (!success && importState.Settings != null)
                                {
                                    Editor.LogWarning($"Failed to apply settings created by automation workflow for asset: {request.InputPath}.");
                                }
                            }
                            else
                            {
                                needSettingsDialog |= !request.SkipSettingsDialog && !importState.SkipDialog && entry.HasSettings;
                            }

                            entries.Add(entry);
                        }
                    }
                    _requests.Clear();

                    // Check if need to show importing dialog or can just pass requests
                    if (needSettingsDialog)
                    {
                        var dialog = new ImportFilesDialog(entries);
                        dialog.Show(Editor.Windows.MainWindow);
                        dialog.Focus();
                    }
                    else
                    {
                        LetThemBeImportedxD(entries);
                    }
                }
                catch (Exception ex)
                {
                    // Error
                    Editor.LogWarning(ex);
                    Editor.LogError("Failed to process files import request.");
                }
            }
        }

        /// <inheritdoc />
        public override void OnExit()
        {
            EndWorker();
        }
    }
}
