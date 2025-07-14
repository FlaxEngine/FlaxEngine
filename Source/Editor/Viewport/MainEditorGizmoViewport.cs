// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.Content;
using FlaxEditor.Gizmo;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.SceneGraph;
using FlaxEditor.Scripting;
using FlaxEditor.Viewport.Modes;
using FlaxEditor.Windows;
using FlaxEngine;
using FlaxEngine.Gizmo;
using FlaxEngine.GUI;
using Object = FlaxEngine.Object;

namespace FlaxEditor.Viewport
{
    /// <summary>
    /// Main editor gizmo viewport used by the <see cref="EditGameWindow"/>.
    /// </summary>
    /// <seealso cref="FlaxEditor.Viewport.EditorGizmoViewport" />
    public class MainEditorGizmoViewport : EditorGizmoViewport, IEditorPrimitivesOwner
    {
        private readonly Editor _editor;
        private readonly ContextMenuButton _showGridButton;
        private readonly ContextMenuButton _showNavigationButton;
        private SelectionOutline _customSelectionOutline;

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
            public EditorSpritesRenderer()
            {
                Order = -100000 + 1; // Draw after grid
                UseSingleTarget = true;
            }

            /// <inheritdoc />
            public override bool CanRender()
            {
                return (Task.View.Flags & ViewFlags.EditorSprites) == ViewFlags.EditorSprites && Level.ScenesCount != 0 && base.CanRender();
            }

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

        private bool _lockedFocus;
        private double _lockedFocusOffset;
        private readonly ViewportDebugDrawData _debugDrawData = new ViewportDebugDrawData(32);
        private EditorSpritesRenderer _editorSpritesRenderer;
        private ViewportRubberBandSelector _rubberBandSelector;

        /// <summary>
        /// Drag and drop handlers
        /// </summary>
        public readonly ViewportDragHandlers DragHandlers;

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
        /// Gets the debug draw data for the viewport.
        /// </summary>
        public ViewportDebugDrawData DebugDrawData => _debugDrawData;

        /// <summary>
        /// Gets or sets a value indicating whether show navigation mesh.
        /// </summary>
        public bool ShowNavigation
        {
            get => _showNavigationButton.Checked;
            set => _showNavigationButton.Checked = value;
        }

        /// <summary>
        /// The sculpt terrain gizmo.
        /// </summary>
        public Tools.Terrain.SculptTerrainGizmoMode SculptTerrainGizmo;

        /// <summary>
        /// The paint terrain gizmo.
        /// </summary>
        public Tools.Terrain.PaintTerrainGizmoMode PaintTerrainGizmo;

        /// <summary>
        /// The edit terrain gizmo.
        /// </summary>
        public Tools.Terrain.EditTerrainGizmoMode EditTerrainGizmo;

        /// <summary>
        /// The paint foliage gizmo.
        /// </summary>
        public Tools.Foliage.PaintFoliageGizmoMode PaintFoliageGizmo;

        /// <summary>
        /// The edit foliage gizmo.
        /// </summary>
        public Tools.Foliage.EditFoliageGizmoMode EditFoliageGizmo;

        /// <summary>
        /// Initializes a new instance of the <see cref="MainEditorGizmoViewport"/> class.
        /// </summary>
        /// <param name="editor">Editor instance.</param>
        public MainEditorGizmoViewport(Editor editor)
        : base(Object.New<SceneRenderTask>(), editor.Undo, editor.Scene.Root)
        {
            _editor = editor;
            DragHandlers = new ViewportDragHandlers(this, this, ValidateDragItem, ValidateDragActorType, ValidateDragScriptItem);

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
            Task.AddCustomPostFx(SelectionOutline);
            EditorPrimitives = Object.New<EditorPrimitives>();
            EditorPrimitives.Viewport = this;
            Task.AddCustomPostFx(EditorPrimitives);
            _editorSpritesRenderer = Object.New<EditorSpritesRenderer>();
            _editorSpritesRenderer.Task = Task;
            Task.AddCustomPostFx(_editorSpritesRenderer);

            // Add transformation gizmo
            TransformGizmo = new TransformGizmo(this);
            TransformGizmo.ApplyTransformation += ApplyTransform;
            TransformGizmo.Duplicate += _editor.SceneEditing.Duplicate;
            Gizmos.Active = TransformGizmo;

            // Add rubber band selector
            _rubberBandSelector = new ViewportRubberBandSelector(this);

            // Add grid
            Grid = new GridGizmo(this);
            Grid.EnabledChanged += gizmo => _showGridButton.Icon = gizmo.Enabled ? Style.Current.CheckBoxTick : SpriteHandle.Invalid;

            editor.SceneEditing.SelectionChanged += OnSelectionChanged;

            // Gizmo widgets
            AddGizmoViewportWidgets(this, TransformGizmo, true);

            // Show grid widget
            _showGridButton = ViewWidgetShowMenu.AddButton("Grid", () => Grid.Enabled = !Grid.Enabled);
            _showGridButton.Icon = Style.Current.CheckBoxTick;
            _showGridButton.CloseMenuOnClick = false;

            // Show navigation widget
            _showNavigationButton = ViewWidgetShowMenu.AddButton("Navigation", () => ShowNavigation = !ShowNavigation);
            _showNavigationButton.CloseMenuOnClick = false;

            // Create camera widget
            ViewWidgetButtonMenu.AddSeparator();
            ViewWidgetButtonMenu.AddButton("Create camera here", CreateCameraAtView);

            // Init gizmo modes
            {
                // Add default modes used by the editor
                Gizmos.AddMode(new TransformGizmoMode());
                Gizmos.AddMode(new NoGizmoMode());
                Gizmos.AddMode(SculptTerrainGizmo = new Tools.Terrain.SculptTerrainGizmoMode());
                Gizmos.AddMode(PaintTerrainGizmo = new Tools.Terrain.PaintTerrainGizmoMode());
                Gizmos.AddMode(EditTerrainGizmo = new Tools.Terrain.EditTerrainGizmoMode());
                Gizmos.AddMode(PaintFoliageGizmo = new Tools.Foliage.PaintFoliageGizmoMode());
                Gizmos.AddMode(EditFoliageGizmo = new Tools.Foliage.EditFoliageGizmoMode());

                // Activate transform mode first
                Gizmos.SetActiveMode<TransformGizmoMode>();
            }

            // Setup input actions
            InputActions.Add(options => options.LockFocusSelection, LockFocusSelection);
            InputActions.Add(options => options.FocusSelection, FocusSelection);
            InputActions.Add(options => options.RotateSelection, RotateSelection);
            InputActions.Add(options => options.Delete, _editor.SceneEditing.Delete);
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);

            var selection = TransformGizmo.SelectedParents;
            var requestUnlockFocus = FlaxEngine.Input.Mouse.GetButtonDown(MouseButton.Right) || FlaxEngine.Input.Mouse.GetButtonDown(MouseButton.Left);
            if (TransformGizmo.SelectedParents.Count == 0 || (requestUnlockFocus && ContainsFocus))
            {
                UnlockFocusSelection();
            }
            else if (_lockedFocus)
            {
                var selectionBounds = BoundingSphere.Empty;
                for (int i = 0; i < selection.Count; i++)
                {
                    selection[i].GetEditorSphere(out var sphere);
                    BoundingSphere.Merge(ref selectionBounds, ref sphere, out selectionBounds);
                }

                if (ContainsFocus)
                {
                    var viewportFocusDistance = Vector3.Distance(ViewPosition, selectionBounds.Center) / 10f;
                    _lockedFocusOffset -= FlaxEngine.Input.Mouse.ScrollDelta * viewportFocusDistance;
                }

                var focusDistance = Mathf.Max(selectionBounds.Radius * 2d, 100d);
                ViewPosition = selectionBounds.Center + (-ViewDirection * (focusDistance + _lockedFocusOffset));
            }
        }

        /// <summary>
        /// Overrides the selection outline effect or restored the default one.
        /// </summary>
        /// <param name="customSelectionOutline">The custom selection outline or null if use default one.</param>
        public void OverrideSelectionOutline(SelectionOutline customSelectionOutline)
        {
            if (_customSelectionOutline != null)
            {
                Task.RemoveCustomPostFx(_customSelectionOutline);
                Object.Destroy(ref _customSelectionOutline);
                Task.AddCustomPostFx(customSelectionOutline ? customSelectionOutline : SelectionOutline);
            }
            else if (customSelectionOutline != null)
            {
                Task.RemoveCustomPostFx(SelectionOutline);
                Task.AddCustomPostFx(customSelectionOutline);
            }

            _customSelectionOutline = customSelectionOutline;
        }

        private void CreateCameraAtView()
        {
            if (!Level.IsAnySceneLoaded)
                return;

            // Create actor
            var parent = Level.GetScene(0);
            var actor = new Camera
            {
                StaticFlags = StaticFlags.None,
                Name = Utilities.Utils.IncrementNameNumber("Camera", x => parent.GetChild(x) == null),
                Transform = ViewTransform,
                NearPlane = NearPlane,
                FarPlane = FarPlane,
                OrthographicScale = OrthographicScale,
                UsePerspective = !UseOrthographicProjection,
                FieldOfView = FieldOfView
            };

            // Spawn
            _editor.SceneEditing.Spawn(actor, parent);
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

        private void OnCollectDrawCalls(ref RenderContext renderContext)
        {
            if (renderContext.View.Pass == DrawPass.Depth)
                return;
            DragHandlers.CollectDrawCalls(_debugDrawData, ref renderContext);
            if (ShowNavigation)
                Editor.Internal_DrawNavMesh();
            _debugDrawData.OnDraw(ref renderContext);
        }

        /// <inheritdoc />
        public void DrawEditorPrimitives(GPUContext context, ref RenderContext renderContext, GPUTexture target, GPUTexture targetDepth)
        {
            // Draw gizmos
            for (int i = 0; i < Gizmos.Count; i++)
            {
                Gizmos[i].Draw(ref renderContext);
            }

            // Draw selected objects debug shapes and visuals
            if (DrawDebugDraw && (renderContext.View.Flags & ViewFlags.DebugDraw) == ViewFlags.DebugDraw)
            {
                _debugDrawData.DrawActors(true);
                DebugDraw.Draw(ref renderContext, target.View(), targetDepth.View(), true);
            }
        }

        private void OnPostRender(GPUContext context, ref RenderContext renderContext)
        {
            bool renderPostFx = true;
            switch (renderContext.View.Mode)
            {
            case ViewMode.Default:
            case ViewMode.PhysicsColliders:
                renderPostFx = false;
                break;
            }
            if (renderPostFx)
            {
                var task = renderContext.Task;

                // Render editor primitives, gizmo and debug shapes in debug view modes
                // Note: can use Output buffer as both input and output because EditorPrimitives is using an intermediate buffer
                if (EditorPrimitives && EditorPrimitives.CanRender())
                {
                    EditorPrimitives.Render(context, ref renderContext, task.Output, task.Output);
                }

                // Render editor sprites
                if (_editorSpritesRenderer && _editorSpritesRenderer.CanRender())
                {
                    _editorSpritesRenderer.Render(context, ref renderContext, task.Output, task.Output);
                }

                // Render selection outline
                var selectionOutline = _customSelectionOutline ?? SelectionOutline;
                if (selectionOutline && selectionOutline.CanRender())
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

        private void OnSelectionChanged()
        {
            var selection = _editor.SceneEditing.Selection;
            Gizmos.ForEach(x => x.OnSelectionChanged(selection));
        }

        /// <summary>
        /// Press "R" to rotate the selected gizmo objects 45 degrees.
        /// </summary>
        public void RotateSelection()
        {
            var win = (WindowRootControl)Root;
            var selection = _editor.SceneEditing.Selection;
            var isShiftDown = win.GetKey(KeyboardKeys.Shift);

            Quaternion rotationDelta;
            if (isShiftDown)
                rotationDelta = Quaternion.Euler(0.0f, -45.0f, 0.0f);
            else
                rotationDelta = Quaternion.Euler(0.0f, 45.0f, 0.0f);

            bool useObjCenter = TransformGizmo.ActivePivot == TransformGizmoBase.PivotType.ObjectCenter;
            Vector3 gizmoPosition = TransformGizmo.Position;

            // Rotate selected objects
            bool isPlayMode = _editor.StateMachine.IsPlayMode;
            TransformGizmo.StartTransforming();
            for (int i = 0; i < selection.Count; i++)
            {
                var obj = selection[i];
                if (isPlayMode && obj.CanTransform == false)
                    continue;
                var trans = obj.Transform;
                var pivotOffset = trans.Translation - gizmoPosition;
                if (useObjCenter || pivotOffset.IsZero)
                {
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
                obj.Transform = trans;
            }
            TransformGizmo.EndTransforming();
        }

        /// <inheritdoc />
        public override void OnLostFocus()
        {
            base.OnLostFocus();

            _rubberBandSelector.StopRubberBand();
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            base.OnMouseLeave();

            _rubberBandSelector.StopRubberBand();
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
        /// Lock focus on the current selection gizmo.
        /// </summary>
        public void LockFocusSelection()
        {
            _lockedFocus = true;
        }

        /// <summary>
        /// Unlock focus on the current selection.
        /// </summary>
        public void UnlockFocusSelection()
        {
            _lockedFocus = false;
            _lockedFocusOffset = 0f;
        }

        /// <summary>
        /// Focuses the viewport on the current selection of the gizmo.
        /// </summary>
        /// <param name="orientation">The target view orientation.</param>
        public void FocusSelection(ref Quaternion orientation)
        {
            ViewportCamera.FocusSelection(Gizmos, ref orientation);
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
            bool isPlayMode = _editor.StateMachine.IsPlayMode;
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

            // Draw rubber band for rectangle selection
            _rubberBandSelector.Draw();
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
        public override void OnMouseMove(Float2 location)
        {
            base.OnMouseMove(location);

            // Don't allow rubber band selection when gizmo is controlling mouse, vertex painting mode, or cloth painting is enabled
            bool canStart = !(IsControllingMouse || IsRightMouseButtonDown || IsAltKeyDown) &&
                            Gizmos?.Active is TransformGizmo && !Gizmos.Active.IsControllingMouse;
            _rubberBandSelector.TryCreateRubberBand(canStart, _viewMousePos);
        }

        /// <inheritdoc />
        protected override void OnLeftMouseButtonDown()
        {
            base.OnLeftMouseButtonDown();

            _rubberBandSelector.TryStartingRubberBandSelection();
        }

        /// <inheritdoc />
        protected override void OnLeftMouseButtonUp()
        {
            // Skip if was controlling mouse or mouse is not over the area
            if (_prevInput.IsControllingMouse || !Bounds.Contains(ref _viewMousePos))
                return;

            // Select rubberbanded rect actor nodes or pick with gizmo
            if (!_rubberBandSelector.ReleaseRubberBandSelection())
            {
                // Try to pick something with the current gizmo
                Gizmos.Active?.Pick();
            }

            // Keep focus
            Focus();

            base.OnLeftMouseButtonUp();
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (base.OnMouseUp(location, button))
                return true;

            // Handle mouse going up when using rubber band with mouse capture that click up outside the view
            if (button == MouseButton.Left && !new Rectangle(Float2.Zero, Size).Contains(ref location))
            {
                _rubberBandSelector.ReleaseRubberBandSelection();
                return true;
            }

            return false;
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
            if (!Level.IsAnySceneLoaded)
                return false;

            if (contentItem is AssetItem assetItem)
            {
                if (assetItem.OnEditorDrag(this))
                    return true;
                if (assetItem.IsOfType<MaterialBase>())
                    return true;
                if (assetItem.IsOfType<SceneAsset>())
                    return true;
            }

            return false;
        }

        private static bool ValidateDragActorType(ScriptType actorType)
        {
            return Level.IsAnySceneLoaded && Editor.Instance.CodeEditing.Actors.Get().Contains(actorType);
        }

        private static bool ValidateDragScriptItem(ScriptItem script)
        {
            return Level.IsAnySceneLoaded && Editor.Instance.CodeEditing.Actors.Get(script) != ScriptType.Null;
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
        public override void Select(List<SceneGraphNode> nodes)
        {
            _editor.SceneEditing.Select(nodes);
        }

        /// <inheritdoc />
        public override void Spawn(Actor actor)
        {
            var parent = actor.Parent ?? Level.GetScene(0);
            actor.Name = Utilities.Utils.IncrementNameNumber(actor.Name, x => parent.GetChild(x) == null);
            _editor.SceneEditing.Spawn(actor);
        }

        /// <inheritdoc />
        public override void OpenContextMenu()
        {
            var mouse = PointFromWindow(Root.MousePosition);
            _editor.Windows.SceneWin.ShowContextMenu(this, mouse);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (IsDisposing)
                return;

            _debugDrawData.Dispose();
            if (_task != null)
            {
                // Release if task is not used to save screenshot for project icon
                Object.Destroy(ref _task);
                ReleaseResources();
            }

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
        }

        internal void SaveProjectIconEnd()
        {
            if (_savedTask)
            {
                _savedTask.Enabled = false;
                Object.Destroy(_savedTask);
                ReleaseResources();
                _savedTask = null;
            }
            Object.Destroy(ref _savedBackBuffer);
        }

        private void ReleaseResources()
        {
            if (Task)
            {
                Task.RemoveCustomPostFx(SelectionOutline);
                Task.RemoveCustomPostFx(EditorPrimitives);
                Task.RemoveCustomPostFx(_editorSpritesRenderer);
                Task.RemoveCustomPostFx(_customSelectionOutline);
            }
            Object.Destroy(ref SelectionOutline);
            Object.Destroy(ref EditorPrimitives);
            Object.Destroy(ref _editorSpritesRenderer);
            Object.Destroy(ref _customSelectionOutline);
        }
    }
}
