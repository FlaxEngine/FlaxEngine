// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System.ComponentModel;
using FlaxEngine;

#pragma warning disable 1591

namespace FlaxEditor.Options
{
    /// <summary>
    /// Input editor options data container.
    /// </summary>
    [CustomEditor(typeof(Editor<InputOptions>))]
    [HideInEditor]
    public sealed class InputOptions
    {
        #region Common

        [DefaultValue(typeof(InputBinding), "Ctrl+S")]
        [EditorDisplay("Common"), EditorOrder(100)]
        public InputBinding Save = new InputBinding(KeyboardKeys.S, KeyboardKeys.Control);

        [DefaultValue(typeof(InputBinding), "F2")]
        [EditorDisplay("Common"), EditorOrder(110)]
        public InputBinding Rename = new InputBinding(KeyboardKeys.F2);

        [DefaultValue(typeof(InputBinding), "Ctrl+C")]
        [EditorDisplay("Common"), EditorOrder(120)]
        public InputBinding Copy = new InputBinding(KeyboardKeys.C, KeyboardKeys.Control);

        [DefaultValue(typeof(InputBinding), "Ctrl+X")]
        [EditorDisplay("Common"), EditorOrder(130)]
        public InputBinding Cut = new InputBinding(KeyboardKeys.X, KeyboardKeys.Control);

        [DefaultValue(typeof(InputBinding), "Ctrl+V")]
        [EditorDisplay("Common"), EditorOrder(140)]
        public InputBinding Paste = new InputBinding(KeyboardKeys.V, KeyboardKeys.Control);

        [DefaultValue(typeof(InputBinding), "Ctrl+D")]
        [EditorDisplay("Common"), EditorOrder(150)]
        public InputBinding Duplicate = new InputBinding(KeyboardKeys.D, KeyboardKeys.Control);

        [DefaultValue(typeof(InputBinding), "Delete")]
        [EditorDisplay("Common"), EditorOrder(160)]
        public InputBinding Delete = new InputBinding(KeyboardKeys.Delete);

        [DefaultValue(typeof(InputBinding), "Ctrl+Z")]
        [EditorDisplay("Common"), EditorOrder(170)]
        public InputBinding Undo = new InputBinding(KeyboardKeys.Z, KeyboardKeys.Control);

        [DefaultValue(typeof(InputBinding), "Ctrl+Y")]
        [EditorDisplay("Common"), EditorOrder(180)]
        public InputBinding Redo = new InputBinding(KeyboardKeys.Y, KeyboardKeys.Control);

        [DefaultValue(typeof(InputBinding), "Ctrl+A")]
        [EditorDisplay("Common"), EditorOrder(190)]
        public InputBinding SelectAll = new InputBinding(KeyboardKeys.A, KeyboardKeys.Control);

        [DefaultValue(typeof(InputBinding), "F")]
        [EditorDisplay("Common"), EditorOrder(200)]
        public InputBinding FocusSelection = new InputBinding(KeyboardKeys.F);

        [DefaultValue(typeof(InputBinding), "Shift+F")]
        [EditorDisplay("Common"), EditorOrder(200)]
        public InputBinding LockFocusSelection = new InputBinding(KeyboardKeys.F, KeyboardKeys.Shift);

        [DefaultValue(typeof(InputBinding), "Ctrl+F")]
        [EditorDisplay("Common"), EditorOrder(210)]
        public InputBinding Search = new InputBinding(KeyboardKeys.F, KeyboardKeys.Control);

        [DefaultValue(typeof(InputBinding), "Ctrl+O")]
        [EditorDisplay("Common"), EditorOrder(220)]
        public InputBinding ContentFinder = new InputBinding(KeyboardKeys.O, KeyboardKeys.Control);

        [DefaultValue(typeof(InputBinding), "R")]
        [EditorDisplay("Common"), EditorOrder(230)]
        public InputBinding RotateSelection = new InputBinding(KeyboardKeys.R);

        #endregion

        #region Scene

        [DefaultValue(typeof(InputBinding), "End")]
        [EditorDisplay("Scene", "Snap To Ground"), EditorOrder(500)]
        public InputBinding SnapToGround = new InputBinding(KeyboardKeys.End);

        [DefaultValue(typeof(InputBinding), "F5")]
        [EditorDisplay("Scene", "Play/Stop"), EditorOrder(510)]
        public InputBinding Play = new InputBinding(KeyboardKeys.F5);

        [DefaultValue(typeof(InputBinding), "None")]
        [EditorDisplay("Scene", "Play Current Scenes/Stop"), EditorOrder(520)]
        public InputBinding PlayCurrentScenes = new InputBinding(KeyboardKeys.None);

        [DefaultValue(typeof(InputBinding), "F6")]
        [EditorDisplay("Scene"), EditorOrder(530)]
        public InputBinding Pause = new InputBinding(KeyboardKeys.F6);

        [DefaultValue(typeof(InputBinding), "F11")]
        [EditorDisplay("Scene"), EditorOrder(540)]
        public InputBinding StepFrame = new InputBinding(KeyboardKeys.F11);

        [DefaultValue(typeof(InputBinding), "None")]
        [EditorDisplay("Scene", "Cook & Run"), EditorOrder(550)]
        public InputBinding CookAndRun = new InputBinding(KeyboardKeys.None);

        [DefaultValue(typeof(InputBinding), "None")]
        [EditorDisplay("Scene", "Run cooked game"), EditorOrder(560)]
        public InputBinding RunCookedGame = new InputBinding(KeyboardKeys.None);

        #endregion

        #region Debugger

        [DefaultValue(typeof(InputBinding), "F5")]
        [EditorDisplay("Debugger", "Continue"), EditorOrder(610)]
        public InputBinding DebuggerContinue = new InputBinding(KeyboardKeys.F5);

        [DefaultValue(typeof(InputBinding), "F10")]
        [EditorDisplay("Debugger", "Step Over"), EditorOrder(620)]
        public InputBinding DebuggerStepOver = new InputBinding(KeyboardKeys.F10);

        [DefaultValue(typeof(InputBinding), "F11")]
        [EditorDisplay("Debugger", "Step Into"), EditorOrder(630)]
        public InputBinding DebuggerStepInto = new InputBinding(KeyboardKeys.F11);

        [DefaultValue(typeof(InputBinding), "Shift+F11")]
        [EditorDisplay("Debugger", "Step Out"), EditorOrder(640)]
        public InputBinding DebuggerStepOut = new InputBinding(KeyboardKeys.F11, KeyboardKeys.Shift);

        #endregion

        #region Gizmo

        [DefaultValue(typeof(InputBinding), "Alpha1")]
        [EditorDisplay("Gizmo"), EditorOrder(1000)]
        public InputBinding TranslateMode = new InputBinding(KeyboardKeys.Alpha1);

        [DefaultValue(typeof(InputBinding), "Alpha2")]
        [EditorDisplay("Gizmo"), EditorOrder(1010)]
        public InputBinding RotateMode = new InputBinding(KeyboardKeys.Alpha2);

        [DefaultValue(typeof(InputBinding), "Alpha3")]
        [EditorDisplay("Gizmo"), EditorOrder(1020)]
        public InputBinding ScaleMode = new InputBinding(KeyboardKeys.Alpha3);

        [DefaultValue(typeof(InputBinding), "Alpha4")]
        [EditorDisplay("Gizmo"), EditorOrder(1030)]
        public InputBinding ToggleTransformSpace = new InputBinding(KeyboardKeys.Alpha4);

        #endregion

        #region Viewport

        [DefaultValue(typeof(InputBinding), "W")]
        [EditorDisplay("Viewport"), EditorOrder(1500)]
        public InputBinding Forward = new InputBinding(KeyboardKeys.W);

        [DefaultValue(typeof(InputBinding), "S")]
        [EditorDisplay("Viewport"), EditorOrder(1510)]
        public InputBinding Backward = new InputBinding(KeyboardKeys.S);

        [DefaultValue(typeof(InputBinding), "A")]
        [EditorDisplay("Viewport"), EditorOrder(1520)]
        public InputBinding Left = new InputBinding(KeyboardKeys.A);

        [DefaultValue(typeof(InputBinding), "D")]
        [EditorDisplay("Viewport"), EditorOrder(1530)]
        public InputBinding Right = new InputBinding(KeyboardKeys.D);

        [DefaultValue(typeof(InputBinding), "E")]
        [EditorDisplay("Viewport"), EditorOrder(1540)]
        public InputBinding Up = new InputBinding(KeyboardKeys.E);

        [DefaultValue(typeof(InputBinding), "Q")]
        [EditorDisplay("Viewport"), EditorOrder(1550)]
        public InputBinding Down = new InputBinding(KeyboardKeys.Q);

        [DefaultValue(typeof(InputBinding), "Numpad0")]
        [EditorDisplay("Viewport"), EditorOrder(1600)]
        public InputBinding ViewpointFront = new InputBinding(KeyboardKeys.Numpad0);

        [DefaultValue(typeof(InputBinding), "Numpad5")]
        [EditorDisplay("Viewport"), EditorOrder(1610)]
        public InputBinding ViewpointBack = new InputBinding(KeyboardKeys.Numpad5);

        [DefaultValue(typeof(InputBinding), "Numpad4")]
        [EditorDisplay("Viewport"), EditorOrder(1620)]
        public InputBinding ViewpointLeft = new InputBinding(KeyboardKeys.Numpad4);

        [DefaultValue(typeof(InputBinding), "Numpad6")]
        [EditorDisplay("Viewport"), EditorOrder(1630)]
        public InputBinding ViewpointRight = new InputBinding(KeyboardKeys.Numpad6);

        [DefaultValue(typeof(InputBinding), "Numpad8")]
        [EditorDisplay("Viewport"), EditorOrder(1640)]
        public InputBinding ViewpointTop = new InputBinding(KeyboardKeys.Numpad8);

        [DefaultValue(typeof(InputBinding), "Numpad2")]
        [EditorDisplay("Viewport"), EditorOrder(1650)]
        public InputBinding ViewpointBottom = new InputBinding(KeyboardKeys.Numpad2);

        #endregion

        #region Interface

        [DefaultValue(typeof(InputBinding), "Ctrl+W")]
        [EditorDisplay("Interface"), EditorOrder(2000)]
        public InputBinding CloseTab = new InputBinding(KeyboardKeys.W, KeyboardKeys.Control);

        [DefaultValue(typeof(InputBinding), "Ctrl+Tab")]
        [EditorDisplay("Interface"), EditorOrder(2010)]
        public InputBinding NextTab = new InputBinding(KeyboardKeys.Tab, KeyboardKeys.Control);

        [DefaultValue(typeof(InputBinding), "Shift+Ctrl+Tab")]
        [EditorDisplay("Interface"), EditorOrder(2020)]
        public InputBinding PreviousTab = new InputBinding(KeyboardKeys.Tab, KeyboardKeys.Control, KeyboardKeys.Shift);

        #endregion
    }
}
