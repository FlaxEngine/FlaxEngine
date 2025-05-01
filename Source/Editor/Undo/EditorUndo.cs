// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.History;
using FlaxEditor.Options;
using FlaxEngine;

namespace FlaxEditor
{
    /// <summary>
    /// Implementation of <see cref="Undo"/> customized for the <see cref="Editor"/>.
    /// </summary>
    /// <seealso cref="FlaxEditor.Undo" />
    public class EditorUndo : Undo
    {
        private readonly Editor _editor;

        internal EditorUndo(Editor editor)
        : base(500)
        {
            _editor = editor;

            editor.Options.OptionsChanged += OnOptionsChanged;
        }

        private void OnOptionsChanged(EditorOptions options)
        {
            Capacity = options.General.UndoActionsCapacity;
        }

        /// <inheritdoc />
        public override bool Enabled
        {
            get => _editor.StateMachine.CurrentState.CanUseUndoRedo;
            set => throw new AccessViolationException("Cannot change enabled state of the editor main undo.");
        }

        /// <inheritdoc />
        protected override void OnAction(IUndoAction action)
        {
            CheckSceneEdited(action);
            base.OnAction(action);
        }

        /// <inheritdoc />
        protected override void OnUndo(IUndoAction action)
        {
            CheckSceneEdited(action);
            base.OnUndo(action);
        }

        /// <inheritdoc />
        protected override void OnRedo(IUndoAction action)
        {
            CheckSceneEdited(action);
            base.OnRedo(action);
        }

        /// <summary>
        /// Checks if the any scene has been edited after performing the given action.
        /// </summary>
        /// <param name="action">The action.</param>
        private void CheckSceneEdited(IUndoAction action)
        {
            // Note: this is automatic tracking system to check if undo action modifies scene objects

            // Skip if all scenes are already modified
            if (Editor.Instance.Scene.IsEverySceneEdited())
                return;

            // ReSharper disable once SuspiciousTypeConversion.Global
            if (action is ISceneEditAction sceneEditAction)
            {
                sceneEditAction.MarkSceneEdited(Editor.Instance.Scene);
            }
            else if (action is UndoActionObject undoActionObject)
            {
                var data = undoActionObject.PrepareData();

                if (data.TargetInstance is SceneGraph.SceneGraphNode node)
                {
                    Editor.Instance.Scene.MarkSceneEdited(node.ParentScene);
                }
                else if (data.TargetInstance is Actor actor)
                {
                    Editor.Instance.Scene.MarkSceneEdited(actor.Scene);
                }
                else if (data.TargetInstance is Script script && script.Actor != null)
                {
                    Editor.Instance.Scene.MarkSceneEdited(script.Actor.Scene);
                }
            }
            else if (action is MultiUndoAction multiUndoAction)
            {
                // Process child actions
                for (int i = 0; i < multiUndoAction.Actions.Length; i++)
                {
                    CheckSceneEdited(multiUndoAction.Actions[i]);
                }
            }
        }
    }
}
