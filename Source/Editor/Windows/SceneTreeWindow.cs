// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.Gizmo;
using FlaxEditor.GUI.Tree;
using FlaxEditor.SceneGraph;
using FlaxEditor.SceneGraph.GUI;
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
        private bool _isUpdatingSelection;
        private bool _isMouseDown;

        /// <summary>
        /// Initializes a new instance of the <see cref="SceneTreeWindow"/> class.
        /// </summary>
        /// <param name="editor">The editor.</param>
        public SceneTreeWindow(Editor editor)
        : base(editor, true, ScrollBars.Both)
        {
            Title = "Scene";
            ScrollMargin = new Margin(0, 0, 0, 100.0f);

            // Scene searching query input box
            var headerPanel = new ContainerControl
            {
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                IsScrollable = true,
                Offsets = new Margin(0, 0, 0, 18 + 6),
                Parent = this,
            };
            _searchBox = new TextBox
            {
                AnchorPreset = AnchorPresets.HorizontalStretchMiddle,
                WatermarkText = "Search...",
                Parent = headerPanel,
                Bounds = new Rectangle(4, 4, headerPanel.Width - 8, 18),
            };
            _searchBox.TextChanged += OnSearchBoxTextChanged;

            // Create scene structure tree
            var root = editor.Scene.Root;
            root.TreeNode.ChildrenIndent = 0;
            root.TreeNode.Expand();
            _tree = new Tree(true)
            {
                Y = headerPanel.Bottom,
                Margin = new Margin(0.0f, 0.0f, -16.0f, 0.0f), // Hide root node
            };
            _tree.AddChild(root.TreeNode);
            _tree.SelectedChanged += Tree_OnSelectedChanged;
            _tree.RightClick += Tree_OnRightClick;
            _tree.Parent = this;

            // Setup input actions
            InputActions.Add(options => options.TranslateMode, () => Editor.MainTransformGizmo.ActiveMode = TransformGizmoBase.Mode.Translate);
            InputActions.Add(options => options.RotateMode, () => Editor.MainTransformGizmo.ActiveMode = TransformGizmoBase.Mode.Rotate);
            InputActions.Add(options => options.ScaleMode, () => Editor.MainTransformGizmo.ActiveMode = TransformGizmoBase.Mode.Scale);
            InputActions.Add(options => options.FocusSelection, () => Editor.Windows.EditWin.ShowSelectedActors());
        }

        private void OnSearchBoxTextChanged()
        {
            // Skip events during setup or init stuff
            if (IsLayoutLocked)
                return;

            var root = Editor.Scene.Root;
            root.TreeNode.LockChildrenRecursive();

            // Update tree
            var query = _searchBox.Text;
            root.TreeNode.UpdateFilter(query);

            root.TreeNode.UnlockChildrenRecursive();
            PerformLayout();
            PerformLayout();
        }

        private void Rename()
        {
            (Editor.SceneEditing.Selection[0] as ActorNode).TreeNode.StartRenaming();
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
                actor.Name = StringUtils.IncrementNameNumber(type.Name, x => parentActor.GetChild(x) == null);
            }

            // Spawn it
            Editor.SceneEditing.Spawn(actor, parentActor);
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

        private void Tree_OnRightClick(TreeNode node, Vector2 location)
        {
            if (!Editor.StateMachine.CurrentState.CanEditScene)
                return;

            ShowContextMenu(node, ref location);
        }

        /// <inheritdoc />
        public override void OnInit()
        {
            Editor.SceneEditing.SelectionChanged += OnOnSelectionChanged;
        }

        private void OnOnSelectionChanged()
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
                    ScrollViewTo(nodes[0]);
                }
            }

            _isUpdatingSelection = false;
        }

        /// <inheritdoc />
        public override void Draw()
        {
            var style = Style.Current;
            // Draw overlay
            string overlayText = null;
            var state = Editor.StateMachine.CurrentState;
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
            }
            if (overlayText != null)
            {
                Render2D.DrawText(Style.Current.FontLarge, overlayText, GetClientArea(), style.ForegroundDisabled, TextAlignment.Center, TextAlignment.Center);
            }

            base.Draw();
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Vector2 location, MouseButton buttons)
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
        public override bool OnMouseUp(Vector2 location, MouseButton buttons)
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
                    ShowContextMenu(this, ref location);
                }

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
        public override void OnDestroy()
        {
            _tree = null;
            _searchBox = null;

            base.OnDestroy();
        }
    }
}
