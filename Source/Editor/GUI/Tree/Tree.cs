// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using FlaxEngine;
using FlaxEngine.Assertions;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Tree
{
    /// <summary>
    /// Tree control.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    [HideInEditor]
    public class Tree : ContainerControl
    {
        /// <summary>
        /// The key updates timeout in seconds.
        /// </summary>
        public static float KeyUpdateTimeout = 0.12f;

        /// <summary>
        /// Delegate for selected tree nodes collection change.
        /// </summary>
        /// <param name="before">The before state.</param>
        /// <param name="after">The after state.</param>
        public delegate void SelectionChangedDelegate(List<TreeNode> before, List<TreeNode> after);

        /// <summary>
        /// Delegate for node click events.
        /// </summary>
        /// <param name="node">The node.</param>
        /// <param name="location">The location.</param>
        public delegate void NodeClickDelegate(TreeNode node, Vector2 location);

        private float _keyUpdateTime;
        private readonly bool _supportMultiSelect;
        private Margin _margin;
        private bool _autoSize = true;

        /// <summary>
        /// Action fired when tree nodes selection gets changed.
        /// </summary>
        public event SelectionChangedDelegate SelectedChanged;

        /// <summary>
        /// Action fired when mouse goes right click up on node.
        /// </summary>
        public event NodeClickDelegate RightClick;

        /// <summary>
        /// List with all selected nodes
        /// </summary>
        [HideInEditor, NoSerialize]
        public readonly List<TreeNode> Selection = new List<TreeNode>();

        /// <summary>
        /// Gets the first selected node or null.
        /// </summary>
        public TreeNode SelectedNode => Selection.Count > 0 ? Selection[0] : null;

        /// <summary>
        /// Gets or sets the margin for the child tree nodes.
        /// </summary>
        [EditorOrder(0), Tooltip("The margin applied to the child tree nodes.")]
        public Margin Margin
        {
            get => _margin;
            set
            {
                _margin = value;
                PerformLayout();
            }
        }

        /// <summary>
        /// Gets or sets the value indicating whenever the tree will auto-size to the tree nodes dimensions.
        /// </summary>
        [EditorOrder(10), Tooltip("If checked, the tree will auto-size to the tree nodes dimensions.")]
        public bool AutoSize
        {
            get => _autoSize;
            set
            {
                _autoSize = value;
                PerformLayout();
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Tree"/> class.
        /// </summary>
        public Tree()
        : this(false)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Tree"/> class.
        /// </summary>
        /// <param name="supportMultiSelect">True if support multi selection for tree nodes, otherwise false.</param>
        public Tree(bool supportMultiSelect)
        : base(0, 0, 100, 100)
        {
            IsScrollable = true;
            AutoFocus = false;

            _supportMultiSelect = supportMultiSelect;
            _keyUpdateTime = KeyUpdateTimeout * 10;
        }

        internal void OnRightClickInternal(TreeNode node, ref Vector2 location)
        {
            RightClick?.Invoke(node, location);
        }

        /// <summary>
        /// Selects single tree node.
        /// </summary>
        /// <param name="node">Node to select.</param>
        public void Select(TreeNode node)
        {
            if (node == null)
                throw new ArgumentNullException();

            // Check if won't change
            if (Selection.Count == 1 && SelectedNode == node)
                return;

            // Cache previous state
            var prev = new List<TreeNode>(Selection);

            // Update selection
            Selection.Clear();
            Selection.Add(node);

            // Ensure that node can be visible (all it's parents are expanded)
            node.ExpandAllParents();

            node.Focus();

            // Fire event
            SelectedChanged?.Invoke(prev, Selection);
        }

        /// <summary>
        /// Selects tree nodes.
        /// </summary>
        /// <param name="nodes">Nodes to select.</param>
        public void Select(List<TreeNode> nodes)
        {
            if (nodes == null)
                throw new ArgumentNullException();

            // Check if won't change
            if (Selection.Count == nodes.Count && Selection.SequenceEqual(nodes))
                return;

            // Cache previous state
            var prev = new List<TreeNode>(Selection);

            // Update selection
            Selection.Clear();
            if (_supportMultiSelect)
                Selection.AddRange(nodes);
            else if (nodes.Count > 0)
                Selection.Add(nodes[0]);

            // Ensure that every selected node can be visible (all it's parents are expanded)
            // TODO: maybe use faster tree walk or faster algorythm?
            for (int i = 0; i < Selection.Count; i++)
            {
                Selection[i].ExpandAllParents();
            }

            // Fire event
            SelectedChanged?.Invoke(prev, Selection);
        }

        /// <summary>
        /// Clears the selection.
        /// </summary>
        public void Deselect()
        {
            // Check if won't change
            if (Selection.Count == 0)
                return;

            // Cache previous state
            var prev = new List<TreeNode>(Selection);

            // Update selection
            Selection.Clear();

            // Fire event
            SelectedChanged?.Invoke(prev, Selection);
        }

        /// <summary>
        /// Adds or removes node to/from the selection
        /// </summary>
        /// <param name="node">The node.</param>
        public void AddOrRemoveSelection(TreeNode node)
        {
            // Cache previous state
            var prev = new List<TreeNode>(Selection);

            // Check if is selected
            int index = Selection.IndexOf(node);
            if (index != -1)
            {
                // Remove
                Selection.RemoveAt(index);
            }
            else
            {
                if (!_supportMultiSelect)
                    Selection.Clear();

                // Add
                Selection.Add(node);
            }

            // Fire event
            SelectedChanged?.Invoke(prev, Selection);
        }

        private void WalkSelectRangeExpandedTree(List<TreeNode> selection, TreeNode node, ref Rectangle range)
        {
            for (int i = 0; i < node.ChildrenCount; i++)
            {
                if (node.GetChild(i) is TreeNode child)
                {
                    Vector2 pos = child.PointToParent(this, Vector2.One);
                    if (range.Contains(pos))
                    {
                        selection.Add(child);
                    }

                    var nodeArea = new Rectangle(pos, child.Size);
                    if (child.IsExpanded && range.Intersects(ref nodeArea))
                        WalkSelectRangeExpandedTree(selection, child, ref range);
                }
            }
        }

        private Rectangle CalcNodeRangeRect(TreeNode node)
        {
            Vector2 pos = node.PointToParent(this, Vector2.One);
            return new Rectangle(pos, new Vector2(10000, 4));
        }

        /// <summary>
        /// Selects tree nodes range (used to select part of the tree using Shift+Mouse).
        /// </summary>
        /// <param name="endNode">End range node</param>
        public void SelectRange(TreeNode endNode)
        {
            if (_supportMultiSelect && Selection.Count > 0)
            {
                // Cache previous state
                var prev = new List<TreeNode>(Selection);

                // Update selection
                var selectionRect = CalcNodeRangeRect(Selection[0]);
                for (int i = 1; i < Selection.Count; i++)
                {
                    selectionRect = Rectangle.Union(selectionRect, CalcNodeRangeRect(Selection[i]));
                }
                var endNodeRect = CalcNodeRangeRect(endNode);
                if (endNodeRect.Top - Mathf.Epsilon <= selectionRect.Top)
                {
                    float diff = selectionRect.Top - endNodeRect.Top;
                    selectionRect.Location.Y -= diff;
                    selectionRect.Size.Y += diff;
                }
                else if (endNodeRect.Bottom + Mathf.Epsilon >= selectionRect.Bottom)
                {
                    float diff = endNodeRect.Bottom - selectionRect.Bottom;
                    selectionRect.Size.Y += diff;
                }
                Selection.Clear();
                WalkSelectRangeExpandedTree(Selection, _children[0] as TreeNode, ref selectionRect);

                // Check if changed
                if (Selection.Count != prev.Count || !Selection.SequenceEqual(prev))
                {
                    // Fire event
                    SelectedChanged?.Invoke(prev, Selection);
                }
            }
            else
            {
                Select(endNode);
            }
        }

        private void WalkSelectExpandedTree(List<TreeNode> selection, TreeNode node)
        {
            for (int i = 0; i < node.ChildrenCount; i++)
            {
                if (node.GetChild(i) is TreeNode child)
                {
                    selection.Add(child);
                    if (child.IsExpanded)
                        WalkSelectExpandedTree(selection, child);
                }
            }
        }

        /// <summary>
        /// Select all expanded nodes
        /// </summary>
        public void SelectAllExpanded()
        {
            if (_supportMultiSelect)
            {
                // Cache previous state
                var prev = new List<TreeNode>(Selection);

                // Update selection
                Selection.Clear();
                WalkSelectExpandedTree(Selection, _children[0] as TreeNode);

                // Check if changed
                if (Selection.Count != prev.Count || !Selection.SequenceEqual(prev))
                {
                    // Fire event
                    SelectedChanged?.Invoke(prev, Selection);
                }
            }
        }

        /// <summary>
        /// Updates the tree size.
        /// </summary>
        public void UpdateSize()
        {
            if (!_autoSize)
                return;

            // Use max of parent clint area width and root node width
            float width = 0;
            if (HasParent)
                width = Parent.GetClientArea().Width;
            var rightBottom = Vector2.Zero;
            for (int i = 0; i < _children.Count; i++)
            {
                if (_children[i] is TreeNode node && node.Visible)
                {
                    width = Mathf.Max(width, node.MinimumWidth);
                    rightBottom = Vector2.Max(rightBottom, node.BottomRight);
                }
            }
            Size = new Vector2(width + _margin.Width, rightBottom.Y + _margin.Bottom);
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            var node = SelectedNode;

            // Check if has focus and if any node is focused and it isn't a root
            if (ContainsFocus && node != null && !node.IsRoot)
            {
                var window = Root;

                // Check if can perform update
                if (_keyUpdateTime >= KeyUpdateTimeout)
                {
                    bool keyUpArrow = window.GetKeyDown(KeyboardKeys.ArrowUp);
                    bool keyDownArrow = window.GetKeyDown(KeyboardKeys.ArrowDown);

                    // Check if arrow flags are different
                    if (keyDownArrow != keyUpArrow)
                    {
                        var nodeParent = node.Parent;
                        var parentNode = nodeParent as TreeNode;
                        var myIndex = nodeParent.GetChildIndex(node);
                        Assert.AreNotEqual(-1, myIndex);

                        // Up
                        if (keyUpArrow)
                        {
                            TreeNode toSelect = null;
                            if (myIndex == 0)
                            {
                                // Select parent (if it exists and it isn't a root)
                                if (parentNode != null && !parentNode.IsRoot)
                                    toSelect = parentNode;
                            }
                            else
                            {
                                // Select previous parent child
                                toSelect = nodeParent.GetChild(myIndex - 1) as TreeNode;

                                // Check if is valid and expanded and has any children
                                if (toSelect != null && toSelect.IsExpanded && toSelect.HasAnyVisibleChild)
                                {
                                    // Select last child
                                    toSelect = toSelect.GetChild(toSelect.ChildrenCount - 1) as TreeNode;
                                }
                            }
                            if (toSelect != null)
                            {
                                // Select
                                Select(toSelect);
                                toSelect.Focus();
                            }
                        }
                        // Down
                        else
                        {
                            TreeNode toSelect = null;
                            if (node.IsExpanded && node.HasAnyVisibleChild)
                            {
                                // Select the first child
                                toSelect = node.GetChild(0) as TreeNode;
                            }
                            else if (myIndex == nodeParent.ChildrenCount - 1)
                            {
                                // Select next node after parent
                                if (parentNode != null)
                                {
                                    int parentIndex = parentNode.IndexInParent;
                                    if (parentIndex != -1 && parentIndex < parentNode.Parent.ChildrenCount - 1)
                                    {
                                        toSelect = parentNode.Parent.GetChild(parentIndex + 1) as TreeNode;
                                    }
                                }
                            }
                            else
                            {
                                // Select next parent child
                                toSelect = nodeParent.GetChild(myIndex + 1) as TreeNode;
                            }
                            if (toSelect != null)
                            {
                                // Select
                                Select(toSelect);
                                toSelect.Focus();
                            }
                        }

                        // Reset time
                        _keyUpdateTime = 0.0f;
                    }
                }
                else
                {
                    // Update time
                    _keyUpdateTime += deltaTime;
                }

                if (window.GetKeyDown(KeyboardKeys.ArrowRight))
                {
                    if (node.IsExpanded)
                    {
                        // Select first child if has
                        if (node.HasAnyVisibleChild)
                        {
                            Select(node.GetChild(0) as TreeNode);
                            node.Focus();
                        }
                    }
                    else
                    {
                        // Expand selected node
                        node.Expand();
                    }
                }
                else if (window.GetKeyDown(KeyboardKeys.ArrowLeft))
                {
                    if (node.IsCollapsed)
                    {
                        // Select parent if has and is not a root
                        if (node.HasParent && node.Parent is TreeNode nodeParentNode && !nodeParentNode.IsRoot)
                        {
                            Select(nodeParentNode);
                            nodeParentNode.Focus();
                        }
                    }
                    else
                    {
                        // Collapse selected node
                        node.Collapse();
                    }
                }
            }

            base.Update(deltaTime);
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            // Check if can use multi selection
            if (_supportMultiSelect)
            {
                bool isCtrlDown = Root.GetKey(KeyboardKeys.Control);

                // Select all expanded nodes
                if (key == KeyboardKeys.A && isCtrlDown)
                {
                    SelectAllExpanded();
                    return true;
                }
            }

            return base.OnKeyDown(key);
        }

        /// <inheritdoc />
        public override void OnGotFocus()
        {
            // Reset timer
            _keyUpdateTime = 0;

            base.OnGotFocus();
        }

        /// <inheritdoc />
        public override void OnChildResized(Control control)
        {
            UpdateSize();

            base.OnChildResized(control);
        }

        /// <inheritdoc />
        public override void OnParentResized()
        {
            UpdateSize();

            base.OnParentResized();
        }

        /// <inheritdoc />
        protected override void PerformLayoutAfterChildren()
        {
            // Arrange children
            float y = _margin.Top;
            for (int i = 0; i < _children.Count; i++)
            {
                if (_children[i] is TreeNode node && node.Visible)
                {
                    node.Location = new Vector2(_margin.Left, y);
                    y += node.Height + TreeNode.DefaultNodeOffsetY;
                }
            }

            UpdateSize();
        }
    }
}
