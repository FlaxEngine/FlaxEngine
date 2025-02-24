// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using FlaxEditor.SceneGraph;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows
{
    /// <summary>
    /// Shared interface for scene editing utilities.
    /// </summary>
    public interface ISceneContextWindow
    {
        /// <summary>
        /// Opends popup for renaming selected objects.
        /// </summary>
        void RenameSelection();

        /// <summary>
        /// Focuses selected objects.
        /// </summary>
        void FocusSelection();
    }

    /// <summary>
    /// Base class for editor windows dedicated to scene editing.
    /// </summary>
    /// <seealso cref="FlaxEditor.Windows.EditorWindow" />
    public abstract class SceneEditorWindow : EditorWindow, ISceneContextWindow
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
        public void FocusSelection()
        {
            Editor.Windows.EditWin.Viewport.FocusSelection();
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
