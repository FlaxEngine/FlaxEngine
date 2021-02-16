// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.ComponentModel;
using FlaxEngine;

namespace FlaxEditor.Options
{
    /// <summary>
    /// General editor options data container.
    /// </summary>
    [CustomEditor(typeof(Editor<GeneralOptions>))]
    public sealed class GeneralOptions
    {
        /// <summary>
        /// The editor startup scene modes.
        /// </summary>
        public enum StartupSceneModes
        {
            /// <summary>
            /// Don't open scene on startup.
            /// </summary>
            None,

            /// <summary>
            /// The project default scene.
            /// </summary>
            ProjectDefault,

            /// <summary>
            /// The last opened scene in the editor.
            /// </summary>
            LastOpened,
        }

        /// <summary>
        /// The build actions.
        /// </summary>
        public enum BuildAction
        {
            /// <summary>
            /// Builds Constructive Solid Geometry brushes into meshes.
            /// </summary>
            [Tooltip("Builds Constructive Solid Geometry brushes into meshes.")]
            CSG,

            /// <summary>
            /// Builds Env Probes and Sky Lights to prerendered cube textures.
            /// </summary>
            [Tooltip("Builds Env Probes and Sky Lights to prerendered cube textures.")]
            EnvProbes,

            /// <summary>
            /// Builds static lighting into lightmaps.
            /// </summary>
            [Tooltip("Builds static lighting into lightmaps.")]
            StaticLighting,

            /// <summary>
            /// Builds navigation meshes.
            /// </summary>
            [Tooltip("Builds navigation meshes.")]
            NavMesh,

            /// <summary>
            /// Compiles the scripts.
            /// </summary>
            [Tooltip("Compiles the scripts.")]
            CompileScripts,
        }

        /// <summary>
        /// Gets or sets the scene to load on editor startup.
        /// </summary>
        [DefaultValue(StartupSceneModes.LastOpened)]
        [EditorDisplay("General"), EditorOrder(10), Tooltip("The scene to load on editor startup")]
        public StartupSceneModes StartupSceneMode { get; set; } = StartupSceneModes.LastOpened;

        /// <summary>
        /// Gets or sets a limit for the editor undo actions. Higher values may increase memory usage but also improve changes rollback history length.
        /// </summary>
        [DefaultValue(500)]
        [EditorDisplay("General"), EditorOrder(100), Tooltip("Limit for the editor undo actions. Higher values may increase memory usage but also improve changes rollback history length.")]
        public int UndoActionsCapacity { get; set; } = 500;

        /// <summary>
        /// Gets or sets a limit for the editor draw/update frames per second rate (FPS). Use higher values if you need more responsive interface or lower values to use less device power. Value 0 disables any limits.
        /// </summary>
        [DefaultValue(60.0f), Limit(0, 666)]
        [EditorDisplay("General", "Editor FPS"), EditorOrder(110), Tooltip("Limit for the editor draw/update frames per second rate (FPS). Use higher values if you need more responsive interface or lower values to use less device power. Value 0 disables any limits.")]
        public float EditorFPS { get; set; } = 60.0f;

        /// <summary>
        /// Gets or sets the sequence of actions to perform when using Build Scenes button. Can be used to configure this as button (eg. compile code or just update navmesh).
        /// </summary>
        [EditorDisplay("General"), EditorOrder(200), Tooltip("The sequence of actions to perform when using Build Scenes button. Can be used to configure this as button (eg. compile code or just update navmesh).")]
        public BuildAction[] BuildActions { get; set; } =
        {
            BuildAction.CSG,
            BuildAction.EnvProbes,
            BuildAction.StaticLighting,
            BuildAction.EnvProbes,
            BuildAction.NavMesh,
        };

        /// <summary>
        /// Gets or sets a value indicating whether perform automatic scripts reload on main window focus.
        /// </summary>
        [DefaultValue(true)]
        [EditorDisplay("Scripting", "Auto Reload Scripts On Main Window Focus"), EditorOrder(500), Tooltip("Determines whether reload scripts after a change on main window focus.")]
        public bool AutoReloadScriptsOnMainWindowFocus { get; set; } = true;

        /// <summary>
        /// Gets or sets a value indicating whether automatically compile game scripts before starting the editor.
        /// </summary>
        [DefaultValue(true)]
        [EditorDisplay("Scripting", "Force Script Compilation On Startup"), EditorOrder(501), Tooltip("Determines whether automatically compile game scripts before starting the editor.")]
        public bool ForceScriptCompilationOnStartup { get; set; } = true;

        /// <summary>
        /// Gets or sets a value indicating whether automatically save the Visual Script asset editors when starting the play mode in editor.
        /// </summary>
        [DefaultValue(true)]
        [EditorDisplay("Scripting", "Auto Save Visual Script On Play Start"), EditorOrder(505), Tooltip("Determines whether automatically save the Visual Script asset editors when starting the play mode in editor.")]
        public bool AutoSaveVisualScriptOnPlayStart { get; set; } = true;

        /// <summary>
        /// Gets or sets a value indicating whether perform automatic CSG rebuild on brush change.
        /// </summary>
        [DefaultValue(true)]
        [EditorDisplay("CSG", "Auto Rebuild CSG"), EditorOrder(600), Tooltip("Determines whether perform automatic CSG rebuild on brush change.")]
        public bool AutoRebuildCSG { get; set; } = true;

        /// <summary>
        /// Gets or sets the auto CSG rebuilding timeout (in milliseconds). Use lower value for more frequent and responsive updates but higher complexity.
        /// </summary>
        [DefaultValue(50.0f), Range(0, 500)]
        [EditorDisplay("CSG", "Auto Rebuild CSG Timeout"), EditorOrder(601), Tooltip("Auto CSG rebuilding timeout (in milliseconds). Use lower value for more frequent and responsive updates but higher complexity.")]
        public float AutoRebuildCSGTimeoutMs { get; set; } = 50.0f;

        /// <summary>
        /// Gets or sets a value indicating whether perform automatic NavMesh rebuild on scene change.
        /// </summary>
        [DefaultValue(true)]
        [EditorDisplay("Navigation Mesh", "Auto Rebuild Nav Mesh"), EditorOrder(700), Tooltip("Determines whether perform automatic NavMesh rebuild on scene change.")]
        public bool AutoRebuildNavMesh { get; set; } = true;

        /// <summary>
        /// Gets or sets the auto CSG rebuilding timeout (in milliseconds). Use lower value for more frequent and responsive updates but higher complexity.
        /// </summary>
        [DefaultValue(100.0f), Range(0, 1000)]
        [EditorDisplay("Navigation Mesh", "Auto Rebuild Nav Mesh Timeout"), EditorOrder(701), Tooltip("Auto NavMesh rebuilding timeout (in milliseconds). Use lower value for more frequent and responsive updates but higher complexity.")]
        public float AutoRebuildNavMeshTimeoutMs { get; set; } = 100.0f;

        /// <summary>
        /// Gets or sets a value indicating whether enable auto saves.
        /// </summary>
        [DefaultValue(true)]
        [EditorDisplay("Auto Save", "Enable Auto Save"), EditorOrder(800), Tooltip("Enables or disables auto saving changes in edited scenes and content")]
        public bool EnableAutoSave { get; set; } = true;

        /// <summary>
        /// Gets or sets a value indicating auto saves interval (in minutes).
        /// </summary>
        [DefaultValue(5), Limit(1)]
        [EditorDisplay("Auto Save", "Auto Save Frequency"), EditorOrder(801), Tooltip("The interval between auto saves (in minutes)")]
        public int AutoSaveFrequency { get; set; } = 5;

        /// <summary>
        /// Gets or sets a value indicating whether enable auto saves for scenes.
        /// </summary>
        [DefaultValue(true)]
        [EditorDisplay("Auto Save", "Auto Save Scenes"), EditorOrder(802), Tooltip("Enables or disables auto saving opened scenes")]
        public bool AutoSaveScenes { get; set; } = true;

        /// <summary>
        /// Gets or sets a value indicating whether enable auto saves for content.
        /// </summary>
        [DefaultValue(true)]
        [EditorDisplay("Auto Save", "Auto Save Content"), EditorOrder(803), Tooltip("Enables or disables auto saving content")]
        public bool AutoSaveContent { get; set; } = true;

        /// <summary>
        /// Gets or sets a value indicating whether enable editor analytics service.
        /// </summary>
        [DefaultValue(true)]
        [EditorDisplay("Analytics"), EditorOrder(1000), Tooltip("Enables or disables anonymous editor analytics service used to improve editor experience and the quality")]
        public bool EnableEditorAnalytics { get; set; } = true;
    }
}
