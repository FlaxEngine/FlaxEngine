// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using FlaxEditor.Content;
using FlaxEditor.Content.Import;
using FlaxEditor.Content.Settings;
using FlaxEditor.Content.Thumbnails;
using FlaxEditor.Modules;
using FlaxEditor.Modules.SourceCodeEditing;
using FlaxEditor.Options;
using FlaxEditor.States;
using FlaxEditor.Windows;
using FlaxEditor.Windows.Assets;
using FlaxEngine;
using FlaxEngine.Assertions;
using FlaxEngine.GUI;
using FlaxEngine.Json;

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
        private bool _isAfterInit, _areModulesInited, _areModulesAfterInitEnd, _isHeadlessMode;
        private string _projectToOpen;
        private float _lastAutoSaveTimer;

        private const string ProjectDataLastScene = "LastScene";
        private const string ProjectDataLastSceneSpawn = "LastSceneSpawn";

        /// <summary>
        /// Gets a value indicating whether Flax Engine is the best in the world.
        /// </summary>
        public bool IsFlaxEngineTheBest => true;

        /// <summary>
        /// Gets a value indicating whether this Editor is running a dev instance of the engine.
        /// </summary>
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool IsDevInstance();

        /// <summary>
        /// Gets a value indicating whether this Editor is running as official build (distributed via Flax services).
        /// </summary>
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool IsOfficialBuild();

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Internal_IsPlayMode();

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
        public readonly WindowsModule Windows;

        /// <summary>
        /// The UI module.
        /// </summary>
        public readonly UIModule UI;

        /// <summary>
        /// The thumbnails module.
        /// </summary>
        public readonly ThumbnailsModule Thumbnails;

        /// <summary>
        /// The simulation module.
        /// </summary>
        public readonly SimulationModule Simulation;

        /// <summary>
        /// The scene module.
        /// </summary>
        public readonly SceneModule Scene;

        /// <summary>
        /// The prefabs module.
        /// </summary>
        public readonly PrefabsModule Prefabs;

        /// <summary>
        /// The scene editing module.
        /// </summary>
        public readonly SceneEditingModule SceneEditing;

        /// <summary>
        /// The progress reporting module.
        /// </summary>
        public readonly ProgressReportingModule ProgressReporting;

        /// <summary>
        /// The content editing module.
        /// </summary>
        public readonly ContentEditingModule ContentEditing;

        /// <summary>
        /// The content database module.
        /// </summary>
        public readonly ContentDatabaseModule ContentDatabase;

        /// <summary>
        /// The content importing module.
        /// </summary>
        public readonly ContentImportingModule ContentImporting;

        /// <summary>
        /// The content finder module.
        /// </summary>
        public readonly ContentFindingModule ContentFinding;

        /// <summary>
        /// The content editing
        /// </summary>
        public readonly CodeEditingModule CodeEditing;

        /// <summary>
        /// The editor state machine.
        /// </summary>
        public readonly EditorStateMachine StateMachine;

        /// <summary>
        /// The editor options manager.
        /// </summary>
        public readonly OptionsModule Options;

        /// <summary>
        /// The editor per-project cache manager.
        /// </summary>
        public readonly ProjectCacheModule ProjectCache;

        /// <summary>
        /// The undo/redo
        /// </summary>
        public readonly EditorUndo Undo;

        /// <summary>
        /// The icons container.
        /// </summary>
        public readonly EditorIcons Icons;

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
        public readonly ProjectInfo GameProject;

        /// <summary>
        /// The engine project info.
        /// </summary>
        public readonly ProjectInfo EngineProject;

        internal Editor()
        {
            Instance = this;

            Log("Setting up C# Editor...");

            EngineProject = ProjectInfo.Load(StringUtils.CombinePaths(Globals.StartupFolder, "Flax.flaxproj"));
            GameProject = ProjectInfo.Load(Internal_GetProjectPath());

            Icons = new EditorIcons();
            Icons.GetIcons();

            // Create common editor modules
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
            RegisterModule(ProgressReporting = new ProgressReportingModule(this));
            RegisterModule(ContentFinding = new ContentFindingModule(this));

            StateMachine = new EditorStateMachine(this);
            Undo = new EditorUndo(this);

            ScriptsBuilder.ScriptsReloadBegin += ScriptsBuilder_ScriptsReloadBegin;
            ScriptsBuilder.ScriptsReloadEnd += ScriptsBuilder_ScriptsReloadEnd;
            UIControl.FallbackParentGetDelegate += OnUIControlFallbackParentGet;
        }

        private ContainerControl OnUIControlFallbackParentGet(UIControl control)
        {
            // Check if prefab root control is this UIControl
            var loadingPreview = Viewport.Previews.PrefabPreview.LoadingPreview;
            if (loadingPreview != null)
            {
                // Link it to the prefab preview to see it in the editor
                loadingPreview.customControlLinked = control;
                return loadingPreview;
            }
            return null;
        }

        private void ScriptsBuilder_ScriptsReloadBegin()
        {
            EnsureState<EditingSceneState>();
            StateMachine.GoToState<ReloadingScriptsState>();
        }

        private void ScriptsBuilder_ScriptsReloadEnd()
        {
            EnsureState<ReloadingScriptsState>();
            StateMachine.GoToState<EditingSceneState>();
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

        internal void Init(bool isHeadless, bool skipCompile)
        {
            EnsureState<LoadingState>();
            _isHeadlessMode = isHeadless;
            Log("Editor init");
            if (isHeadless)
                Log("Running in headless mode");

            // Note: we don't sort modules before Init (optimized)
            _modules.Sort((a, b) => a.InitOrder - b.InitOrder);
            _isAfterInit = true;

            // Initialize modules (from front to back)
            for (int i = 0; i < _modules.Count; i++)
            {
                _modules[i].OnInit();
            }
            _areModulesInited = true;

            InitializationStart?.Invoke();

            // Start Editor initialization ending phrase (will wait for scripts compilation result)
            StateMachine.LoadingState.StartInitEnding(skipCompile);
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
                    _modules[i].OnEndInit();
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
                if (ProjectCache.TryGetCustomData(ProjectDataLastScene, out var lastSceneIdName) && Guid.TryParse(lastSceneIdName, out var lastSceneId))
                {
                    var lastScene = ContentDatabase.Find(lastSceneId);
                    if (lastScene is SceneItem)
                    {
                        Editor.Log("Loading last opened scene");
                        Scene.OpenScene(lastSceneId);

                        // Restore view
                        if (ProjectCache.TryGetCustomData(ProjectDataLastSceneSpawn, out var lastSceneSpawnName))
                            Windows.EditWin.Viewport.ViewRay = JsonSerializer.Deserialize<Ray>(lastSceneSpawnName);
                    }
                }
                break;
            }
            }
        }

        internal void Update()
        {
            Profiler.BeginEvent("Editor.Update");

            try
            {
                StateMachine.Update();
                UpdateAutoSave();

                if (!StateMachine.IsPlayMode)
                {
                    StateMachine.CurrentState.UpdateFPS();
                }

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
                var countDownDuration = 4.0f;

                if (timeToNextSave <= 0.0f)
                {
                    Log("Auto save");
                    _lastAutoSaveTimer = Time.UnscaledGameTime;
                    if (options.AutoSaveScenes)
                        Scene.SaveScenes();
                    if (options.AutoSaveContent)
                        SaveContent();
                }
                else if (timeToNextSave < countDownDuration)
                {
                    msg = string.Format("Auto save in {0}s...", Mathf.CeilToInt(timeToNextSave));
                }
            }
            if (StateMachine.EditingSceneState.AutoSaveStatus != msg)
            {
                StateMachine.EditingSceneState.AutoSaveStatus = msg;
                UI.UpdateStatusBar();
            }
        }

        internal void OnPlayBeginning()
        {
            for (int i = 0; i < _modules.Count; i++)
                _modules[i].OnPlayBeginning();
        }

        internal void OnPlayBegin()
        {
            for (int i = 0; i < _modules.Count; i++)
                _modules[i].OnPlayBegin();
        }

        internal void OnPlayEnd()
        {
            for (int i = 0; i < _modules.Count; i++)
                _modules[i].OnPlayEnd();
        }

        internal void Exit()
        {
            Log("Editor exit");

            // Start exit
            StateMachine.GoToState<ClosingState>();

            // Cache last opened scene
            {
                var lastSceneId = Level.ScenesCount > 0 ? Level.Scenes[0].ID : Guid.Empty;
                var lastSceneSpawn = Windows.EditWin.Viewport.ViewRay;
                ProjectCache.SetCustomData(ProjectDataLastScene, lastSceneId.ToString());
                ProjectCache.SetCustomData(ProjectDataLastSceneSpawn, JsonSerializer.Serialize(lastSceneSpawn));
            }

            // Cleanup
            Scene.ClearRefsToSceneObjects(true);

            // Release modules (from back to front)
            for (int i = _modules.Count - 1; i >= 0; i--)
            {
                _modules[i].OnExit();
            }

            // Cleanup
            Undo.Dispose();
            Surface.VisualScriptSurface.NodesCache.Clear();
            Instance = null;

            ScriptsBuilder.ScriptsReloadBegin -= ScriptsBuilder_ScriptsReloadBegin;
            ScriptsBuilder.ScriptsReloadEnd -= ScriptsBuilder_ScriptsReloadEnd;

            // Invoke new instance if need to open a project
            if (!string.IsNullOrEmpty(_projectToOpen))
            {
                string args = string.Format("-project \"{0}\"", _projectToOpen);
                _projectToOpen = null;
                Platform.StartProcess(Platform.ExecutableFilePath, args, null);
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
                // Error
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
        /// </summary>
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
        }

        /// <summary>
        /// Imports the asset file to the target location.
        /// </summary>
        /// <param name="inputPath">The source file path.</param>
        /// <param name="outputPath">The result asset file path.</param>
        /// <returns>True if importing failed, otherwise false.</returns>
        public static bool Import(string inputPath, string outputPath)
        {
            return Internal_Import(inputPath, outputPath, IntPtr.Zero);
        }

        /// <summary>
        /// Imports the texture asset file to the target location.
        /// </summary>
        /// <param name="inputPath">The source file path.</param>
        /// <param name="outputPath">The result asset file path.</param>
        /// <param name="settings">The settings.</param>
        /// <returns>True if importing failed, otherwise false.</returns>
        public static bool Import(string inputPath, string outputPath, TextureImportSettings settings)
        {
            if (settings == null)
                throw new ArgumentNullException();
            settings.ToInternal(out var internalOptions);
            return Internal_ImportTexture(inputPath, outputPath, ref internalOptions);
        }

        /// <summary>
        /// Imports the model asset file to the target location.
        /// </summary>
        /// <param name="inputPath">The source file path.</param>
        /// <param name="outputPath">The result asset file path.</param>
        /// <param name="settings">The settings.</param>
        /// <returns>True if importing failed, otherwise false.</returns>
        public static bool Import(string inputPath, string outputPath, ModelImportSettings settings)
        {
            if (settings == null)
                throw new ArgumentNullException();
            settings.ToInternal(out var internalOptions);
            return Internal_ImportModel(inputPath, outputPath, ref internalOptions);
        }

        /// <summary>
        /// Imports the audio asset file to the target location.
        /// </summary>
        /// <param name="inputPath">The source file path.</param>
        /// <param name="outputPath">The result asset file path.</param>
        /// <param name="settings">The settings.</param>
        /// <returns>True if importing failed, otherwise false.</returns>
        public static bool Import(string inputPath, string outputPath, AudioImportSettings settings)
        {
            if (settings == null)
                throw new ArgumentNullException();
            settings.ToInternal(out var internalOptions);
            return Internal_ImportAudio(inputPath, outputPath, ref internalOptions);
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
        /// <param name="convexFlags">The convex mesh generation flags.</param>
        /// <param name="convexVertexLimit">The convex mesh vertex limit. Use values in range [8;255]</param>
        /// <returns>True if failed, otherwise false.</returns>
        public static bool CookMeshCollision(string path, CollisionDataType type, Model model, int modelLodIndex = 0, ConvexMeshGenerationFlags convexFlags = ConvexMeshGenerationFlags.None, int convexVertexLimit = 255)
        {
            if (string.IsNullOrEmpty(path))
                throw new ArgumentNullException(nameof(path));
            if (model == null)
                throw new ArgumentNullException(nameof(model));
            if (type == CollisionDataType.None)
                throw new ArgumentException(nameof(type));

            return Internal_CookMeshCollision(path, type, FlaxEngine.Object.GetUnmanagedPtr(model), modelLodIndex, convexFlags, convexVertexLimit);
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
                throw new FlaxException("Failed to load asset.");
            var source = Internal_GetShaderAssetSourceCode(FlaxEngine.Object.GetUnmanagedPtr(asset));
            if (source == null)
                throw new FlaxException("Failed to get source code.");
            return source;
        }

        /// <summary>
        /// Gets the actor bounding sphere (including child actors).
        /// </summary>
        /// <param name="actor">The actor.</param>
        /// <param name="sphere">The bounding sphere.</param>
        public static void GetActorEditorSphere(Actor actor, out BoundingSphere sphere)
        {
            Internal_GetEditorBoxWithChildren(FlaxEngine.Object.GetUnmanagedPtr(actor), out var box);
            BoundingSphere.FromBox(ref box, out sphere);
            sphere.Radius = Math.Max(sphere.Radius, 15.0f);
        }

        /// <summary>
        /// Gets the actor bounding box (including child actors).
        /// </summary>
        /// <param name="actor">The actor.</param>
        /// <param name="box">The bounding box.</param>
        public static void GetActorEditorBox(Actor actor, out BoundingBox box)
        {
            Internal_GetEditorBoxWithChildren(FlaxEngine.Object.GetUnmanagedPtr(actor), out box);
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
        /// </summary>
        /// <param name="type">New asset type.</param>
        /// <param name="outputPath">Output asset path.</param>
        public static bool CreateAsset(NewAssetType type, string outputPath)
        {
            return Internal_CreateAsset(type, outputPath);
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
        /// <returns>True if can import files with given extension, otherwise false.</returns>
        public static bool CanImport(string extension)
        {
            return Internal_CanImport(extension);
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
            if (StateMachine.BuildingScenesState.IsActive)
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

        #endregion

        #region Internal Calls

        [StructLayout(LayoutKind.Sequential)]
        internal struct InternalOptions
        {
            public byte AutoReloadScriptsOnMainWindowFocus;
            public byte ForceScriptCompilationOnStartup;
            public byte AutoRebuildCSG;
            public float AutoRebuildCSGTimeoutMs;
            public byte AutoRebuildNavMesh;
            public float AutoRebuildNavMeshTimeoutMs;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct VisualScriptLocal
        {
            public string Value;
            public string ValueTypeName;
            public uint NodeId;
            public int BoxId;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct VisualScriptStackFrame
        {
            public VisualScript Script;
            public uint NodeId;
            public int BoxId;
        }

        internal void BuildCommand(string arg)
        {
            if (TryBuildCommand(arg))
                Engine.RequestExit();
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

                Windows.GameCookerWin.Build(target);
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
                if (win != null && win.RootWindow is WindowRootControl root && root.Window.IsFocused)
                {
                    return true;
                }
            }
            return false;
        }

        internal void Internal_ScreenToGameViewport(ref Vector2 pos)
        {
            if (Windows.GameWin != null && Windows.GameWin.ContainsFocus)
            {
                var win = Windows.GameWin.Root;
                if (win != null && win.RootWindow is WindowRootControl root && root.Window.IsFocused)
                {
                    pos = Vector2.Round(Windows.GameWin.Viewport.PointFromWindow(root.Window.ScreenToClient(pos)));
                }
                else
                {
                    pos = Vector2.Minimum;
                }
            }
            else
            {
                pos = Vector2.Minimum;
            }
        }

        internal void Internal_GameViewportToScreen(ref Vector2 pos)
        {
            if (Windows.GameWin != null && Windows.GameWin.ContainsFocus)
            {
                var win = Windows.GameWin.Root;
                if (win != null && win.RootWindow is WindowRootControl root && root.Window.IsFocused)
                {
                    pos = Vector2.Round(root.Window.ClientToScreen(Windows.GameWin.Viewport.PointToWindow(pos)));
                }
                else
                {
                    pos = Vector2.Minimum;
                }
            }
            else
            {
                pos = Vector2.Minimum;
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

        internal void Internal_GetGameWindowSize(out Vector2 resultAsRef)
        {
            resultAsRef = Vector2.Zero;
            var gameWin = Windows.GameWin;
            if (gameWin != null)
            {
                // Handle case when Game window is not selected in tab view
                var dockedTo = gameWin.ParentDockPanel;
                if (dockedTo != null && dockedTo.SelectedTab != gameWin && dockedTo.SelectedTab != null)
                    resultAsRef = dockedTo.SelectedTab.Size;
                else
                    resultAsRef = gameWin.Size;
                resultAsRef = Vector2.Round(resultAsRef);
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

        [StructLayout(LayoutKind.Sequential)]
        internal struct AnimGraphDebugFlowInfo
        {
            public Asset Asset;
            public FlaxEngine.Object Object;
            public uint NodeId;
            public int BoxId;
        }

        internal static event Action<AnimGraphDebugFlowInfo> AnimGraphDebugFlow;

        internal static void Internal_OnAnimGraphDebugFlow(ref AnimGraphDebugFlowInfo debugFlow)
        {
            AnimGraphDebugFlow?.Invoke(debugFlow);
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern int Internal_ReadOutputLogs(string[] outMessages, byte[] outLogTypes, long[] outLogTimes);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Internal_SetPlayMode(bool value);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern string Internal_GetProjectPath();

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Internal_CloneAssetFile(string dstPath, string srcPath, ref Guid dstId);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Internal_Import(string inputPath, string outputPath, IntPtr arg);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Internal_ImportTexture(string inputPath, string outputPath, ref TextureImportSettings.InternalOptions options);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Internal_ImportModel(string inputPath, string outputPath, ref ModelImportSettings.InternalOptions options);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Internal_ImportAudio(string inputPath, string outputPath, ref AudioImportSettings.InternalOptions options);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Internal_GetAudioClipMetadata(IntPtr obj, out int originalSize, out int importedSize);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Internal_SaveJsonAsset(string outputPath, string data, string typename);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Internal_CopyCache(ref Guid dstId, ref Guid srcId);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Internal_BakeLightmaps(bool cancel);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern string Internal_GetShaderAssetSourceCode(IntPtr obj);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Internal_CookMeshCollision(string path, CollisionDataType type, IntPtr model, int modelLodIndex, ConvexMeshGenerationFlags convexFlags, int convexVertexLimit);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Internal_GetCollisionWires(IntPtr collisionData, out Vector3[] triangles, out int[] indices);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Internal_GetEditorBoxWithChildren(IntPtr obj, out BoundingBox resultAsRef);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Internal_SetOptions(ref InternalOptions options);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Internal_DrawNavMesh();

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Internal_CloseSplashScreen();

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Internal_CreateAsset(NewAssetType type, string outputPath);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Internal_CreateVisualScript(string outputPath, string baseTypename);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Internal_CanImport(string extension);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Internal_CanExport(string path);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Internal_Export(string inputPath, string outputFolder);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Internal_GetIsEveryAssemblyLoaded();

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern int Internal_GetLastProjectOpenedEngineBuild();

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Internal_GetIsCSGActive();

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Internal_RunVisualScriptBreakpointLoopTick(float deltaTime);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern VisualScriptLocal[] Internal_GetVisualScriptLocals();

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern VisualScriptStackFrame[] Internal_GetVisualScriptStackFrames();

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern VisualScriptStackFrame Internal_GetVisualScriptPreviousScopeFrame();

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Internal_EvaluateVisualScriptLocal(IntPtr script, ref VisualScriptLocal local);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Internal_DeserializeSceneObject(IntPtr sceneObject, string json);

        #endregion
    }
}
