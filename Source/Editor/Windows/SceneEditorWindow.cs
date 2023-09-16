// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

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
            InputActions.Add(options => options.Play, Editor.Simulation.DelegatePlayOrStopPlayInEditor);
            InputActions.Add(options => options.PlayCurrentScenes, Editor.Simulation.RequestPlayScenesOrStopPlay);
            InputActions.Add(options => options.Pause, Editor.Simulation.RequestResumeOrPause);
            InputActions.Add(options => options.StepFrame, Editor.Simulation.RequestPlayOneFrame);
            InputActions.Add(options => options.CookAndRun, () => Editor.Windows.GameCookerWin.BuildAndRun());
            InputActions.Add(options => options.RunCookedGame, () => Editor.Windows.GameCookerWin.RunCooked());
            InputActions.Add(options => options.BuildScenesData, Editor.BuildScenesOrCancel);
            InputActions.Add(options => options.BakeLightmaps, Editor.BakeLightmapsOrCancel);
            InputActions.Add(options => options.ClearLightmaps, Editor.ClearLightmaps);
            InputActions.Add(options => options.BakeEnvProbes, Editor.BakeAllEnvProbes);
            InputActions.Add(options => options.BuildCSG, Editor.BuildCSG);
            InputActions.Add(options => options.BuildNav, Editor.BuildNavMesh);
            InputActions.Add(options => options.BuildSDF, Editor.BuildAllMeshesSDF);
            InputActions.Add(options => options.TakeScreenshot, Editor.Windows.TakeScreenshot);
            InputActions.Add(options => options.ProfilerWindow, () => Editor.Windows.ProfilerWin.FocusOrShow());
            InputActions.Add(options => options.ProfilerStartStop, () => { Editor.Windows.ProfilerWin.LiveRecording = !Editor.Windows.ProfilerWin.LiveRecording; Editor.UI.AddStatusMessage($"Profiling {(Editor.Windows.ProfilerWin.LiveRecording ? "started" : "stopped")}."); });
            InputActions.Add(options => options.ProfilerClear, () => { Editor.Windows.ProfilerWin.Clear(); Editor.UI.AddStatusMessage($"Profiling results cleared."); });
        }
    }
}
