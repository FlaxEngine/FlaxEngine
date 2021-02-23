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
using FlaxEditor.Viewport.Widgets;
using FlaxEditor.Windows;
using FlaxEngine;
using FlaxEngine.GUI;
using Object = FlaxEngine.Object;

namespace FlaxEditor.Viewport
{
    /// <summary>
    /// Main editor gizmo viewport used by the <see cref="EditGameWindow"/>.
    /// </summary>
    /// <seealso cref="FlaxEditor.Viewport.EditorGizmoViewport" />
    public partial class MainEditorGizmoViewport : EditorGizmoViewport, IEditorPrimitivesOwner
    {
        private readonly Editor _editor;

        private readonly ContextMenuButton _showGridButton;
        private readonly ContextMenuButton _showNavigationButton;
        private readonly ViewportWidgetButton _gizmoModeTranslate;
        private readonly ViewportWidgetButton _gizmoModeRotate;
        private readonly ViewportWidgetButton _gizmoModeScale;

        private readonly ViewportWidgetButton _translateSnapping;
        private readonly ViewportWidgetButton _rotateSnapping;
        private readonly ViewportWidgetButton _scaleSnapping;

        private readonly DragAssets<DragDropEventArgs> _dragAssets = new DragAssets<DragDropEventArgs>(ValidateDragItem);
        private readonly DragActorType<DragDropEventArgs> _dragActorType = new DragActorType<DragDropEventArgs>(ValidateDragActorType);

        private SelectionOutline _customSelectionOutline;

        /// <summary>
        /// The custom drag drop event arguments.
        /// </summary>
        /// <seealso cref="FlaxEditor.GUI.Drag.DragEventArgs" />
        public class DragDropEventArgs : DragEventArgs
        {
            /// <summary>
            /// The hit.
            /// </summary>
            public SceneGraphNode Hit;

            /// <summary>
            /// The hit location.
            /// </summary>
            public Vector3 HitLocation;
        }

        /// <summary>
        /// The editor sprites rendering effect.
        /// </summary>
        /// <seealso cref="FlaxEngine.PostProcessEffect" />
        [HideInEditor]
        public class EditorSpritesRenderer : PostProcessEffect
        {
            /// <summary>
            /// The rendering task.
            /// </summary>
            public SceneRenderTask Task;

            /// <inheritdoc />
            public override int Order => -10000000;

            /// <inheritdoc />
            public override bool UseSingleTarget => true;

            /// <inheritdoc />
            public override bool CanRender => (Task.View.Flags & ViewFlags.EditorSprites) == ViewFlags.EditorSprites && Level.ScenesCount != 0 && base.CanRender;

            /// <inheritdoc />
            public override void Render(GPUContext context, ref RenderContext renderContext, GPUTexture input, GPUTexture output)
            {
                Profiler.BeginEventGPU("Editor Primitives");

                // Prepare
                var renderList = RenderList.GetFromPool();
                var prevList = renderContext.List;
                renderContext.List = renderList;
                renderContext.View.Pass = DrawPass.Forward;

                // Bind output
                float width = input.Width;
                float height = input.Height;
                context.SetViewport(width, height);
                var depthBuffer = renderContext.Buffers.DepthBuffer;
                var depthBufferHandle = depthBuffer.View();
                if ((depthBuffer.Flags & GPUTextureFlags.ReadOnlyDepthView) == GPUTextureFlags.ReadOnlyDepthView)
                    depthBufferHandle = depthBuffer.ViewReadOnlyDepth();
                context.SetRenderTarget(depthBufferHandle, input.View());

                // Collect draw calls
                Draw(ref renderContext);

                // Sort draw calls
                renderList.SortDrawCalls(ref renderContext, true, DrawCallsListType.Forward);

                // Perform the rendering
                renderList.ExecuteDrawCalls(ref renderContext, DrawCallsListType.Forward);

                // Cleanup
                RenderList.ReturnToPool(renderList);
                renderContext.List = prevList;

                Profiler.EndEventGPU();
            }

            /// <summary>
            /// Draws the icons.
            /// </summary>
            protected virtual void Draw(ref RenderContext renderContext)
            {
                for (int i = 0; i < Level.ScenesCount; i++)
                {
                    var scene = Level.GetScene(i);
                    ViewportIconsRenderer.DrawIcons(ref renderContext, scene);
                }
            }
        }

        private readonly ViewportDebugDrawData _debugDrawData = new ViewportDebugDrawData(32);
        private StaticModel _previewStaticModel;
        private int _previewModelEntryIndex;
        private BrushSurface _previewBrushSurface;
        private EditorSpritesRenderer _editorSpritesRenderer;

        /// <summary>
        /// Drag and drop handlers
        /// </summary>
        public readonly DragHandlers DragHandlers = new DragHandlers();

        /// <summary>
        /// The transform gizmo.
        /// </summary>
        public readonly TransformGizmo TransformGizmo;

        /// <summary>
        /// The grid gizmo.
        /// </summary>
        public readonly GridGizmo Grid;

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
        /// Gets or sets a value indicating whether show navigation mesh.
        /// </summary>
        public bool ShowNavigation
        {
            get => _showNavigationButton.Checked;
            set => _showNavigationButton.Checked = value;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="MainEditorGizmoViewport"/> class.
        /// </summary>
        /// <param name="editor">Editor instance.</param>
        public MainEditorGizmoViewport(Editor editor)
        : base(Object.New<SceneRenderTask>(), editor.Undo)
        {
            _editor = editor;

            // Prepare rendering task
            Task.ActorsSource = ActorsSources.Scenes;
            Task.ViewFlags = ViewFlags.DefaultEditor;
            Task.Begin += OnBegin;
            Task.CollectDrawCalls += OnCollectDrawCalls;
            Task.PostRender += OnPostRender;

            // Render task after the main game task so streaming and render state data will use main game task instead of editor preview
            Task.Order = 1;

            // Create post effects
            SelectionOutline = Object.New<SelectionOutline>();
            SelectionOutline.SelectionGetter = () => TransformGizmo.SelectedParents;
            Task.CustomPostFx.Add(SelectionOutline);
            EditorPrimitives = Object.New<EditorPrimitives>();
            EditorPrimitives.Viewport = this;
            Task.CustomPostFx.Add(EditorPrimitives);
            _editorSpritesRenderer = Object.New<EditorSpritesRenderer>();
            _editorSpritesRenderer.Task = Task;
            Task.CustomPostFx.Add(_editorSpritesRenderer);

            // Add transformation gizmo
            TransformGizmo = new TransformGizmo(this);
            TransformGizmo.ApplyTransformation += ApplyTransform;
            TransformGizmo.ModeChanged += OnGizmoModeChanged;
            TransformGizmo.Duplicate += Editor.Instance.SceneEditing.Duplicate;
            Gizmos.Active = TransformGizmo;

            // Add grid
            Grid = new GridGizmo(this);
            Grid.EnabledChanged += gizmo => _showGridButton.Icon = gizmo.Enabled ? Style.Current.CheckBoxTick : SpriteHandle.Invalid;

            editor.SceneEditing.SelectionChanged += OnSelectionChanged;

            // Transform space widget
            var transformSpaceWidget = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperRight);
            var transformSpaceToggle = new ViewportWidgetButton(string.Empty, editor.Icons.World16, null, true)
            {
                Checked = TransformGizmo.ActiveTransformSpace == TransformGizmoBase.TransformSpace.World,
                TooltipText = "Gizmo transform space (world or local)",
                Parent = transformSpaceWidget
            };
            transformSpaceToggle.Toggled += OnTransformSpaceToggle;
            transformSpaceWidget.Parent = this;

            // Scale snapping widget
            var scaleSnappingWidget = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperRight);
            var enableScaleSnapping = new ViewportWidgetButton(string.Empty, editor.Icons.ScaleStep16, null, true)
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
            var enableRotateSnapping = new ViewportWidgetButton(string.Empty, editor.Icons.RotateStep16, null, true)
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
            var enableTranslateSnapping = new ViewportWidgetButton(string.Empty, editor.Icons.Grid16, null, true)
            {
                Checked = TransformGizmo.TranslationSnapEnable,
                TooltipText = "Enable position snapping",
                Parent = translateSnappingWidget
            };
            enableTranslateSnapping.Toggled += OnTranslateSnappingToggle;

            var translateSnappingCM = new ContextMenu();
            _translateSnapping = new ViewportWidgetButton(TransformGizmo.TranslationSnapValue.ToString(), SpriteHandle.Invalid, translateSnappingCM)
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
            _translateSnapping.Parent = translateSnappingWidget;
            translateSnappingWidget.Parent = this;

            // Gizmo mode widget
            var gizmoMode = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperRight);
            _gizmoModeTranslate = new ViewportWidgetButton(string.Empty, editor.Icons.Translate16, null, true)
            {
                Tag = TransformGizmoBase.Mode.Translate,
                TooltipText = "Translate gizmo mode",
                Checked = true,
                Parent = gizmoMode
            };
            _gizmoModeTranslate.Toggled += OnGizmoModeToggle;
            _gizmoModeRotate = new ViewportWidgetButton(string.Empty, editor.Icons.Rotate16, null, true)
            {
                Tag = TransformGizmoBase.Mode.Rotate,
                TooltipText = "Rotate gizmo mode",
                Parent = gizmoMode
            };
            _gizmoModeRotate.Toggled += OnGizmoModeToggle;
            _gizmoModeScale = new ViewportWidgetButton(string.Empty, editor.Icons.Scale16, null, true)
            {
                Tag = TransformGizmoBase.Mode.Scale,
                TooltipText = "Scale gizmo mode",
                Parent = gizmoMode
            };
            _gizmoModeScale.Toggled += OnGizmoModeToggle;
            gizmoMode.Parent = this;

            // Show grid widget
            _showGridButton = ViewWidgetShowMenu.AddButton("Grid", () => Grid.Enabled = !Grid.Enabled);
            _showGridButton.Icon = Style.Current.CheckBoxTick;

            // Show navigation widget
            _showNavigationButton = ViewWidgetShowMenu.AddButton("Navigation", () => ShowNavigation = !ShowNavigation);

            // Create camera widget
            ViewWidgetButtonMenu.AddSeparator();
            ViewWidgetButtonMenu.AddButton("Create camera here", CreateCameraAtView);

            DragHandlers.Add(_dragActorType);
            DragHandlers.Add(_dragAssets);

            InitModes();

            // Setup input actions
            InputActions.Add(options => options.TranslateMode, () => TransformGizmo.ActiveMode = TransformGizmoBase.Mode.Translate);
            InputActions.Add(options => options.RotateMode, () => TransformGizmo.ActiveMode = TransformGizmoBase.Mode.Rotate);
            InputActions.Add(options => options.ScaleMode, () => TransformGizmo.ActiveMode = TransformGizmoBase.Mode.Scale);
            InputActions.Add(options => options.FocusSelection, () => _editor.Windows.EditWin.ShowSelectedActors());
            InputActions.Add(options => options.Delete, _editor.SceneEditing.Delete);
        }

        /// <summary>
        /// Overrides the selection outline effect or restored the default one.
        /// </summary>
        /// <param name="customSelectionOutline">The custom selection outline or null if use default one.</param>
        public void OverrideSelectionOutline(SelectionOutline customSelectionOutline)
        {
            if (_customSelectionOutline != null)
            {
                Object.Destroy(ref _customSelectionOutline);

                Task.CustomPostFx.Remove(_customSelectionOutline);

                Task.CustomPostFx.Add(customSelectionOutline ? customSelectionOutline : SelectionOutline);
            }
            else if (customSelectionOutline != null)
            {
                Task.CustomPostFx.Remove(SelectionOutline);

                Task.CustomPostFx.Add(customSelectionOutline);
            }

            _customSelectionOutline = customSelectionOutline;
        }

        private void CreateCameraAtView()
        {
            if (!Level.IsAnySceneLoaded)
                return;

            // Create actor
            var actor = new Camera
            {
                StaticFlags = StaticFlags.None,
                Name = "Camera",
                Transform = ViewTransform,
                NearPlane = NearPlane,
                FarPlane = FarPlane,
                OrthographicScale = OrthographicScale,
                UsePerspective = !UseOrthographicProjection,
                FieldOfView = FieldOfView
            };

            // Spawn
            Editor.Instance.SceneEditing.Spawn(actor);
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
            if (_previewStaticModel)
            {
                _debugDrawData.HighlightModel(_previewStaticModel, _previewModelEntryIndex);
            }
            if (_previewBrushSurface.Brush != null)
            {
                _debugDrawData.HighlightBrushSurface(_previewBrushSurface);
            }

            if (ShowNavigation)
            {
                Editor.Internal_DrawNavMesh();
            }

            _debugDrawData.OnDraw(ref renderContext);
        }

        /// <inheritdoc />
        public void DrawEditorPrimitives(GPUContext context, ref RenderContext renderContext, GPUTexture target, GPUTexture targetDepth)
        {
            // Draw selected objects debug shapes and visuals
            if (DrawDebugDraw && (renderContext.View.Flags & ViewFlags.DebugDraw) == ViewFlags.DebugDraw)
            {
                unsafe
                {
                    fixed (IntPtr* actors = _debugDrawData.ActorsPtrs)
                    {
                        DebugDraw.DrawActors(new IntPtr(actors), _debugDrawData.ActorsCount, true);
                    }
                }

                DebugDraw.Draw(ref renderContext, target.View(), targetDepth.View(), true);
            }
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
                if (_editorSpritesRenderer && _editorSpritesRenderer.CanRender)
                {
                    _editorSpritesRenderer.Render(context, ref renderContext, task.Output, task.Output);
                }

                // Render selection outline
                var selectionOutline = _customSelectionOutline ?? SelectionOutline;
                if (selectionOutline && selectionOutline.CanRender)
                {
                    // Use temporary intermediate buffer
                    var desc = task.Output.Description;
                    var temp = RenderTargetPool.Get(ref desc);
                    selectionOutline.Render(context, ref renderContext, task.Output, temp);

                    // Copy the results back to the output
                    context.CopyTexture(task.Output, 0, 0, 0, 0, temp, 0);

                    RenderTargetPool.Release(temp);
                }
            }
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
            _translateSnapping.Text = v.ToString();
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
            var selection = _editor.SceneEditing.Selection;
            Gizmos.ForEach(x => x.OnSelectionChanged(selection));
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
            bool isPlayMode = Editor.Instance.StateMachine.IsPlayMode;
            for (int i = 0; i < selection.Count; i++)
            {
                var obj = selection[i];

                // Block transforming static objects in play mode
                if (isPlayMode && obj.CanTransform == false)
                    continue;
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
        protected override void OnLeftMouseButtonUp()
        {
            // Skip if was controlling mouse or mouse is not over the area
            if (_prevInput.IsControllingMouse || !Bounds.Contains(ref _viewMousePos))
                return;

            // Try to pick something with the current gizmo
            Gizmos.Active?.Pick();

            // Keep focus
            Focus();

            base.OnLeftMouseButtonUp();
        }

        private void GetHitLocation(ref Vector2 location, out SceneGraphNode hit, out Vector3 hitLocation)
        {
            // Get mouse ray and try to hit any object
            var ray = ConvertMouseToRay(ref location);
            var view = new Ray(ViewPosition, ViewDirection);
            var gridPlane = new Plane(Vector3.Zero, Vector3.Up);
            var flags = SceneGraphNode.RayCastData.FlagTypes.SkipColliders | SceneGraphNode.RayCastData.FlagTypes.SkipEditorPrimitives;
            hit = Editor.Instance.Scene.Root.RayCast(ref ray, ref view, out var closest, flags);
            if (hit != null)
            {
                // Use hit location
                hitLocation = ray.Position + ray.Direction * closest;
            }
            else if (Grid.Enabled && CollisionsHelper.RayIntersectsPlane(ref ray, ref gridPlane, out closest) && closest < 4000.0f)
            {
                // Use grid location
                hitLocation = ray.Position + ray.Direction * closest;
            }
            else
            {
                // Use area in front of the viewport
                hitLocation = ViewPosition + ViewDirection * 100;
            }
        }

        private void SetDragEffects(ref Vector2 location)
        {
            if (_dragAssets.HasValidDrag && _dragAssets.Objects[0].IsOfType<MaterialBase>())
            {
                GetHitLocation(ref location, out var hit, out _);
                ClearDragEffects();

                if (hit is StaticModelNode staticModelNode)
                {
                    _previewStaticModel = (StaticModel)staticModelNode.Actor;
                    var ray = ConvertMouseToRay(ref location);
                    _previewStaticModel.IntersectsEntry(ref ray, out _, out _, out _previewModelEntryIndex);
                }
                else if (hit is BoxBrushNode.SideLinkNode brushSurfaceNode)
                {
                    _previewBrushSurface = brushSurfaceNode.Surface;
                }
            }
        }

        private void ClearDragEffects()
        {
            _previewStaticModel = null;
            _previewModelEntryIndex = -1;
            _previewBrushSurface = new BrushSurface();
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragEnter(ref Vector2 location, DragData data)
        {
            ClearDragEffects();

            var result = base.OnDragEnter(ref location, data);
            if (result != DragDropEffect.None)
                return result;

            result = DragHandlers.OnDragEnter(data);

            SetDragEffects(ref location);

            return result;
        }

        private static bool ValidateDragItem(ContentItem contentItem)
        {
            if (!Level.IsAnySceneLoaded)
                return false;

            if (contentItem is AssetItem assetItem)
            {
                if (assetItem.IsOfType<ParticleSystem>())
                    return true;
                if (assetItem.IsOfType<SceneAnimation>())
                    return true;
                if (assetItem.IsOfType<MaterialBase>())
                    return true;
                if (assetItem.IsOfType<ModelBase>())
                    return true;
                if (assetItem.IsOfType<AudioClip>())
                    return true;
                if (assetItem.IsOfType<Prefab>())
                    return true;
                if (assetItem.IsOfType<SceneAsset>())
                    return true;
                if (assetItem is VisualScriptItem visualScriptItem && new ScriptType(typeof(Actor)).IsAssignableFrom(visualScriptItem.ScriptType) && visualScriptItem.ScriptType.CanCreateInstance)
                    return true;
            }

            return false;
        }

        private static bool ValidateDragActorType(ScriptType actorType)
        {
            return Level.IsAnySceneLoaded;
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragMove(ref Vector2 location, DragData data)
        {
            ClearDragEffects();

            var result = base.OnDragMove(ref location, data);
            if (result != DragDropEffect.None)
                return result;

            SetDragEffects(ref location);

            return DragHandlers.Effect;
        }

        /// <inheritdoc />
        public override void OnDragLeave()
        {
            ClearDragEffects();

            DragHandlers.OnDragLeave();

            base.OnDragLeave();
        }

        private Vector3 PostProcessSpawnedActorLocation(Actor actor, ref Vector3 hitLocation)
        {
            Editor.GetActorEditorBox(actor, out _);

            // Place the object
            //var location = hitLocation - (box.Size.Length * 0.5f) * ViewDirection;
            var location = hitLocation;

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
            if (item is AssetItem assetItem)
            {
                if (assetItem.IsOfType<ParticleSystem>())
                {
                    var asset = FlaxEngine.Content.LoadAsync<ParticleSystem>(item.ID);
                    var actor = new ParticleEffect
                    {
                        Name = item.ShortName,
                        ParticleSystem = asset
                    };
                    actor.Position = PostProcessSpawnedActorLocation(actor, ref hitLocation);
                    Editor.Instance.SceneEditing.Spawn(actor);
                    return;
                }
                if (assetItem.IsOfType<SceneAnimation>())
                {
                    var asset = FlaxEngine.Content.LoadAsync<SceneAnimation>(item.ID);
                    var actor = new SceneAnimationPlayer
                    {
                        Name = item.ShortName,
                        Animation = asset
                    };
                    actor.Position = PostProcessSpawnedActorLocation(actor, ref hitLocation);
                    Editor.Instance.SceneEditing.Spawn(actor);
                    return;
                }
                if (assetItem.IsOfType<MaterialBase>())
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
                    else if (hit is BoxBrushNode.SideLinkNode brushSurfaceNode)
                    {
                        var material = FlaxEngine.Content.LoadAsync<MaterialBase>(item.ID);
                        using (new UndoBlock(Undo, brushSurfaceNode.Brush, "Change material"))
                        {
                            var surface = brushSurfaceNode.Surface;
                            surface.Material = material;
                            brushSurfaceNode.Surface = surface;
                        }
                    }
                    return;
                }
                if (assetItem.IsOfType<SkinnedModel>())
                {
                    var model = FlaxEngine.Content.LoadAsync<SkinnedModel>(item.ID);
                    var actor = new AnimatedModel
                    {
                        Name = item.ShortName,
                        SkinnedModel = model
                    };
                    actor.Position = PostProcessSpawnedActorLocation(actor, ref hitLocation);
                    Editor.Instance.SceneEditing.Spawn(actor);
                    return;
                }
                if (assetItem.IsOfType<Model>())
                {
                    var model = FlaxEngine.Content.LoadAsync<Model>(item.ID);
                    var actor = new StaticModel
                    {
                        Name = item.ShortName,
                        Model = model
                    };
                    actor.Position = PostProcessSpawnedActorLocation(actor, ref hitLocation);
                    Editor.Instance.SceneEditing.Spawn(actor);
                    return;
                }
                if (assetItem.IsOfType<AudioClip>())
                {
                    var clip = FlaxEngine.Content.LoadAsync<AudioClip>(item.ID);
                    var actor = new AudioSource
                    {
                        Name = item.ShortName,
                        Clip = clip
                    };
                    actor.Position = PostProcessSpawnedActorLocation(actor, ref hitLocation);
                    Editor.Instance.SceneEditing.Spawn(actor);
                    return;
                }
                if (assetItem.IsOfType<Prefab>())
                {
                    var prefab = FlaxEngine.Content.LoadAsync<Prefab>(item.ID);
                    var actor = PrefabManager.SpawnPrefab(prefab, null);
                    actor.Name = item.ShortName;
                    actor.Position = PostProcessSpawnedActorLocation(actor, ref hitLocation);
                    Editor.Instance.SceneEditing.Spawn(actor);
                    return;
                }
                if (assetItem.IsOfType<SceneAsset>())
                {
                    Editor.Instance.Scene.OpenScene(item.ID, true);
                    return;
                }
                if (assetItem is VisualScriptItem visualScriptItem && new ScriptType(typeof(Actor)).IsAssignableFrom(visualScriptItem.ScriptType) && visualScriptItem.ScriptType.CanCreateInstance)
                {
                    var actor = (Actor)visualScriptItem.ScriptType.CreateInstance();
                    actor.Name = item.ShortName;
                    actor.Position = PostProcessSpawnedActorLocation(actor, ref hitLocation);
                    Editor.Instance.SceneEditing.Spawn(actor);
                    return;
                }
            }
        }

        private void Spawn(ScriptType item, SceneGraphNode hit, ref Vector2 location, ref Vector3 hitLocation)
        {
            var actor = item.CreateInstance() as Actor;
            if (actor == null)
            {
                Editor.LogWarning("Failed to spawn actor of type " + item.TypeName);
                return;
            }
            actor.Name = item.Name;
            actor.Position = PostProcessSpawnedActorLocation(actor, ref hitLocation);
            Editor.Instance.SceneEditing.Spawn(actor);
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragDrop(ref Vector2 location, DragData data)
        {
            ClearDragEffects();

            var result = base.OnDragDrop(ref location, data);
            if (result != DragDropEffect.None)
                return result;

            // Check if drag sth
            Vector3 hitLocation = ViewPosition;
            SceneGraphNode hit = null;
            if (DragHandlers.HasValidDrag)
            {
                GetHitLocation(ref location, out hit, out hitLocation);
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
                    Spawn(item, hit, ref location, ref hitLocation);
                }
            }

            DragHandlers.OnDragDrop(new DragDropEventArgs { Hit = hit, HitLocation = hitLocation });

            return result;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (IsDisposing)
                return;

            DisposeModes();
            _debugDrawData.Dispose();
            //Object.Destroy(ref SelectionOutline);
            //Object.Destroy(ref EditorPrimitives);
            //Object.Destroy(ref _editorSpritesRenderer);
            //Object.Destroy(ref _customSelectionOutline);

            base.OnDestroy();
        }

        private RenderTask _savedTask;
        private GPUTexture _savedBackBuffer;

        internal void SaveProjectIcon()
        {
            TakeScreenshot(StringUtils.CombinePaths(Globals.ProjectCacheFolder, "icon.png"));

            _savedTask = _task;
            _savedBackBuffer = _backBuffer;

            _task = null;
            _backBuffer = null;
            _editorSpritesRenderer = null;
        }

        internal void SaveProjectIconEnd()
        {
            if (_savedTask)
            {
                _savedTask.Enabled = false;
                _savedTask = null;
            }
            Object.Destroy(ref _savedBackBuffer);
        }
    }
}
