// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using FlaxEditor.Content;
using FlaxEditor.Gizmo;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Input;
using FlaxEditor.Modules;
using FlaxEditor.SceneGraph;
using FlaxEditor.Scripting;
using FlaxEditor.Viewport.Cameras;
using FlaxEditor.Viewport.Previews;
using FlaxEditor.Windows.Assets;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Json;
using Utils = FlaxEditor.Utilities.Utils;

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

            public override bool CanRender()
            {
                return (Task.View.Flags & ViewFlags.EditorSprites) == ViewFlags.EditorSprites && Enabled;
            }

            protected override void Draw(ref RenderContext renderContext)
            {
                ViewportIconsRenderer.DrawIcons(ref renderContext, Viewport.Instance);
            }
        }

        [HideInEditor]
        private sealed class PrefabUIEditorRoot : UIEditorRoot
        {
            private readonly PrefabWindowViewport _viewport;
            private bool UI => _viewport.ShowUI;

            public PrefabUIEditorRoot(PrefabWindowViewport viewport)
            : base(true)
            {
                _viewport = viewport;
                Parent = viewport;
            }

            public override bool EnableInputs => !UI;
            public override bool EnableSelecting => UI;
            public override bool EnableBackground => UI;
            public override TransformGizmo TransformGizmo => _viewport.TransformGizmo;
        }

        private readonly PrefabWindow _window;
        private UpdateDelegate _update;

        private readonly ViewportDebugDrawData _debugDrawData = new ViewportDebugDrawData(32);
        private readonly List<Actor> _debugDrawActors = new List<Actor>();
        private PrefabSpritesRenderer _spritesRenderer;
        private IntPtr _tempDebugDrawContext;

        private PrefabUIEditorRoot _uiRoot;
        private bool _showUI = false;
        
        private int _defaultScaleActiveIndex = 0;
        private int _customScaleActiveIndex = -1;
        private ContextMenuButton _uiModeButton;

        /// <summary>
        /// Event fired when the UI Mode is toggled.
        /// </summary>
        public event Action<bool> UIModeToggled;

        /// <summary>
        /// set the initial UI mod
        /// </summary>
        /// <param name="value">the initial ShowUI value</param>
        public void SetInitialUIMode(bool value)
        {
            ShowUI = value;
            _uiModeButton.Checked = value;
        }

        /// <summary>
        /// Whether to show the UI mode or not.
        /// </summary>
        public bool ShowUI
        {
            get => _showUI;
            set
            {
                _showUI = value;
                if (_showUI)
                {
                    // UI widget
                    Gizmos.Active = null;
                    ViewportCamera = new UIEditorCamera { UIEditor = _uiRoot };

                    // Hide 3D visuals
                    ShowEditorPrimitives = false;
                    ShowDefaultSceneActors = false;
                    ShowDebugDraw = false;

                    // Show whole UI on startup
                    var canvas = (CanvasRootControl)_uiParentLink.Children.FirstOrDefault(x => x is CanvasRootControl);
                    if (canvas != null)
                        ViewportCamera.ShowActor(canvas.Canvas);
                    else if (Instance is UIControl)
                        ViewportCamera.ShowActor(Instance);
                    _uiRoot.Visible = true;
                }
                else
                {
                    // Generic prefab
                    Gizmos.Active = TransformGizmo;
                    ViewportCamera = new FPSCamera();
                    _uiRoot.Visible = false;
                }

                // Update default components usage
                bool defaultFeatures = !_showUI;
                _disableInputUpdate = _showUI;
                _spritesRenderer.Enabled = defaultFeatures;
                SelectionOutline.Enabled = defaultFeatures;
                _showDefaultSceneButton.Visible = defaultFeatures;
                _cameraWidget.Visible = defaultFeatures;
                _cameraButton.Visible = defaultFeatures;
                _orthographicModeButton.Visible = defaultFeatures;
                Task.Enabled = defaultFeatures;
                UseAutomaticTaskManagement = defaultFeatures;
                ShowDefaultSceneActors = defaultFeatures;
                TintColor = defaultFeatures ? Color.White : Color.Transparent;
                UIModeToggled?.Invoke(_showUI);
            }
        }

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

            _gridGizmo = new GridGizmo(this);
            var showGridButton = ViewWidgetShowMenu.AddButton("Grid");
            showGridButton.Clicked += () =>
            {
                _gridGizmo.Enabled = !_gridGizmo.Enabled;
                showGridButton.Checked = _gridGizmo.Enabled;
            };
            showGridButton.Checked = true;
            showGridButton.CloseMenuOnClick = false;

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
            TransformGizmo.Duplicate += _window.Duplicate;
            Gizmos.Active = TransformGizmo;

            // Use custom root for UI controls
            _uiRoot = new PrefabUIEditorRoot(this);
            _uiRoot.IndexInParent = 0; // Move viewport down below other widgets in the viewport
            _uiParentLink = _uiRoot.UIRoot;

            // UI mode buton
            _uiModeButton = ViewWidgetShowMenu.AddButton("UI Mode", (button) => ShowUI = button.Checked);
            _uiModeButton.AutoCheck = true;
            _uiModeButton.VisibleChanged += control => (control as ContextMenuButton).Checked = ShowUI;
            
            EditorGizmoViewport.AddGizmoViewportWidgets(this, TransformGizmo);

            // Setup input actions
            InputActions.Add(options => options.FocusSelection, ShowSelectedActors);

            SetUpdate(ref _update, OnUpdate);
        }

        /// <summary>
        /// Creates the view scaling options. Needs to be called after a Prefab is valid and loaded.
        /// </summary>
        public void CreateViewScalingOptions()
        {
            // View Scaling
            var uiViewCM = ViewWidgetButtonMenu.AddChildMenu("UI View Scaling");
            LoadCustomUIScalingOption();
            CreateUIViewScalingContextMenu(uiViewCM.ContextMenu);
        }

        private void ChangeUIView(UIModule.ViewportScaleOption uiViewScaleOption)
        {
            _uiRoot.SetViewSize((Float2)uiViewScaleOption.Size);
        }

         private void CreateUIViewScalingContextMenu(ContextMenu vsMenu)
        {
            // Add default viewport sizing options
            var defaultOptions = Editor.Instance.UI.DefaultViewportScaleOptions;
            for (int i = 0; i < defaultOptions.Count; i++)
            {
                var viewportScale = defaultOptions[i];

                // Skip aspect ratio types in prefab
                if (viewportScale.ScaleType == UIModule.ViewportScaleOption.ViewportScaleType.Aspect)
                    continue;

                var button = vsMenu.AddButton(viewportScale.Label);
                button.CloseMenuOnClick = false;
                button.Tag = viewportScale;

                // No default index is active.
                if (_defaultScaleActiveIndex == -1)
                {
                    button.Icon = SpriteHandle.Invalid;
                }
                // This is the active index.
                else if (_defaultScaleActiveIndex == i)
                {
                    button.Icon = Style.Current.CheckBoxTick;
                    ChangeUIView(viewportScale);
                }

                button.Clicked += () =>
                {
                    if (button.Tag == null)
                        return;

                    // Reset selected icon on all buttons
                    foreach (var child in vsMenu.Items)
                    {
                        if (child is ContextMenuButton cmb && cmb.Tag is UIModule.ViewportScaleOption v)
                        {
                            if (cmb == button)
                            {
                                var index = defaultOptions.FindIndex(x => x == v);
                                _defaultScaleActiveIndex = index;
                                _customScaleActiveIndex = -1; // Reset custom index because default was chosen.
                                button.Icon = Style.Current.CheckBoxTick;
                                ChangeUIView(v);
                            }
                            else if (cmb.Icon != SpriteHandle.Invalid)
                            {
                                cmb.Icon = SpriteHandle.Invalid;
                            }
                        }
                    }
                };
            }
            if (defaultOptions.Count != 0)
                vsMenu.AddSeparator();

            // Add custom viewport options
            var customOptions = Editor.Instance.UI.CustomViewportScaleOptions;
            for (int i = 0; i < customOptions.Count; i++)
            {
                var viewportScale = customOptions[i];

                // Skip aspect ratio types for prefabs
                if (viewportScale.ScaleType == UIModule.ViewportScaleOption.ViewportScaleType.Aspect)
                    continue;

                var childCM = vsMenu.AddChildMenu(viewportScale.Label);
                childCM.CloseMenuOnClick = false;
                childCM.Tag = viewportScale;
                
                // No custom index is active.
                if (_customScaleActiveIndex == -1)
                {
                    childCM.Icon = SpriteHandle.Invalid;
                }
                // This is the active index.
                else if (_customScaleActiveIndex == i)
                {
                    childCM.Icon = Style.Current.CheckBoxTick;
                    ChangeUIView(viewportScale);
                }
                
                var applyButton = childCM.ContextMenu.AddButton("Apply");
                applyButton.Tag = childCM.Tag = viewportScale;
                applyButton.CloseMenuOnClick = false;
                applyButton.Clicked += () =>
                {
                    if (childCM.Tag == null)
                        return;

                    // Reset selected icon on all buttons
                    foreach (var child in vsMenu.Items)
                    {
                        if (child is ContextMenuButton cmb && cmb.Tag is UIModule.ViewportScaleOption v)
                        {
                            if (child == childCM)
                            {
                                var index = customOptions.FindIndex(x => x == v);
                                _defaultScaleActiveIndex = -1; // Reset default index because custom was chosen.
                                _customScaleActiveIndex = index;
                                childCM.Icon = Style.Current.CheckBoxTick;
                                ChangeUIView(v);
                            }
                            else if (cmb.Icon != SpriteHandle.Invalid)
                            {
                                cmb.Icon = SpriteHandle.Invalid;
                            }
                        }
                    }
                };

                var deleteButton = childCM.ContextMenu.AddButton("Delete");
                deleteButton.CloseMenuOnClick = false;
                deleteButton.Clicked += () =>
                {
                    if (childCM.Tag == null)
                        return;

                    var v = (UIModule.ViewportScaleOption)childCM.Tag;
                    if (childCM.Icon != SpriteHandle.Invalid)
                    {
                        _customScaleActiveIndex = -1;
                        _defaultScaleActiveIndex = 0;
                        ChangeUIView(defaultOptions[0]);
                    }
                    customOptions.Remove(v);
                    Editor.Instance.UI.SaveCustomViewportScalingOptions();
                    vsMenu.DisposeAllItems();
                    CreateUIViewScalingContextMenu(vsMenu);
                    vsMenu.PerformLayout();
                };
            }
            if (customOptions.Count != 0)
                vsMenu.AddSeparator();

            // Add button
            var add = vsMenu.AddButton("Add...");
            add.CloseMenuOnClick = false;
            add.Clicked += () =>
            {
                var popup = new ContextMenuBase
                {
                    Size = new Float2(230, 95),
                    ClipChildren = false,
                    CullChildren = false,
                };
                popup.Show(add, new Float2(add.Width, 0));

                var nameLabel = new Label
                {
                    Parent = popup,
                    AnchorPreset = AnchorPresets.TopLeft,
                    Text = "Name",
                    HorizontalAlignment = TextAlignment.Near,
                };
                nameLabel.LocalX += 10;
                nameLabel.LocalY += 10;

                var nameTextBox = new TextBox
                {
                    Parent = popup,
                    AnchorPreset = AnchorPresets.TopLeft,
                    IsMultiline = false,
                };
                nameTextBox.LocalX += 100;
                nameTextBox.LocalY += 10;

                var whLabel = new Label
                {
                    Parent = popup,
                    AnchorPreset = AnchorPresets.TopLeft,
                    Text = "Width & Height",
                    HorizontalAlignment = TextAlignment.Near,
                };
                whLabel.LocalX += 10;
                whLabel.LocalY += 30;

                var wValue = new IntValueBox(1920)
                {
                    Parent = popup,
                    AnchorPreset = AnchorPresets.TopLeft,
                    MinValue = 1,
                    Width = 55,
                };
                wValue.LocalY += 30;
                wValue.LocalX += 100;

                var hValue = new IntValueBox(1080)
                {
                    Parent = popup,
                    AnchorPreset = AnchorPresets.TopLeft,
                    MinValue = 1,
                    Width = 55,
                };
                hValue.LocalY += 30;
                hValue.LocalX += 165;

                var submitButton = new Button
                {
                    Parent = popup,
                    AnchorPreset = AnchorPresets.TopLeft,
                    Text = "Submit",
                    Width = 70,
                };
                submitButton.LocalX += 40;
                submitButton.LocalY += 60;

                submitButton.Clicked += () =>
                {
                    var name = nameTextBox.Text + " (" + wValue.Value + "x" + hValue.Value + ")";

                    var newViewportOption = new UIModule.ViewportScaleOption
                    {
                        Label = name,
                        ScaleType = UIModule.ViewportScaleOption.ViewportScaleType.Resolution,
                        Size = new Int2(wValue.Value, hValue.Value),
                    };

                    customOptions.Add(newViewportOption);
                    Editor.Instance.UI.SaveCustomViewportScalingOptions();
                    vsMenu.DisposeAllItems();
                    CreateUIViewScalingContextMenu(vsMenu);
                    vsMenu.PerformLayout();
                };

                var cancelButton = new Button
                {
                    Parent = popup,
                    AnchorPreset = AnchorPresets.TopLeft,
                    Text = "Cancel",
                    Width = 70,
                };
                cancelButton.LocalX += 120;
                cancelButton.LocalY += 60;

                cancelButton.Clicked += () =>
                {
                    nameTextBox.Clear();
                    hValue.Value = 9;
                    wValue.Value = 16;
                    popup.Hide();
                };
            };
        }

        /// <summary>
        /// Saves the active ui scaling option.
        /// </summary>
        public void SaveActiveUIScalingOption()
        {
            var defaultKey = $"{Prefab.ID}:DefaultViewportScalingIndex";
            Editor.Instance.ProjectCache.SetCustomData(defaultKey, _defaultScaleActiveIndex.ToString());
            var customKey = $"{Prefab.ID}:CustomViewportScalingIndex";
            Editor.Instance.ProjectCache.SetCustomData(customKey, _customScaleActiveIndex.ToString());
        }

        private void LoadCustomUIScalingOption()
        {
            Prefab.WaitForLoaded();
            var defaultKey = $"{Prefab.ID}:DefaultViewportScalingIndex";
            if (Editor.Instance.ProjectCache.TryGetCustomData(defaultKey, out string defaultData))
            {
                if (int.TryParse(defaultData, out var index))
                {
                    var options = Editor.Instance.UI.DefaultViewportScaleOptions;
                    if (options.Count > index)
                    {
                        _defaultScaleActiveIndex = index;
                        if (index != -1)
                            ChangeUIView(Editor.Instance.UI.DefaultViewportScaleOptions[index]);
                    }
                    // Assume option does not exist anymore so move to default.
                    else if (index != -1)
                    {
                        _defaultScaleActiveIndex = 0;
                    }
                }
            }
            
            var customKey = $"{Prefab.ID}:CustomViewportScalingIndex";
            if (Editor.Instance.ProjectCache.TryGetCustomData(customKey, out string data))
            {
                if (int.TryParse(data, out var index))
                {
                    var options = Editor.Instance.UI.CustomViewportScaleOptions;
                    if (options.Count > index)
                    {
                        _customScaleActiveIndex = index;
                        if (index != -1)
                            ChangeUIView(options[index]);
                    }
                    // Assume option does not exist anymore so move to default.
                    else if (index != -1)
                    {
                        _defaultScaleActiveIndex = 0;
                        _customScaleActiveIndex = -1;
                    }
                }
            }
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
            if (renderContext.View.Pass == DrawPass.Depth)
                return;
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
            ViewportCamera.ShowActors(TransformGizmo.SelectedParents, ref orient);
        }

        /// <inheritdoc />
        public EditorViewport Viewport => this;

        /// <inheritdoc />
        public GizmosCollection Gizmos { get; }

        private GridGizmo _gridGizmo;

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
        public bool SnapToVertex => ContainsFocus && Editor.Instance.Options.Options.Input.SnapToVertex.Process(Root);

        /// <inheritdoc />
        public Float2 MouseDelta => _mouseDelta;

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
        public void OpenContextMenu()
        {
            var mouse = PointFromWindow(Root.MousePosition);
            _window.ShowContextMenu(this, ref mouse);
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
            return Editor.Instance.CodeEditing.Actors.Get().Contains(actorType);
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
            ViewportCamera.FocusSelection(Gizmos, ref orientation);
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
            _debugDrawData.DrawActors();

            // Debug draw all actors in prefab and collect actors
            var view = Task.View;
            var collectActors = (view.Flags & ViewFlags.PhysicsDebug) != 0 || view.Mode == ViewMode.PhysicsColliders || (view.Flags & ViewFlags.LightsDebug) != 0;
            _debugDrawActors.Clear();
            foreach (var child in SceneGraphRoot.ChildNodes)
            {
                if (child is not ActorNode actorNode || !actorNode.Actor)
                    continue;
                var actor = actorNode.Actor;
                if (collectActors)
                    Utils.GetActorsTree(_debugDrawActors, actor);
                DebugDraw.DrawActorsTree(actor);
            }

            // Draw physics debug
            if ((view.Flags & ViewFlags.PhysicsDebug) != 0 || view.Mode == ViewMode.PhysicsColliders)
            {
                foreach (var actor in _debugDrawActors)
                    DebugDraw.DrawDebugPhysics(actor, renderContext.View);
            }

            // Draw lights debug
            if ((view.Flags & ViewFlags.LightsDebug) != 0)
            {
                foreach (var actor in _debugDrawActors)
                {
                    if (actor is Light l && l.IsActiveInHierarchy)
                        DebugDraw.DrawLightDebug(l, renderContext.View);
                }
            }

            _debugDrawActors.Clear();
        }
    }
}
