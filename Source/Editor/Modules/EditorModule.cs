// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Modules
{
    /// <summary>
    /// Base class for all Editor modules.
    /// </summary>
    [HideInEditor]
    public abstract class EditorModule
    {
        /// <summary>
        /// Gets the initialization order. Lower first ordering.
        /// </summary>
        public int InitOrder { get; protected set; }

        /// <summary>
        /// Gets the editor object.
        /// </summary>
        public readonly Editor Editor;

        /// <summary>
        /// Gets the editor undo.
        /// </summary>
        public EditorUndo Undo => Editor.Undo;

        /// <summary>
        /// Initializes a new instance of the <see cref="EditorModule"/> class.
        /// </summary>
        /// <param name="editor">The editor.</param>
        protected EditorModule(Editor editor)
        {
            Editor = editor;
        }

        /// <summary>
        /// Called when Editor is startup up. Performs module initialization.
        /// </summary>
        public virtual void OnInit()
        {
        }

        /// <summary>
        /// Called when Editor is ready and will start work.
        /// </summary>
        public virtual void OnEndInit()
        {
        }

        /// <summary>
        /// Called when every Editor update tick.
        /// </summary>
        public virtual void OnUpdate()
        {
        }

        /// <summary>
        /// Called when Editor is closing. Performs module cleanup.
        /// </summary>
        public virtual void OnExit()
        {
        }

        /// <summary>
        /// Called before Editor will enter play mode.
        /// </summary>
        public virtual void OnPlayBeginning()
        {
        }

        /// <summary>
        /// Called when Editor is entering play mode.
        /// </summary>
        public virtual void OnPlayBegin()
        {
        }

        /// <summary>
        /// Called when Editor will leave the play mode.
        /// </summary>
        public virtual void OnPlayEnding()
        {
        }

        /// <summary>
        /// Called when Editor leaves the play mode.
        /// </summary>
        public virtual void OnPlayEnd()
        {
        }
    }
}
