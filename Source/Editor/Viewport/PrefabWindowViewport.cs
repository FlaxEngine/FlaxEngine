// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.Content;
using FlaxEditor.Gizmo;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Drag;
using FlaxEditor.SceneGraph;
using FlaxEditor.SceneGraph.Actors;
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
    public class PrefabWindowViewport : PrefabPreview, IEditorPrimitivesOwner
    {
        private sealed class PrefabSpritesRenderer : MainEditorGizmoViewport.EditorSpritesRenderer
        {
            public PrefabWindowViewport Viewport;

            public override bool CanRender => (Task.View.Flags & ViewFlags.EditorSprites) == ViewFlags.EditorSprites && Enabled;

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
        private IntPtr _debugDrawContext;
        private PrefabSpritesRenderer _spritesRenderer;
        private readonly DragAssets _dragAssets = new DragAssets(ValidateDragItem);
        private readonly DragActorType _dragActorType = new DragActorType(ValidateDragActorType);
        private readonly DragHandlers _dragHandlers = new DragHandlers();

        /// <summary>
        /// The transform gizmo.
        /// </summary>
        public readonly TransformGizmo TransformGizmo;

        /// <summary>
        /// The selection outline postFx.
        /// </summary>
        public SelectionOutline SelectionOutline;

        /// <summary>
        /// The editor primitives postFx.
        /// </summary>
        public EditorPrimitives EditorPrimitives;

        /// <summary>
        /// Gets or sets a value indicating whether draw <see cref="DebugDraw"/> shapes.
        /// </summary>
        public bool DrawDebugDraw = true;

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
            _debugDrawContext = DebugDraw.AllocateContext();

            // Prepare rendering task
            Task.ActorsSource = ActorsSources.CustomActors;
            Task.ViewFlags = ViewFlags.DefaultEditor;
            Task.Begin += OnBegin;
            Task.CollectDrawCalls += OnCollectDrawCalls;
            Task.PostRender += OnPostRender;

            // Create post effects
            SelectionOutline = FlaxEngine.Object.New<SelectionOutline>();
            SelectionOutline.SelectionGetter = () => TransformGizmo.SelectedParents;
            Task.CustomPostFx.Add(SelectionOutline);
            EditorPrimitives = FlaxEngine.Object.New<EditorPrimitives>();
            EditorPrimitives.Viewport = this;
            Task.CustomPostFx.Add(EditorPrimitives);
            _spritesRenderer = FlaxEngine.Object.New<PrefabSpritesRenderer>();
            _spritesRenderer.Task = Task;
            _spritesRenderer.Viewport = this;
            Task.CustomPostFx.Add(_spritesRenderer);

            // Add transformation gizmo
            TransformGizmo = new TransformGizmo(this);
            TransformGizmo.ApplyTransformation += ApplyTransform;
            TransformGizmo.ModeChanged += OnGizmoModeChanged;
            TransformGizmo.Duplicate += _window.Duplicate;
            Gizmos.Active = TransformGizmo;

            // Transform space widget
            var transformSpaceWidget = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperRight);
            var transformSpaceToggle = new ViewportWidgetButton(string.Empty, window.Editor.Icons.World16, null, true)
            {
                Checked = TransformGizmo.ActiveTransformSpace == TransformGizmoBase.TransformSpace.World,
                TooltipText = "Gizmo transform space (world or local)",
                Parent = transformSpaceWidget
            };
            transformSpaceToggle.Toggled += OnTransformSpaceToggle;
            transformSpaceWidget.Parent = this;

            // Scale snapping widget
            var scaleSnappingWidget = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperRight);
            var enableScaleSnapping = new ViewportWidgetButton(string.Empty, window.Editor.Icons.ScaleStep16, null, true)
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

            for (int i = 0; i < EditorViewportScaleSnapValues.Length; i++)
            {
                var v = EditorViewportScaleSnapValues[i];
                var button = scaleSnappingCM.AddButton(v.ToString());
                button.Tag = v;
            }
            scaleSnappingCM.ButtonClicked += OnWidgetScaleSnapClick;
            scaleSnappingCM.VisibleChanged += OnWidgetScaleSnapShowHide;
            _scaleSnapping.Parent = scaleSnappingWidget;
            scaleSnappingWidget.Parent = this;

            // Rotation snapping widget
            var rotateSnappingWidget = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperRight);
            var enableRotateSnapping = new ViewportWidgetButton(string.Empty, window.Editor.Icons.RotateStep16, null, true)
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

            for (int i = 0; i < EditorViewportRotateSnapValues.Length; i++)
            {
                var v = EditorViewportRotateSnapValues[i];
                var button = rotateSnappingCM.AddButton(v.ToString());
                button.Tag = v;
            }
            rotateSnappingCM.ButtonClicked += OnWidgetRotateSnapClick;
            rotateSnappingCM.VisibleChanged += OnWidgetRotateSnapShowHide;
            _rotateSnapping.Parent = rotateSnappingWidget;
            rotateSnappingWidget.Parent = this;

            // Translation snapping widget
            var translateSnappingWidget = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperRight);
            var enableTranslateSnapping = new ViewportWidgetButton(string.Empty, window.Editor.Icons.Grid16, null, true)
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

            for (int i = 0; i < EditorViewportTranslateSnapValues.Length; i++)
            {
                var v = EditorViewportTranslateSnapValues[i];
                var button = translateSnappingCM.AddButton(v.ToString());
                button.Tag = v;
            }
            translateSnappingCM.ButtonClicked += OnWidgetTranslateSnapClick;
            translateSnappingCM.VisibleChanged += OnWidgetTranslateSnapShowHide;
            _translateSnappng.Parent = translateSnappingWidget;
            translateSnappingWidget.Parent = this;

            // Gizmo mode widget
            var gizmoMode = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperRight);
            _gizmoModeTranslate = new ViewportWidgetButton(string.Empty, window.Editor.Icons.Translate16, null, true)
            {
                Tag = TransformGizmoBase.Mode.Translate,
                TooltipText = "Translate gizmo mode",
                Checked = true,
                Parent = gizmoMode
            };
            _gizmoModeTranslate.Toggled += OnGizmoModeToggle;
            _gizmoModeRotate = new ViewportWidgetButton(string.Empty, window.Editor.Icons.Rotate16, null, true)
            {
                Tag = TransformGizmoBase.Mode.Rotate,
                TooltipText = "Rotate gizmo mode",
                Parent = gizmoMode
            };
            _gizmoModeRotate.Toggled += OnGizmoModeToggle;
            _gizmoModeScale = new ViewportWidgetButton(string.Empty, window.Editor.Icons.Scale16, null, true)
            {
                Tag = TransformGizmoBase.Mode.Scale,
                TooltipText = "Scale gizmo mode",
                Parent = gizmoMode
            };
            _gizmoModeScale.Toggled += OnGizmoModeToggle;
            gizmoMode.Parent = this;

            _dragHandlers.Add(_dragActorType);
            _dragHandlers.Add(_dragAssets);

            // Setup input actions
            InputActions.Add(options => options.TranslateMode, () => TransformGizmo.ActiveMode = TransformGizmoBase.Mode.Translate);
            InputActions.Add(options => options.RotateMode, () => TransformGizmo.ActiveMode = TransformGizmoBase.Mode.Rotate);
            InputActions.Add(options => options.ScaleMode, () => TransformGizmo.ActiveMode = TransformGizmoBase.Mode.Scale);
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
                for (int i = 0; i < selectedParents.Count; i++)
                {
                    if (selectedParents[i].IsActiveInHierarchy)
                        selectedParents[i].OnDebugDraw(_debugDrawData);
                }
            }
        }

        private void OnCollectDrawCalls(RenderContext renderContext)
        {
            _debugDrawData.OnDraw(ref renderContext);
        }

        private void OnPostRender(GPUContext context, RenderContext renderContext)
        {
            if (renderContext.View.Mode != ViewMode.Default)
            {
                var task = renderContext.Task;

                // Render editor primitives, gizmo and debug shapes in debug view modes
                // Note: can use Output buffer as both input and output because EditorPrimitives is using a intermediate buffers
                if (EditorPrimitives && EditorPrimitives.CanRender)
                {
                    EditorPrimitives.Render(context, ref renderContext, task.Output, task.Output);
                }

                // Render editor sprites
                if (_spritesRenderer && _spritesRenderer.CanRender)
                {
                    _spritesRenderer.Render(context, ref renderContext, task.Output, task.Output);
                }

                // Render selection outline
                if (SelectionOutline && SelectionOutline.CanRender)
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
            ((FPSCamera)ViewportCamera).ShowActors(TransformGizmo.SelectedParents);
        }

        /// <inheritdoc />
        public GizmosCollection Gizmos { get; } = new GizmosCollection();

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
        public Vector2 MouseDelta => _mouseDeltaLeft * 1000;

        /// <inheritdoc />
        public bool UseSnapping => Root.GetKey(KeyboardKeys.Control);

        /// <inheritdoc />
        public bool UseDuplicate => Root.GetKey(KeyboardKeys.Shift);

        /// <inheritdoc />
        public Undo Undo { get; }

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

        private static readonly float[] EditorViewportScaleSnapValues =
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
                    b.Icon = Mathf.Abs(TransformGizmo.ScaleSnapValue - v) < 0.001f
                             ? Style.Current.CheckBoxTick
                             : SpriteHandle.Invalid;
                }
            }
        }

        private static readonly float[] EditorViewportRotateSnapValues =
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
                    b.Icon = Mathf.Abs(TransformGizmo.RotationSnapValue - v) < 0.001f
                             ? Style.Current.CheckBoxTick
                             : SpriteHandle.Invalid;
                }
            }
        }

        private static readonly float[] EditorViewportTranslateSnapValues =
        {
            0.1f,
            0.5f,
            1.0f,
            5.0f,
            10.0f,
            100.0f,
            1000.0f,
        };

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
                    b.Icon = Mathf.Abs(TransformGizmo.TranslationSnapValue - v) < 0.001f
                             ? Style.Current.CheckBoxTick
                             : SpriteHandle.Invalid;
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
                trans.Scale = Vector3.Clamp(trans.Scale + scaleDelta, new Vector3(-scaleLimit), new Vector3(scaleLimit));

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
            for (var i = 0; i < _window.Selection.Count; i++)
            {
                if (_window.Selection[i].EditableObject is UIControl controlActor && controlActor.Control != null)
                {
                    var control = controlActor.Control;
                    var bounds = Rectangle.FromPoints(control.PointToParent(this, Vector2.Zero), control.PointToParent(this, control.Size));
                    Render2D.DrawRectangle(bounds, Editor.Instance.Options.Options.Visual.SelectionOutlineColor0, Editor.Instance.Options.Options.Visual.UISelectionOutlineSize);
                }
            }
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
        public override DragDropEffect OnDragEnter(ref Vector2 location, DragData data)
        {
            var result = base.OnDragEnter(ref location, data);
            if (result != DragDropEffect.None)
                return result;

            return _dragHandlers.OnDragEnter(data);
        }

        private static bool ValidateDragItem(ContentItem contentItem)
        {
            if (contentItem is AssetItem assetItem)
            {
                if (assetItem.IsOfType<ParticleSystem>())
                    return true;
                if (assetItem.IsOfType<MaterialBase>())
                    return true;
                if (assetItem.IsOfType<ModelBase>())
                    return true;
                if (assetItem.IsOfType<AudioClip>())
                    return true;
                if (assetItem.IsOfType<Prefab>())
                    return true;
            }

            return false;
        }

        private static bool ValidateDragActorType(ScriptType actorType)
        {
            return true;
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragMove(ref Vector2 location, DragData data)
        {
            var result = base.OnDragMove(ref location, data);
            if (result != DragDropEffect.None)
                return result;

            return _dragHandlers.Effect;
        }

        /// <inheritdoc />
        public override void OnDragLeave()
        {
            _dragHandlers.OnDragLeave();

            base.OnDragLeave();
        }

        private Vector3 PostProcessSpawnedActorLocation(Actor actor, ref Vector3 hitLocation)
        {
            Editor.GetActorEditorBox(actor, out BoundingBox box);

            // Place the object
            var location = hitLocation - (box.Size.Length * 0.5f) * ViewDirection;

            // Apply grid snapping if enabled
            if (UseSnapping || TransformGizmo.TranslationSnapEnable)
            {
                float snapValue = TransformGizmo.TranslationSnapValue;
                location = new Vector3(
                                       (int)(location.X / snapValue) * snapValue,
                                       (int)(location.Y / snapValue) * snapValue,
                                       (int)(location.Z / snapValue) * snapValue);
            }

            return location;
        }

        private void Spawn(AssetItem item, SceneGraphNode hit, ref Vector2 location, ref Vector3 hitLocation)
        {
            if (item is BinaryAssetItem binaryAssetItem)
            {
                if (binaryAssetItem.Type == typeof(ParticleSystem))
                {
                    var particleSystem = FlaxEngine.Content.LoadAsync<ParticleSystem>(item.ID);
                    var actor = new ParticleEffect
                    {
                        Name = item.ShortName,
                        ParticleSystem = particleSystem
                    };
                    actor.Position = PostProcessSpawnedActorLocation(actor, ref hitLocation);
                    Spawn(actor);
                    return;
                }
                if (typeof(MaterialBase).IsAssignableFrom(binaryAssetItem.Type))
                {
                    if (hit is StaticModelNode staticModelNode)
                    {
                        var staticModel = (StaticModel)staticModelNode.Actor;
                        var ray = ConvertMouseToRay(ref location);
                        if (staticModel.IntersectsEntry(ref ray, out _, out _, out var entryIndex))
                        {
                            var material = FlaxEngine.Content.LoadAsync<MaterialBase>(item.ID);
                            using (new UndoBlock(Undo, staticModel, "Change material"))
                                staticModel.SetMaterial(entryIndex, material);
                        }
                    }
                    return;
                }
                if (typeof(SkinnedModel).IsAssignableFrom(binaryAssetItem.Type))
                {
                    var model = FlaxEngine.Content.LoadAsync<SkinnedModel>(item.ID);
                    var actor = new AnimatedModel
                    {
                        Name = item.ShortName,
                        SkinnedModel = model
                    };
                    actor.Position = PostProcessSpawnedActorLocation(actor, ref hitLocation);
                    Spawn(actor);
                    return;
                }
                if (typeof(Model).IsAssignableFrom(binaryAssetItem.Type))
                {
                    var model = FlaxEngine.Content.LoadAsync<Model>(item.ID);
                    var actor = new StaticModel
                    {
                        Name = item.ShortName,
                        Model = model
                    };
                    actor.Position = PostProcessSpawnedActorLocation(actor, ref hitLocation);
                    Spawn(actor);
                    return;
                }
                if (typeof(AudioClip).IsAssignableFrom(binaryAssetItem.Type))
                {
                    var clip = FlaxEngine.Content.LoadAsync<AudioClip>(item.ID);
                    var actor = new AudioSource
                    {
                        Name = item.ShortName,
                        Clip = clip
                    };
                    actor.Position = PostProcessSpawnedActorLocation(actor, ref hitLocation);
                    Spawn(actor);
                    return;
                }
                if (typeof(Prefab).IsAssignableFrom(binaryAssetItem.Type))
                {
                    var prefab = FlaxEngine.Content.LoadAsync<Prefab>(item.ID);
                    var actor = PrefabManager.SpawnPrefab(prefab, null);
                    actor.Name = item.ShortName;
                    actor.Position = PostProcessSpawnedActorLocation(actor, ref hitLocation);
                    Spawn(actor);
                    return;
                }
            }
        }

        private void Spawn(Actor actor)
        {
            _window.Spawn(actor);
        }

        private void Spawn(ScriptType item, SceneGraphNode hit, ref Vector3 hitLocation)
        {
            var actor = item.CreateInstance() as Actor;
            if (actor == null)
            {
                Editor.LogWarning("Failed to spawn actor of type " + item.TypeName);
                return;
            }
            actor.Name = item.Name;
            actor.Position = PostProcessSpawnedActorLocation(actor, ref hitLocation);
            Spawn(actor);
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragDrop(ref Vector2 location, DragData data)
        {
            var result = base.OnDragDrop(ref location, data);
            if (result != DragDropEffect.None)
                return result;

            // Check if drag sth
            Vector3 hitLocation = ViewPosition;
            SceneGraphNode hit = null;
            if (_dragHandlers.HasValidDrag)
            {
                // Get mouse ray and try to hit any object
                var ray = ConvertMouseToRay(ref location);
                var view = new Ray(ViewPosition, ViewDirection);
                hit = _window.Graph.Root.RayCast(ref ray, ref view, out var closest, SceneGraphNode.RayCastData.FlagTypes.SkipColliders);
                if (hit != null)
                {
                    // Use hit location
                    hitLocation = ray.Position + ray.Direction * closest;
                }
                else
                {
                    // Use area in front of the viewport
                    hitLocation = ViewPosition + ViewDirection * 100;
                }
            }

            // Drag assets
            if (_dragAssets.HasValidDrag)
            {
                result = _dragAssets.Effect;

                // Process items
                for (int i = 0; i < _dragAssets.Objects.Count; i++)
                {
                    var item = _dragAssets.Objects[i];
                    Spawn(item, hit, ref location, ref hitLocation);
                }
            }
            // Drag actor type
            else if (_dragActorType.HasValidDrag)
            {
                result = _dragActorType.Effect;

                // Process items
                for (int i = 0; i < _dragActorType.Objects.Count; i++)
                {
                    var item = _dragActorType.Objects[i];
                    Spawn(item, hit, ref hitLocation);
                }
            }

            return result;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (_debugDrawContext != IntPtr.Zero)
            {
                DebugDraw.FreeContext(_debugDrawContext);
                _debugDrawContext = IntPtr.Zero;
            }
            FlaxEngine.Object.Destroy(ref SelectionOutline);
            FlaxEngine.Object.Destroy(ref EditorPrimitives);
            FlaxEngine.Object.Destroy(ref _spritesRenderer);

            base.OnDestroy();
        }

        /// <inheritdoc />
        public void DrawEditorPrimitives(GPUContext context, ref RenderContext renderContext, GPUTexture target, GPUTexture targetDepth)
        {
            // Draw selected objects debug shapes and visuals
            if (DrawDebugDraw && (renderContext.View.Flags & ViewFlags.DebugDraw) == ViewFlags.DebugDraw)
            {
                DebugDraw.SetContext(_debugDrawContext);
                DebugDraw.UpdateContext(_debugDrawContext, 1.0f / Engine.FramesPerSecond);
                unsafe
                {
                    fixed (IntPtr* actors = _debugDrawData.ActorsPtrs)
                    {
                        DebugDraw.DrawActors(new IntPtr(actors), _debugDrawData.ActorsCount, false);
                    }
                }
                DebugDraw.Draw(ref renderContext, target.View(), targetDepth.View(), true);
                DebugDraw.SetContext(IntPtr.Zero);
            }
        }
    }
}
