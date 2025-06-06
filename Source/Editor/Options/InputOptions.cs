// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Security.Cryptography.X509Certificates;
using FlaxEditor.InputConfig;
using FlaxEngine;

#pragma warning disable 1591

namespace FlaxEditor.Options
{
    public enum InputOptionName
    {
        None,
        // Common
        Save,
        Rename,
        Copy,
        Cut,
        Paste,
        Duplicate,
        Delete,
        Undo,
        Redo,
        SelectAll,
        DeselectAll,
        FocusSelection,
        LockFocusSelection,
        Search,
        ContentFinder,
        RotateSelection,
        ToggleFullscreen,

        // File
        SaveScenes,
        CloseScenes,
        OpenScriptsProject,
        GenerateScriptsProject,
        RecompileScripts,

        // Scene
        SnapToGround,
        SnapToVertex,
        Play,
        PlayCurrentScenes,
        Pause,
        StepFrame,
        CookAndRun,
        RunCookedGame,
        MoveActorToViewport,
        AlignActorWithViewport,
        AlignViewportWithActor,
        PilotActor,
        GroupSelectedActors,

        // Tools
        BuildScenesData,
        BakeLightmaps,
        ClearLightmaps,
        BakeEnvProbes,
        BuildCSG,
        BuildNav,
        BuildSDF,
        TakeScreenshot,

        // Profiler
        ProfilerWindow,
        ProfilerStartStop,
        ProfilerClear,

        // Debugger
        DebuggerContinue,
        DebuggerUnlockMouse,
        DebuggerStepOver,
        DebuggerStepInto,
        DebuggerStepOut,

        // Gizmo
        TranslateMode,
        RotateMode,
        ScaleMode,
        ToggleTransformSpace,

        // Viewport
        Forward,
        Backward,
        Left,
        Right,
        Up,
        Down,
        Orbit,
        Pan,
        Rotate,
        ZoomIn,
        ZoomOut,
        CameraToggleRotation,
        CameraIncreaseMoveSpeed,
        CameraDecreaseMoveSpeed,
        ViewpointFront,
        ViewpointBack,
        ViewpointLeft,
        ViewpointRight,
        ViewpointTop,
        ViewpointBottom,
        ToggleOrthographic,

        // Interface
        CloseTab,
        NextTab,
        PreviousTab,
        DoubleClickSceneNode
    }

    /// <summary>
    /// Action to perform when a Scene Node receive a double mouse left click.
    /// </summary>
    public enum SceneNodeDoubleClick
    {
        /// <summary>
        /// Toggles expand/state of the node.
        /// </summary>
        Expand,

        /// <summary>
        /// Rename the node.
        /// </summary>
        RenameActor,

        /// <summary>
        /// Focus the object in the viewport.
        /// </summary>
        FocusActor,

        /// <summary>
        /// If possible, open the scene node in an associated Editor (eg. Prefab Editor).
        /// </summary>
        OpenPrefab,
    }

    /// <summary>
    /// Input editor options data container.
    /// </summary>
    [CustomEditor(typeof(Editor<InputOptions>))]
    [HideInEditor]
    public sealed class InputOptions
    {
        public static Dictionary<InputOptionName, InputBinding> Dictionary = [];

        #region Common
        [DefaultValue(typeof(InputBinding), KeyboardKeysString.Control + "+" + KeyboardKeysString.S)]
        [EditorDisplay("Common"), EditorOrder(100)]
        public static InputBinding Save { get; set; } = new InputBinding(KeyboardKeys.S + "+" + KeyboardKeys.Control);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.F2)]
        [EditorDisplay("Common"), EditorOrder(110)]
        public static InputBinding Rename { get; set; } = new InputBinding(KeyboardKeys.F2);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.Control + "+" + KeyboardKeysString.C)]
        [EditorDisplay("Common"), EditorOrder(120)]
        public static InputBinding Copy { get; set; } = new InputBinding(KeyboardKeys.C + "+" + KeyboardKeys.Control);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.Control + "+" + KeyboardKeysString.X)]
        [EditorDisplay("Common"), EditorOrder(130)]
        public static InputBinding Cut { get; set; } = new InputBinding(KeyboardKeys.X + "+" + KeyboardKeys.Control);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.Control + "+" + KeyboardKeysString.V)]
        [EditorDisplay("Common"), EditorOrder(140)]
        public static InputBinding Paste { get; set; } = new InputBinding(KeyboardKeys.V + "+" + KeyboardKeys.Control);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.Control + "+" + KeyboardKeysString.D)]
        [EditorDisplay("Common"), EditorOrder(150)]
        public static InputBinding Duplicate { get; set; } = new InputBinding(KeyboardKeys.D + "+" + KeyboardKeys.Control);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.Delete)]
        [EditorDisplay("Common"), EditorOrder(160)]
        public static InputBinding Delete { get; set; } = new InputBinding(KeyboardKeys.Delete);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.Control + "+" + KeyboardKeysString.Z)]
        [EditorDisplay("Common"), EditorOrder(170)]
        public static InputBinding Undo { get; set; } = new InputBinding(KeyboardKeys.Z + "+" + KeyboardKeys.Control);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.Control + "+" + KeyboardKeysString.Y)]
        [EditorDisplay("Common"), EditorOrder(180)]
        public static InputBinding Redo { get; set; } = new InputBinding(KeyboardKeys.Y + "+" + KeyboardKeys.Control);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.Control + "+" + KeyboardKeysString.A)]
        [EditorDisplay("Common"), EditorOrder(190)]
        public static InputBinding SelectAll { get; set; } = new InputBinding(KeyboardKeys.A + "+" + KeyboardKeys.Control);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.Control + "+" + KeyboardKeysString.Shift + "+" + KeyboardKeysString.A)]
        [EditorDisplay("Common"), EditorOrder(195)]
        public static InputBinding DeselectAll { get; set; } = new InputBinding(KeyboardKeys.A + "+" + KeyboardKeys.Shift + "+" + KeyboardKeys.Control);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.F)]
        [EditorDisplay("Common"), EditorOrder(200)]
        public static InputBinding FocusSelection { get; set; } = new InputBinding(KeyboardKeys.F);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.Shift + "+" + KeyboardKeysString.F)]
        [EditorDisplay("Common"), EditorOrder(200)]
        public static InputBinding LockFocusSelection { get; set; } = new InputBinding(KeyboardKeys.F + "+" + KeyboardKeys.Shift);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.Control + "+" + KeyboardKeysString.F)]
        [EditorDisplay("Common"), EditorOrder(210)]
        public static InputBinding Search { get; set; } = new InputBinding(KeyboardKeys.F + "+" + KeyboardKeys.Control);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.Control + "+" + KeyboardKeysString.O)]
        [EditorDisplay("Common"), EditorOrder(220)]
        public static InputBinding ContentFinder { get; set; } = new InputBinding(KeyboardKeys.O + "+" + KeyboardKeys.Control);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.R)]
        [EditorDisplay("Common"), EditorOrder(230)]
        public static InputBinding RotateSelection { get; set; } = new InputBinding(KeyboardKeys.R);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.F11)]
        [EditorDisplay("Common"), EditorOrder(240)]
        public static InputBinding ToggleFullscreen { get; set; } = new InputBinding(KeyboardKeys.F11);

        #endregion

        #region File

        [DefaultValue(typeof(InputBinding), "")]
        [EditorDisplay("File"), EditorOrder(300)]
        public static InputBinding SaveScenes { get; set; } = new InputBinding(KeyboardKeys.None);

        [DefaultValue(typeof(InputBinding), "")]
        [EditorDisplay("File"), EditorOrder(310)]
        public static InputBinding CloseScenes { get; set; } = new InputBinding(KeyboardKeys.None);

        [DefaultValue(typeof(InputBinding), "")]
        [EditorDisplay("File"), EditorOrder(320)]
        public static InputBinding OpenScriptsProject { get; set; } = new InputBinding(KeyboardKeys.None);

        [DefaultValue(typeof(InputBinding), "")]
        [EditorDisplay("File"), EditorOrder(330)]
        public static InputBinding GenerateScriptsProject { get; set; } = new InputBinding(KeyboardKeys.None);

        [DefaultValue(typeof(InputBinding), "")]
        [EditorDisplay("File"), EditorOrder(340)]
        public static InputBinding RecompileScripts { get; set; } = new InputBinding(KeyboardKeys.None);

        #endregion

        #region Scene

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.End)]
        [EditorDisplay("Scene", "Snap To Ground"), EditorOrder(500)]
        public static InputBinding SnapToGround { get; set; } = new InputBinding(KeyboardKeys.End);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.V)]
        [EditorDisplay("Scene", "Vertex Snapping"), EditorOrder(550)]
        public static InputBinding SnapToVertex { get; set; } = new InputBinding(KeyboardKeys.V);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.F5)]
        [EditorDisplay("Scene", "Play/Stop"), EditorOrder(510)]
        public static InputBinding Play { get; set; } = new InputBinding(KeyboardKeys.F5);

        [DefaultValue(typeof(InputBinding), "")]
        [EditorDisplay("Scene", "Play Current Scenes/Stop"), EditorOrder(520)]
        public static InputBinding PlayCurrentScenes { get; set; } = new InputBinding(KeyboardKeys.None);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.F6)]
        [EditorDisplay("Scene"), EditorOrder(530)]
        public static InputBinding Pause { get; set; } = new InputBinding(KeyboardKeys.F6);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.F11)]
        [EditorDisplay("Scene"), EditorOrder(540)]
        public static InputBinding StepFrame { get; set; } = new InputBinding(KeyboardKeys.F11);

        [DefaultValue(typeof(InputBinding), "")]
        [EditorDisplay("Scene", "Cook & Run"), EditorOrder(550)]
        public static InputBinding CookAndRun { get; set; } = new InputBinding(KeyboardKeys.None);

        [DefaultValue(typeof(InputBinding), "")]
        [EditorDisplay("Scene", "Run cooked game"), EditorOrder(560)]
        public static InputBinding RunCookedGame { get; set; } = new InputBinding(KeyboardKeys.None);

        [DefaultValue(typeof(InputBinding), "")]
        [EditorDisplay("Scene", "Move actor to viewport"), EditorOrder(570)]
        public static InputBinding MoveActorToViewport { get; set; } = new InputBinding(KeyboardKeys.None);

        [DefaultValue(typeof(InputBinding), "")]
        [EditorDisplay("Scene", "Align actor with viewport"), EditorOrder(571)]
        public static InputBinding AlignActorWithViewport { get; set; } = new InputBinding(KeyboardKeys.None);

        [DefaultValue(typeof(InputBinding), "")]
        [EditorDisplay("Scene", "Align viewport with actor"), EditorOrder(572)]
        public static InputBinding AlignViewportWithActor { get; set; } = new InputBinding(KeyboardKeys.None);

        [DefaultValue(typeof(InputBinding), "")]
        [EditorDisplay("Scene"), EditorOrder(573)]
        public static InputBinding PilotActor { get; set; } = new InputBinding(KeyboardKeys.None);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.Control + "+" + KeyboardKeysString.G)]
        [EditorDisplay("Scene"), EditorOrder(574)]
        public static InputBinding GroupSelectedActors { get; set; } = new InputBinding(KeyboardKeys.G + "+" + KeyboardKeys.Control);

        #endregion

        #region Tools

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.Control + "+" + KeyboardKeysString.F10)]
        [EditorDisplay("Tools", "Build scenes data"), EditorOrder(600)]
        public static InputBinding BuildScenesData { get; set; } = new InputBinding(KeyboardKeys.F10 + "+" + KeyboardKeys.Control);

        [DefaultValue(typeof(InputBinding), "")]
        [EditorDisplay("Tools", "Bake lightmaps"), EditorOrder(601)]
        public static InputBinding BakeLightmaps { get; set; } = new InputBinding(KeyboardKeys.None);

        [DefaultValue(typeof(InputBinding), "")]
        [EditorDisplay("Tools", "Clear lightmaps data"), EditorOrder(602)]
        public static InputBinding ClearLightmaps { get; set; } = new InputBinding(KeyboardKeys.None);

        [DefaultValue(typeof(InputBinding), "")]
        [EditorDisplay("Tools", "Bake all env probes"), EditorOrder(603)]
        public static InputBinding BakeEnvProbes { get; set; } = new InputBinding(KeyboardKeys.None);

        [DefaultValue(typeof(InputBinding), "")]
        [EditorDisplay("Tools", "Build CSG mesh"), EditorOrder(604)]
        public static InputBinding BuildCSG { get; set; } = new InputBinding(KeyboardKeys.None);

        [DefaultValue(typeof(InputBinding), "")]
        [EditorDisplay("Tools", "Build Nav Mesh"), EditorOrder(605)]
        public static InputBinding BuildNav { get; set; } = new InputBinding(KeyboardKeys.None);

        [DefaultValue(typeof(InputBinding), "")]
        [EditorDisplay("Tools", "Build all meshes SDF"), EditorOrder(606)]
        public static InputBinding BuildSDF { get; set; } = new InputBinding(KeyboardKeys.None);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.F12)]
        [EditorDisplay("Tools", "Take screenshot"), EditorOrder(607)]
        public static InputBinding TakeScreenshot { get; set; } = new InputBinding(KeyboardKeys.F12);

        #endregion

        #region Profiler

        [DefaultValue(typeof(InputBinding), "")]
        [EditorDisplay("Profiler", "Open Profiler Window"), EditorOrder(630)]
        public static InputBinding ProfilerWindow { get; set; } = new InputBinding(KeyboardKeys.None);

        [DefaultValue(typeof(InputBinding), "")]
        [EditorDisplay("Profiler", "Start/Stop Profiler"), EditorOrder(631)]
        public static InputBinding ProfilerStartStop { get; set; } = new InputBinding(KeyboardKeys.None);

        [DefaultValue(typeof(InputBinding), "")]
        [EditorDisplay("Profiler", "Clear Profiler data"), EditorOrder(632)]
        public static InputBinding ProfilerClear { get; set; } = new InputBinding(KeyboardKeys.None);

        #endregion

        #region Debugger

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.F5)]
        [EditorDisplay("Debugger", "Continue"), EditorOrder(810)]
        public static InputBinding DebuggerContinue { get; set; } = new InputBinding(KeyboardKeys.F5);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.Shift + "+" + KeyboardKeysString.F11)]
        [EditorDisplay("Debugger", "Unlock mouse in Play Mode"), EditorOrder(820)]
        public static InputBinding DebuggerUnlockMouse { get; set; } = new InputBinding(KeyboardKeys.F11 + "+" + KeyboardKeys.Shift);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.F10)]
        [EditorDisplay("Debugger", "Step Over"), EditorOrder(830)]
        public static InputBinding DebuggerStepOver { get; set; } = new InputBinding(KeyboardKeys.F10);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.F11)]
        [EditorDisplay("Debugger", "Step Into"), EditorOrder(840)]
        public static InputBinding DebuggerStepInto { get; set; } = new InputBinding(KeyboardKeys.F11);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.Shift + "+" + KeyboardKeysString.F11)]
        [EditorDisplay("Debugger", "Step Out"), EditorOrder(850)]
        public static InputBinding DebuggerStepOut { get; set; } = new InputBinding(KeyboardKeys.F11 + "+" + KeyboardKeys.Shift);

        #endregion

        #region Gizmo

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.Alpha1)]
        [EditorDisplay("Gizmo"), EditorOrder(1000)]
        public static InputBinding TranslateMode { get; set; } = new InputBinding(KeyboardKeys.Alpha1);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.Alpha2)]
        [EditorDisplay("Gizmo"), EditorOrder(1010)]
        public static InputBinding RotateMode { get; set; } = new InputBinding(KeyboardKeys.Alpha2);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.Alpha3)]
        [EditorDisplay("Gizmo"), EditorOrder(1020)]
        public static InputBinding ScaleMode { get; set; } = new InputBinding(KeyboardKeys.Alpha3);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.Alpha4)]
        [EditorDisplay("Gizmo"), EditorOrder(1030)]
        public static InputBinding ToggleTransformSpace { get; set; } = new InputBinding(KeyboardKeys.Alpha4);

        #endregion

        #region Viewport

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.W)]
        [EditorDisplay("Viewport"), EditorOrder(1500)]
        public static InputBinding Forward { get; set; } = new InputBinding(KeyboardKeys.W);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.S)]
        [EditorDisplay("Viewport"), EditorOrder(1510)]
        public static InputBinding Backward { get; set; } = new InputBinding(KeyboardKeys.S);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.A)]
        [EditorDisplay("Viewport"), EditorOrder(1520)]
        public static InputBinding Left { get; set; } = new InputBinding(KeyboardKeys.A);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.D)]
        [EditorDisplay("Viewport"), EditorOrder(1530)]
        public static InputBinding Right { get; set; } = new InputBinding(KeyboardKeys.D);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.E)]
        [EditorDisplay("Viewport"), EditorOrder(1540)]
        public static InputBinding Up { get; set; } = new InputBinding(KeyboardKeys.E);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.Q)]
        [EditorDisplay("Viewport"), EditorOrder(1550)]
        public static InputBinding Down { get; set; } = new InputBinding(KeyboardKeys.Q);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.Alt + "+" + MouseButtonString.Left)]
        [EditorDisplay("Viewport"), EditorOrder(1551)]
        public static InputBinding Orbit { get; set; } = new InputBinding(MouseButton.Middle + "+" + KeyboardKeys.Alt);

        [DefaultValue(typeof(InputBinding), MouseButtonString.Middle)]
        [EditorDisplay("Viewport"), EditorOrder(1551)]
        public static InputBinding Pan { get; set; } = new InputBinding(MouseButton.Middle);

        [DefaultValue(typeof(InputBinding), MouseButtonString.Right)]
        [EditorDisplay("Viewport"), EditorOrder(1551)]
        public static InputBinding Rotate { get; set; } = new InputBinding(MouseButton.Right);

        [DefaultValue(typeof(InputBinding), MouseScrollString.Up)]
        [EditorDisplay("Viewport"), EditorOrder(1551)]
        public static InputBinding ZoomIn { get; set; } = new InputBinding(MouseScroll.ScrollUp);

        [DefaultValue(typeof(InputBinding), MouseScrollString.Down)]
        [EditorDisplay("Viewport"), EditorOrder(1551)]
        public static InputBinding ZoomOut { get; set; } = new InputBinding(MouseScroll.ScrollDown);

        [DefaultValue(typeof(InputBinding), "")]
        [EditorDisplay("Viewport", "Toggle Camera Rotation"), EditorOrder(1560)]
        public static InputBinding CameraToggleRotation { get; set; } = new InputBinding(KeyboardKeys.None);

        [DefaultValue(typeof(InputBinding), "")]
        [EditorDisplay("Viewport", "Increase Camera Move Speed"), EditorOrder(1570)]
        public static InputBinding CameraIncreaseMoveSpeed { get; set; } = new InputBinding(KeyboardKeys.None);

        [DefaultValue(typeof(InputBinding), "")]
        [EditorDisplay("Viewport", "Decrease Camera Move Speed"), EditorOrder(1571)]
        public static InputBinding CameraDecreaseMoveSpeed { get; set; } = new InputBinding(KeyboardKeys.None);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.Numpad0)]
        [EditorDisplay("Viewport"), EditorOrder(1700)]
        public static InputBinding ViewpointFront { get; set; } = new InputBinding(KeyboardKeys.Numpad0);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.Numpad5)]
        [EditorDisplay("Viewport"), EditorOrder(1710)]
        public static InputBinding ViewpointBack { get; set; } = new InputBinding(KeyboardKeys.Numpad5);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.Numpad4)]
        [EditorDisplay("Viewport"), EditorOrder(1720)]
        public static InputBinding ViewpointLeft { get; set; } = new InputBinding(KeyboardKeys.Numpad4);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.Numpad6)]
        [EditorDisplay("Viewport"), EditorOrder(1730)]
        public static InputBinding ViewpointRight { get; set; } = new InputBinding(KeyboardKeys.Numpad6);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.Numpad8)]
        [EditorDisplay("Viewport"), EditorOrder(1740)]
        public static InputBinding ViewpointTop { get; set; } = new InputBinding(KeyboardKeys.Numpad8);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.Numpad2)]
        [EditorDisplay("Viewport"), EditorOrder(1750)]
        public static InputBinding ViewpointBottom { get; set; } = new InputBinding(KeyboardKeys.Numpad2);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.NumpadDecimal)]
        [EditorDisplay("Viewport"), EditorOrder(1760)]
        public static InputBinding ToggleOrthographic { get; set; } = new InputBinding(KeyboardKeys.NumpadDecimal);

        #endregion

        #region Interface

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.Control + "+" + KeyboardKeysString.W)]
        [EditorDisplay("Interface"), EditorOrder(2000)]
        public static InputBinding CloseTab { get; set; } = new InputBinding(KeyboardKeys.W + "+" + KeyboardKeys.Control);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.Control + "+" + KeyboardKeysString.Tab)]
        [EditorDisplay("Interface"), EditorOrder(2010)]
        public static InputBinding NextTab { get; set; } = new InputBinding(KeyboardKeys.Tab + "+" + KeyboardKeys.Control);

        [DefaultValue(typeof(InputBinding), KeyboardKeysString.Control + "+" + KeyboardKeysString.Shift + "+" + KeyboardKeysString.Tab)]
        [EditorDisplay("Interface"), EditorOrder(2020)]
        public static InputBinding PreviousTab { get; set; } = new InputBinding(KeyboardKeys.Tab + "+" + KeyboardKeys.Control + "+" + KeyboardKeys.Shift);

        [DefaultValue(SceneNodeDoubleClick.Expand)]
        [EditorDisplay("Interface"), EditorOrder(2030)]
        public SceneNodeDoubleClick DoubleClickSceneNode = SceneNodeDoubleClick.Expand;

        #endregion

        //populate the dictionary for our ground truth inputs
        public InputOptions()
        {
            var type = typeof(InputOptions);
            foreach (InputOptionName name in Enum.GetValues(typeof(InputOptionName)))
            {
                var field = type.GetField(name.ToString());
                if (field != null && field.FieldType == typeof(InputBinding))
                {
                    var binding = (InputBinding)field.GetValue(this);
                    Dictionary[name] = binding;
                }
                else
                {
                    Debug.LogWarning($"InputOptionName '{name}' does not match any InputBinding field.");
                }
            }
        }
    }
}
