// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.Linq;
using FlaxEngine;

namespace FlaxEditor.CustomEditors
{
    /// <summary>
    /// Synchronizes objects modifications and records undo operations.
    /// Allows to override undo action target objects for the part of the custom editors hierarchy.
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.CustomEditor" />
    [HideInEditor]
    public class SyncPointEditor : CustomEditor
    {
        private object[] _snapshotUndoInternal;

        /// <summary>
        /// The 'is dirty' flag.
        /// </summary>
        protected bool _isDirty;

        /// <summary>
        /// The cached token used by the value setter to support batching Undo actions (eg. for sliders or color pickers).
        /// </summary>
        protected object _setValueToken;

        /// <summary>
        /// Gets the undo objects used to record undo operation changes.
        /// </summary>
        public virtual IEnumerable<object> UndoObjects => Presenter.GetUndoObjects(Presenter);

        /// <summary>
        /// Gets the undo.
        /// </summary>
        public virtual Undo Undo => Presenter.Undo;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            _isDirty = false;
        }

        /// <inheritdoc />
        protected override void Deinitialize()
        {
            EndUndoRecord();

            base.Deinitialize();
        }

        internal override void RefreshInternal()
        {
            bool isDirty = _isDirty;
            _isDirty = false;

            // If any UI control has been modified we should try to record selected objects change
            if (isDirty && Undo != null && Undo.Enabled)
            {
                string actionName = "Edit object(s)";

                // Check if use token
                if (_setValueToken != null)
                {
                    // Check if record start
                    if (_snapshotUndoInternal == null)
                    {
                        // Record undo action start only (end called in EndUndoRecord)
                        _snapshotUndoInternal = UndoObjects.ToArray();
                        Undo.RecordMultiBegin(_snapshotUndoInternal, actionName);
                    }

                    Refresh();
                }
                else
                {
                    // Normal undo action recording
                    using (new UndoMultiBlock(Undo, UndoObjects, actionName))
                        Refresh();
                }
            }
            else
            {
                Refresh();
            }

            if (isDirty)
                OnModified();
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            RefreshRoot();
        }

        /// <summary>
        /// Called when data gets modified by the custom editors.
        /// </summary>
        protected virtual void OnModified()
        {
            var parent = ParentEditor;
            while (parent != null)
            {
                if (parent is SyncPointEditor syncPointEditor)
                {
                    syncPointEditor.OnModified();
                    break;
                }
                parent = parent.ParentEditor;
            }
        }

        /// <inheritdoc />
        protected override bool OnDirty(CustomEditor editor, object value, object token = null)
        {
            // End any active Undo action batching
            if (token != _setValueToken)
                EndUndoRecord();
            _setValueToken = token;

            // Mark as modified and don't pass event further to the higher editors (don't call parent)
            _isDirty = true;
            return true;
        }

        /// <inheritdoc />
        protected override void ClearToken()
        {
            EndUndoRecord();
        }

        /// <summary>
        /// Ends the undo recording if started with custom token (eg. by value slider).
        /// </summary>
        protected void EndUndoRecord()
        {
            if (_snapshotUndoInternal != null)
            {
                Undo.RecordMultiEnd(_snapshotUndoInternal);
                _snapshotUndoInternal = null;
            }

            // Clear token
            _setValueToken = null;
        }
    }
}
