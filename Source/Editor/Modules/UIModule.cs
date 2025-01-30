// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Collections.Generic;
using FlaxEditor.Gizmo;
using FlaxEditor.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Dialogs;
using FlaxEditor.GUI.Input;
using FlaxEditor.Progress.Handlers;
using FlaxEditor.SceneGraph;
using FlaxEditor.Utilities;
using FlaxEditor.Viewport.Cameras;
using FlaxEditor.Windows;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Json;
using DockHintWindow = FlaxEditor.GUI.Docking.DockHintWindow;
using MasterDockPanel = FlaxEditor.GUI.Docking.MasterDockPanel;
using FlaxEditor.Content.Settings;
using FlaxEditor.Options;

namespace FlaxEditor.Modules
{
    /// <summary>
    /// Manages Editor UI. Especially main window UI.
    /// </summary>
    /// <seealso cref="FlaxEditor.Modules.EditorModule" />
    public sealed class UIModule : EditorModule
    {
        private Label _progressLabel;
        private ProgressBar _progressBar;
        private Button _outputLogButton;
        private List<KeyValuePair<string, DateTime>> _statusMessages;
        private ContentStats _contentStats;
        private bool _progressFailed;

        ContextMenuSingleSelectGroup<int> _numberOfClientsGroup = new ContextMenuSingleSelectGroup<int>();

        private ContextMenuButton _menuFileSaveScenes;
        private ContextMenuButton _menuFileReloadScenes;
        private ContextMenuButton _menuFileCloseScenes;
        private ContextMenuButton _menuFileOpenScriptsProject;
        private ContextMenuButton _menuFileGenerateScriptsProjectFiles;
        private ContextMenuButton _menuFileRecompileScripts;
        private ContextMenuButton _menuFileSaveAll;
        private ContextMenuButton _menuEditUndo;
        private ContextMenuButton _menuEditRedo;
        private ContextMenuButton _menuEditCut;
        private ContextMenuButton _menuEditCopy;
        private ContextMenuButton _menuEditPaste;
        private ContextMenuButton _menuCreateParentForSelectedActors;
        private ContextMenuButton _menuEditDelete;
        private ContextMenuButton _menuEditDuplicate;
        private ContextMenuButton _menuEditSelectAll;
        private ContextMenuButton _menuEditDeselectAll;
        private ContextMenuButton _menuEditFind;
        private ContextMenuButton _menuSceneMoveActorToViewport;
        private ContextMenuButton _menuSceneAlignActorWithViewport;
        private ContextMenuButton _menuSceneAlignViewportWithActor;
        private ContextMenuButton _menuScenePilotActor;
        private ContextMenuButton _menuSceneCreateTerrain;
        private ContextMenuButton _menuGamePlayGame;
        private ContextMenuButton _menuGamePlayCurrentScenes;
        private ContextMenuButton _menuGameStop;
        private ContextMenuButton _menuGamePause;
        private ContextMenuButton _menuGameCookAndRun;
        private ContextMenuButton _menuGameRunCookedGame;
        private ContextMenuButton _menuToolsBuildScenes;
        private ContextMenuButton _menuToolsBakeLightmaps;
        private ContextMenuButton _menuToolsClearLightmaps;
        private ContextMenuButton _menuToolsBakeAllEnvProbes;
        private ContextMenuButton _menuToolsBuildCSGMesh;
        private ContextMenuButton _menuToolsBuildNavMesh;
        private ContextMenuButton _menuToolsBuildAllMeshesSDF;
        private ContextMenuButton _menuToolsCancelBuilding;
        private ContextMenuButton _menuToolsProfilerWindow;
        private ContextMenuButton _menuToolsSetTheCurrentSceneViewAsDefault;
        private ContextMenuButton _menuToolsTakeScreenshot;
        private ContextMenuChildMenu _menuWindowApplyWindowLayout;

        private ToolStripButton _toolStripSaveAll;
        private ToolStripButton _toolStripUndo;
        private ToolStripButton _toolStripRedo;
        private ToolStripButton _toolStripTranslate;
        private ToolStripButton _toolStripRotate;
        private ToolStripButton _toolStripScale;
        private ToolStripButton _toolStripBuildScenes;
        private ToolStripButton _toolStripCook;
        private ToolStripButton _toolStripPlay;
        private ToolStripButton _toolStripPause;
        private ToolStripButton _toolStripStep;

        /// <summary>
        /// The main menu control.
        /// </summary>
        public MainMenu MainMenu;

        /// <summary>
        /// The tool strip control.
        /// </summary>
        public ToolStrip ToolStrip;

        /// <summary>
        /// The master dock panel for all Editor windows.
        /// </summary>
        public MasterDockPanel MasterPanel = new MasterDockPanel();

        /// <summary>
        /// The status strip control.
        /// </summary>
        public StatusBar StatusBar;

        /// <summary>
        /// The visject surface background texture. Cached to be used globally.
        /// </summary>
        public Texture VisjectSurfaceBackground;

        /// <summary>
        /// Gets the File menu.
        /// </summary>
        public MainMenuButton MenuFile { get; private set; }

        /// <summary>
        /// Gets the Edit menu.
        /// </summary>
        public MainMenuButton MenuEdit { get; private set; }

        /// <summary>
        /// Gets the Scene menu.
        /// </summary>
        public MainMenuButton MenuScene { get; private set; }

        /// <summary>
        /// Gets the Game menu.
        /// </summary>
        public MainMenuButton MenuGame { get; private set; }

        /// <summary>
        /// Gets the Tools menu.
        /// </summary>
        public MainMenuButton MenuTools { get; private set; }

        /// <summary>
        /// Gets the Window menu.
        /// </summary>
        public MainMenuButton MenuWindow { get; private set; }

        /// <summary>
        /// Gets the Help menu.
        /// </summary>
        public MainMenuButton MenuHelp { get; private set; }

        /// <summary>
        /// Fired when the main menu short cut keys are updated. Can be used to update plugin short cut keys.
        /// </summary>
        public event Action MainMenuShortcutKeysUpdated;

        internal UIModule(Editor editor)
        : base(editor)
        {
            InitOrder = -90;
            VisjectSurfaceBackground = FlaxEngine.Content.LoadAsyncInternal<Texture>("Editor/VisjectSurface");
            ColorValueBox.ShowPickColorDialog += ShowPickColorDialog;
        }

        /// <summary>
        /// Unchecks toolstrip pause button.
        /// </summary>
        public void UncheckPauseButton()
        {
            if (_toolStripPause != null)
                _toolStripPause.Checked = false;
        }

        /// <summary>
        /// Checks if toolstrip pause button is being checked.
        /// </summary>
        public bool IsPauseButtonChecked => _toolStripPause != null && _toolStripPause.Checked;

        /// <summary>
        /// Updates the toolstrip.
        /// </summary>
        public void UpdateToolstrip()
        {
            if (ToolStrip == null)
                return;

            var undoRedo = Editor.Undo;
            var gizmo = Editor.MainTransformGizmo;
            var state = Editor.StateMachine.CurrentState;
            var canEditScene = state.CanEditScene && Level.IsAnySceneLoaded;
            var canUseUndoRedo = state.CanUseUndoRedo;
            var canEnterPlayMode = state.CanEnterPlayMode && Level.IsAnySceneLoaded;
            var isPlayMode = Editor.StateMachine.IsPlayMode;
            var isDuringBreakpointHang = Editor.Simulation.IsDuringBreakpointHang;

            // Update buttons
            //
            _toolStripSaveAll.Enabled = !isDuringBreakpointHang;
            //
            _toolStripUndo.Enabled = canEditScene && undoRedo.CanUndo && canUseUndoRedo;
            _toolStripRedo.Enabled = canEditScene && undoRedo.CanRedo && canUseUndoRedo;
            //
            var gizmoMode = gizmo.ActiveMode;
            _toolStripTranslate.Checked = gizmoMode == TransformGizmoBase.Mode.Translate;
            _toolStripRotate.Checked = gizmoMode == TransformGizmoBase.Mode.Rotate;
            _toolStripScale.Checked = gizmoMode == TransformGizmoBase.Mode.Scale;
            //
            _toolStripBuildScenes.Enabled = (canEditScene && !isPlayMode) || Editor.StateMachine.BuildingScenesState.IsActive;
            _toolStripBuildScenes.Visible = Editor.Options.Options.General.BuildActions?.Length != 0;
            _toolStripCook.Enabled = Editor.Windows.GameCookerWin.CanBuild(Platform.PlatformType) && !GameCooker.IsRunning;
            //
            var play = _toolStripPlay;
            var pause = _toolStripPause;
            var step = _toolStripStep;
            play.Enabled = canEnterPlayMode;
            if (isDuringBreakpointHang)
            {
                play.Checked = false;
                play.Icon = Editor.Icons.Stop64;
                pause.Enabled = false;
                pause.Checked = true;
                pause.AutoCheck = false;
                step.Enabled = false;
            }
            else if (isPlayMode)
            {
                play.Checked = false;
                play.Icon = Editor.Icons.Stop64;
                pause.Enabled = true;
                pause.Checked = Editor.StateMachine.PlayingState.IsPaused;
                pause.AutoCheck = false;
                step.Enabled = true;
            }
            else
            {
                play.Checked = Editor.Simulation.IsPlayModeRequested;
                play.Icon = Editor.Icons.Play64;
                pause.Enabled = canEnterPlayMode;
                pause.AutoCheck = true;
                step.Enabled = false;
            }
        }

        internal void UpdateMainMenu()
        {
            if (MenuFile == null)
                return;

            var isNotDuringBreakpointHang = !Editor.Simulation.IsDuringBreakpointHang;
            MenuFile.Enabled = isNotDuringBreakpointHang;
            MenuEdit.Enabled = isNotDuringBreakpointHang;
            MenuScene.Enabled = isNotDuringBreakpointHang;
            MenuGame.Enabled = isNotDuringBreakpointHang;
            MenuTools.Enabled = isNotDuringBreakpointHang;
        }

        /// <summary>
        /// Adds the menu button.
        /// </summary>
        /// <param name="group">The group.</param>
        /// <param name="text">The text.</param>
        /// <param name="clicked">The button clicked event.</param>
        /// <returns>The created menu item.</returns>
        public ContextMenuButton AddMenuButton(string group, string text, Action clicked)
        {
            var menuGroup = MainMenu.GetButton(group) ?? MainMenu.AddButton(group);
            return menuGroup.ContextMenu.AddButton(text, clicked);
        }

        /// <summary>
        /// Updates the status bar.
        /// </summary>
        public void UpdateStatusBar()
        {
            if (StatusBar == null)
                return;

            if (ScriptsBuilder.LastCompilationFailed)
            {
                ProgressFailed("Scripts Compilation Failed");
                return;
            }
            var contentStats = FlaxEngine.Content.Stats;

            Color color;
            if (Editor.StateMachine.IsPlayMode)
                color = Style.Current.Statusbar.PlayMode;
            else
                color = Style.Current.BackgroundSelected;

            string text;
            if (_statusMessages != null && _statusMessages.Count != 0)
                text = _statusMessages[0].Key;
            else if (Editor.StateMachine.CurrentState.Status != null)
                text = Editor.StateMachine.CurrentState.Status;
            else if (contentStats.LoadingAssetsCount != 0)
                text = string.Format("Loading {0}/{1}", contentStats.LoadingAssetsCount, contentStats.AssetsCount);
            else
                text = "Ready";

            if (ProgressVisible)
            {
                color = Style.Current.Statusbar.Loading;
            }

            StatusBar.Text = text;
            StatusBar.StatusColor = color;
            _contentStats = contentStats;
        }

        /// <summary>
        /// Adds the status bar message text to be displayed as a notification.
        /// </summary>
        /// <param name="message">The message to display.</param>
        public void AddStatusMessage(string message)
        {
            if (_statusMessages == null)
                _statusMessages = new List<KeyValuePair<string, DateTime>>();
            _statusMessages.Add(new KeyValuePair<string, DateTime>(message, DateTime.Now + TimeSpan.FromSeconds(3.0f)));
            if (_statusMessages.Count == 1)
                UpdateStatusBar();
        }

        internal bool ProgressVisible
        {
            get => _progressLabel?.Parent.Visible ?? false;
            set
            {
                if (_progressLabel != null)
                    _progressLabel.Parent.Visible = value;
            }
        }

        internal void UpdateProgress(string text, float progress)
        {
            if (_progressLabel != null)
                _progressLabel.Text = text;
            if (_progressBar != null)
            {
                if (_progressFailed)
                {
                    ResetProgressFailure();
                }
                _progressBar.Value = progress * 100.0f;
            }
        }

        internal void ProgressFailed(string message)
        {
            _progressFailed = true;
            StatusBar.StatusColor = Style.Current.Statusbar.Failed;
            StatusBar.Text = message;
            _outputLogButton.Visible = true;
        }

        internal void ResetProgressFailure()
        {
            _outputLogButton.Visible = false;
            _progressFailed = false;
            UpdateStatusBar();
        }

        /// <inheritdoc />
        public override void OnInit()
        {
            Editor.Windows.MainWindowClosing += OnMainWindowClosing;
            var mainWindow = Editor.Windows.MainWindow.GUI;

            // Update window background
            mainWindow.BackgroundColor = Style.Current.Background;

            InitSharedMenus();
            InitMainMenu(mainWindow);
            InitToolstrip(mainWindow);
            InitStatusBar(mainWindow);
            InitDockPanel(mainWindow);

            Editor.Options.OptionsChanged += OnOptionsChanged;

            // Add dummy control for drawing the main window borders if using a custom style
#if PLATFORM_WINDOWS
            if (!Editor.Options.Options.Interface.UseNativeWindowSystem)
#endif
            {
                mainWindow.AddChild(new CustomWindowBorderControl
                {
                    Size = Float2.Zero,
                });
            }
        }

        /// <inheritdoc />
        public override void OnUpdate()
        {
            if (_statusMessages != null && _statusMessages.Count > 0 && _statusMessages[0].Value - DateTime.Now < TimeSpan.Zero)
            {
                _statusMessages.RemoveAt(0);
                UpdateStatusBar();
            }
            else if (FlaxEngine.Content.Stats.LoadingAssetsCount != _contentStats.LoadingAssetsCount)
            {
                UpdateStatusBar();
            }
            else if (ProgressVisible)
            {
                UpdateStatusBar();
            }
        }

        private class CustomWindowBorderControl : Control
        {
            /// <inheritdoc />
            public override void Draw()
            {
                var win = RootWindow.Window;
                if (win.IsMaximized)
                    return;

                var color = Editor.Instance.UI.StatusBar.StatusColor;
                var rect = new Rectangle(0.5f, 0.5f, Parent.Width - 1.0f, Parent.Height - 1.0f - StatusBar.DefaultHeight);
                Render2D.DrawLine(rect.UpperLeft, rect.UpperRight, color);
                Render2D.DrawLine(rect.UpperLeft, rect.BottomLeft, color);
                Render2D.DrawLine(rect.UpperRight, rect.BottomRight, color);
            }
        }

        /// <inheritdoc />
        public override void OnEndInit()
        {
            Editor.MainTransformGizmo.ModeChanged += UpdateToolstrip;
            Editor.StateMachine.StateChanged += StateMachineOnStateChanged;
            Editor.Undo.UndoDone += OnUndoEvent;
            Editor.Undo.RedoDone += OnUndoEvent;
            Editor.Undo.ActionDone += OnUndoEvent;
            GameCooker.Event += OnGameCookerEvent;

            UpdateToolstrip();
        }

        private void OnUndoEvent(IUndoAction action)
        {
            UpdateToolstrip();
        }

        private void StateMachineOnStateChanged()
        {
            UpdateToolstrip();
            UpdateStatusBar();
        }

        private void OnGameCookerEvent(GameCooker.EventType type)
        {
            UpdateToolstrip();
        }

        /// <inheritdoc />
        public override void OnExit()
        {
            // Cleanup dock panel hint proxy windows (Flax will destroy them by var but it's better to clear them earlier)
            DockHintWindow.Proxy.Dispose();
        }

        private IColorPickerDialog ShowPickColorDialog(Control targetControl, Color initialValue, ColorValueBox.ColorPickerEvent colorChanged, ColorValueBox.ColorPickerClosedEvent pickerClosed, bool useDynamicEditing)
        {
            var dialog = new ColorPickerDialog(initialValue, colorChanged, pickerClosed, useDynamicEditing);
            dialog.Show(targetControl);

            if (targetControl != null)
            {
                // Place dialog nearby the target control
                var targetControlDesktopCenter = targetControl.PointToScreen(targetControl.Size * 0.5f);
                var desktopSize = Platform.GetMonitorBounds(targetControlDesktopCenter);
                var pos = targetControlDesktopCenter + new Float2(10.0f, -dialog.DialogSize.Y * 0.5f);
                var dialogEnd = pos + dialog.DialogSize;
                var desktopEnd = desktopSize.BottomRight - new Float2(10.0f);
                if (dialogEnd.X >= desktopEnd.X || dialogEnd.Y >= desktopEnd.Y)
                    pos = targetControl.PointToScreen(Float2.Zero) - new Float2(10.0f + dialog.DialogSize.X, dialog.DialogSize.Y);
                var desktopBounds = Platform.VirtualDesktopBounds;
                pos = Float2.Clamp(pos, desktopBounds.UpperLeft, desktopBounds.BottomRight - dialog.DialogSize);
                dialog.RootWindow.Window.Position = pos;

                // Register for context menu (prevent auto-closing context menu when selecting color)
                var c = targetControl;
                while (c != null)
                {
                    if (c is ContextMenuBase cm)
                    {
                        cm.ExternalPopups.Add(dialog.RootWindow?.Window);
                        break;
                    }
                    c = c.Parent;
                }
            }

            return dialog;
        }

        private void InitSharedMenus()
        {
            for (int i = 1; i <= 4; i++)
                _numberOfClientsGroup.AddItem(i.ToString(), i);

            _numberOfClientsGroup.Selected = Editor.Options.Options.Interface.NumberOfGameClientsToLaunch;
            _numberOfClientsGroup.SelectedChanged = value =>
            {
                var options = Editor.Options.Options;
                options.Interface.NumberOfGameClientsToLaunch = value;
                Editor.Options.Apply(options);
            };

            Editor.Options.OptionsChanged += options => { _numberOfClientsGroup.Selected = options.Interface.NumberOfGameClientsToLaunch; };
        }

        private void InitMainMenu(RootControl mainWindow)
        {
            MainMenu = new MainMenu(mainWindow)
            {
                Parent = mainWindow
            };

            var inputOptions = Editor.Options.Options.Input;

            // File
            MenuFile = MainMenu.AddButton("File");
            var cm = MenuFile.ContextMenu;
            cm.VisibleChanged += OnMenuFileShowHide;
            _menuFileSaveAll = cm.AddButton("Save All", inputOptions.Save, Editor.SaveAll);
            _menuFileSaveScenes = cm.AddButton("Save scenes", inputOptions.SaveScenes, Editor.Scene.SaveScenes);
            _menuFileCloseScenes = cm.AddButton("Close scenes", inputOptions.CloseScenes, Editor.Scene.CloseAllScenes);
            _menuFileReloadScenes = cm.AddButton("Reload scenes", Editor.Scene.ReloadScenes);
            cm.AddSeparator();
            _menuFileOpenScriptsProject = cm.AddButton("Open scripts project", inputOptions.OpenScriptsProject, Editor.CodeEditing.OpenSolution);
            _menuFileGenerateScriptsProjectFiles = cm.AddButton("Generate scripts project files", inputOptions.GenerateScriptsProject, Editor.ProgressReporting.GenerateScriptsProjectFiles.RunAsync);
            _menuFileRecompileScripts = cm.AddButton("Recompile scripts", inputOptions.RecompileScripts, ScriptsBuilder.Compile);
            cm.AddSeparator();
            cm.AddButton("Open project...", OpenProject);
            cm.AddButton("Reload project", ReloadProject);
            cm.AddButton("Open project folder", () => FileSystem.ShowFileExplorer(Editor.Instance.GameProject.ProjectFolderPath));
            cm.AddSeparator();
            cm.AddButton("Exit", "Alt+F4", () => Editor.Windows.MainWindow.Close(ClosingReason.User));

            // Edit
            MenuEdit = MainMenu.AddButton("Edit");
            cm = MenuEdit.ContextMenu;
            cm.VisibleChanged += OnMenuEditShowHide;
            _menuEditUndo = cm.AddButton(string.Empty, inputOptions.Undo, Editor.PerformUndo);
            _menuEditRedo = cm.AddButton(string.Empty, inputOptions.Redo, Editor.PerformRedo);
            cm.AddSeparator();
            _menuEditCut = cm.AddButton("Cut", inputOptions.Cut, Editor.SceneEditing.Cut);
            _menuEditCopy = cm.AddButton("Copy", inputOptions.Copy, Editor.SceneEditing.Copy);
            _menuEditPaste = cm.AddButton("Paste", inputOptions.Paste, Editor.SceneEditing.Paste);
            _menuEditDelete = cm.AddButton("Delete", inputOptions.Delete, Editor.SceneEditing.Delete);
            _menuEditDuplicate = cm.AddButton("Duplicate", inputOptions.Duplicate, Editor.SceneEditing.Duplicate);
            cm.AddSeparator();
            _menuEditSelectAll = cm.AddButton("Select all", inputOptions.SelectAll, Editor.SceneEditing.SelectAllScenes);
            _menuEditDeselectAll = cm.AddButton("Deselect all", inputOptions.DeselectAll, Editor.SceneEditing.DeselectAllScenes);
            _menuCreateParentForSelectedActors = cm.AddButton("Parent to new Actor", inputOptions.GroupSelectedActors, Editor.SceneEditing.CreateParentForSelectedActors);
            _menuEditFind = cm.AddButton("Find", inputOptions.Search, Editor.Windows.SceneWin.Search);
            cm.AddSeparator();
            cm.AddButton("Game Settings", () =>
            {
                var item = Editor.ContentDatabase.Find(GameSettings.GameSettingsAssetPath);
                if (item != null)
                    Editor.ContentEditing.Open(item);
            });
            cm.AddButton("Editor Options", () => Editor.Windows.EditorOptionsWin.Show());

            // Scene
            MenuScene = MainMenu.AddButton("Scene");
            cm = MenuScene.ContextMenu;
            cm.VisibleChanged += OnMenuSceneShowHide;
            _menuSceneMoveActorToViewport = cm.AddButton("Move actor to viewport", inputOptions.MoveActorToViewport, MoveActorToViewport);
            _menuSceneAlignActorWithViewport = cm.AddButton("Align actor with viewport", inputOptions.AlignActorWithViewport, AlignActorWithViewport);
            _menuSceneAlignViewportWithActor = cm.AddButton("Align viewport with actor", inputOptions.AlignViewportWithActor, AlignViewportWithActor);
            _menuScenePilotActor = cm.AddButton("Pilot actor", inputOptions.PilotActor, PilotActor);
            cm.AddSeparator();
            _menuSceneCreateTerrain = cm.AddButton("Create terrain", CreateTerrain);

            // Game
            MenuGame = MainMenu.AddButton("Game");
            cm = MenuGame.ContextMenu;
            cm.VisibleChanged += OnMenuGameShowHide;

            _menuGamePlayGame = cm.AddButton("Play Game", inputOptions.Play, Editor.Simulation.RequestPlayGameOrStopPlay);
            _menuGamePlayCurrentScenes = cm.AddButton("Play Current Scenes", inputOptions.PlayCurrentScenes, Editor.Simulation.RequestPlayScenesOrStopPlay);
            _menuGameStop = cm.AddButton("Stop Game", inputOptions.Play, Editor.Simulation.RequestStopPlay);
            _menuGamePause = cm.AddButton("Pause", inputOptions.Pause, Editor.Simulation.RequestPausePlay);

            cm.AddSeparator();
            var numberOfClientsMenu = cm.AddChildMenu("Number of game clients");
            _numberOfClientsGroup.AddItemsToContextMenu(numberOfClientsMenu.ContextMenu);

            cm.AddSeparator();
            _menuGameCookAndRun = cm.AddButton("Cook & Run", inputOptions.CookAndRun, Editor.Windows.GameCookerWin.BuildAndRun);
            _menuGameCookAndRun.LinkTooltip("Runs Game Cooker to build the game for this platform and runs the game after.");
            _menuGameRunCookedGame = cm.AddButton("Run cooked game", inputOptions.RunCookedGame, Editor.Windows.GameCookerWin.RunCooked);
            _menuGameRunCookedGame.LinkTooltip("Runs the game build from the last cooking output. Use 'Cook & Run' or Game Cooker first.");

            // Tools
            MenuTools = MainMenu.AddButton("Tools");
            cm = MenuTools.ContextMenu;
            cm.VisibleChanged += OnMenuToolsShowHide;
            _menuToolsBuildScenes = cm.AddButton("Build scenes data", inputOptions.BuildScenesData, Editor.BuildScenesOrCancel);
            cm.AddSeparator();
            _menuToolsBakeLightmaps = cm.AddButton("Bake lightmaps", inputOptions.BakeLightmaps, Editor.BakeLightmapsOrCancel);
            _menuToolsClearLightmaps = cm.AddButton("Clear lightmaps data", inputOptions.ClearLightmaps, Editor.ClearLightmaps);
            _menuToolsBakeAllEnvProbes = cm.AddButton("Bake all env probes", inputOptions.BakeEnvProbes, Editor.BakeAllEnvProbes);
            _menuToolsBuildCSGMesh = cm.AddButton("Build CSG mesh", inputOptions.BuildCSG, Editor.BuildCSG);
            _menuToolsBuildNavMesh = cm.AddButton("Build Nav Mesh", inputOptions.BuildNav, Editor.BuildNavMesh);
            _menuToolsBuildAllMeshesSDF = cm.AddButton("Build all meshes SDF", inputOptions.BuildSDF, Editor.BuildAllMeshesSDF);
            cm.AddSeparator();
            cm.AddButton("Game Cooker", Editor.Windows.GameCookerWin.FocusOrShow);
            _menuToolsCancelBuilding = cm.AddButton("Cancel building game", () => GameCooker.Cancel());
            cm.AddSeparator();
            _menuToolsProfilerWindow = cm.AddButton("Profiler", inputOptions.ProfilerWindow, () => Editor.Windows.ProfilerWin.FocusOrShow());
            cm.AddSeparator();
            _menuToolsSetTheCurrentSceneViewAsDefault = cm.AddButton("Set current scene view as project default", SetTheCurrentSceneViewAsDefault);
            _menuToolsTakeScreenshot = cm.AddButton("Take screenshot", inputOptions.TakeScreenshot, Editor.Windows.TakeScreenshot);
            cm.AddSeparator();
            cm.AddButton("Plugins", () => Editor.Windows.PluginsWin.Show());

            // Window
            MenuWindow = MainMenu.AddButton("Window");
            cm = MenuWindow.ContextMenu;
            cm.VisibleChanged += OnMenuWindowVisibleChanged;
            cm.AddButton("Content", Editor.Windows.ContentWin.FocusOrShow);
            cm.AddButton("Scene", Editor.Windows.SceneWin.FocusOrShow);
            cm.AddButton("Toolbox", Editor.Windows.ToolboxWin.FocusOrShow);
            cm.AddButton("Properties", Editor.Windows.PropertiesWin.FocusOrShow);
            cm.AddButton("Game", Editor.Windows.GameWin.FocusOrShow);
            cm.AddButton("Editor", Editor.Windows.EditWin.FocusOrShow);
            cm.AddButton("Debug Log", Editor.Windows.DebugLogWin.FocusOrShow);
            cm.AddButton("Output Log", Editor.Windows.OutputLogWin.FocusOrShow);
            cm.AddButton("Graphics Quality", Editor.Windows.GraphicsQualityWin.FocusOrShow);
            cm.AddButton("Game Cooker", Editor.Windows.GameCookerWin.FocusOrShow);
            cm.AddButton("Profiler", inputOptions.ProfilerWindow, Editor.Windows.ProfilerWin.FocusOrShow);
            cm.AddButton("Content Search", Editor.ContentFinding.ShowSearch);
            cm.AddButton("Visual Script Debugger", Editor.Windows.VisualScriptDebuggerWin.FocusOrShow);
            cm.AddSeparator();
            cm.AddButton("Save window layout", Editor.Windows.SaveLayout);
            _menuWindowApplyWindowLayout = cm.AddChildMenu("Window layouts");
            cm.AddButton("Restore default layout", Editor.Windows.LoadDefaultLayout);

            // Help
            MenuHelp = MainMenu.AddButton("Help");
            cm = MenuHelp.ContextMenu;
            cm.AddButton("Discord", () => Platform.OpenUrl(Constants.DiscordUrl));
            cm.AddButton("Documentation", () => Platform.OpenUrl(Constants.DocsUrl));
            cm.AddButton("Report an issue", () => Platform.OpenUrl(Constants.BugTrackerUrl));
            cm.AddSeparator();
            cm.AddButton("Official Website", () => Platform.OpenUrl(Constants.WebsiteUrl));
            cm.AddButton("Facebook Fanpage", () => Platform.OpenUrl(Constants.FacebookUrl));
            cm.AddButton("Youtube Channel", () => Platform.OpenUrl(Constants.YoutubeUrl));
            cm.AddButton("Twitter", () => Platform.OpenUrl(Constants.TwitterUrl));
            cm.AddSeparator();
            cm.AddButton("Information about Flax", () => new AboutDialog().Show());
        }

        private void OnOptionsChanged(EditorOptions options)
        {
            var inputOptions = options.Input;

            _menuFileSaveAll.ShortKeys = inputOptions.Save.ToString();
            _menuFileSaveScenes.ShortKeys = inputOptions.SaveScenes.ToString();
            _menuFileCloseScenes.ShortKeys = inputOptions.CloseScenes.ToString();
            _menuFileOpenScriptsProject.ShortKeys = inputOptions.OpenScriptsProject.ToString();
            _menuFileGenerateScriptsProjectFiles.ShortKeys = inputOptions.GenerateScriptsProject.ToString();
            _menuFileRecompileScripts.ShortKeys = inputOptions.RecompileScripts.ToString();
            _menuEditUndo.ShortKeys = inputOptions.Undo.ToString();
            _menuEditRedo.ShortKeys = inputOptions.Redo.ToString();
            _menuEditCut.ShortKeys = inputOptions.Cut.ToString();
            _menuEditCopy.ShortKeys = inputOptions.Copy.ToString();
            _menuEditDelete.ShortKeys = inputOptions.Delete.ToString();
            _menuEditDuplicate.ShortKeys = inputOptions.Duplicate.ToString();
            _menuEditSelectAll.ShortKeys = inputOptions.SelectAll.ToString();
            _menuEditDeselectAll.ShortKeys = inputOptions.DeselectAll.ToString();
            _menuEditFind.ShortKeys = inputOptions.Search.ToString();
            _menuGamePlayGame.ShortKeys = inputOptions.Play.ToString();
            _menuGamePlayCurrentScenes.ShortKeys = inputOptions.PlayCurrentScenes.ToString();
            _menuGamePause.ShortKeys = inputOptions.Pause.ToString();
            _menuGameStop.ShortKeys = inputOptions.Play.ToString();
            _menuGameCookAndRun.ShortKeys = inputOptions.CookAndRun.ToString();
            _menuGameRunCookedGame.ShortKeys = inputOptions.RunCookedGame.ToString();
            _menuToolsBuildScenes.ShortKeys = inputOptions.BuildScenesData.ToString();
            _menuToolsBakeLightmaps.ShortKeys = inputOptions.BakeLightmaps.ToString();
            _menuToolsClearLightmaps.ShortKeys = inputOptions.ClearLightmaps.ToString();
            _menuToolsBakeAllEnvProbes.ShortKeys = inputOptions.BakeEnvProbes.ToString();
            _menuToolsBuildCSGMesh.ShortKeys = inputOptions.BuildCSG.ToString();
            _menuToolsBuildNavMesh.ShortKeys = inputOptions.BuildNav.ToString();
            _menuToolsBuildAllMeshesSDF.ShortKeys = inputOptions.BuildSDF.ToString();
            _menuToolsProfilerWindow.ShortKeys = inputOptions.ProfilerWindow.ToString();
            _menuToolsTakeScreenshot.ShortKeys = inputOptions.TakeScreenshot.ToString();

            MainMenuShortcutKeysUpdated?.Invoke();

            UpdateToolstrip();
        }

        private void InitToolstrip(RootControl mainWindow)
        {
            var inputOptions = Editor.Options.Options.Input;

            ToolStrip = new ToolStrip(34.0f, MainMenu.Bottom)
            {
                Parent = mainWindow,
            };

            _toolStripSaveAll = ToolStrip.AddButton(Editor.Icons.Save64, Editor.SaveAll).LinkTooltip("Save all", ref inputOptions.Save);
            ToolStrip.AddSeparator();
            _toolStripUndo = ToolStrip.AddButton(Editor.Icons.Undo64, Editor.PerformUndo).LinkTooltip("Undo", ref inputOptions.Undo);
            _toolStripRedo = ToolStrip.AddButton(Editor.Icons.Redo64, Editor.PerformRedo).LinkTooltip("Redo", ref inputOptions.Redo);
            ToolStrip.AddSeparator();
            _toolStripTranslate = ToolStrip.AddButton(Editor.Icons.Translate32, () => Editor.MainTransformGizmo.ActiveMode = TransformGizmoBase.Mode.Translate).LinkTooltip("Change Gizmo tool mode to Translate", ref inputOptions.TranslateMode);
            _toolStripRotate = ToolStrip.AddButton(Editor.Icons.Rotate32, () => Editor.MainTransformGizmo.ActiveMode = TransformGizmoBase.Mode.Rotate).LinkTooltip("Change Gizmo tool mode to Rotate", ref inputOptions.RotateMode);
            _toolStripScale = ToolStrip.AddButton(Editor.Icons.Scale32, () => Editor.MainTransformGizmo.ActiveMode = TransformGizmoBase.Mode.Scale).LinkTooltip("Change Gizmo tool mode to Scale", ref inputOptions.ScaleMode);
            ToolStrip.AddSeparator();

            // Play
            _toolStripPlay = ToolStrip.AddButton(Editor.Icons.Play64, Editor.Simulation.DelegatePlayOrStopPlayInEditor).LinkTooltip("Play In Editor", ref inputOptions.Play);
            _toolStripPlay.ContextMenu = new ContextMenu();
            var playSubMenu = _toolStripPlay.ContextMenu.AddChildMenu("Play button action");
            var playActionGroup = new ContextMenuSingleSelectGroup<InterfaceOptions.PlayAction>();
            playActionGroup.AddItem("Play Game", InterfaceOptions.PlayAction.PlayGame, null, "Launches the game from the First Scene defined in the project settings.");
            playActionGroup.AddItem("Play Scenes", InterfaceOptions.PlayAction.PlayScenes, null, "Launches the game using the scenes currently loaded in the editor.");
            playActionGroup.AddItemsToContextMenu(playSubMenu.ContextMenu);
            playActionGroup.Selected = Editor.Options.Options.Interface.PlayButtonAction;
            playActionGroup.SelectedChanged = SetPlayAction;
            Editor.Options.OptionsChanged += options => { playActionGroup.Selected = options.Interface.PlayButtonAction; };
            var windowModesGroup = new ContextMenuSingleSelectGroup<InterfaceOptions.GameWindowMode>();
            var windowTypeMenu = _toolStripPlay.ContextMenu.AddChildMenu("Game window mode");
            windowModesGroup.AddItem("Docked", InterfaceOptions.GameWindowMode.Docked, null, "Shows the game window docked, inside the editor");
            windowModesGroup.AddItem("Popup", InterfaceOptions.GameWindowMode.PopupWindow, null, "Shows the game window as a popup");
            windowModesGroup.AddItem("Maximized", InterfaceOptions.GameWindowMode.MaximizedWindow, null, "Shows the game window maximized (Same as pressing F11)");
            windowModesGroup.AddItem("Borderless", InterfaceOptions.GameWindowMode.BorderlessWindow, null, "Shows the game window borderless");
            windowModesGroup.AddItemsToContextMenu(windowTypeMenu.ContextMenu);
            windowModesGroup.Selected = Editor.Options.Options.Interface.DefaultGameWindowMode;
            windowModesGroup.SelectedChanged = SetGameWindowMode;
            Editor.Options.OptionsChanged += options => { windowModesGroup.Selected = options.Interface.DefaultGameWindowMode; };

            _toolStripPause = ToolStrip.AddButton(Editor.Icons.Pause64, Editor.Simulation.RequestResumeOrPause).LinkTooltip("Pause/Resume game", ref inputOptions.Pause);
            _toolStripStep = ToolStrip.AddButton(Editor.Icons.Skip64, Editor.Simulation.RequestPlayOneFrame).LinkTooltip("Step one frame in game", ref inputOptions.StepFrame);

            ToolStrip.AddSeparator();

            // Build scenes
            _toolStripBuildScenes = ToolStrip.AddButton(Editor.Icons.Build64, Editor.BuildScenesOrCancel).LinkTooltip("Build scenes data - CSG, navmesh, static lighting, env probes - configurable via Build Actions in editor options", ref inputOptions.BuildScenesData);

            // Cook and run
            _toolStripCook = ToolStrip.AddButton(Editor.Icons.ShipIt64, Editor.Windows.GameCookerWin.BuildAndRun).LinkTooltip("Cook & Run - build game for the current platform and run it locally", ref inputOptions.CookAndRun);
            _toolStripCook.ContextMenu = new ContextMenu();
            _toolStripCook.ContextMenu.AddButton("Run cooked game", Editor.Windows.GameCookerWin.RunCooked);
            _toolStripCook.ContextMenu.AddSeparator();
            var numberOfClientsMenu = _toolStripCook.ContextMenu.AddChildMenu("Number of game clients");
            _numberOfClientsGroup.AddItemsToContextMenu(numberOfClientsMenu.ContextMenu);

            UpdateToolstrip();
        }

        private void InitStatusBar(RootControl mainWindow)
        {
            // Status Bar
            StatusBar = new StatusBar
            {
                Text = "Loading...",
                Parent = mainWindow,
                Offsets = new Margin(0, 0, -StatusBar.DefaultHeight, StatusBar.DefaultHeight),
            };
            // Output log button
            _outputLogButton = new Button()
            {
                AnchorPreset = AnchorPresets.TopLeft,
                Parent = StatusBar,
                Visible = false,
                Text = "",
                Width = 200,
                TooltipText = "Opens or shows the output log window.",
                BackgroundColor = Color.Transparent,
                BorderColor = Color.Transparent,
                BackgroundColorHighlighted = Color.Transparent,
                BackgroundColorSelected = Color.Transparent,
                BorderColorHighlighted = Color.Transparent,
                BorderColorSelected = Color.Transparent,
            };
            _outputLogButton.LocalY -= 2;
            var defaultTextColor = StatusBar.TextColor;
            _outputLogButton.HoverBegin += () => StatusBar.TextColor = Style.Current.BackgroundSelected;
            _outputLogButton.HoverEnd += () => StatusBar.TextColor = defaultTextColor;
            _outputLogButton.Clicked += () => { Editor.Windows.OutputLogWin.FocusOrShow(); };

            // Progress bar with label
            const float progressBarWidth = 120.0f;
            const float progressBarHeight = 18;
            const float progressBarRightMargin = 4;
            const float progressBarLeftMargin = 4;
            var progressPanel = new Panel(ScrollBars.None)
            {
                Visible = false,
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                Parent = StatusBar,
            };
            _progressBar = new ProgressBar
            {
                AnchorPreset = AnchorPresets.MiddleRight,
                Parent = progressPanel,
                Offsets = new Margin(-progressBarWidth - progressBarRightMargin, progressBarWidth, progressBarHeight * -0.5f, progressBarHeight),
            };
            _progressLabel = new Label
            {
                HorizontalAlignment = TextAlignment.Far,
                AnchorPreset = AnchorPresets.HorizontalStretchMiddle,
                Parent = progressPanel,
                Offsets = new Margin(progressBarRightMargin, progressBarWidth + progressBarLeftMargin + progressBarRightMargin, 0, 0)
            };

            UpdateStatusBar();
        }

        private void InitDockPanel(RootControl mainWindow)
        {
            // Dock Panel
            MasterPanel.AnchorPreset = AnchorPresets.StretchAll;
            MasterPanel.Parent = mainWindow;
            MasterPanel.Offsets = new Margin(0, 0, ToolStrip.Bottom, StatusBar.Height);
        }

        private void OpenProject()
        {
            // Ask user to select project file
            if (FileSystem.ShowOpenFileDialog(Editor.Windows.MainWindow, null, "Project files (*.flaxproj)\0*.flaxproj\0All files (*.*)\0*.*\0", false, "Select project file", out var files))
                return;
            if (files != null && files.Length > 0)
            {
                Editor.OpenProject(files[0]);
            }
        }

        private void ReloadProject()
        {
            // Open project, then close it
            Editor.OpenProject(Editor.GameProject.ProjectPath);
        }

        private void OnMenuFileShowHide(Control control)
        {
            if (control.Visible == false)
                return;
            var c = (ContextMenu)control;

            bool hasOpenedScene = Level.IsAnySceneLoaded;

            _menuFileSaveScenes.Enabled = hasOpenedScene;
            _menuFileCloseScenes.Enabled = hasOpenedScene;
            _menuFileReloadScenes.Enabled = hasOpenedScene;
            _menuFileGenerateScriptsProjectFiles.Enabled = !Editor.ProgressReporting.GenerateScriptsProjectFiles.IsActive;

            c.PerformLayout();
        }

        private void OnMenuEditShowHide(Control control)
        {
            if (control.Visible == false)
                return;

            var undoRedo = Editor.Undo;
            var hasSthSelected = Editor.SceneEditing.HasSthSelected;
            var state = Editor.StateMachine.CurrentState;
            var canEditScene = Level.IsAnySceneLoaded && state.CanEditScene;
            var canUseUndoRedo = state.CanUseUndoRedo;

            _menuEditUndo.Enabled = canEditScene && canUseUndoRedo && undoRedo.CanUndo;
            _menuEditUndo.Text = undoRedo.CanUndo ? string.Format("Undo \'{0}\'", undoRedo.FirstUndoName) : "No undo";

            _menuEditRedo.Enabled = canEditScene && canUseUndoRedo && undoRedo.CanRedo;
            _menuEditRedo.Text = undoRedo.CanRedo ? string.Format("Redo \'{0}\'", undoRedo.FirstRedoName) : "No redo";

            _menuEditCut.Enabled = hasSthSelected;
            _menuEditCopy.Enabled = hasSthSelected;
            _menuEditPaste.Enabled = canEditScene;
            _menuCreateParentForSelectedActors.Enabled = canEditScene && hasSthSelected;
            _menuEditDelete.Enabled = hasSthSelected;
            _menuEditDuplicate.Enabled = hasSthSelected;
            _menuEditSelectAll.Enabled = Level.IsAnySceneLoaded;
            _menuEditDeselectAll.Enabled = hasSthSelected;

            control.PerformLayout();
        }

        private void OnMenuSceneShowHide(Control control)
        {
            if (control.Visible == false)
                return;

            var selection = Editor.SceneEditing;
            bool hasActorSelected = selection.HasSthSelected && selection.Selection[0] is ActorNode;
            bool isPilotActorActive = Editor.Windows.EditWin.IsPilotActorActive;

            _menuSceneMoveActorToViewport.Enabled = hasActorSelected;
            _menuSceneAlignActorWithViewport.Enabled = hasActorSelected;
            _menuSceneAlignViewportWithActor.Enabled = hasActorSelected;
            _menuScenePilotActor.Enabled = hasActorSelected || isPilotActorActive;
            _menuScenePilotActor.Text = isPilotActorActive ? "Stop piloting actor" : "Pilot actor";
            _menuSceneCreateTerrain.Enabled = Level.IsAnySceneLoaded && Editor.StateMachine.CurrentState.CanEditScene && !Editor.StateMachine.IsPlayMode;

            control.PerformLayout();
        }

        private void OnMenuGameShowHide(Control control)
        {
            if (control.Visible == false)
                return;

            var c = (ContextMenu)control;
            var isPlayMode = Editor.StateMachine.IsPlayMode;
            var canPlay = Level.IsAnySceneLoaded;

            _menuGamePlayGame.Enabled = !isPlayMode && canPlay;
            _menuGamePlayCurrentScenes.Enabled = !isPlayMode && canPlay;
            _menuGameStop.Enabled = isPlayMode && canPlay;
            _menuGamePause.Enabled = isPlayMode && canPlay;

            c.PerformLayout();
        }

        private void OnMenuToolsShowHide(Control control)
        {
            if (control.Visible == false)
                return;

            var c = (ContextMenu)control;
            bool canBakeLightmaps = BakeLightmapsProgress.CanBake;
            bool canEdit = Level.IsAnySceneLoaded && Editor.StateMachine.IsEditMode;
            bool isBakingLightmaps = Editor.ProgressReporting.BakeLightmaps.IsActive;
            bool isBuildingScenes = Editor.StateMachine.BuildingScenesState.IsActive;

            _menuToolsBuildScenes.Enabled = canEdit || isBuildingScenes;
            _menuToolsBuildScenes.Text = isBuildingScenes ? "Cancel building scenes data" : "Build scenes data";
            _menuToolsBakeLightmaps.Enabled = (canEdit && canBakeLightmaps) || isBakingLightmaps;
            _menuToolsBakeLightmaps.Text = isBakingLightmaps ? "Cancel baking lightmaps" : "Bake lightmaps";
            _menuToolsClearLightmaps.Enabled = canEdit;
            _menuToolsBakeAllEnvProbes.Enabled = canEdit;
            _menuToolsBuildAllMeshesSDF.Enabled = canEdit && !isBakingLightmaps;
            _menuToolsBuildCSGMesh.Enabled = canEdit;
            _menuToolsBuildNavMesh.Enabled = canEdit;
            _menuToolsCancelBuilding.Enabled = GameCooker.IsRunning;
            _menuToolsSetTheCurrentSceneViewAsDefault.Enabled = Level.ScenesCount > 0;

            c.PerformLayout();
        }

        private void OnMenuWindowVisibleChanged(Control menu)
        {
            if (!menu.Visible)
                return;

            // Find layout to use
            var searchFolder = StringUtils.CombinePaths(Editor.LocalCachePath, "LayoutsCache");
            if (!Directory.Exists(searchFolder))
                Directory.CreateDirectory(searchFolder);
            var files = Directory.GetFiles(searchFolder, "Layout_*.xml", SearchOption.TopDirectoryOnly);
            var layouts = _menuWindowApplyWindowLayout.ContextMenu;
            layouts.DisposeAllItems();
            for (int i = 0; i < files.Length; i++)
            {
                var file = files[i];
                var name = file.Substring(searchFolder.Length + 8, file.Length - searchFolder.Length - 12);
                var nameCM = layouts.AddChildMenu(name);
                var applyButton = nameCM.ContextMenu.AddButton("Apply", OnApplyLayoutButtonClicked);
                applyButton.TooltipText = "Applies the selected layout.";
                nameCM.ContextMenu.AddButton("Delete", () => File.Delete(file)).TooltipText = "Permanently deletes the selected layout.";
                applyButton.Tag = file;
            }
            _menuWindowApplyWindowLayout.Enabled = files.Length > 0;
        }

        private void OnApplyLayoutButtonClicked(ContextMenuButton button)
        {
            Editor.Windows.LoadLayout((string)button.Tag);
        }

        internal void AlignViewportWithActor()
        {
            var selection = Editor.SceneEditing;
            if (selection.HasSthSelected && selection.Selection[0] is ActorNode node)
            {
                var actor = node.Actor;
                var viewport = Editor.Windows.EditWin.Viewport;
                ((FPSCamera)viewport.ViewportCamera).MoveViewport(actor.Transform);
            }
        }

        internal void MoveActorToViewport()
        {
            var selection = Editor.SceneEditing;
            if (selection.HasSthSelected && selection.Selection[0] is ActorNode node)
            {
                var actor = node.Actor;
                var viewport = Editor.Windows.EditWin.Viewport;
                using (new UndoBlock(Undo, actor, "Move to viewport"))
                {
                    actor.Position = viewport.ViewPosition;
                }
            }
        }

        internal void AlignActorWithViewport()
        {
            var selection = Editor.SceneEditing;
            if (selection.HasSthSelected && selection.Selection[0] is ActorNode node)
            {
                var actor = node.Actor;
                var viewport = Editor.Windows.EditWin.Viewport;
                using (new UndoBlock(Undo, actor, "Align with viewport"))
                {
                    actor.Position = viewport.ViewPosition;
                    actor.Orientation = viewport.ViewOrientation;
                }
            }
        }

        internal void PilotActor()
        {
            if (Editor.Windows.EditWin.IsPilotActorActive)
            {
                Editor.Windows.EditWin.EndPilot();
            }
            else
            {
                var selection = Editor.SceneEditing;
                if (selection.HasSthSelected && selection.Selection[0] is ActorNode node)
                {
                    Editor.Windows.EditWin.PilotActor(node.Actor);
                }
            }
        }

        internal void CreateTerrain()
        {
            new Tools.Terrain.CreateTerrainDialog().Show(Editor.Windows.MainWindow);
        }

        private void SetTheCurrentSceneViewAsDefault()
        {
            var projectInfo = Editor.GameProject;
            projectInfo.DefaultScene = JsonSerializer.GetStringID(Level.Scenes[0].ID);
            projectInfo.DefaultSceneSpawn = Editor.Windows.EditWin.Viewport.ViewRay;
            projectInfo.Save();
        }

        private void SetPlayAction(InterfaceOptions.PlayAction newPlayAction)
        {
            var options = Editor.Options.Options;
            options.Interface.PlayButtonAction = newPlayAction;
            Editor.Options.Apply(options);
        }

        private void SetGameWindowMode(InterfaceOptions.GameWindowMode newGameWindowMode)
        {
            var options = Editor.Options.Options;
            options.Interface.DefaultGameWindowMode = newGameWindowMode;
            Editor.Options.Apply(options);
        }

        private void OnMainWindowClosing()
        {
            // Clear UI references (GUI cannot be used after window closing)
            MainMenu = null;
            ToolStrip = null;
            MasterPanel = null;
            StatusBar = null;
            _progressLabel = null;
            _progressBar = null;
            _statusMessages = null;

            MenuFile = null;
            MenuGame = null;
            MenuEdit = null;
            MenuWindow = null;
            MenuScene = null;
            MenuTools = null;
            MenuHelp = null;
        }
    }
}
