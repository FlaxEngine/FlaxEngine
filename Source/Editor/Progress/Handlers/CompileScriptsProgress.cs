// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEditor.Utilities;

namespace FlaxEditor.Progress.Handlers
{
    /// <summary>
    /// Scripts compilation progress reporting handler.
    /// </summary>
    /// <seealso cref="FlaxEditor.Progress.ProgressHandler" />
    public sealed class CompileScriptsProgress : ProgressHandler
    {
        private SelectionCache _selectionCache;

        /// <summary>
        /// Initializes a new instance of the <see cref="CompileScriptsProgress"/> class.
        /// </summary>
        public CompileScriptsProgress()
        {
            // Link for events
            ScriptsBuilder.CompilationBegin += OnStart;
            ScriptsBuilder.CompilationSuccess += OnEnd;
            ScriptsBuilder.CompilationFailed += OnEnd;
            ScriptsBuilder.CompilationStarted += () => OnUpdate(0.2f, "Compiling scripts...");
            ScriptsBuilder.ScriptsReloadCalled += () => OnUpdate(0.8f, "Reloading scripts...");
            ScriptsBuilder.ScriptsReloadBegin += OnScriptsReloadBegin;
            ScriptsBuilder.ScriptsReloadEnd += OnScriptsReloadEnd;
            ScriptsBuilder.ScriptsReload += OnScriptsReload;
        }

        private void OnScriptsReloadBegin()
        {
            if (_selectionCache == null)
                _selectionCache = new SelectionCache();
            _selectionCache.Cache();

            // Clear references to the user scripts (we gonna reload an assembly)
            Editor.Instance.Scene.ClearRefsToSceneObjects(true);
        }

        private void OnScriptsReload()
        {
            // Clear types cache
            Newtonsoft.Json.JsonSerializer.ClearCache();
        }

        private void OnScriptsReloadEnd()
        {
            _selectionCache.Restore();
        }

        /// <inheritdoc />
        protected override void OnStart()
        {
            base.OnStart();

            OnUpdate(0, "Starting scripts compilation...");
        }
    }
}
