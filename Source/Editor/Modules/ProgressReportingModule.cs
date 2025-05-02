// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using FlaxEditor.Progress;
using FlaxEditor.Progress.Handlers;

namespace FlaxEditor.Modules
{
    /// <summary>
    /// Helper module for engine long-operations progress reporting in the editor (eg. files importing, static light baking, etc.).
    /// </summary>
    /// <seealso cref="FlaxEditor.Modules.EditorModule" />
    public sealed class ProgressReportingModule : EditorModule
    {
        private readonly List<ProgressHandler> _handlers = new List<ProgressHandler>(8);

        /// <summary>
        /// The game building progress handler.
        /// </summary>
        public readonly BuildingGameProgress BuildingGame = new BuildingGameProgress();

        /// <summary>
        /// The assets importing progress handler.
        /// </summary>
        public readonly ImportAssetsProgress ImportAssets = new ImportAssetsProgress();

        /// <summary>
        /// The scripts compilation progress handler.
        /// </summary>
        public readonly CompileScriptsProgress CompileScripts = new CompileScriptsProgress();

        /// <summary>
        /// The lightmaps baking progress handler.
        /// </summary>
        public readonly BakeLightmapsProgress BakeLightmaps = new BakeLightmapsProgress();

        /// <summary>
        /// The environment probes baking progress handler.
        /// </summary>
        public readonly BakeEnvProbesProgress BakeEnvProbes = new BakeEnvProbesProgress();

        /// <summary>
        /// The code editor async open progress handler.
        /// </summary>
        public readonly CodeEditorOpenProgress CodeEditorOpen = new CodeEditorOpenProgress();

        /// <summary>
        /// The navigation mesh building progress handler.
        /// </summary>
        public readonly NavMeshBuildingProgress NavMeshBuilding = new NavMeshBuildingProgress();

        /// <summary>
        /// The scripts project files generation progress handler.
        /// </summary>
        public readonly GenerateScriptsProjectFilesProgress GenerateScriptsProjectFiles = new GenerateScriptsProjectFilesProgress();

        /// <summary>
        /// The assets loading progress handler.
        /// </summary>
        public readonly LoadAssetsProgress LoadAssets = new LoadAssetsProgress();

        /// <summary>
        /// Gets the first active handler.
        /// </summary>
        public ProgressHandler FirstActiveHandler => _handlers.FirstOrDefault(x => x.IsActive);

        /// <summary>
        /// Gets a value indicating whether any progress handler is active.
        /// </summary>
        public bool IsAnyActive => FirstActiveHandler != null;

        internal ProgressReportingModule(Editor editor)
        : base(editor)
        {
            InitOrder = 1000;

            // Register common handlers
            RegisterHandler(BuildingGame);
            RegisterHandler(ImportAssets);
            RegisterHandler(CompileScripts);
            RegisterHandler(BakeLightmaps);
            RegisterHandler(BakeEnvProbes);
            RegisterHandler(CodeEditorOpen);
            RegisterHandler(NavMeshBuilding);
            RegisterHandler(GenerateScriptsProjectFiles);
            RegisterHandler(LoadAssets);
        }

        /// <summary>
        /// Registers the handler.
        /// </summary>
        /// <param name="handler">The handler.</param>
        public void RegisterHandler(ProgressHandler handler)
        {
            if (handler == null)
                throw new ArgumentNullException();

            _handlers.Add(handler);

            handler.ProgressStart += HandlerOnProgressStart;
            handler.ProgressChanged += HandlerOnProgressChanged;
            handler.ProgressEnd += HandlerOnProgressEnd;
            handler.ProgressFailed += HandlerOnProgressFail;
        }

        /// <summary>
        /// Unregisters the handler.
        /// </summary>
        /// <param name="handler">The handler.</param>
        public void UnregisterHandler(ProgressHandler handler)
        {
            if (handler == null)
                throw new ArgumentNullException();

            if (_handlers.Remove(handler) == false)
                throw new InvalidOperationException("Cannot unregister not registered handler");

            handler.ProgressStart -= HandlerOnProgressStart;
            handler.ProgressChanged -= HandlerOnProgressChanged;
            handler.ProgressEnd -= HandlerOnProgressEnd;
            handler.ProgressFailed -= HandlerOnProgressFail;
        }

        private void UpdateProgress()
        {
            var activeHandler = FirstActiveHandler;
            bool showProgress = activeHandler != null;
            Editor.UI.ProgressVisible = showProgress;
            if (showProgress)
            {
                Editor.UI.UpdateProgress(activeHandler.InfoText, activeHandler.Progress);
            }
            else
            {
                Editor.UI.UpdateProgress(string.Empty, 0);
                Editor.UI.UpdateStatusBar();
            }
        }

        private void HandlerOnProgressStart(ProgressHandler handler)
        {
            UpdateProgress();
        }

        private void HandlerOnProgressChanged(ProgressHandler handler)
        {
            UpdateProgress();
        }

        private void HandlerOnProgressEnd(ProgressHandler handler)
        {
            UpdateProgress();

            // Flash main window when no stuff to wait for (user could come back to app)
            if (!IsAnyActive)
            {
                Editor.Windows.FlashMainWindow();
            }
        }

        private void HandlerOnProgressFail(ProgressHandler handler, string message)
        {
            UpdateProgress();
            Editor.UI.ProgressFailed(message);
        }
    }
}
