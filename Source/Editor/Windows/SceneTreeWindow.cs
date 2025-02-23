// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.Gizmo;
using FlaxEditor.Content;
using FlaxEditor.GUI.Tree;
using FlaxEditor.GUI.Drag;
using FlaxEditor.GUI.Input;
using FlaxEditor.SceneGraph;
using FlaxEditor.SceneGraph.GUI;
using FlaxEditor.Scripting;
using FlaxEditor.States;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows
{
    /// <summary>
    /// Windows used to present loaded scenes collection and whole scene graph.
    /// </summary>
    /// <seealso cref="FlaxEditor.Windows.SceneEditorWindow" />
    public partial class SceneTreeWindow : SceneEditorWindow
    {
        private TextBox _searchBox;
        private Tree _tree;
        private Panel _sceneTreePanel;
        private bool _isUpdatingSelection;
        private bool _isMouseDown;

        private DragAssets _dragAssets;
        private DragActorType _dragActorType;
        private DragControlType _dragControlType;
        private DragScriptItems _dragScriptItems;
        private DragHandlers _dragHandlers;
        private bool _isDropping = false;

        /// <summary>
        /// Scene tree panel.
        /// </summary>
        public Panel SceneTreePanel => _sceneTreePanel;

        /// <summary>
        /// Initializes a new instance of the <see cref="SceneTreeWindow"/> class.
        /// </summary>
        /// <param name="editor">The editor.</param>
        public SceneTreeWindow(Editor editor)
        : base(editor, true, ScrollBars.None)
        {
            Title = "Scene";

            // Scene searching query input box
            var headerPanel = new ContainerControl
            {
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                BackgroundColor = Style.Current.Background,
                IsScrollable = false,
                Offsets = new Margin(0, 0, 0, 18 + 6),
            };
            _searchBox = new SearchBox
            {
                AnchorPreset = AnchorPresets.HorizontalStretchMiddle,
                Parent = headerPanel,
                Bounds = new Rectangle(4, 4, headerPanel.Width - 8, 18),
            };
            _searchBox.TextChanged += OnSearchBoxTextChanged;

            // Scene tree panel
            _sceneTreePanel = new Panel
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, headerPanel.Bottom, 0),
                IsScrollable = true,
                ScrollBars = ScrollBars.Both,
                Parent = this,
            };

            // Create scene structure tree
            var root = editor.Scene.Root;
            root.TreeNode.ChildrenIndent = 0;
            root.TreeNode.Expand();
            _tree = new Tree(true)
            {
                Margin = new Margin(0.0f, 0.0f, -16.0f, _sceneTreePanel.ScrollBarsSize), // Hide root node
                IsScrollable = true,
            };
            _tree.AddChild(root.TreeNode);
            _tree.SelectedChanged += Tree_OnSelectedChanged;
            _tree.RightClick += OnTreeRightClick;
            _tree.Parent = _sceneTreePanel;
            headerPanel.Parent = this;

            // Setup input actions
            InputActions.Add(options => options.TranslateMode, () => Editor.MainTransformGizmo.ActiveMode = TransformGizmoBase.Mode.Translate);
            InputActions.Add(options => options.RotateMode, () => Editor.MainTransformGizmo.ActiveMode = TransformGizmoBase.Mode.Rotate);
            InputActions.Add(options => options.ScaleMode, () => Editor.MainTransformGizmo.ActiveMode = TransformGizmoBase.Mode.Scale);
            InputActions.Add(options => options.FocusSelection, () => Editor.Windows.EditWin.Viewport.FocusSelection());
            InputActions.Add(options => options.LockFocusSelection, () => Editor.Windows.EditWin.Viewport.LockFocusSelection());
            InputActions.Add(options => options.Rename, RenameSelection);
        }

        /// <summary>
        /// Enables or disables vertical and horizontal scrolling on the scene tree panel.
        /// </summary>
        /// <param name="enabled">The state to set scrolling to</param>
        public void ScrollingOnSceneTreeView(bool enabled)
        {
            if (_sceneTreePanel.VScrollBar != null)
                _sceneTreePanel.VScrollBar.ThumbEnabled = enabled;
            if (_sceneTreePanel.HScrollBar != null)
                _sceneTreePanel.HScrollBar.ThumbEnabled = enabled;
        }

        /// <summary>
        /// Scrolls to the selected node in the scene tree.
        /// </summary>
        public void ScrollToSelectedNode()
        {
            // Scroll to node
            var nodeSelection = _tree.Selection;
            if (nodeSelection.Count != 0)
            {
                var scrollControl = nodeSelection[nodeSelection.Count - 1];
                _sceneTreePanel.ScrollViewTo(scrollControl);
            }
        }

        private void OnSearchBoxTextChanged()
        {
            // Skip events during setup or init stuff
            if (IsLayoutLocked)
                return;

            _tree.LockChildrenRecursive();

            // Update tree
            var query = _searchBox.Text;
            var root = Editor.Scene.Root;
            root.TreeNode.UpdateFilter(query);

            _tree.UnlockChildrenRecursive();
            PerformLayout();
            PerformLayout();
        }

        private void Spawn(Type type)
        {
            // Create actor
            Actor actor = (Actor)FlaxEngine.Object.New(type);
            Actor parentActor = null;
            if (Editor.SceneEditing.HasSthSelected && Editor.SceneEditing.Selection[0] is ActorNode actorNode)
            {
                parentActor = actorNode.Actor;
                actorNode.TreeNode.Expand();
            }
            if (parentActor == null)
            {
                var scenes = Level.Scenes;
                if (scenes.Length > 0)
                    parentActor = scenes[scenes.Length - 1];
            }
            if (parentActor != null)
            {
                // Use the same location
                actor.Transform = parentActor.Transform;

                // Rename actor to identify it easily
                actor.Name = Utilities.Utils.IncrementNameNumber(type.Name, x => parentActor.GetChild(x) == null);
            }

            // Spawn it
            Editor.SceneEditing.Spawn(actor, parentActor);

            Editor.SceneEditing.Select(actor);
            RenameSelection();
        }

        /// <summary>
        /// Focuses search box.
        /// </summary>
        public void Search()
        {
            _searchBox.Focus();
        }

        private void Tree_OnSelectedChanged(List<TreeNode> before, List<TreeNode> after)
        {
            // Check if lock events
            if (_isUpdatingSelection)
                return;

            if (after.Count > 0)
            {
                // Get actors from nodes
                var actors = new List<SceneGraphNode>(after.Count);
                for (int i = 0; i < after.Count; i++)
                {
                    if (after[i] is ActorTreeNode node && node.Actor)
                        actors.Add(node.ActorNode);
                }

                // Select
                Editor.SceneEditing.Select(actors);
            }
            else
            {
                // Deselect
                Editor.SceneEditing.Deselect();
            }
        }

        private void OnTreeRightClick(TreeNode node, Float2 location)
        {
            if (!Editor.StateMachine.CurrentState.CanEditScene)
                return;
            ShowContextMenu(node, location);
        }

        /// <inheritdoc />
        public override void OnInit()
        {
            Editor.SceneEditing.SelectionChanged += OnSelectionChanged;
        }

        private void OnSelectionChanged()
        {
            _isUpdatingSelection = true;

            var selection = Editor.SceneEditing.Selection;
            if (selection.Count == 0)
            {
                _tree.Deselect();
            }
            else
            {
                // Find nodes to select
                var nodes = new List<TreeNode>(selection.Count);
                for (int i = 0; i < selection.Count; i++)
                {
                    if (selection[i] is ActorNode node)
                    {
                        nodes.Add(node.TreeNode);
                    }
                }

                // Select nodes
                _tree.Select(nodes);

                // For single node selected scroll view so user can see it
                if (nodes.Count == 1)
                {
                    nodes[0].ExpandAllParents(true);
                    _sceneTreePanel.ScrollViewTo(nodes[0]);
                }
            }

            _isUpdatingSelection = false;
        }

        private bool ValidateDragAsset(AssetItem assetItem)
        {
            if (assetItem.IsOfType<SceneAsset>())
                return true;
            return assetItem.OnEditorDrag(this);
        }

        private static bool ValidateDragActorType(ScriptType actorType)
        {
            return Editor.Instance.CodeEditing.Actors.Get().Contains(actorType);
        }

        private static bool ValidateDragControlType(ScriptType controlType)
        {
            return Editor.Instance.CodeEditing.Controls.Get().Contains(controlType);
        }

        private static bool ValidateDragScriptItem(ScriptItem script)
        {
            return Editor.Instance.CodeEditing.Actors.Get(script) != ScriptType.Null;
        }

        /// <inheritdoc />
        public override void Draw()
        {
            var style = Style.Current;

            // Draw overlay
            string overlayText = null;
            var state = Editor.StateMachine.CurrentState;
            var textWrap = TextWrapping.NoWrap;
            if (state is LoadingState)
            {
                overlayText = "Loading...";
            }
            else if (state is ChangingScenesState)
            {
                overlayText = "Loading scene...";
            }
            else if (((ContainerControl)_tree.GetChild(0)).ChildrenCount == 0)
            {
                overlayText = "No scene\nOpen one from the content window";
                textWrap = TextWrapping.WrapWords;
            }
            if (overlayText != null)
            {
                Render2D.DrawText(style.FontLarge, overlayText, GetClientArea(), style.ForegroundDisabled, TextAlignment.Center, TextAlignment.Center, textWrap);
            }

            base.Draw();
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton buttons)
        {
            if (base.OnMouseDown(location, buttons))
                return true;

            if (buttons == MouseButton.Right)
            {
                _isMouseDown = true;
                return true;
            }

            return false;
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton buttons)
        {
            if (base.OnMouseUp(location, buttons))
                return true;

            if (_isMouseDown && buttons == MouseButton.Right)
            {
                _isMouseDown = false;

                if (Editor.StateMachine.CurrentState.CanEditScene)
                {
                    // Show context menu
                    Editor.SceneEditing.Deselect();
                    ShowContextMenu(Parent, location + _searchBox.BottomLeft);
                }

                return true;
            }

            if (buttons == MouseButton.Left)
            {
                if (Editor.StateMachine.CurrentState.CanEditScene && !_isDropping)
                {
                    Editor.SceneEditing.Deselect();
                }
                if (_isDropping)
                    _isDropping = false;
                return true;
            }

            return false;
        }

        /// <inheritdoc />
        public override void OnLostFocus()
        {
            _isMouseDown = false;

            base.OnLostFocus();
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragEnter(ref Float2 location, DragData data)
        {
            var result = base.OnDragEnter(ref location, data);
            if (Editor.StateMachine.CurrentState.CanEditScene)
            {
                if (_dragHandlers == null)
                    _dragHandlers = new DragHandlers();
                if (_dragAssets == null)
                {
                    _dragAssets = new DragAssets(ValidateDragAsset);
                    _dragHandlers.Add(_dragAssets);
                }
                if (_dragAssets.OnDragEnter(data) && result == DragDropEffect.None)
                    return _dragAssets.Effect;
                if (_dragActorType == null)
                {
                    _dragActorType = new DragActorType(ValidateDragActorType);
                    _dragHandlers.Add(_dragActorType);
                }
                if (_dragActorType.OnDragEnter(data) && result == DragDropEffect.None)
                    return _dragActorType.Effect;
                if (_dragControlType == null)
                {
                    _dragControlType = new DragControlType(ValidateDragControlType);
                    _dragHandlers.Add(_dragControlType);
                }
                if (_dragControlType.OnDragEnter(data) && result == DragDropEffect.None)
                    return _dragControlType.Effect;
                if (_dragScriptItems == null)
                {
                    _dragScriptItems = new DragScriptItems(ValidateDragScriptItem);
                    _dragHandlers.Add(_dragScriptItems);
                }
                if (_dragScriptItems.OnDragEnter(data) && result == DragDropEffect.None)
                    return _dragScriptItems.Effect;
            }
            return result;
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragMove(ref Float2 location, DragData data)
        {
            var result = base.OnDragMove(ref location, data);
            if (result == DragDropEffect.None && Editor.StateMachine.CurrentState.CanEditScene && _dragHandlers != null)
            {
                result = _dragHandlers.Effect;
            }
            return result;
        }

        /// <inheritdoc />
        public override void OnDragLeave()
        {
            base.OnDragLeave();

            _dragHandlers?.OnDragLeave();
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragDrop(ref Float2 location, DragData data)
        {
            var result = base.OnDragDrop(ref location, data);
            if (result == DragDropEffect.None)
            {
                _isDropping = true;
                // Drag assets
                if (_dragAssets != null && _dragAssets.HasValidDrag)
                {
                    List<SceneGraphNode> graphNodes = new List<SceneGraphNode>();
                    for (int i = 0; i < _dragAssets.Objects.Count; i++)
                    {
                        var item = _dragAssets.Objects[i];
                        if (item.IsOfType<SceneAsset>())
                        {
                            Editor.Instance.Scene.OpenScene(item.ID, true);
                            continue;
                        }
                        var actor = item.OnEditorDrop(this);
                        actor.Name = item.ShortName;
                        Level.SpawnActor(actor);
                        var graphNode = Editor.Scene.GetActorNode(actor.ID);
                        if (graphNode != null)
                            graphNodes.Add(graphNode);
                        Editor.Scene.MarkSceneEdited(actor.Scene);
                    }
                    if (graphNodes.Count > 0)
                        Editor.SceneEditing.Select(graphNodes);
                    result = DragDropEffect.Move;
                }
                // Drag actor type
                else if (_dragActorType != null && _dragActorType.HasValidDrag)
                {
                    for (int i = 0; i < _dragActorType.Objects.Count; i++)
                    {
                        var item = _dragActorType.Objects[i];
                        var actor = item.CreateInstance() as Actor;
                        if (actor == null)
                        {
                            Editor.LogWarning("Failed to spawn actor of type " + item.TypeName);
                            continue;
                        }
                        actor.Name = item.Name;
                        Level.SpawnActor(actor);
                        Editor.Scene.MarkSceneEdited(actor.Scene);
                    }
                    result = DragDropEffect.Move;
                }
                // Drag control type
                else if (_dragControlType != null && _dragControlType.HasValidDrag)
                {
                    for (int i = 0; i < _dragControlType.Objects.Count; i++)
                    {
                        var item = _dragControlType.Objects[i];
                        var control = item.CreateInstance() as Control;
                        if (control == null)
                        {
                            Editor.LogWarning("Failed to spawn UIControl with control type " + item.TypeName);
                            continue;
                        }
                        var uiControl = new UIControl
                        {
                            Control = control,
                            Name = item.Name,
                        };
                        Level.SpawnActor(uiControl);
                        Editor.Scene.MarkSceneEdited(uiControl.Scene);
                    }
                    result = DragDropEffect.Move;
                }
                // Drag script item
                else if (_dragScriptItems != null && _dragScriptItems.HasValidDrag)
                {
                    List<SceneGraphNode> graphNodes = new List<SceneGraphNode>();
                    for (int i = 0; i < _dragScriptItems.Objects.Count; i++)
                    {
                        var item = _dragScriptItems.Objects[i];
                        var actorType = Editor.Instance.CodeEditing.Actors.Get(item);
                        if (actorType != ScriptType.Null)
                        {
                            var actor = actorType.CreateInstance() as Actor;
                            if (actor == null)
                            {
                                Editor.LogWarning("Failed to spawn actor of type " + actorType.TypeName);
                                continue;
                            }
                            actor.Name = actorType.Name;
                            Level.SpawnActor(actor);
                            var graphNode = Editor.Scene.GetActorNode(actor.ID);
                            if (graphNode != null)
                                graphNodes.Add(graphNode);
                            Editor.Scene.MarkSceneEdited(actor.Scene);
                        }
                    }
                    if (graphNodes.Count > 0)
                        Editor.SceneEditing.Select(graphNodes);
                    result = DragDropEffect.Move;
                }

                _dragHandlers.OnDragDrop(null);
            }
            return result;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _dragAssets = null;
            _dragActorType = null;
            _dragControlType = null;
            _dragScriptItems = null;
            _dragHandlers?.Clear();
            _dragHandlers = null;
            _tree = null;
            _searchBox = null;

            base.OnDestroy();
        }
    }
}
