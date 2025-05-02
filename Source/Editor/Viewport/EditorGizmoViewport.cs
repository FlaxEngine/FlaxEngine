// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.Gizmo;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Input;
using FlaxEditor.SceneGraph;
using FlaxEditor.Viewport.Cameras;
using FlaxEditor.Viewport.Widgets;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Viewport
{
    /// <summary>
    /// Viewport with free camera and gizmo tools.
    /// </summary>
    /// <seealso cref="FlaxEditor.Viewport.EditorViewport" />
    /// <seealso cref="IGizmoOwner" />
    public abstract class EditorGizmoViewport : EditorViewport, IGizmoOwner
    {
        private UpdateDelegate _update;

        /// <summary>
        /// Initializes a new instance of the <see cref="EditorGizmoViewport"/> class.
        /// </summary>
        /// <param name="task">The task.</param>
        /// <param name="undo">The undo.</param>
        /// <param name="sceneGraphRoot">The scene graph root.</param>
        public EditorGizmoViewport(SceneRenderTask task, Undo undo, SceneGraph.RootNode sceneGraphRoot)
        : base(task, new FPSCamera(), true)
        {
            Undo = undo;
            SceneGraphRoot = sceneGraphRoot;
            Gizmos = new GizmosCollection(this);

            SetUpdate(ref _update, OnUpdate);
        }

        private void OnUpdate(float deltaTime)
        {
            for (int i = 0; i < Gizmos.Count; i++)
            {
                Gizmos[i].Update(deltaTime);
            }
        }

        /// <inheritdoc />
        public EditorViewport Viewport => this;

        /// <inheritdoc />
        public GizmosCollection Gizmos { get; }

        /// <inheritdoc />
        public SceneRenderTask RenderTask => Task;

        /// <inheritdoc />
        public float ViewFarPlane => FarPlane;

        /// <inheritdoc />
        public bool IsLeftMouseButtonDown => _input.IsMouseLeftDown;

        /// <inheritdoc />
        public bool IsRightMouseButtonDown => _input.IsMouseRightDown;

        /// <inheritdoc />
        public bool IsAltKeyDown => _input.IsAltDown;

        /// <inheritdoc />
        public bool IsControlDown => _input.IsControlDown;

        /// <inheritdoc />
        public bool SnapToGround => ContainsFocus && Editor.Instance.Options.Options.Input.SnapToGround.Process(Root);

        /// <inheritdoc />
        public bool SnapToVertex => ContainsFocus && Editor.Instance.Options.Options.Input.SnapToVertex.Process(Root);

        /// <inheritdoc />
        public Float2 MouseDelta => _mouseDelta;

        /// <inheritdoc />
        public bool UseSnapping => Root?.GetKey(KeyboardKeys.Control) ?? false;

        /// <inheritdoc />
        public bool UseDuplicate => Root?.GetKey(KeyboardKeys.Shift) ?? false;

        /// <inheritdoc />
        public Undo Undo { get; }

        /// <inheritdoc />
        public SceneGraph.RootNode SceneGraphRoot { get; }

        /// <inheritdoc />
        public abstract void Select(List<SceneGraphNode> nodes);

        /// <inheritdoc />
        public abstract void Spawn(Actor actor);

        /// <inheritdoc />
        public abstract void OpenContextMenu();

        /// <inheritdoc />
        protected override bool IsControllingMouse => Gizmos.Active?.IsControllingMouse ?? false;

        /// <inheritdoc />
        protected override void AddUpdateCallbacks(RootControl root)
        {
            base.AddUpdateCallbacks(root);

            root.UpdateCallbacksToAdd.Add(_update);
        }

        /// <inheritdoc />
        protected override void RemoveUpdateCallbacks(RootControl root)
        {
            base.RemoveUpdateCallbacks(root);

            root.UpdateCallbacksToRemove.Add(_update);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (IsDisposing)
                return;

            Gizmos.Clear();

            base.OnDestroy();
        }

        internal static void AddGizmoViewportWidgets(EditorViewport viewport, TransformGizmo transformGizmo, bool useProjectCache = false)
        {
            var editor = Editor.Instance;
            var inputOptions = editor.Options.Options.Input;

            if (useProjectCache)
            {
                // Initialize snapping enabled from cached values
                if (editor.ProjectCache.TryGetCustomData("AbsoluteSnapState", out bool cachedBool))
                    transformGizmo.AbsoluteSnapEnabled = cachedBool;
                if (editor.ProjectCache.TryGetCustomData("TranslateSnapState", out cachedBool))
                    transformGizmo.TranslationSnapEnable = cachedBool;
                if (editor.ProjectCache.TryGetCustomData("RotationSnapState", out cachedBool))
                    transformGizmo.RotationSnapEnabled = cachedBool;
                if (editor.ProjectCache.TryGetCustomData("ScaleSnapState", out cachedBool))
                    transformGizmo.ScaleSnapEnabled = cachedBool;
                if (editor.ProjectCache.TryGetCustomData("TranslateSnapValue", out float cachedFloat))
                    transformGizmo.TranslationSnapValue = cachedFloat;
                if (editor.ProjectCache.TryGetCustomData("RotationSnapValue", out cachedFloat))
                    transformGizmo.RotationSnapValue = cachedFloat;
                if (editor.ProjectCache.TryGetCustomData("ScaleSnapValue", out cachedFloat))
                    transformGizmo.ScaleSnapValue = cachedFloat;
                if (editor.ProjectCache.TryGetCustomData("TransformSpaceState", out string cachedText) && Enum.TryParse(cachedText, out TransformGizmoBase.TransformSpace space))
                    transformGizmo.ActiveTransformSpace = space;
            }

            // Transform space widget
            var transformSpaceWidget = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperRight);
            var transformSpaceToggle = new ViewportWidgetButton(string.Empty, editor.Icons.Globe32, null, true)
            {
                Checked = transformGizmo.ActiveTransformSpace == TransformGizmoBase.TransformSpace.World,
                TooltipText = $"Gizmo transform space (world or local) ({inputOptions.ToggleTransformSpace})",
                Parent = transformSpaceWidget
            };
            transformSpaceWidget.Parent = viewport;

            // Absolute snapping widget
            var absoluteSnappingWidget = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperRight);
            var enableAbsoluteSnapping = new ViewportWidgetButton("A", SpriteHandle.Invalid, null, true)
            {
                Checked = transformGizmo.AbsoluteSnapEnabled,
                TooltipText = "Enable absolute grid snapping (world-space absolute grid, rather than object-relative grid)",
                Parent = absoluteSnappingWidget
            };
            enableAbsoluteSnapping.Toggled += _ =>
            {
                transformGizmo.AbsoluteSnapEnabled = !transformGizmo.AbsoluteSnapEnabled;
                if (useProjectCache)
                    editor.ProjectCache.SetCustomData("AbsoluteSnapState", transformGizmo.AbsoluteSnapEnabled);
            };
            absoluteSnappingWidget.Parent = viewport;

            transformSpaceToggle.Toggled += _ =>
            {
                transformGizmo.ToggleTransformSpace();
                if (useProjectCache)
                    editor.ProjectCache.SetCustomData("TransformSpaceState", transformGizmo.ActiveTransformSpace.ToString());
                absoluteSnappingWidget.Visible = transformGizmo.ActiveTransformSpace == TransformGizmoBase.TransformSpace.World;
            };

            // Scale snapping widget
            var scaleSnappingWidget = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperRight);
            var enableScaleSnapping = new ViewportWidgetButton(string.Empty, editor.Icons.ScaleSnap32, null, true)
            {
                Checked = transformGizmo.ScaleSnapEnabled,
                TooltipText = "Enable scale snapping",
                Parent = scaleSnappingWidget
            };
            enableScaleSnapping.Toggled += _ =>
            {
                transformGizmo.ScaleSnapEnabled = !transformGizmo.ScaleSnapEnabled;
                if (useProjectCache)
                    editor.ProjectCache.SetCustomData("ScaleSnapState", transformGizmo.ScaleSnapEnabled);
            };
            var scaleSnappingCM = new ContextMenu();
            var scaleSnapping = new ViewportWidgetButton(transformGizmo.ScaleSnapValue.ToString(), SpriteHandle.Invalid, scaleSnappingCM)
            {
                TooltipText = "Scale snapping values"
            };
            for (int i = 0; i < ScaleSnapValues.Length; i++)
            {
                var v = ScaleSnapValues[i];
                var button = scaleSnappingCM.AddButton(v.ToString());
                button.Tag = v;
            }
            scaleSnappingCM.ButtonClicked += button =>
            {
                var v = (float)button.Tag;
                transformGizmo.ScaleSnapValue = v;
                scaleSnapping.Text = v.ToString();
                if (useProjectCache)
                    editor.ProjectCache.SetCustomData("ScaleSnapValue", transformGizmo.ScaleSnapValue);
            };
            scaleSnappingCM.VisibleChanged += control =>
            {
                if (control.Visible == false)
                    return;
                var ccm = (ContextMenu)control;
                foreach (var e in ccm.Items)
                {
                    if (e is ContextMenuButton b)
                    {
                        var v = (float)b.Tag;
                        b.Icon = Mathf.Abs(transformGizmo.ScaleSnapValue - v) < 0.001f ? Style.Current.CheckBoxTick : SpriteHandle.Invalid;
                    }
                }
            };
            scaleSnapping.Parent = scaleSnappingWidget;
            scaleSnappingWidget.Parent = viewport;

            // Rotation snapping widget
            var rotateSnappingWidget = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperRight);
            var enableRotateSnapping = new ViewportWidgetButton(string.Empty, editor.Icons.RotateSnap32, null, true)
            {
                Checked = transformGizmo.RotationSnapEnabled,
                TooltipText = "Enable rotation snapping",
                Parent = rotateSnappingWidget
            };
            enableRotateSnapping.Toggled += _ =>
            {
                transformGizmo.RotationSnapEnabled = !transformGizmo.RotationSnapEnabled;
                if (useProjectCache)
                    editor.ProjectCache.SetCustomData("RotationSnapState", transformGizmo.RotationSnapEnabled);
            };
            var rotateSnappingCM = new ContextMenu();
            var rotateSnapping = new ViewportWidgetButton(transformGizmo.RotationSnapValue.ToString(), SpriteHandle.Invalid, rotateSnappingCM)
            {
                TooltipText = "Rotation snapping values"
            };
            for (int i = 0; i < RotateSnapValues.Length; i++)
            {
                var v = RotateSnapValues[i];
                var button = rotateSnappingCM.AddButton(v.ToString());
                button.Tag = v;
            }
            rotateSnappingCM.ButtonClicked += button =>
            {
                var v = (float)button.Tag;
                transformGizmo.RotationSnapValue = v;
                rotateSnapping.Text = v.ToString();
                if (useProjectCache)
                    editor.ProjectCache.SetCustomData("RotationSnapValue", transformGizmo.RotationSnapValue);
            };
            rotateSnappingCM.VisibleChanged += control =>
            {
                if (control.Visible == false)
                    return;
                var ccm = (ContextMenu)control;
                foreach (var e in ccm.Items)
                {
                    if (e is ContextMenuButton b)
                    {
                        var v = (float)b.Tag;
                        b.Icon = Mathf.Abs(transformGizmo.RotationSnapValue - v) < 0.001f ? Style.Current.CheckBoxTick : SpriteHandle.Invalid;
                    }
                }
            };
            rotateSnapping.Parent = rotateSnappingWidget;
            rotateSnappingWidget.Parent = viewport;

            // Translation snapping widget
            var translateSnappingWidget = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperRight);
            var enableTranslateSnapping = new ViewportWidgetButton(string.Empty, editor.Icons.Grid32, null, true)
            {
                Checked = transformGizmo.TranslationSnapEnable,
                TooltipText = "Enable position snapping",
                Parent = translateSnappingWidget
            };
            enableTranslateSnapping.Toggled += _ =>
            {
                transformGizmo.TranslationSnapEnable = !transformGizmo.TranslationSnapEnable;
                if (useProjectCache)
                    editor.ProjectCache.SetCustomData("TranslateSnapState", transformGizmo.TranslationSnapEnable);
            };
            var translateSnappingCM = new ContextMenu();
            var translateSnapping = new ViewportWidgetButton(transformGizmo.TranslationSnapValue.ToString(), SpriteHandle.Invalid, translateSnappingCM)
            {
                TooltipText = "Position snapping values"
            };
            if (transformGizmo.TranslationSnapValue < 0.0f)
                translateSnapping.Text = "Bounding Box";
            for (int i = 0; i < TranslateSnapValues.Length; i++)
            {
                var v = TranslateSnapValues[i];
                var button = translateSnappingCM.AddButton(v.ToString());
                button.Tag = v;
            }
            var buttonBB = translateSnappingCM.AddButton("Bounding Box").LinkTooltip("Snaps the selection based on it's bounding volume");
            buttonBB.Tag = -1.0f;
            var buttonCustom = translateSnappingCM.AddButton("Custom");
            buttonCustom.LinkTooltip("Custom grid size");
            const float defaultCustomTranslateSnappingValue = 250.0f;
            float customTranslateSnappingValue = transformGizmo.TranslationSnapValue;
            if (customTranslateSnappingValue < 0.0f)
                customTranslateSnappingValue = defaultCustomTranslateSnappingValue;
            foreach (var v in TranslateSnapValues)
            {
                if (Mathf.Abs(transformGizmo.TranslationSnapValue - v) < 0.001f)
                {
                    customTranslateSnappingValue = defaultCustomTranslateSnappingValue;
                    break;
                }
            }
            buttonCustom.Tag = customTranslateSnappingValue;
            var customValue = new FloatValueBox(customTranslateSnappingValue, Style.Current.FontMedium.MeasureText(buttonCustom.Text).X + 5, 2, 70.0f, 0.001f, float.MaxValue, 0.1f)
            {
                Parent = buttonCustom
            };
            customValue.ValueChanged += () =>
            {
                buttonCustom.Tag = customValue.Value;
                buttonCustom.Click();
            };
            translateSnappingCM.ButtonClicked += b =>
            {
                var v = (float)b.Tag;
                transformGizmo.TranslationSnapValue = v;
                if (v < 0.0f)
                    translateSnapping.Text = "Bounding Box";
                else
                    translateSnapping.Text = v.ToString();
                if (useProjectCache)
                    editor.ProjectCache.SetCustomData("TranslateSnapValue", transformGizmo.TranslationSnapValue);
            };
            translateSnappingCM.VisibleChanged += control =>
            {
                if (control.Visible == false)
                    return;
                var ccm = (ContextMenu)control;
                foreach (var e in ccm.Items)
                {
                    if (e is ContextMenuButton b)
                    {
                        var v = (float)b.Tag;
                        b.Icon = Mathf.Abs(transformGizmo.TranslationSnapValue - v) < 0.001f ? Style.Current.CheckBoxTick : SpriteHandle.Invalid;
                    }
                }
            };
            translateSnapping.Parent = translateSnappingWidget;
            translateSnappingWidget.Parent = viewport;

            // Gizmo mode widget
            var gizmoMode = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperRight);
            var gizmoModeTranslate = new ViewportWidgetButton(string.Empty, editor.Icons.Translate32, null, true)
            {
                Tag = TransformGizmoBase.Mode.Translate,
                TooltipText = $"Translate gizmo mode ({inputOptions.TranslateMode})",
                Checked = true,
                Parent = gizmoMode
            };
            gizmoModeTranslate.Toggled += _ => transformGizmo.ActiveMode = TransformGizmoBase.Mode.Translate;
            var gizmoModeRotate = new ViewportWidgetButton(string.Empty, editor.Icons.Rotate32, null, true)
            {
                Tag = TransformGizmoBase.Mode.Rotate,
                TooltipText = $"Rotate gizmo mode ({inputOptions.RotateMode})",
                Parent = gizmoMode
            };
            gizmoModeRotate.Toggled += _ => transformGizmo.ActiveMode = TransformGizmoBase.Mode.Rotate;
            var gizmoModeScale = new ViewportWidgetButton(string.Empty, editor.Icons.Scale32, null, true)
            {
                Tag = TransformGizmoBase.Mode.Scale,
                TooltipText = $"Scale gizmo mode ({inputOptions.ScaleMode})",
                Parent = gizmoMode
            };
            gizmoModeScale.Toggled += _ => transformGizmo.ActiveMode = TransformGizmoBase.Mode.Scale;
            gizmoMode.Parent = viewport;
            transformGizmo.ModeChanged += () =>
            {
                var mode = transformGizmo.ActiveMode;
                gizmoModeTranslate.Checked = mode == TransformGizmoBase.Mode.Translate;
                gizmoModeRotate.Checked = mode == TransformGizmoBase.Mode.Rotate;
                gizmoModeScale.Checked = mode == TransformGizmoBase.Mode.Scale;
            };

            // Setup input actions
            viewport.InputActions.Add(options => options.TranslateMode, () =>
            {
                viewport.GetInput(out var input);
                if (input.IsMouseRightDown)
                    return;

                transformGizmo.ActiveMode = TransformGizmoBase.Mode.Translate;
            });
            viewport.InputActions.Add(options => options.RotateMode, () =>
            {
                viewport.GetInput(out var input);
                if (input.IsMouseRightDown)
                    return;

                transformGizmo.ActiveMode = TransformGizmoBase.Mode.Rotate;
            });
            viewport.InputActions.Add(options => options.ScaleMode, () =>
            {
                viewport.GetInput(out var input);
                if (input.IsMouseRightDown)
                    return;

                transformGizmo.ActiveMode = TransformGizmoBase.Mode.Scale;
            });
            viewport.InputActions.Add(options => options.ToggleTransformSpace, () =>
            {
                viewport.GetInput(out var input);
                if (input.IsMouseRightDown)
                    return;

                transformGizmo.ToggleTransformSpace();
                if (useProjectCache)
                    editor.ProjectCache.SetCustomData("TransformSpaceState", transformGizmo.ActiveTransformSpace.ToString());
                transformSpaceToggle.Checked = !transformSpaceToggle.Checked;
            });
        }

        internal static readonly float[] TranslateSnapValues =
        {
            0.1f,
            0.5f,
            1.0f,
            5.0f,
            10.0f,
            100.0f,
            1000.0f,
        };

        internal static readonly float[] RotateSnapValues =
        {
            1.0f,
            5.0f,
            10.0f,
            15.0f,
            30.0f,
            45.0f,
            60.0f,
            90.0f,
        };

        internal static readonly float[] ScaleSnapValues =
        {
            0.05f,
            0.1f,
            0.25f,
            0.5f,
            1.0f,
            2.0f,
            4.0f,
            6.0f,
            8.0f,
        };
    }
}
