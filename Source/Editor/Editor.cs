// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.Marshalling;
using System.Threading.Tasks;
using FlaxEditor.Content;
using FlaxEditor.Content.Settings;
using FlaxEditor.Content.Thumbnails;
using FlaxEditor.Modules;
using FlaxEditor.Modules.SourceCodeEditing;
using FlaxEditor.Options;
using FlaxEditor.SceneGraph.Actors;
using FlaxEditor.States;
using FlaxEditor.Windows;
using FlaxEditor.Windows.Assets;
using FlaxEngine;
using FlaxEngine.Assertions;
using FlaxEngine.GUI;
using FlaxEngine.Interop;
using FlaxEngine.Json;

#pragma warning disable CS1591

namespace FlaxEditor
{
    /// <summary>
    /// The main managed editor class. Editor root object.
    /// </summary>
    public sealed partial class Editor
    {
        /// <summary>
        /// Gets the Editor instance.
        /// </summary>
        public static Editor Instance { get; private set; }

        /// <summary>
        /// The path to the local cache folder shared by all the installed editor instance for a given user (used also by the Flax Launcher).
        /// </summary>
        public static readonly string LocalCachePath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "Flax");

        static Editor()
        {
            JsonSerializer.Settings.Converters.Add(new SceneTreeNodeConverter());
        }

        private readonly List<EditorModule> _modules = new List<EditorModule>(16);
        private bool _isAfterInit, _areModulesInited, _areModulesAfterInitEnd, _isHeadlessMode, _autoExit;
        private string _projectToOpen;
        private float _lastAutoSaveTimer, _autoExitTimeout = 0.1f;
        private Button _saveNowButton;
        private Button _cancelSaveButton;
        private bool _autoSaveNow;
        private Guid _startupSceneCmdLine;

        private const string ProjectDataLastScene = "LastScene";
        private const string ProjectDataLastSceneSpawn = "LastSceneSpawn";

        /// <summary>
        /// Gets a value indicating whether Flax Engine is the best in the world.
        /// </summary>
        public bool IsFlaxEngineTheBest => true;

        /// <summary>
        /// Gets a value indicating whether this Editor is running a dev instance of the engine.
        /// </summary>
        [LibraryImport("FlaxEngine", EntryPoint = "EditorInternal_IsDevInstance", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(StringMarshaller))]
        [return: MarshalAs(UnmanagedType.U1)]
        internal static partial bool IsDevInstance();

        /// <summary>
        /// Gets a value indicating whether this Editor is running as official build (distributed via Flax services).
        /// </summary>
        [LibraryImport("FlaxEngine", EntryPoint = "EditorInternal_IsOfficialBuild", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(StringMarshaller))]
        [return: MarshalAs(UnmanagedType.U1)]
        internal static partial bool IsOfficialBuild();

        [LibraryImport("FlaxEngine", EntryPoint = "EditorInternal_IsPlayMode", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(StringMarshaller))]
        [return: MarshalAs(UnmanagedType.U1)]
        internal static partial bool Internal_IsPlayMode();

        /// <summary>
        /// True if the editor is running now in a play mode. Assigned by the managed editor instance.
        /// </summary>
        public static bool IsPlayMode => Internal_IsPlayMode();

        /// <summary>
        /// Gets the build number of the last editor instance that opened the current project. Value 0 means the undefined version.
        /// </summary>
        public static int LastProjectOpenedEngineBuild => Internal_GetLastProjectOpenedEngineBuild();

        /// <summary>
        /// The windows module.
        /// </summary>
        public WindowsModule Windows;

        /// <summary>
        /// The UI module.
        /// </summary>
        public UIModule UI;

        /// <summary>
        /// The thumbnails module.
        /// </summary>
        public ThumbnailsModule Thumbnails;

        /// <summary>
        /// The simulation module.
        /// </summary>
        public SimulationModule Simulation;

        /// <summary>
        /// The scene module.
        /// </summary>
        public SceneModule Scene;

        /// <summary>
        /// The prefabs module.
        /// </summary>
        public PrefabsModule Prefabs;

        /// <summary>
        /// The scene editing module.
        /// </summary>
        public SceneEditingModule SceneEditing;

        /// <summary>
        /// The progress reporting module.
        /// </summary>
        public ProgressReportingModule ProgressReporting;

        /// <summary>
        /// The content editing module.
        /// </summary>
        public ContentEditingModule ContentEditing;

        /// <summary>
        /// The content database module.
        /// </summary>
        public ContentDatabaseModule ContentDatabase;

        /// <summary>
        /// The content importing module.
        /// </summary>
        public ContentImportingModule ContentImporting;

        /// <summary>
        /// The content finder module.
        /// </summary>
        public ContentFindingModule ContentFinding;

        /// <summary>
        /// The scripts editing.
        /// </summary>
        public CodeEditingModule CodeEditing;

        /// <summary>
        /// The scripts documentation.
        /// </summary>
        public CodeDocsModule CodeDocs;

        /// <summary>
        /// The editor state machine.
        /// </summary>
        public EditorStateMachine StateMachine;

        /// <summary>
        /// The editor options manager.
        /// </summary>
        public OptionsModule Options;

        /// <summary>
        /// The editor per-project cache manager.
        /// </summary>
        public ProjectCacheModule ProjectCache;

        /// <summary>
        /// The undo/redo.
        /// </summary>
        public EditorUndo Undo;

        /// <summary>
        /// The icons container.
        /// </summary>
        public EditorIcons Icons;

        /// <summary>
        /// Gets the main transform gizmo used by the <see cref="SceneEditorWindow"/>.
        /// </summary>
        public Gizmo.TransformGizmo MainTransformGizmo => Windows.EditWin.Viewport.TransformGizmo;

        /// <summary>
        /// Gets a value indicating whether this Editor is running in `headless` mode. No windows or popups should be shown. Used in CL environment (without a graphical user interface).
        /// </summary>
        public bool IsHeadlessMode => _isHeadlessMode;

        /// <summary>
        /// Gets a value indicating whether Editor instance is initialized.
        /// </summary>
        public bool IsInitialized => _areModulesAfterInitEnd;

        /// <summary>
        /// Occurs when editor initialization starts. All editor modules already received OnInit callback and editor splash screen is visible.
        /// </summary>
        public event Action InitializationStart;

        /// <summary>
        /// Occurs when editor initialization ends. All editor modules already received OnEndInit callback and editor splash screen will be closed.
        /// </summary>
        public event Action InitializationEnd;

        /// <summary>
        /// The custom data container that is stored in Editor instance. Can be used by plugins to store the state during editor session (state is preserved during scripts reloads).
        /// </summary>
        public Dictionary<string, string> CustomData = new Dictionary<string, string>();

        /// <summary>
        /// The game project info.
        /// </summary>
        public ProjectInfo GameProject;

        /// <summary>
        /// The engine project info.
        /// </summary>
        public ProjectInfo EngineProject;

        /// <summary>
        /// Occurs when play mode is beginning (before entering play mode).
        /// </summary>
        public event Action PlayModeBeginning;

        /// <summary>
        /// Occurs when play mode begins (after entering play mode).
        /// </summary>
        public event Action PlayModeBegin;

        /// <summary>
        /// Occurs when play mode is ending (before leaving play mode).
        /// </summary>
        public event Action PlayModeEnding;

        /// <summary>
        /// Occurs when play mode ends (after leaving play mode).
        /// </summary>
        public event Action PlayModeEnd;

        /// <summary>
        /// Fired on Editor update
        /// </summary>
        public event Action EditorUpdate;

        internal Editor()
        {
            Instance = this;
        }

        internal void Init(StartupFlags flags, Guid startupScene)
        {
            Log("Setting up C# Editor...");
            _isHeadlessMode = flags.HasFlag(StartupFlags.Headless);
            _autoExit = flags.HasFlag(StartupFlags.Exit);
            _startupSceneCmdLine = startupScene;

            Profiler.BeginEvent("Projects");
            EngineProject = ProjectInfo.Load(StringUtils.CombinePaths(Globals.StartupFolder, "Flax.flaxproj"));
            GameProject = ProjectInfo.Load(Internal_GetProjectPath());
            Profiler.EndEvent();

            Profiler.BeginEvent("Icons");
            Icons = new EditorIcons();
            Icons.LoadIcons();
            Profiler.EndEvent();

            // Create common editor modules
            Profiler.BeginEvent("Modules");
            RegisterModule(Options = new OptionsModule(this));
            RegisterModule(ProjectCache = new ProjectCacheModule(this));
            RegisterModule(Scene = new SceneModule(this));
            RegisterModule(Windows = new WindowsModule(this));
            RegisterModule(UI = new UIModule(this));
            RegisterModule(Thumbnails = new ThumbnailsModule(this));
            RegisterModule(Simulation = new SimulationModule(this));
            RegisterModule(Prefabs = new PrefabsModule(this));
            RegisterModule(SceneEditing = new SceneEditingModule(this));
            RegisterModule(ContentEditing = new ContentEditingModule(this));
            RegisterModule(ContentDatabase = new ContentDatabaseModule(this));
            RegisterModule(ContentImporting = new ContentImportingModule(this));
            RegisterModule(CodeEditing = new CodeEditingModule(this));
            RegisterModule(CodeDocs = new CodeDocsModule(this));
            RegisterModule(ProgressReporting = new ProgressReportingModule(this));
            RegisterModule(ContentFinding = new ContentFindingModule(this));
            Profiler.EndEvent();

            StateMachine = new EditorStateMachine(this);
            Undo = new EditorUndo(this);

            if (flags.HasFlag(StartupFlags.NewProject))
                InitProject();
            EnsureState<LoadingState>();
            Log("Editor init");
            if (_isHeadlessMode)
                Log("Running in headless mode");

            // Note: we don't sort modules before Init (optimized)
            _modules.Sort((a, b) => a.InitOrder - b.InitOrder);
            _isAfterInit = true;

            // Initialize modules (from front to back)
            for (int i = 0; i < _modules.Count; i++)
            {
                var module = _modules[i];
                Profiler.BeginEvent(module.GetType().Name);
                module.OnInit();
                Profiler.EndEvent();
            }
            _areModulesInited = true;

            // Preload initial scene asset
            try
            {
                var startupSceneMode = Options.Options.General.StartupSceneMode;
                if (startupSceneMode == GeneralOptions.StartupSceneModes.LastOpened && !ProjectCache.HasCustomData(ProjectDataLastScene))
                    startupSceneMode = GeneralOptions.StartupSceneModes.ProjectDefault;
                switch (startupSceneMode)
                {
                case GeneralOptions.StartupSceneModes.ProjectDefault:
                {
                    if (string.IsNullOrEmpty(GameProject.DefaultScene))
                        break;
                    JsonSerializer.ParseID(GameProject.DefaultScene, out var defaultSceneId);
                    Internal_LoadAsset(ref defaultSceneId);
                    break;
                }
                case GeneralOptions.StartupSceneModes.LastOpened:
                {
                    if (ProjectCache.TryGetCustomData(ProjectDataLastScene, out string lastSceneIdName))
                    {
                        var lastScenes = JsonSerializer.Deserialize<Guid[]>(lastSceneIdName);
                        foreach (var scene in lastScenes)
                        {
                            var lastScene = scene;
                            Internal_LoadAsset(ref lastScene);
                        }
                    }
                    break;
                }
                }
            }
            catch (Exception)
            {
                // Ignore errors
            }

            InitializationStart?.Invoke();

            // Start Editor initialization ending phrase (will wait for scripts compilation result)
            StateMachine.LoadingState.StartInitEnding(flags.HasFlag(StartupFlags.SkipCompile));
        }

        internal void RegisterModule(EditorModule module)
        {
            Log("Register Editor module " + module);

            _modules.Add(module);
            if (_isAfterInit)
                _modules.Sort((a, b) => a.InitOrder - b.InitOrder);
            if (_areModulesInited)
                module.OnInit();
            if (_areModulesAfterInitEnd)
                module.OnEndInit();
        }

        internal void EndInit()
        {
            EnsureState<LoadingState>();
            Log("Editor end init");

            // Change state
            StateMachine.GoToState<EditingSceneState>();

            // Initialize modules (from front to back)
            for (int i = 0; i < _modules.Count; i++)
            {
                try
                {
                    var module = _modules[i];
                    Profiler.BeginEvent(module.GetType().Name);
                    module.OnEndInit();
                    Profiler.EndEvent();
                }
                catch (Exception ex)
                {
                    LogWarning(ex);
                    LogError("Failed to initialize editor module " + _modules[i]);
                }
            }
            _areModulesAfterInitEnd = true;
            _lastAutoSaveTimer = Time.UnscaledGameTime;

            InitializationEnd?.Invoke();

            // Close splash and show main window
            CloseSplashScreen();
            Assert.IsNotNull(Windows.MainWindow);
            if (!IsHeadlessMode)
            {
                Windows.MainWindow.Show();
                Windows.MainWindow.Focus();
            }

            // Load scene
            try
            {
                // Scene cmd line argument
                var scene = ContentDatabase.Find(_startupSceneCmdLine);
                if (scene is SceneItem)
                {
                    Editor.Log("Loading scene specified in command line");
                    Scene.OpenScene(_startupSceneCmdLine);
                    return;
                }
                var startupSceneMode = Options.Options.General.StartupSceneMode;
                if (startupSceneMode == GeneralOptions.StartupSceneModes.LastOpened && !ProjectCache.HasCustomData(ProjectDataLastScene))
                {
                    // Fallback to default project scene if nothing saved in the cache
                    startupSceneMode = GeneralOptions.StartupSceneModes.ProjectDefault;
                }
                switch (startupSceneMode)
                {
                case GeneralOptions.StartupSceneModes.ProjectDefault:
                {
                    if (string.IsNullOrEmpty(GameProject.DefaultScene))
                        break;
                    JsonSerializer.ParseID(GameProject.DefaultScene, out var defaultSceneId);
                    var defaultScene = ContentDatabase.Find(defaultSceneId);
                    if (defaultScene is SceneItem)
                    {
                        Editor.Log("Loading default project scene");
                        Scene.OpenScene(defaultSceneId);

                        // Use spawn point
                        Windows.EditWin.Viewport.ViewRay = GameProject.DefaultSceneSpawn;
                    }
                    break;
                }
                case GeneralOptions.StartupSceneModes.LastOpened:
                {
                    if (ProjectCache.TryGetCustomData(ProjectDataLastScene, out string lastSceneIdName))
                    {
                        var lastScenes = JsonSerializer.Deserialize<Guid[]>(lastSceneIdName);
                        foreach (var sceneId in lastScenes)
                        {
                            var lastScene = ContentDatabase.Find(sceneId);
                            if (!(lastScene is SceneItem))
                                continue;

                            Editor.Log($"Loading last opened scene: {lastScene.ShortName}");
                            if (sceneId == lastScenes[0])
                                Scene.OpenScene(sceneId);
                            else
                                Level.LoadSceneAsync(sceneId);
                        }

                        // Restore view
                        if (ProjectCache.TryGetCustomData(ProjectDataLastSceneSpawn, out string lastSceneSpawnName))
                            Windows.EditWin.Viewport.ViewRay = JsonSerializer.Deserialize<Ray>(lastSceneSpawnName);
                    }
                    break;
                }
                }
            }
            catch (Exception)
            {
                // Ignore errors
            }
        }

        internal void Update()
        {
            Profiler.BeginEvent("Editor.Update");

            try
            {
                StateMachine.Update();
                UpdateAutoSave();
                if (_autoExit && StateMachine.CurrentState == StateMachine.EditingSceneState)
                {
                    _autoExitTimeout -= Time.UnscaledGameTime;
                    if (_autoExitTimeout < 0.0f)
                    {
                        Log("Auto exit");
                        Engine.RequestExit(0);
                    }
                }

                if (!StateMachine.IsPlayMode)
                {
                    StateMachine.CurrentState.UpdateFPS();
                }
                
                EditorUpdate?.Invoke();

                // Update modules
                for (int i = 0; i < _modules.Count; i++)
                {
                    _modules[i].OnUpdate();
                }
            }
            catch (Exception ex)
            {
                Debug.LogException(ex);
            }

            Profiler.EndEvent();
        }

        private void UpdateAutoSave()
        {
            string msg = null;
            var options = Options.Options.General;
            var canSave = StateMachine.IsEditMode && options.EnableAutoSave;
            if (canSave)
            {
                var timeSinceLastSave = Time.UnscaledGameTime - _lastAutoSaveTimer;
                var timeToNextSave = options.AutoSaveFrequency * 60.0f - timeSinceLastSave;

                if (timeToNextSave <= 0.0f || _autoSaveNow)
                {
                    Log("Auto save");
                    _lastAutoSaveTimer = Time.UnscaledGameTime;
                    if (options.AutoSaveScenes)
                        Scene.SaveScenes();
                    if (options.AutoSaveContent)
                        SaveContent();

                    _autoSaveNow = false;

                    // Hide save now and cancel save buttons
                    if (_saveNowButton != null && _cancelSaveButton != null)
                    {
                        _saveNowButton.Visible = false;
                        _cancelSaveButton.Visible = false;
                    }
                }
                else if (timeToNextSave <= options.AutoSaveReminderTime)
                {
                    msg = string.Format("Auto save in {0}s...", Mathf.CeilToInt(timeToNextSave));

                    // Create save now and cancel save buttons if needed
                    if (_saveNowButton == null)
                    {
                        _saveNowButton = new Button
                        {
                            Parent = UI.StatusBar,
                            Height = 14,
                            Width = 60,
                            AnchorPreset = AnchorPresets.MiddleLeft,
                            BackgroundColor = Color.Transparent,
                            BorderColor = Color.Transparent,
                            BackgroundColorHighlighted = Color.Transparent,
                            BackgroundColorSelected = Color.Transparent,
                            BorderColorHighlighted = Color.Transparent,
                            Text = "Save Now",
                            TooltipText = "Saves now and restarts the auto save timer."
                        };
                        _saveNowButton.LocalX += 120;
                        _saveNowButton.Clicked += () => _autoSaveNow = true;
                        _saveNowButton.HoverBegin += () => _saveNowButton.TextColor = Style.Current.BackgroundHighlighted;
                        _saveNowButton.HoverEnd += () => _saveNowButton.TextColor = UI.StatusBar.TextColor;
                    }

                    if (_cancelSaveButton == null)
                    {
                        _cancelSaveButton = new Button
                        {
                            Parent = UI.StatusBar,
                            Height = 14,
                            Width = 70,
                            AnchorPreset = AnchorPresets.MiddleLeft,
                            BackgroundColor = Color.Transparent,
                            BorderColor = Color.Transparent,
                            BackgroundColorHighlighted = Color.Transparent,
                            BackgroundColorSelected = Color.Transparent,
                            BorderColorHighlighted = Color.Transparent,
                            Text = "Cancel",
                            TooltipText = "Cancels this auto save."
                        };
                        _cancelSaveButton.LocalX += 180;
                        _cancelSaveButton.Clicked += () =>
                        {
                            Log("Auto save canceled");
                            _saveNowButton.Visible = false;
                            _cancelSaveButton.Visible = false;
                            _lastAutoSaveTimer = Time.UnscaledGameTime; // Reset timer
                        };
                        _cancelSaveButton.HoverBegin += () => _cancelSaveButton.TextColor = Style.Current.BackgroundHighlighted;
                        _cancelSaveButton.HoverEnd += () => _cancelSaveButton.TextColor = UI.StatusBar.TextColor;
                    }

                    // Show save now and cancel save buttons
                    if (!_saveNowButton.Visible || !_cancelSaveButton.Visible)
                    {
                        _saveNowButton.Visible = true;
                        _cancelSaveButton.Visible = true;
                    }
                }
            }
            if (StateMachine.EditingSceneState.AutoSaveStatus != msg)
            {
                StateMachine.EditingSceneState.AutoSaveStatus = msg;
                UI.UpdateStatusBar();
            }

            if (UI?.StatusBar?.Text != null && !UI.StatusBar.Text.Contains("Auto") &&
                _saveNowButton != null && _cancelSaveButton != null &&
                (_saveNowButton.Visible || _cancelSaveButton.Visible))
            {
                _saveNowButton.Visible = false;
                _cancelSaveButton.Visible = false;
            }
        }

        private void InitProject()
        {
            // Initialize empty project with default game configuration
            Log("Initialize new project");
            var project = GameProject;
            var gameSettings = new GameSettings
            {
                ProductName = project.Name,
                CompanyName = project.Company,
            };
            GameSettings.Save(gameSettings);
            GameSettings.Save(new TimeSettings());
            GameSettings.Save(new AudioSettings());
            GameSettings.Save(new PhysicsSettings());
            GameSettings.Save(new LayersAndTagsSettings());
            GameSettings.Save(new InputSettings());
            GameSettings.Save(new GraphicsSettings());
            GameSettings.Save(new NetworkSettings());
            GameSettings.Save(new NavigationSettings());
            GameSettings.Save(new LocalizationSettings());
            GameSettings.Save(new BuildSettings());
            GameSettings.Save(new StreamingSettings());
            GameSettings.Save(new WindowsPlatformSettings());
            GameSettings.Save(new LinuxPlatformSettings());
            GameSettings.Save(new AndroidPlatformSettings());
            GameSettings.Save(new UWPPlatformSettings());
        }

        internal void OnPlayBeginning()
        {
            for (int i = 0; i < _modules.Count; i++)
                _modules[i].OnPlayBeginning();
            PlayModeBeginning?.Invoke();
        }

        internal void OnPlayBegin()
        {
            for (int i = 0; i < _modules.Count; i++)
                _modules[i].OnPlayBegin();
            PlayModeBegin?.Invoke();
        }

        internal void OnPlayEnding()
        {
            FlaxEngine.Networking.NetworkManager.Stop(); // Shutdown any multiplayer from playmode
            PlayModeEnding?.Invoke();
        }

        internal void OnPlayEnd()
        {
            PlayModeEnd?.Invoke();
            for (int i = 0; i < _modules.Count; i++)
                _modules[i].OnPlayEnd();
        }

        internal void Exit()
        {
            Log("Editor exit");

            // Deinitialize Editor Plugins
            foreach (var plugin in PluginManager.EditorPlugins)
            {
                if (plugin is EditorPlugin editorPlugin && editorPlugin._isEditorInitialized)
                {
                    editorPlugin._isEditorInitialized = false;
                    editorPlugin.DeinitializeEditor();
                }
            }

            // Start exit
            StateMachine.GoToState<ClosingState>();

            // Cache last opened scenes
            {
                var lastScenes = Level.Scenes;
                var lastSceneIds = new Guid[lastScenes.Length];
                for (int i = 0; i < lastScenes.Length; i++)
                    lastSceneIds[i] = lastScenes[i].ID;
                var lastSceneSpawn = Windows.EditWin.Viewport.ViewRay;
                ProjectCache.SetCustomData(ProjectDataLastScene, JsonSerializer.Serialize(lastSceneIds));
                ProjectCache.SetCustomData(ProjectDataLastSceneSpawn, JsonSerializer.Serialize(lastSceneSpawn));
            }

            // Cleanup
            Scene.ClearRefsToSceneObjects(true);

            // Release modules (from back to front)
            for (int i = _modules.Count - 1; i >= 0; i--)
            {
                var module = _modules[i];
                Profiler.BeginEvent(module.GetType().Name);
                module.OnExit();
                Profiler.EndEvent();
            }

            // Cleanup
            Undo.Dispose();
            foreach (var cache in Surface.VisjectSurface.NodesCache.Caches.ToArray())
                cache.Clear();
            Instance = null;

            // Invoke new instance if need to open a project
            if (!string.IsNullOrEmpty(_projectToOpen))
            {
                var procSettings = new CreateProcessSettings
                {
                    FileName = Platform.ExecutableFilePath,
                    Arguments = string.Format("-project \"{0}\"", _projectToOpen),
                    ShellExecute = true,
                    WaitForEnd = false,
                    HiddenWindow = false,
                };
                _projectToOpen = null;
                Platform.CreateProcess(ref procSettings);
            }
        }

        /// <summary>
        /// Undo last action.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void PerformUndo()
        {
            Undo.PerformUndo();
        }

        /// <summary>
        /// Redo last action.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void PerformRedo()
        {
            Undo.PerformRedo();
        }

        /// <summary>
        /// Saves all changes (scenes, assets, etc.).
        /// </summary>
        public void SaveAll()
        {
            Windows.SaveCurrentLayout();
            Scene.SaveScenes();
            SaveContent();
            UI.AddStatusMessage("Saved!");
        }

        /// <summary>
        /// Saves all content (assets, etc.).
        /// </summary>
        public void SaveContent()
        {
            for (int i = 0; i < Windows.Windows.Count; i++)
            {
                if (Windows.Windows[i] is AssetEditorWindow win)
                {
                    win.Save();
                }
            }
        }

        /// <summary>
        /// Closes this project with running editor and opens the given project.
        /// </summary>
        /// <param name="projectFilePath">The project file path.</param>
        public void OpenProject(string projectFilePath)
        {
            if (projectFilePath == null || !File.Exists(projectFilePath))
            {
                MessageBox.Show("Missing project");
                return;
            }

            // Cache project path and start editor exit (it will open new instance on valid closing)
            _projectToOpen = StringUtils.NormalizePath(Path.GetDirectoryName(projectFilePath));
            Windows.MainWindow.Close(ClosingReason.User);
        }

        /// <summary>
        /// Ensure that editor is in a given state, otherwise throws <see cref="InvalidStateException"/>.
        /// </summary>
        /// <param name="state">Valid state to check.</param>
        /// <exception cref="InvalidStateException"></exception>
        public void EnsureState(EditorState state)
        {
            if (StateMachine.CurrentState != state)
                throw new InvalidStateException($"Operation cannot be performed in the current editor state. Current: {StateMachine.CurrentState}, Expected: {state}");
        }

        /// <summary>
        /// Ensure that editor is in a state of given type, otherwise throws <see cref="InvalidStateException"/>.
        /// </summary>
        /// <typeparam name="TStateType">The type of the state type.</typeparam>
        public void EnsureState<TStateType>()
        {
            var state = StateMachine.GetState<TStateType>() as EditorState;
            EnsureState(state);
        }

        /// <summary>
        /// Logs the specified message to the log file.
        /// </summary>
        /// <param name="msg">The message.</param>
        [HideInEditor]
        public static void Log(string msg)
        {
            Debug.Logger.LogHandler.LogWrite(LogType.Info, msg);
        }

        /// <summary>
        /// Logs the specified warning message to the log file.
        /// </summary>
        /// <param name="msg">The message.</param>
        [HideInEditor]
        public static void LogWarning(string msg)
        {
            Debug.Logger.LogHandler.LogWrite(LogType.Warning, msg);
        }

        /// <summary>
        /// Logs the specified warning exception to the log file.
        /// </summary>
        /// <param name="ex">The exception.</param>
        [HideInEditor]
        public static void LogWarning(Exception ex)
        {
            LogWarning("Exception: " + ex.Message);
            LogWarning(ex.StackTrace);
        }

        /// <summary>
        /// Logs the specified error message to the log file.
        /// </summary>
        /// <param name="msg">The message.</param>
        [HideInEditor]
        public static void LogError(string msg)
        {
            Debug.Logger.LogHandler.LogWrite(LogType.Error, msg);
        }

        /// <summary>
        /// New asset types allowed to create.
        /// [Deprecated in v1.8]
        /// </summary>
        [Obsolete("Use CreateAsset with named tag instead")]
        public enum NewAssetType
        {
            /// <summary>
            /// The <see cref="FlaxEngine.Material"/>.
            /// </summary>
            Material = 0,

            /// <summary>
            /// The <see cref="FlaxEngine.MaterialInstance"/>.
            /// </summary>
            MaterialInstance = 1,

            /// <summary>
            /// The <see cref="FlaxEngine.CollisionData"/>.
            /// </summary>
            CollisionData = 2,

            /// <summary>
            /// The <see cref="FlaxEngine.AnimationGraph"/>.
            /// </summary>
            AnimationGraph = 3,

            /// <summary>
            /// The <see cref="FlaxEngine.SkeletonMask"/>.
            /// </summary>
            SkeletonMask = 4,

            /// <summary>
            /// The <see cref="FlaxEngine.ParticleEmitter"/>.
            /// </summary>
            ParticleEmitter = 5,

            /// <summary>
            /// The <see cref="FlaxEngine.ParticleSystem"/>.
            /// </summary>
            ParticleSystem = 6,

            /// <summary>
            /// The <see cref="FlaxEngine.SceneAnimation"/>.
            /// </summary>
            SceneAnimation = 7,

            /// <summary>
            /// The <see cref="FlaxEngine.MaterialFunction"/>.
            /// </summary>
            MaterialFunction = 8,

            /// <summary>
            /// The <see cref="FlaxEngine.ParticleEmitterFunction"/>.
            /// </summary>
            ParticleEmitterFunction = 9,

            /// <summary>
            /// The <see cref="FlaxEngine.AnimationGraphFunction"/>.
            /// </summary>
            AnimationGraphFunction = 10,

            /// <summary>
            /// The <see cref="FlaxEngine.Animation"/>.
            /// </summary>
            Animation = 11,

            /// <summary>
            /// The <see cref="FlaxEngine.BehaviorTree"/>.
            /// </summary>
            BehaviorTree = 12,
        }

        /// <summary>
        /// Serializes the given object to json asset.
        /// </summary>
        /// <param name="outputPath">The result asset file path.</param>
        /// <param name="obj">The obj to serialize.</param>
        /// <returns>True if saving failed, otherwise false.</returns>
        public static bool SaveJsonAsset(string outputPath, object obj)
        {
            if (obj == null)
                throw new ArgumentNullException();
            string str = JsonSerializer.Serialize(obj);
            return Internal_SaveJsonAsset(outputPath, str, obj.GetType().FullName);
        }

        /// <summary>
        /// Cooks the mesh collision data and saves it to the asset using <see cref="CollisionData"/> format. action cannot be performed on a main thread.
        /// </summary>
        /// <param name="path">The output asset path.</param>
        /// <param name="type">The collision data type.</param>
        /// <param name="model">The source model.</param>
        /// <param name="modelLodIndex">The source model LOD index.</param>
        /// <param name="materialSlotsMask">The source model material slots mask. One bit per-slot. Can be used to exclude particular material slots from collision cooking.</param>
        /// <param name="convexFlags">The convex mesh generation flags.</param>
        /// <param name="convexVertexLimit">The convex mesh vertex limit. Use values in range [8;255]</param>
        /// <returns>True if failed, otherwise false.</returns>
        public static bool CookMeshCollision(string path, CollisionDataType type, ModelBase model, int modelLodIndex = 0, uint materialSlotsMask = uint.MaxValue, ConvexMeshGenerationFlags convexFlags = ConvexMeshGenerationFlags.None, int convexVertexLimit = 255)
        {
            if (string.IsNullOrEmpty(path))
                throw new ArgumentNullException(nameof(path));
            if (model == null)
                throw new ArgumentNullException(nameof(model));
            if (type == CollisionDataType.None)
                throw new ArgumentException(nameof(type));

            return Internal_CookMeshCollision(path, type, FlaxEngine.Object.GetUnmanagedPtr(model), modelLodIndex, materialSlotsMask, convexFlags, convexVertexLimit);
        }

        /// <summary>
        /// Gets the shader source code (HLSL shader code).
        /// </summary>
        /// <param name="asset">The shader asset.</param>
        /// <returns>The generated source code.</returns>
        public static string GetShaderSourceCode(Shader asset)
        {
            return GetShaderAssetSourceCode(asset);
        }

        /// <summary>
        /// Gets the material shader source code (HLSL shader code).
        /// </summary>
        /// <param name="asset">The material asset.</param>
        /// <returns>The generated source code.</returns>
        public static string GetShaderSourceCode(Material asset)
        {
            return GetShaderAssetSourceCode(asset);
        }

        /// <summary>
        /// Gets the particle emitter GPU simulation shader source code (HLSL shader code).
        /// </summary>
        /// <param name="asset">The particle emitter asset.</param>
        /// <returns>The generated source code.</returns>
        public static string GetShaderSourceCode(ParticleEmitter asset)
        {
            return GetShaderAssetSourceCode(asset);
        }

        private static string GetShaderAssetSourceCode(Asset asset)
        {
            if (asset == null)
                throw new ArgumentNullException(nameof(asset));
            if (asset.WaitForLoaded())
                throw new Exception("Failed to load asset.");
            var source = Internal_GetShaderAssetSourceCode(FlaxEngine.Object.GetUnmanagedPtr(asset));
            if (source == null)
                throw new Exception("Failed to get source code.");
            return source;
        }

        /// <summary>
        /// Gets the actor bounding sphere (including child actors).
        /// </summary>
        /// <param name="actor">The actor.</param>
        /// <param name="sphere">The bounding sphere.</param>
        public static void GetActorEditorSphere(Actor actor, out BoundingSphere sphere)
        {
            if (actor)
            {
                Internal_GetEditorBoxWithChildren(FlaxEngine.Object.GetUnmanagedPtr(actor), out var box);
                BoundingSphere.FromBox(ref box, out sphere);
                sphere.Radius = Math.Max(sphere.Radius, 15.0f);
            }
            else
            {
                sphere = BoundingSphere.Empty;
            }
        }

        /// <summary>
        /// Closes editor splash screen popup window.
        /// </summary>
        public static void CloseSplashScreen()
        {
            Internal_CloseSplashScreen();
        }

        /// <summary>
        /// Creates new asset at the target location.
        /// [Deprecated in v1.8]
        /// </summary>
        /// <param name="type">New asset type.</param>
        /// <param name="outputPath">Output asset path.</param>
        [Obsolete("Use CreateAsset with named tag instead")]
        public static bool CreateAsset(NewAssetType type, string outputPath)
        {
            // [Deprecated on 18.02.2024, expires on 18.02.2025]
            string tag;
            switch (type)
            {
            case NewAssetType.Material:
                tag = "Material";
                break;
            case NewAssetType.MaterialInstance:
                tag = "MaterialInstance";
                break;
            case NewAssetType.CollisionData:
                tag = "CollisionData";
                break;
            case NewAssetType.AnimationGraph:
                tag = "AnimationGraph";
                break;
            case NewAssetType.SkeletonMask:
                tag = "SkeletonMask";
                break;
            case NewAssetType.ParticleEmitter:
                tag = "ParticleEmitter";
                break;
            case NewAssetType.ParticleSystem:
                tag = "ParticleSystem";
                break;
            case NewAssetType.SceneAnimation:
                tag = "SceneAnimation";
                break;
            case NewAssetType.MaterialFunction:
                tag = "MaterialFunction";
                break;
            case NewAssetType.ParticleEmitterFunction:
                tag = "ParticleEmitterFunction";
                break;
            case NewAssetType.AnimationGraphFunction:
                tag = "AnimationGraphFunction";
                break;
            case NewAssetType.Animation:
                tag = "Animation";
                break;
            case NewAssetType.BehaviorTree:
                tag = "BehaviorTree";
                break;
            default: return true;
            }
            return CreateAsset(tag, outputPath);
        }

        /// <summary>
        /// Creates new Visual Script asset at the target location.
        /// </summary>
        /// <param name="outputPath">Output asset path.</param>
        /// <param name="baseTypename">Base class typename.</param>
        public static bool CreateVisualScript(string outputPath, string baseTypename)
        {
            return Internal_CreateVisualScript(outputPath, baseTypename);
        }

        /// <summary>
        /// Checks if can import asset with the given extension.
        /// </summary>
        /// <param name="extension">The file extension.</param>
        /// <param name="outputExtension">The output file extension (flax, json, etc.).</param>
        /// <returns>True if can import files with given extension, otherwise false.</returns>
        public static bool CanImport(string extension, out string outputExtension)
        {
            outputExtension = Internal_CanImport(extension);
            return outputExtension != null;
        }

        /// <summary>
        /// Checks if the given asset can be exported.
        /// </summary>
        /// <param name="path">The asset path (absolute path with an extension).</param>
        /// <returns>True if can export given asset, otherwise false.</returns>
        public static bool CanExport(string path)
        {
            return Internal_CanExport(path);
        }

        /// <summary>
        /// Exports the given asset to the specified file location.
        /// </summary>
        /// <param name="inputPath">The input asset path (absolute path with an extension).</param>
        /// <param name="outputFolder">The output folder path (filename with extension is computed by auto).</param>
        /// <returns>True if given asset has been exported, otherwise false.</returns>
        public static bool Export(string inputPath, string outputFolder)
        {
            return Internal_Export(inputPath, outputFolder);
        }

        /// <summary>
        /// Checks if every managed assembly has been loaded (including user scripts assembly).
        /// </summary>
        public static bool IsEveryAssemblyLoaded => Internal_GetIsEveryAssemblyLoaded();

        #region Env Probes Baking

        /// <summary>
        /// Occurs when environment probe baking starts.
        /// </summary>
        public static event Action<EnvironmentProbe> EnvProbeBakeStart;

        /// <summary>
        /// Occurs when environment probe baking ends.
        /// </summary>
        public static event Action<EnvironmentProbe> EnvProbeBakeEnd;

        internal static void Internal_EnvProbeBake(bool started, EnvironmentProbe probe)
        {
            if (started)
                EnvProbeBakeStart?.Invoke(probe);
            else
                EnvProbeBakeEnd?.Invoke(probe);
        }

        #endregion

        #region Lightmaps Baking

        /// <summary>
        /// Lightmaps baking steps.
        /// </summary>
        public enum LightmapsBakeSteps
        {
            /// <summary>
            /// The service initialize stage.
            /// </summary>
            Initialize,

            /// <summary>
            /// The cache entries stage.
            /// </summary>
            CacheEntries,

            /// <summary>
            /// The generate lightmap charts stage.
            /// </summary>
            GenerateLightmapCharts,

            /// <summary>
            /// The pack lightmap charts stage.
            /// </summary>
            PackLightmapCharts,

            /// <summary>
            /// The update lightmaps collection stage.
            /// </summary>
            UpdateLightmapsCollection,

            /// <summary>
            /// The update entries stage.
            /// </summary>
            UpdateEntries,

            /// <summary>
            /// The generate hemispheres cache stage.
            /// </summary>
            GenerateHemispheresCache,

            /// <summary>
            /// The render hemispheres stage.
            /// </summary>
            RenderHemispheres,

            /// <summary>
            /// The cleanup stage.
            /// </summary>
            Cleanup
        }

        /// <summary>
        /// Lightmaps baking progress event delegate.
        /// </summary>
        /// <param name="step">The current step.</param>
        /// <param name="stepProgress">The current step progress (normalized to [0;1]).</param>
        /// <param name="totalProgress">The total baking progress (normalized to [0;1]).</param>
        public delegate void LightmapsBakeProgressDelegate(LightmapsBakeSteps step, float stepProgress, float totalProgress);

        /// <summary>
        /// Lightmaps baking nd event delegate.
        /// </summary>
        /// <param name="failed">True if baking failed or has been canceled, otherwise false.</param>
        public delegate void LightmapsBakeEndDelegate(bool failed);

        /// <summary>
        /// Occurs when lightmaps baking starts.
        /// </summary>
        public static event Action LightmapsBakeStart;

        /// <summary>
        /// Occurs when lightmaps baking ends.
        /// </summary>
        public static event LightmapsBakeEndDelegate LightmapsBakeEnd;

        /// <summary>
        /// Occurs when lightmaps baking progress changes.
        /// </summary>
        public static event LightmapsBakeProgressDelegate LightmapsBakeProgress;

        internal void Internal_StartLightingBake()
        {
            StateMachine.GoToState<BuildingLightingState>();
        }

        internal static void Internal_LightmapsBake(LightmapsBakeSteps step, float stepProgress, float totalProgress, bool isProgressEvent)
        {
            if (isProgressEvent)
                LightmapsBakeProgress?.Invoke(step, stepProgress, totalProgress);
            else if (step == LightmapsBakeSteps.Initialize)
                LightmapsBakeStart?.Invoke();
            else if (step == LightmapsBakeSteps.GenerateLightmapCharts)
                LightmapsBakeEnd?.Invoke(false);
            else
                LightmapsBakeEnd?.Invoke(true);
        }

        /// <summary>
        /// Starts building scenes data or cancels it if already running.
        /// </summary>
        public void BuildScenesOrCancel()
        {
            var isActive = StateMachine.BuildingScenesState.IsActive;
            var msg = isActive ? "Cancel baking scenes data?" : "Start baking scenes data?";
            if (MessageBox.Show(msg, "Scene Data building", MessageBoxButtons.OKCancel, MessageBoxIcon.Question) != DialogResult.OK)
                return;
            if (isActive)
                StateMachine.BuildingScenesState.Cancel();
            else
                StateMachine.GoToState<BuildingScenesState>();
        }

        /// <summary>
        /// Starts lightmaps baking for the open scenes or cancels it if already running.
        /// </summary>
        public void BakeLightmapsOrCancel()
        {
            bool isBakingLightmaps = ProgressReporting.BakeLightmaps.IsActive;
            if (isBakingLightmaps)
                Internal_BakeLightmaps(true);
            else
                StateMachine.GoToState<BuildingLightingState>();
        }

        /// <summary>
        /// Clears the lightmaps linkage for all open scenes.
        /// </summary>
        public void ClearLightmaps()
        {
            var scenes = Level.Scenes;
            for (int i = 0; i < scenes.Length; i++)
            {
                scenes[i].ClearLightmaps();
            }

            Scene.MarkSceneEdited(scenes);
        }

        /// <summary>
        /// Bakes all environmental probes in the scene.
        /// </summary>
        public void BakeAllEnvProbes()
        {
            Scene.ExecuteOnGraph(node =>
            {
                if (node is EnvironmentProbeNode envProbeNode && envProbeNode.IsActive)
                {
                    ((EnvironmentProbe)envProbeNode.Actor).Bake();
                    node.ParentScene.IsEdited = true;
                }
                else if (node is SkyLightNode skyLightNode && skyLightNode.IsActive && skyLightNode.Actor is SkyLight skyLight && skyLight.Mode == SkyLight.Modes.CaptureScene)
                {
                    skyLight.Bake();
                    node.ParentScene.IsEdited = true;
                }

                return node.IsActive;
            });
        }

        /// <summary>
        /// Builds CSG for all open scenes.
        /// </summary>
        public void BuildCSG()
        {
            var scenes = Level.Scenes;
            scenes.ToList().ForEach(x => x.BuildCSG(0));
            Scene.MarkSceneEdited(scenes);
        }

        /// <summary>
        /// Builds Nav mesh for all open scenes.
        /// </summary>
        public void BuildNavMesh()
        {
            var scenes = Level.Scenes;
            scenes.ToList().ForEach(x => Navigation.BuildNavMesh(x, 0));
            Scene.MarkSceneEdited(scenes);
        }

        /// <summary>
        /// Builds SDF for all static models in the scene.
        /// </summary>
        public void BuildAllMeshesSDF()
        {
            var models = new List<Model>();
            Scene.ExecuteOnGraph(node =>
            {
                if (node is StaticModelNode staticModelNode && staticModelNode.Actor is StaticModel staticModel)
                {
                    var model = staticModel.Model;
                    if (staticModel.DrawModes.HasFlag(DrawPass.GlobalSDF) &&
                        model != null &&
                        !models.Contains(model) &&
                        !model.IsVirtual &&
                        model.SDF.Texture == null)
                    {
                        models.Add(model);
                    }
                }
                return true;
            });
            Task.Run(() =>
            {
                for (int i = 0; i < models.Count; i++)
                {
                    var model = models[i];
                    Log($"[{i}/{models.Count}] Generating SDF for {model}");
                    if (!model.GenerateSDF())
                        model.Save();
                }
            });
        }

        #endregion

        #region Internal Calls

        [StructLayout(LayoutKind.Sequential)]
        internal struct InternalOptions
        {
            public byte AutoReloadScriptsOnMainWindowFocus;
            public byte ForceScriptCompilationOnStartup;
            public byte UseAssetImportPathRelative;
            public byte EnableParticlesPreview;
            public byte AutoRebuildCSG;
            public float AutoRebuildCSGTimeoutMs;
            public byte AutoRebuildNavMesh;
            public float AutoRebuildNavMeshTimeoutMs;
        }

        internal void BuildCommand(string arg)
        {
            if (TryBuildCommand(arg))
                Engine.RequestExit(1);
        }

        private bool TryBuildCommand(string arg)
        {
            if (GameCooker.IsRunning)
                return true;
            if (arg == null)
                return true;

            Editor.Log("Using CL build for \"" + arg + "\"");

            int dotPos = arg.IndexOf('.');
            string presetName, targetName;
            if (dotPos == -1)
            {
                presetName = arg;
                targetName = string.Empty;
            }
            else
            {
                presetName = arg.Substring(0, dotPos);
                targetName = arg.Substring(dotPos + 1);
            }

            var settings = GameSettings.Load<BuildSettings>();
            var preset = settings.GetPreset(presetName);
            if (preset == null)
            {
                Editor.LogWarning("Missing preset.");
                return true;
            }

            if (string.IsNullOrEmpty(targetName))
            {
                Windows.GameCookerWin.BuildAll(preset);
            }
            else
            {
                var target = preset.GetTarget(targetName);
                if (target == null)
                {
                    Editor.LogWarning("Missing target.");
                    return true;
                }

                Windows.GameCookerWin.Build(preset, target);
            }

            Windows.GameCookerWin.ExitOnBuildQueueEnd();
            return false;
        }

        internal IntPtr GetMainWindowPtr()
        {
            return FlaxEngine.Object.GetUnmanagedPtr(Windows.MainWindow);
        }

        internal bool Internal_CanReloadScripts()
        {
            return StateMachine.CurrentState.CanReloadScripts;
        }

        internal bool Internal_CanAutoBuildCSG()
        {
            return StateMachine.CurrentState.CanEditScene;
        }

        internal bool Internal_CanAutoBuildNavMesh()
        {
            return StateMachine.CurrentState.CanEditScene;
        }

        internal bool Internal_HasGameViewportFocus()
        {
            if (Windows.GameWin != null && Windows.GameWin.ContainsFocus)
            {
                var win = Windows.GameWin.Root;
                if (win?.RootWindow is WindowRootControl root && root.Window && root.Window.IsFocused)
                {
                    if (StateMachine.IsPlayMode && StateMachine.PlayingState.IsPaused)
                        return false;
                    return true;
                }
            }
            return false;
        }

        internal void Internal_FocusGameViewport()
        {
            if (Windows.GameWin != null)
            {
                if (StateMachine.IsPlayMode && !StateMachine.PlayingState.IsPaused)
                {
                    Windows.GameWin.FocusGameViewport();
                }
            }
        }

        internal void Internal_ScreenToGameViewport(ref Float2 pos)
        {
            var win = Windows.GameWin?.Root;
            if (win?.RootWindow is WindowRootControl root)
            {
                pos = Float2.Round(Windows.GameWin.Viewport.PointFromScreen(pos) * root.DpiScale);
            }
            else
            {
                pos = Float2.Minimum;
            }
        }

        internal void Internal_GameViewportToScreen(ref Float2 pos)
        {
            var win = Windows.GameWin?.Root;
            if (win?.RootWindow is WindowRootControl root)
            {
                pos = Float2.Round(Windows.GameWin.Viewport.PointToScreen(pos / root.DpiScale));
            }
            else
            {
                pos = Float2.Minimum;
            }
        }

        internal void Internal_GetGameWinPtr(bool forceGet, out IntPtr result)
        {
            result = IntPtr.Zero;
            if (Windows.GameWin != null && (forceGet || Windows.GameWin.ContainsFocus))
            {
                var win = Windows.GameWin.Root as WindowRootControl;
                if (win != null)
                    result = FlaxEngine.Object.GetUnmanagedPtr(win.Window);
            }
        }

        internal void Internal_GetGameWindowSize(out Float2 result)
        {
            result = new Float2(1280, 720);
            var gameWin = Windows.GameWin;
            if (gameWin?.Root?.RootWindow is WindowRootControl root)
            {
                // Handle case when Game window is not selected in tab view
                var dockedTo = gameWin.ParentDockPanel;
                if (dockedTo != null && dockedTo.SelectedTab != gameWin && dockedTo.SelectedTab != null)
                    result = dockedTo.SelectedTab.Size;
                else
                    result = gameWin.Viewport.Size;

                result = Float2.Round(result);
            }
        }

        internal bool Internal_OnAppExit()
        {
            // In editor play mode (when main window is not closed) just skip engine exit and leave the play mode
            if (StateMachine.IsPlayMode && Windows.MainWindow != null)
            {
                Simulation.RequestStopPlay();
                return false;
            }

            return true;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct VisualScriptingDebugFlowInfo
        {
            public VisualScript Script;
            public FlaxEngine.Object ScriptInstance;
            public uint NodeId;
            public int BoxId;
        }

        internal static event Action<VisualScriptingDebugFlowInfo> VisualScriptingDebugFlow;

        internal static void Internal_OnVisualScriptingDebugFlow(ref VisualScriptingDebugFlowInfo debugFlow)
        {
            VisualScriptingDebugFlow?.Invoke(debugFlow);
        }

        private static void RequestStartPlayOnEditMode()
        {
            if (Instance.StateMachine.IsEditMode)
                Instance.Simulation.RequestStartPlayScenes();
            if (Instance.StateMachine.IsPlayMode)
                Instance.StateMachine.StateChanged -= RequestStartPlayOnEditMode;
        }

        internal static void Internal_RequestStartPlayOnEditMode()
        {
            Instance.StateMachine.StateChanged += RequestStartPlayOnEditMode;
        }

        [LibraryImport("FlaxEngine", EntryPoint = "EditorInternal_ReadOutputLogs", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(FlaxEngine.Interop.StringMarshaller))]
        internal static partial int Internal_ReadOutputLogs([MarshalUsing(typeof(FlaxEngine.Interop.ArrayMarshaller<,>), CountElementName = "outCapacity")] ref string[] outMessages, [MarshalUsing(typeof(FlaxEngine.Interop.ArrayMarshaller<,>), CountElementName = "outCapacity")] ref byte[] outLogTypes, [MarshalUsing(typeof(FlaxEngine.Interop.ArrayMarshaller<,>), CountElementName = "outCapacity")] ref long[] outLogTimes, int outCapacity);

        [LibraryImport("FlaxEngine", EntryPoint = "EditorInternal_SetPlayMode", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(FlaxEngine.Interop.StringMarshaller))]
        internal static partial void Internal_SetPlayMode([MarshalAs(UnmanagedType.U1)] bool value);

        [LibraryImport("FlaxEngine", EntryPoint = "EditorInternal_GetProjectPath", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(StringMarshaller))]
        internal static partial string Internal_GetProjectPath();

        [LibraryImport("FlaxEngine", EntryPoint = "EditorInternal_CloneAssetFile", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(StringMarshaller))]
        [return: MarshalAs(UnmanagedType.U1)]
        internal static partial bool Internal_CloneAssetFile(string dstPath, string srcPath, ref Guid dstId);

        [LibraryImport("FlaxEngine", EntryPoint = "EditorInternal_GetAudioClipMetadata", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(StringMarshaller))]
        internal static partial void Internal_GetAudioClipMetadata(IntPtr obj, out int originalSize, out int importedSize);

        [LibraryImport("FlaxEngine", EntryPoint = "EditorInternal_SaveJsonAsset", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(StringMarshaller))]
        [return: MarshalAs(UnmanagedType.U1)]
        internal static partial bool Internal_SaveJsonAsset(string outputPath, string data, string typename);

        [LibraryImport("FlaxEngine", EntryPoint = "EditorInternal_CopyCache", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(StringMarshaller))]
        internal static partial void Internal_CopyCache(ref Guid dstId, ref Guid srcId);

        [LibraryImport("FlaxEngine", EntryPoint = "EditorInternal_BakeLightmaps", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(StringMarshaller))]
        internal static partial void Internal_BakeLightmaps([MarshalAs(UnmanagedType.U1)] bool cancel);

        [LibraryImport("FlaxEngine", EntryPoint = "EditorInternal_GetShaderAssetSourceCode", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(StringMarshaller))]
        internal static partial string Internal_GetShaderAssetSourceCode(IntPtr obj);

        [LibraryImport("FlaxEngine", EntryPoint = "EditorInternal_CookMeshCollision", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(StringMarshaller))]
        [return: MarshalAs(UnmanagedType.U1)]
        internal static partial bool Internal_CookMeshCollision(string path, CollisionDataType type, IntPtr model, int modelLodIndex, uint materialSlotsMask, ConvexMeshGenerationFlags convexFlags, int convexVertexLimit);

        [LibraryImport("FlaxEngine", EntryPoint = "EditorInternal_GetCollisionWires", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(StringMarshaller))]
        internal static partial void Internal_GetCollisionWires(IntPtr collisionData, [MarshalUsing(typeof(FlaxEngine.Interop.ArrayMarshaller<,>), CountElementName = "trianglesCount")] out Float3[] triangles, [MarshalUsing(typeof(FlaxEngine.Interop.ArrayMarshaller<,>), CountElementName = "indicesCount")] out int[] indices, out int trianglesCount, out int indicesCount);

        [LibraryImport("FlaxEngine", EntryPoint = "EditorInternal_GetEditorBoxWithChildren", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(StringMarshaller))]
        internal static partial void Internal_GetEditorBoxWithChildren(IntPtr obj, out BoundingBox resultAsRef);

        [LibraryImport("FlaxEngine", EntryPoint = "EditorInternal_SetOptions", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(StringMarshaller))]
        internal static partial void Internal_SetOptions(ref InternalOptions options);

        [LibraryImport("FlaxEngine", EntryPoint = "EditorInternal_DrawNavMesh", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(StringMarshaller))]
        internal static partial void Internal_DrawNavMesh();

        [LibraryImport("FlaxEngine", EntryPoint = "EditorInternal_CloseSplashScreen", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(StringMarshaller))]
        internal static partial void Internal_CloseSplashScreen();

        [LibraryImport("FlaxEngine", EntryPoint = "EditorInternal_CreateVisualScript", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(StringMarshaller))]
        [return: MarshalAs(UnmanagedType.U1)]
        internal static partial bool Internal_CreateVisualScript(string outputPath, string baseTypename);

        [LibraryImport("FlaxEngine", EntryPoint = "EditorInternal_CanImport", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(StringMarshaller))]
        internal static partial string Internal_CanImport(string extension);

        [LibraryImport("FlaxEngine", EntryPoint = "EditorInternal_CanExport", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(StringMarshaller))]
        [return: MarshalAs(UnmanagedType.U1)]
        internal static partial bool Internal_CanExport(string path);

        [LibraryImport("FlaxEngine", EntryPoint = "EditorInternal_Export", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(StringMarshaller))]
        [return: MarshalAs(UnmanagedType.U1)]
        internal static partial bool Internal_Export(string inputPath, string outputFolder);

        [LibraryImport("FlaxEngine", EntryPoint = "EditorInternal_GetIsEveryAssemblyLoaded", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(StringMarshaller))]
        [return: MarshalAs(UnmanagedType.U1)]
        internal static partial bool Internal_GetIsEveryAssemblyLoaded();

        [LibraryImport("FlaxEngine", EntryPoint = "EditorInternal_GetLastProjectOpenedEngineBuild", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(StringMarshaller))]
        internal static partial int Internal_GetLastProjectOpenedEngineBuild();

        [LibraryImport("FlaxEngine", EntryPoint = "EditorInternal_GetIsCSGActive", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(StringMarshaller))]
        [return: MarshalAs(UnmanagedType.U1)]
        internal static partial bool Internal_GetIsCSGActive();

        [LibraryImport("FlaxEngine", EntryPoint = "EditorInternal_RunVisualScriptBreakpointLoopTick", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(StringMarshaller))]
        internal static partial void Internal_RunVisualScriptBreakpointLoopTick(float deltaTime);

        [LibraryImport("FlaxEngine", EntryPoint = "EditorInternal_DeserializeSceneObject", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(StringMarshaller))]
        internal static partial void Internal_DeserializeSceneObject(IntPtr sceneObject, string json);

        [LibraryImport("FlaxEngine", EntryPoint = "EditorInternal_LoadAsset", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(StringMarshaller))]
        internal static partial void Internal_LoadAsset(ref Guid id);

        [LibraryImport("FlaxEngine", EntryPoint = "EditorInternal_CanSetToRoot", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(StringMarshaller))]
        [return: MarshalAs(UnmanagedType.U1)]
        internal static partial bool Internal_CanSetToRoot(IntPtr prefab, IntPtr newRoot);

        [LibraryImport("FlaxEngine", EntryPoint = "EditorInternal_GetAnimationTime", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(StringMarshaller))]
        internal static partial float Internal_GetAnimationTime(IntPtr animatedModel);

        [LibraryImport("FlaxEngine", EntryPoint = "EditorInternal_SetAnimationTime", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(StringMarshaller))]
        internal static partial void Internal_SetAnimationTime(IntPtr animatedModel, float time);

        #endregion
    }
}
