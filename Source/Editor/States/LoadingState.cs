// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.IO;
using FlaxEngine;
using FlaxEngine.Utilities;

namespace FlaxEditor.States
{
    /// <summary>
    /// Editor loading initial state
    /// </summary>
    /// <seealso cref="FlaxEditor.States.EditorState" />
    [HideInEditor]
    public sealed class LoadingState : EditorState
    {
        private bool _loadScriptsFlag;
        private DateTime _loadScriptsDelayTime;
        private bool _compilationFailed;

        /// <inheritdoc />
        public override bool CanEditContent => false;

        /// <inheritdoc />
        public override bool IsEditorReady => false;

        /// <inheritdoc />
        public override bool CanReloadScripts => true;

        /// <inheritdoc />
        public override string Status => "Loading...";

        internal LoadingState(Editor editor)
        : base(editor)
        {
        }

        /// <summary>
        /// Starts the Editor initialization process ending.
        /// </summary>
        /// <param name="skipCompile">True if skip scripts compilation on startup.</param>
        internal void StartInitEnding(bool skipCompile)
        {
            ScriptsBuilder.CompilationEnd += OnCompilationEnd;

            // Check source code has been compiled on start
            if (ScriptsBuilder.CompilationsCount > 0)
            {
                // Check if compilation has been ended
                if (ScriptsBuilder.IsReady)
                {
                    // We assume source code has been compiled before Editor init
                    OnCompilationEnd(true);
                }
            }
            else if (Editor.Options.Options.General.ForceScriptCompilationOnStartup && !skipCompile)
            {
                // Generate project files when Cache is missing or was cleared previously
                var projectFolderPath = Editor.GameProject?.ProjectFolderPath;
                if (!string.IsNullOrEmpty(projectFolderPath) &&
                    (!Directory.Exists(Path.Combine(projectFolderPath, "Cache", "Intermediate")) ||
                     !Directory.Exists(Path.Combine(projectFolderPath, "Cache", "Projects"))))
                {
                    var customArgs = Editor.CodeEditing.SelectedEditor?.GenerateProjectCustomArgs;
                    ScriptsBuilder.GenerateProject(customArgs);
                }

                // Compile scripts before loading any scenes, then we load them and can open scenes
                ScriptsBuilder.Compile();
            }
            else
            {
                // Skip compilation on startup
                OnCompilationEnd(true);
            }
        }

        private void OnCompilationEnd(bool success)
        {
            // Check if compilation success
            if (success)
            {
                // Request loading scripts (we need to do this on main thread)
                _loadScriptsFlag = true;
                _loadScriptsDelayTime = DateTime.Now + TimeSpan.FromMilliseconds(30);
            }
            else
            {
                // Cannot compile user scripts, let's end init but on main thread in Update
                _compilationFailed = true;
            }
        }

        /// <inheritdoc />
        public override void Update()
        {
            // Check flag
            if (_loadScriptsFlag)
            {
                if (DateTime.Now > _loadScriptsDelayTime)
                {
                    _loadScriptsFlag = false;

                    // End init
                    Editor.EndInit();
                }
            }
            else if (_compilationFailed)
            {
                _compilationFailed = false;

                // Compilation failed so just end init
                Editor.EndInit();
            }
        }

        /// <inheritdoc />
        public override void UpdateFPS()
        {
            Time.DrawFPS = 30;
            Time.UpdateFPS = 60;
            Time.PhysicsFPS = 30;
        }

        /// <inheritdoc />
        public override void OnExit(State nextState)
        {
            ScriptsBuilder.CompilationEnd -= OnCompilationEnd;
        }
    }
}
