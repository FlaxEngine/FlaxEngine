// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.Content;
using FlaxEditor.Gizmo;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.SceneGraph;
using FlaxEditor.Scripting;
using FlaxEditor.Viewport.Cameras;
using FlaxEditor.Viewport.Previews;
using FlaxEditor.Viewport.Widgets;
using FlaxEditor.Windows.Assets;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Viewport
{
    /// <summary>
    /// Editor viewport used by the <see cref="PrefabWindow"/>
    /// </summary>
    /// <seealso cref="PrefabWindow" />
    /// <seealso cref="PrefabPreview" />
    /// <seealso cref="IGizmoOwner" />
    public class PrefabWindowViewport : PrefabPreview, IGizmoOwner
    {
        [HideInEditor]
        private sealed class PrefabSpritesRenderer : MainEditorGizmoViewport.EditorSpritesRenderer
        {
            public PrefabWindowViewport Viewport;

            /// <inheritdoc />
            public override bool CanRender()
            {
                return (Task.View.Flags & ViewFlags.EditorSprites) == ViewFlags.EditorSprites && Enabled;
            }

            protected override void Draw(ref RenderContext renderContext)
            {
                ViewportIconsRenderer.DrawIcons(ref renderContext, Viewport.Instance);
            }
        }

        private readonly PrefabWindow _window;
        private UpdateDelegate _update;

        private readonly ViewportWidgetButton _gizmoModeTranslate;
        private readonly ViewportWidgetButton _gizmoModeRotate;
        private readonly ViewportWidgetButton _gizmoModeScale;

        private ViewportWidgetButton _translateSnappng;
        private ViewportWidgetButton _rotateSnapping;
        private ViewportWidgetButton _scaleSnapping;

        private readonly ViewportDebugDrawData _debugDrawData = new ViewportDebugDrawData(32);
        private PrefabSpritesRenderer _spritesRenderer;
        private IntPtr _tempDebugDrawContext;

        /// <summary>
        /// Drag and drop handlers
        /// </summary>
        public readonly ViewportDragHandlers DragHandlers;

        /// <summary>
        /// The transform gizmo.
        /// </summary>
        public readonly TransformGizmo TransformGizmo;

        /// <summary>
        /// The selection outline postFx.
        /// </summary>
        public SelectionOutline SelectionOutline;

        /// <summary>
        /// Initializes a new instance of the <see cref="PrefabWindowViewport"/> class.
        /// </summary>
        /// <param name="window">Editor window.</param>
        public PrefabWindowViewport(PrefabWindow window)
        : base(true)
        {
            _window = window;
            _window.SelectionChanged += OnSelectionChanged;
            Undo = window.Undo;
            ViewportCamera = new FPSCamera();
            DragHandlers = new ViewportDragHandlers(this, this, ValidateDragItem, ValidateDragActorType, ValidateDragScriptItem);
            ShowDebugDraw = true;
            ShowEditorPrimitives = true;
            Gizmos = new GizmosCollection(this);
            var inputOptions = window.Editor.Options.Options.Input;

            // Prepare rendering task
            Task.ActorsSource = ActorsSources.CustomActors;
            Task.ViewFlags = ViewFlags.DefaultEditor;
            Task.Begin += OnBegin;
            Task.CollectDrawCalls += OnCollectDrawCalls;
            Task.PostRender += OnPostRender;

            // Create post effects
            SelectionOutline = FlaxEngine.Object.New<SelectionOutline>();
            SelectionOutline.SelectionGetter = () => TransformGizmo.SelectedParents;
            Task.AddCustomPostFx(SelectionOutline);
            _spritesRenderer = FlaxEngine.Object.New<PrefabSpritesRenderer>();
            _spritesRenderer.Task = Task;
            _spritesRenderer.Viewport = this;
            Task.AddCustomPostFx(_spritesRenderer);

            // Add transformation gizmo
            TransformGizmo = new TransformGizmo(this);
            TransformGizmo.ApplyTransformation += ApplyTransform;
            TransformGizmo.ModeChanged += OnGizmoModeChanged;
            TransformGizmo.Duplicate += _window.Duplicate;
            Gizmos.Active = TransformGizmo;

            // Transform space widget
            var transformSpaceWidget = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperRight);
            var transformSpaceToggle = new ViewportWidgetButton(string.Empty, window.Editor.Icons.Globe32, null, true)
            {
                Checked = TransformGizmo.ActiveTransformSpace == TransformGizmoBase.TransformSpace.World,
                TooltipText = $"Gizmo transform space (world or local) ({inputOptions.ToggleTransformSpace})",
                Parent = transformSpaceWidget
            };
            transformSpaceToggle.Toggled += OnTransformSpaceToggle;
            transformSpaceWidget.Parent = this;

            // Scale snapping widget
            var scaleSnappingWidget = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperRight);
            var enableScaleSnapping = new ViewportWidgetButton(string.Empty, window.Editor.Icons.ScaleSnap32, null, true)
            {
                Checked = TransformGizmo.ScaleSnapEnabled,
                TooltipText = "Enable scale snapping",
                Parent = scaleSnappingWidget
            };
            enableScaleSnapping.Toggled += OnScaleSnappingToggle;
            var scaleSnappingCM = new ContextMenu();
            _scaleSnapping = new ViewportWidgetButton(TransformGizmo.ScaleSnapValue.ToString(), SpriteHandle.Invalid, scaleSnappingCM)
            {
                TooltipText = "Scale snapping values"
            };
            for (int i = 0; i < EditorGizmoViewport.ScaleSnapValues.Length; i++)
            {
                var v = EditorGizmoViewport.ScaleSnapValues[i];
                var button = scaleSnappingCM.AddButton(v.ToString());
                button.Tag = v;
            }
            scaleSnappingCM.ButtonClicked += OnWidgetScaleSnapClick;
            scaleSnappingCM.VisibleChanged += OnWidgetScaleSnapShowHide;
            _scaleSnapping.Parent = scaleSnappingWidget;
            scaleSnappingWidget.Parent = this;

            // Rotation snapping widget
            var rotateSnappingWidget = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperRight);
            var enableRotateSnapping = new ViewportWidgetButton(string.Empty, window.Editor.Icons.RotateSnap32, null, true)
            {
                Checked = TransformGizmo.RotationSnapEnabled,
                TooltipText = "Enable rotation snapping",
                Parent = rotateSnappingWidget
            };
            enableRotateSnapping.Toggled += OnRotateSnappingToggle;
            var rotateSnappingCM = new ContextMenu();
            _rotateSnapping = new ViewportWidgetButton(TransformGizmo.RotationSnapValue.ToString(), SpriteHandle.Invalid, rotateSnappingCM)
            {
                TooltipText = "Rotation snapping values"
            };
            for (int i = 0; i < EditorGizmoViewport.RotateSnapValues.Length; i++)
            {
                var v = EditorGizmoViewport.RotateSnapValues[i];
                var button = rotateSnappingCM.AddButton(v.ToString());
                button.Tag = v;
            }
            rotateSnappingCM.ButtonClicked += OnWidgetRotateSnapClick;
            rotateSnappingCM.VisibleChanged += OnWidgetRotateSnapShowHide;
            _rotateSnapping.Parent = rotateSnappingWidget;
            rotateSnappingWidget.Parent = this;

            // Translation snapping widget
            var translateSnappingWidget = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperRight);
            var enableTranslateSnapping = new ViewportWidgetButton(string.Empty, window.Editor.Icons.Grid32, null, true)
            {
                Checked = TransformGizmo.TranslationSnapEnable,
                TooltipText = "Enable position snapping",
                Parent = translateSnappingWidget
            };
            enableTranslateSnapping.Toggled += OnTranslateSnappingToggle;
            var translateSnappingCM = new ContextMenu();
            _translateSnappng = new ViewportWidgetButton(TransformGizmo.TranslationSnapValue.ToString(), SpriteHandle.Invalid, translateSnappingCM)
            {
                TooltipText = "Position snapping values"
            };
            for (int i = 0; i < EditorGizmoViewport.TranslateSnapValues.Length; i++)
            {
                var v = EditorGizmoViewport.TranslateSnapValues[i];
                var button = translateSnappingCM.AddButton(v.ToString());
                button.Tag = v;
            }
            translateSnappingCM.ButtonClicked += OnWidgetTranslateSnapClick;
            translateSnappingCM.VisibleChanged += OnWidgetTranslateSnapShowHide;
            _translateSnappng.Parent = translateSnappingWidget;
            translateSnappingWidget.Parent = this;

            // Gizmo mode widget
            var gizmoMode = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperRight);
            _gizmoModeTranslate = new ViewportWidgetButton(string.Empty, window.Editor.Icons.Translate32, null, true)
            {
                Tag = TransformGizmoBase.Mode.Translate,
                TooltipText = $"Translate gizmo mode ({inputOptions.TranslateMode})",
                Checked = true,
                Parent = gizmoMode
            };
            _gizmoModeTranslate.Toggled += OnGizmoModeToggle;
            _gizmoModeRotate = new ViewportWidgetButton(string.Empty, window.Editor.Icons.Rotate32, null, true)
            {
                Tag = TransformGizmoBase.Mode.Rotate,
                TooltipText = $"Rotate gizmo mode ({inputOptions.RotateMode})",
                Parent = gizmoMode
            };
            _gizmoModeRotate.Toggled += OnGizmoModeToggle;
            _gizmoModeScale = new ViewportWidgetButton(string.Empty, window.Editor.Icons.Scale32, null, true)
            {
                Tag = TransformGizmoBase.Mode.Scale,
                TooltipText = $"Scale gizmo mode ({inputOptions.ScaleMode})",
                Parent = gizmoMode
            };
            _gizmoModeScale.Toggled += OnGizmoModeToggle;
            gizmoMode.Parent = this;

            // Setup input actions
            InputActions.Add(options => options.TranslateMode, () => TransformGizmo.ActiveMode = TransformGizmoBase.Mode.Translate);
            InputActions.Add(options => options.RotateMode, () => TransformGizmo.ActiveMode = TransformGizmoBase.Mode.Rotate);
            InputActions.Add(options => options.ScaleMode, () => TransformGizmo.ActiveMode = TransformGizmoBase.Mode.Scale);
            InputActions.Add(options => options.ToggleTransformSpace, () =>
            {
                OnTransformSpaceToggle(transformSpaceToggle);
                transformSpaceToggle.Checked = !transformSpaceToggle.Checked;
            });
            InputActions.Add(options => options.FocusSelection, ShowSelectedActors);

            SetUpdate(ref _update, OnUpdate);
        }

        private void OnUpdate(float deltaTime)
        {
            for (int i = 0; i < Gizmos.Count; i++)
            {
                Gizmos[i].Update(deltaTime);
            }
        }

        private void OnBegin(RenderTask task, GPUContext context)
        {
            _debugDrawData.Clear();

            // Collect selected objects debug shapes and visuals
            var selectedParents = TransformGizmo.SelectedParents;
            if (selectedParents.Count > 0)
            {
                // Use temporary Debug Draw context to pull any debug shapes drawing in Scene Graph Nodes - those are used in OnDebugDraw down below
                if (_tempDebugDrawContext == IntPtr.Zero)
                    _tempDebugDrawContext = DebugDraw.AllocateContext();
                DebugDraw.SetContext(_tempDebugDrawContext);
                DebugDraw.UpdateContext(_tempDebugDrawContext, 1.0f);

                for (int i = 0; i < selectedParents.Count; i++)
                {
                    if (selectedParents[i].IsActiveInHierarchy)
                        selectedParents[i].OnDebugDraw(_debugDrawData);
                }

                DebugDraw.SetContext(IntPtr.Zero);
            }
        }

        private void OnCollectDrawCalls(ref RenderContext renderContext)
        {
            DragHandlers.CollectDrawCalls(_debugDrawData, ref renderContext);
            _debugDrawData.OnDraw(ref renderContext);
        }

        private void OnPostRender(GPUContext context, ref RenderContext renderContext)
        {
            if (renderContext.View.Mode != ViewMode.Default)
            {
                var task = renderContext.Task;

                // Render editor sprites
                if (_spritesRenderer && _spritesRenderer.CanRender())
                {
                    _spritesRenderer.Render(context, ref renderContext, task.Output, task.Output);
                }

                // Render selection outline
                if (SelectionOutline && SelectionOutline.CanRender())
                {
                    // Use temporary intermediate buffer
                    var desc = task.Output.Description;
                    var temp = RenderTargetPool.Get(ref desc);
                    SelectionOutline.Render(context, ref renderContext, task.Output, temp);

                    // Copy the results back to the output
                    context.CopyTexture(task.Output, 0, 0, 0, 0, temp, 0);

                    RenderTargetPool.Release(temp);
                }
            }
        }

        /// <summary>
        /// Moves the viewport to visualize selected actors.
        /// </summary>
        public void ShowSelectedActors()
        {
            var orient = ViewOrientation;
            ((FPSCamera)ViewportCamera).ShowActors(TransformGizmo.SelectedParents, ref orient);
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
        public bool SnapToGround => false;

        /// <inheritdoc />
        public bool SnapToVertex => Editor.Instance.Options.Options.Input.SnapToVertex.Process(Root);

        /// <inheritdoc />
        public Float2 MouseDelta => _mouseDelta * 1000;

        /// <inheritdoc />
        public bool UseSnapping => Root.GetKey(KeyboardKeys.Control);

        /// <inheritdoc />
        public bool UseDuplicate => Root.GetKey(KeyboardKeys.Shift);

        /// <inheritdoc />
        public Undo Undo { get; }

        /// <inheritdoc />
        public RootNode SceneGraphRoot => _window.Graph.Root;

        /// <inheritdoc />
        public void Select(List<SceneGraphNode> nodes)
        {
            _window.Select(nodes);
        }

        /// <inheritdoc />
        public void Spawn(Actor actor)
        {
            _window.Spawn(actor);
        }

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

        private void OnGizmoModeToggle(ViewportWidgetButton button)
        {
            TransformGizmo.ActiveMode = (TransformGizmoBase.Mode)(int)button.Tag;
        }

        private void OnTranslateSnappingToggle(ViewportWidgetButton button)
        {
            TransformGizmo.TranslationSnapEnable = !TransformGizmo.TranslationSnapEnable;
        }

        private void OnRotateSnappingToggle(ViewportWidgetButton button)
        {
            TransformGizmo.RotationSnapEnabled = !TransformGizmo.RotationSnapEnabled;
        }

        private void OnScaleSnappingToggle(ViewportWidgetButton button)
        {
            TransformGizmo.ScaleSnapEnabled = !TransformGizmo.ScaleSnapEnabled;
        }

        private void OnTransformSpaceToggle(ViewportWidgetButton button)
        {
            TransformGizmo.ToggleTransformSpace();
        }

        private void OnGizmoModeChanged()
        {
            // Update all viewport widgets status
            var mode = TransformGizmo.ActiveMode;
            _gizmoModeTranslate.Checked = mode == TransformGizmoBase.Mode.Translate;
            _gizmoModeRotate.Checked = mode == TransformGizmoBase.Mode.Rotate;
            _gizmoModeScale.Checked = mode == TransformGizmoBase.Mode.Scale;
        }

        private void OnWidgetScaleSnapClick(ContextMenuButton button)
        {
            var v = (float)button.Tag;
            TransformGizmo.ScaleSnapValue = v;
            _scaleSnapping.Text = v.ToString();
        }

        private void OnWidgetScaleSnapShowHide(Control control)
        {
            if (control.Visible == false)
                return;

            var ccm = (ContextMenu)control;
            foreach (var e in ccm.Items)
            {
                if (e is ContextMenuButton b)
                {
                    var v = (float)b.Tag;
                    b.Icon = Mathf.Abs(TransformGizmo.ScaleSnapValue - v) < 0.001f ? Style.Current.CheckBoxTick : SpriteHandle.Invalid;
                }
            }
        }

        private void OnWidgetRotateSnapClick(ContextMenuButton button)
        {
            var v = (float)button.Tag;
            TransformGizmo.RotationSnapValue = v;
            _rotateSnapping.Text = v.ToString();
        }

        private void OnWidgetRotateSnapShowHide(Control control)
        {
            if (control.Visible == false)
                return;

            var ccm = (ContextMenu)control;
            foreach (var e in ccm.Items)
            {
                if (e is ContextMenuButton b)
                {
                    var v = (float)b.Tag;
                    b.Icon = Mathf.Abs(TransformGizmo.RotationSnapValue - v) < 0.001f ? Style.Current.CheckBoxTick : SpriteHandle.Invalid;
                }
            }
        }

        private void OnWidgetTranslateSnapClick(ContextMenuButton button)
        {
            var v = (float)button.Tag;
            TransformGizmo.TranslationSnapValue = v;
            _translateSnappng.Text = v.ToString();
        }

        private void OnWidgetTranslateSnapShowHide(Control control)
        {
            if (control.Visible == false)
                return;

            var ccm = (ContextMenu)control;
            foreach (var e in ccm.Items)
            {
                if (e is ContextMenuButton b)
                {
                    var v = (float)b.Tag;
                    b.Icon = Mathf.Abs(TransformGizmo.TranslationSnapValue - v) < 0.001f ? Style.Current.CheckBoxTick : SpriteHandle.Invalid;
                }
            }
        }

        private void OnSelectionChanged()
        {
            Gizmos.ForEach(x => x.OnSelectionChanged(_window.Selection));
        }

        /// <summary>
        /// Applies the transform to the collection of scene graph nodes.
        /// </summary>
        /// <param name="selection">The selection.</param>
        /// <param name="translationDelta">The translation delta.</param>
        /// <param name="rotationDelta">The rotation delta.</param>
        /// <param name="scaleDelta">The scale delta.</param>
        public void ApplyTransform(List<SceneGraphNode> selection, ref Vector3 translationDelta, ref Quaternion rotationDelta, ref Vector3 scaleDelta)
        {
            bool applyRotation = !rotationDelta.IsIdentity;
            bool useObjCenter = TransformGizmo.ActivePivot == TransformGizmoBase.PivotType.ObjectCenter;
            Vector3 gizmoPosition = TransformGizmo.Position;

            // Transform selected objects
            for (int i = 0; i < selection.Count; i++)
            {
                var obj = selection[i];
                var trans = obj.Transform;

                // Apply rotation
                if (applyRotation)
                {
                    Vector3 pivotOffset = trans.Translation - gizmoPosition;
                    if (useObjCenter || pivotOffset.IsZero)
                    {
                        //trans.Orientation *= rotationDelta;
                        trans.Orientation *= Quaternion.Invert(trans.Orientation) * rotationDelta * trans.Orientation;
                    }
                    else
                    {
                        Matrix.RotationQuaternion(ref trans.Orientation, out var transWorld);
                        Matrix.RotationQuaternion(ref rotationDelta, out var deltaWorld);
                        Matrix world = transWorld * Matrix.Translation(pivotOffset) * deltaWorld * Matrix.Translation(-pivotOffset);
                        trans.SetRotation(ref world);
                        trans.Translation += world.TranslationVector;
                    }
                }

                // Apply scale
                const float scaleLimit = 99_999_999.0f;
                trans.Scale = Float3.Clamp(trans.Scale + scaleDelta, new Float3(-scaleLimit), new Float3(scaleLimit));

                // Apply translation
                trans.Translation += translationDelta;

                obj.Transform = trans;
            }
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            // Selected UI controls outline
            bool drawAnySelectedControl = false;
            // TODO: optimize this (eg. cache list of selected UIControl's when selection gets changed)
            for (var i = 0; i < _window.Selection.Count; i++)
            {
                if (_window.Selection[i]?.EditableObject is UIControl controlActor && controlActor && controlActor.Control != null && controlActor.Control.VisibleInHierarchy && controlActor.Control.RootWindow != null)
                {
                    if (!drawAnySelectedControl)
                    {
                        drawAnySelectedControl = true;
                        Render2D.PushTransform(ref _cachedTransform);
                    }
                    var control = controlActor.Control;
                    var bounds = control.EditorBounds;
                    var p1 = control.PointToParent(this, bounds.UpperLeft);
                    var p2 = control.PointToParent(this, bounds.UpperRight);
                    var p3 = control.PointToParent(this, bounds.BottomLeft);
                    var p4 = control.PointToParent(this, bounds.BottomRight);
                    var min = Float2.Min(Float2.Min(p1, p2), Float2.Min(p3, p4));
                    var max = Float2.Max(Float2.Max(p1, p2), Float2.Max(p3, p4));
                    bounds = new Rectangle(min, Float2.Max(max - min, Float2.Zero));
                    var options = Editor.Instance.Options.Options.Visual;
                    Render2D.DrawRectangle(bounds, options.SelectionOutlineColor0, options.UISelectionOutlineSize);
                }
            }
            if (drawAnySelectedControl)
                Render2D.PopTransform();
        }

        /// <inheritdoc />
        protected override void OnLeftMouseButtonUp()
        {
            // Skip if was controlling mouse or mouse is not over the area
            if (_prevInput.IsControllingMouse || !Bounds.Contains(ref _viewMousePos))
                return;

            if (TransformGizmo.IsActive)
            {
                // Ensure player is not moving objects
                if (TransformGizmo.ActiveAxis != TransformGizmoBase.Axis.None)
                    return;
            }
            else
            {
                // For now just pick objects in transform gizmo mode
                return;
            }

            // Get mouse ray and try to hit any object
            var ray = MouseRay;
            var view = new Ray(ViewPosition, ViewDirection);
            var hit = _window.Graph.Root.RayCast(ref ray, ref view, out _, SceneGraphNode.RayCastData.FlagTypes.SkipColliders);

            // Update selection
            if (hit != null)
            {
                // For child actor nodes (mesh, link or sth) we need to select it's owning actor node first or any other child node (but not a child actor)
                if (hit is ActorChildNode actorChildNode && !actorChildNode.CanBeSelectedDirectly)
                {
                    var parentNode = actorChildNode.ParentNode;
                    bool canChildBeSelected = _window.Selection.Contains(parentNode);
                    if (!canChildBeSelected)
                    {
                        for (int i = 0; i < parentNode.ChildNodes.Count; i++)
                        {
                            if (_window.Selection.Contains(parentNode.ChildNodes[i]))
                            {
                                canChildBeSelected = true;
                                break;
                            }
                        }
                    }

                    if (!canChildBeSelected)
                    {
                        // Select parent
                        hit = parentNode;
                    }
                }

                bool addRemove = Root.GetKey(KeyboardKeys.Control);
                bool isSelected = _window.Selection.Contains(hit);

                if (addRemove)
                {
                    if (isSelected)
                        _window.Deselect(hit);
                    else
                        _window.Select(hit, true);
                }
                else
                {
                    _window.Select(hit);
                }
            }
            else
            {
                _window.Deselect();
            }

            // Keep focus
            Focus();

            base.OnLeftMouseButtonUp();
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragEnter(ref Float2 location, DragData data)
        {
            DragHandlers.ClearDragEffects();
            var result = base.OnDragEnter(ref location, data);
            if (result != DragDropEffect.None)
                return result;
            return DragHandlers.DragEnter(ref location, data);
        }

        private bool ValidateDragItem(ContentItem contentItem)
        {
            if (contentItem is AssetItem assetItem)
            {
                if (assetItem.OnEditorDrag(this))
                    return true;
                if (assetItem.IsOfType<MaterialBase>())
                    return true;
            }
            return false;
        }

        private static bool ValidateDragActorType(ScriptType actorType)
        {
            return true;
        }

        private static bool ValidateDragScriptItem(ScriptItem script)
        {
            return Editor.Instance.CodeEditing.Actors.Get(script) != ScriptType.Null;
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragMove(ref Float2 location, DragData data)
        {
            DragHandlers.ClearDragEffects();
            var result = base.OnDragMove(ref location, data);
            if (result != DragDropEffect.None)
                return result;
            return DragHandlers.DragEnter(ref location, data);
        }

        /// <inheritdoc />
        public override void OnDragLeave()
        {
            DragHandlers.ClearDragEffects();
            DragHandlers.OnDragLeave();
            base.OnDragLeave();
        }

        /// <summary>
        /// Focuses the viewport on the current selection of the gizmo.
        /// </summary>
        public void FocusSelection()
        {
            var orientation = ViewOrientation;
            FocusSelection(ref orientation);
        }

        /// <summary>
        /// Focuses the viewport on the current selection of the gizmo.
        /// </summary>
        /// <param name="orientation">The target view orientation.</param>
        public void FocusSelection(ref Quaternion orientation)
        {
            if (TransformGizmo.SelectedParents.Count == 0)
                return;

            var gizmoBounds = Gizmos.Active.FocusBounds;
            if (gizmoBounds != BoundingSphere.Empty)
                ((FPSCamera)ViewportCamera).ShowSphere(ref gizmoBounds, ref orientation);
            else
                ((FPSCamera)ViewportCamera).ShowActors(TransformGizmo.SelectedParents, ref orientation);
        }

        /// <inheritdoc />
        protected override void OrientViewport(ref Quaternion orientation)
        {
            if (TransformGizmo.SelectedParents.Count != 0)
                FocusSelection(ref orientation);
            else
                base.OrientViewport(ref orientation);
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragDrop(ref Float2 location, DragData data)
        {
            DragHandlers.ClearDragEffects();
            var result = base.OnDragDrop(ref location, data);
            if (result != DragDropEffect.None)
                return result;
            return DragHandlers.DragDrop(ref location, data);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (IsDisposing)
                return;
            if (_tempDebugDrawContext != IntPtr.Zero)
            {
                DebugDraw.FreeContext(_tempDebugDrawContext);
                _tempDebugDrawContext = IntPtr.Zero;
            }
            FlaxEngine.Object.Destroy(ref SelectionOutline);
            FlaxEngine.Object.Destroy(ref _spritesRenderer);

            base.OnDestroy();
        }

        /// <inheritdoc />
        public override void DrawEditorPrimitives(GPUContext context, ref RenderContext renderContext, GPUTexture target, GPUTexture targetDepth)
        {
            // Draw gizmos
            for (int i = 0; i < Gizmos.Count; i++)
            {
                Gizmos[i].Draw(ref renderContext);
            }

            base.DrawEditorPrimitives(context, ref renderContext, target, targetDepth);
        }

        /// <inheritdoc />
        protected override void OnDebugDraw(GPUContext context, ref RenderContext renderContext)
        {
            base.OnDebugDraw(context, ref renderContext);

            // Collect selected objects debug shapes again when DebugDraw is active with a custom context
            _debugDrawData.Clear();
            var selectedParents = TransformGizmo.SelectedParents;
            for (int i = 0; i < selectedParents.Count; i++)
            {
                if (selectedParents[i].IsActiveInHierarchy)
                    selectedParents[i].OnDebugDraw(_debugDrawData);
            }

            unsafe
            {
                fixed (IntPtr* actors = _debugDrawData.ActorsPtrs)
                {
                    DebugDraw.DrawActors(new IntPtr(actors), _debugDrawData.ActorsCount, false);
                }
            }
        }
    }
}
