// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.Utilities;

namespace FlaxEditor.States
{
    /// <summary>
    /// Base class for all Editor states.
    /// </summary>
    /// <seealso cref="FlaxEngine.Utilities.State" />
    [HideInEditor]
    public abstract class EditorState : State
    {
        /// <summary>
        /// Gets the editor state machine.
        /// </summary>
        public new EditorStateMachine StateMachine => owner as EditorStateMachine;

        /// <summary>
        /// Gets the editor object.
        /// </summary>
        public readonly Editor Editor;

        /// <summary>
        /// Checks if can edit assets in this state.
        /// </summary>
        public virtual bool CanEditContent => true;

        /// <summary>
        /// Checks if can edit scene in this state
        /// </summary>
        public virtual bool CanEditScene => false;

        /// <summary>
        /// Checks if can use toolbox in this state.
        /// </summary>
        public virtual bool CanUseToolbox => false;

        /// <summary>
        /// Checks if can use undo/redo in this state.
        /// </summary>
        public virtual bool CanUseUndoRedo => false;

        /// <summary>
        /// Checks if can change scene in this state.
        /// </summary>
        public virtual bool CanChangeScene => false;

        /// <summary>
        /// Checks if can enter play mode in this state.
        /// </summary>
        public virtual bool CanEnterPlayMode => false;

        /// <summary>
        /// Checks if can enter recompile scripts in this state.
        /// </summary>
        public virtual bool CanReloadScripts => false;

        /// <summary>
        /// Checks if static is valid for Editor UI calls and other stuff.
        /// </summary>
        public virtual bool IsEditorReady => true;

        /// <summary>
        /// Checks if state requires more device resources (eg. for lightmaps baking or navigation cooking). Can be used to disable other engine and editor features during this state.
        /// </summary>
        public virtual bool IsPerformanceHeavy => false;

        /// <summary>
        /// Gets the state status message for the UI. Returns null if use the default value.
        /// </summary>
        public virtual string Status => null;

        /// <summary>
        /// Initializes a new instance of the <see cref="EditorState"/> class.
        /// </summary>
        /// <param name="editor">The editor.</param>
        protected EditorState(Editor editor)
        {
            Editor = editor;
        }

        /// <summary>
        /// Update state. Called every Editor tick.
        /// </summary>
        public virtual void Update()
        {
        }

        /// <summary>
        /// Updates the Time settings for Update/FixedUpdate/Draw frequency. Can be overriden per-state.
        /// </summary>
        public virtual void UpdateFPS()
        {
            var generalOptions = Editor.Options.Options.General;
            var editorFps = generalOptions.EditorFPS;
            if (!Platform.HasFocus)
            {
                // Drop performance if app has no focus
                Time.DrawFPS = generalOptions.EditorFPSWhenNotFocused;
                Time.UpdateFPS = generalOptions.EditorFPSWhenNotFocused;
            }
            else if (editorFps < 1)
            {
                // Unlimited power!!!
                Time.DrawFPS = 0;
                Time.UpdateFPS = 0;
            }
            else
            {
                // Custom or default value but just don't go too low
                editorFps = Mathf.Max(editorFps, 10);
                Time.DrawFPS = editorFps;
                Time.UpdateFPS = editorFps;
            }

            // Physics is not so much used in edit time
            Time.PhysicsFPS = 30;
        }

        /// <inheritdoc />
        public override bool CanExit(State nextState)
        {
            // TODO: add blocking changing states based on this state properties
            return base.CanExit(nextState);
        }
    }
}
