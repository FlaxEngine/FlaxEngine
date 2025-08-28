// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using FlaxEditor.SceneGraph;
using FlaxEditor.Viewport;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows
{
    /// <summary>
    /// Base class for editor windows dedicated to scene editing.
    /// </summary>
    /// <seealso cref="FlaxEditor.Windows.EditorWindow" />
    public abstract class SceneEditorWindow : EditorWindow, ISceneEditingContext
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="SceneEditorWindow"/> class.
        /// </summary>
        /// <param name="editor">The editor.</param>
        /// <param name="hideOnClose">True if hide window on closing, otherwise it will be destroyed.</param>
        /// <param name="scrollBars">The scroll bars.</param>
        protected SceneEditorWindow(Editor editor, bool hideOnClose, ScrollBars scrollBars)
        : base(editor, hideOnClose, scrollBars)
        {
            FlaxEditor.Utilities.Utils.SetupCommonInputActions(this);
        }

        /// <inheritdoc />
        public void DeleteSelection()
        {
            Editor.SceneEditing.Delete();
        }

        /// <inheritdoc />
        public void FocusSelection()
        {
            Editor.Windows.EditWin.Viewport.FocusSelection();
        }

        /// <inheritdoc />
        public void Spawn(Actor actor, Actor parent = null, int orderInParent = -1, bool autoSelect = true)
        {
            Editor.SceneEditing.Spawn(actor, parent, orderInParent, autoSelect);
        }

        /// <inheritdoc />
        public EditorViewport Viewport => Editor.Windows.EditWin.Viewport;

        /// <inheritdoc />
        public List<SceneGraphNode> Selection => Editor.SceneEditing.Selection;

        /// <inheritdoc />
        public void Select(SceneGraphNode node, bool additive = false)
        {
            Editor.SceneEditing.Select(node, additive);
        }

        /// <inheritdoc />
        public void Deselect(SceneGraphNode node)
        {
            Editor.SceneEditing.Deselect(node);
        }

        /// <inheritdoc />
        public void RenameSelection()
        {
            var selection = Editor.SceneEditing.Selection;
            var selectionCount = selection.Count;

            // Show a window with options to rename multiple actors.
            if (selectionCount > 1)
            {
                var selectedActors = new Actor[selectionCount];

                for (int i = 0; i < selectionCount; i++)
                    if (selection[i] is ActorNode actorNode)
                        selectedActors[i] = actorNode.Actor;

                RenameWindow.Show(selectedActors, Editor);
                return;
            }

            if (selectionCount != 0 && selection[0] is ActorNode actor)
            {
                Editor.SceneEditing.Select(actor);
                var sceneWindow = Editor.Windows.SceneWin;
                actor.TreeNode.StartRenaming(sceneWindow, sceneWindow.SceneTreePanel);
            }
        }
    }
}
