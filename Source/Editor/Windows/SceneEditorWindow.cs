// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using FlaxEngine.GUI;

namespace FlaxEditor.Windows
{
    /// <summary>
    /// Base class for editor windows dedicated to scene editing.
    /// </summary>
    /// <seealso cref="FlaxEditor.Windows.EditorWindow" />
    public abstract class SceneEditorWindow : EditorWindow
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
            // Setup input actions
            InputActions.Add(options => options.Save, Editor.SaveAll);
            InputActions.Add(options => options.Undo, () =>
            {
                Editor.PerformUndo();
                Focus();
            });
            InputActions.Add(options => options.Redo, () =>
            {
                Editor.PerformRedo();
                Focus();
            });
            InputActions.Add(options => options.Cut, Editor.SceneEditing.Cut);
            InputActions.Add(options => options.Copy, Editor.SceneEditing.Copy);
            InputActions.Add(options => options.Paste, Editor.SceneEditing.Paste);
            InputActions.Add(options => options.Duplicate, Editor.SceneEditing.Duplicate);
            InputActions.Add(options => options.SelectAll, Editor.SceneEditing.SelectAllScenes);
            InputActions.Add(options => options.Delete, Editor.SceneEditing.Delete);
            InputActions.Add(options => options.Search, () => Editor.Windows.SceneWin.Search());
            InputActions.Add(options => options.Play, Editor.Simulation.RequestStartPlay);
            InputActions.Add(options => options.Pause, Editor.Simulation.RequestResumeOrPause);
            InputActions.Add(options => options.StepFrame, Editor.Simulation.RequestPlayOneFrame);
        }
    }
}
