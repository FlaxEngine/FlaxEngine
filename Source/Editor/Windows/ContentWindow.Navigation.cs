// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using FlaxEditor.Content;
using FlaxEditor.Content.GUI;
using FlaxEditor.GUI;
using FlaxEditor.GUI.Tree;

namespace FlaxEditor.Windows
{
    public partial class ContentWindow
    {
        private static readonly List<ContentTreeNode> NavUpdateCache = new List<ContentTreeNode>(8);

        private void OnTreeSelectionChanged(List<TreeNode> from, List<TreeNode> to)
        {
            // Navigate
            var source = from.Count > 0 ? from[0] as ContentTreeNode : null;
            var target = to.Count > 0 ? to[0] as ContentTreeNode : null;
            Navigate(source, target);

            target?.Focus();
        }

        /// <summary>
        /// Navigates to the specified target content location.
        /// </summary>
        /// <param name="target">The target.</param>
        public void Navigate(ContentTreeNode target)
        {
            Navigate(SelectedNode, target);
        }

        private void Navigate(ContentTreeNode source, ContentTreeNode target)
        {
            if (target == null)
                target = _root;

            // Check if can do this action
            if (_navigationUnlocked && source != target)
            {
                // Lock navigation
                _navigationUnlocked = false;

                // Check if already added to the Undo on the top
                if (source != null && (_navigationUndo.Count == 0 || _navigationUndo.Peek() != source))
                {
                    // Add to Undo list
                    _navigationUndo.Push(source);
                }

                // Show folder contents and select tree node
                RefreshView(target);
                _tree.Select(target);
                target.ExpandAllParents();

                // Clear redo list
                _navigationRedo.Clear();

                // Set valid sizes for stacks
                //RedoList.SetSize(32);
                //UndoList.SetSize(32);

                // Update search
                UpdateItemsSearch();

                // Unlock navigation
                _navigationUnlocked = true;

                // Update UI
                UpdateUI();
            }
        }

        /// <summary>
        /// Navigates backward.
        /// </summary>
        public void NavigateBackward()
        {
            // Check if navigation is unlocked
            if (_navigationUnlocked && _navigationUndo.Count > 0)
            {
                // Pop node
                ContentTreeNode node = _navigationUndo.Pop();

                // Lock navigation
                _navigationUnlocked = false;

                // Add to Redo list
                _navigationRedo.Push(SelectedNode);

                // Select node
                RefreshView(node);
                _tree.Select(node);
                node.ExpandAllParents();

                // Set valid sizes for stacks
                //RedoList.SetSize(32);
                //UndoList.SetSize(32);

                // Update search
                UpdateItemsSearch();

                // Unlock navigation
                _navigationUnlocked = true;

                // Update UI
                UpdateUI();
                _view.SelectFirstItem();
            }
        }

        /// <summary>
        /// Navigates forward.
        /// </summary>
        public void NavigateForward()
        {
            // Check if navigation is unlocked
            if (_navigationUnlocked && _navigationRedo.Count > 0)
            {
                // Pop node
                ContentTreeNode node = _navigationRedo.Pop();

                // Lock navigation
                _navigationUnlocked = false;

                // Add to Undo list
                _navigationUndo.Push(SelectedNode);

                // Select node
                RefreshView(node);
                _tree.Select(node);
                node.ExpandAllParents();

                // Set valid sizes for stacks
                //RedoList.SetSize(32);
                //UndoList.SetSize(32);

                // Update search
                UpdateItemsSearch();

                // Unlock navigation
                _navigationUnlocked = true;

                // Update UI
                UpdateUI();
                _view.SelectFirstItem();
            }
        }

        /// <summary>
        /// Navigates directory up.
        /// </summary>
        public void NavigateUp()
        {
            ContentTreeNode target = _root;
            ContentTreeNode current = SelectedNode;

            if (current?.Folder.ParentFolder != null)
            {
                target = current.Folder.ParentFolder.Node;
            }

            Navigate(target);
        }

        /// <summary>
        /// Clears the navigation history.
        /// </summary>
        public void NavigationClearHistory()
        {
            _navigationUndo.Clear();
            _navigationRedo.Clear();
            UpdateUI();
        }

        private void UpdateNavigationBar()
        {
            if (_navigationBar == null)
                return;

            bool wasLayoutLocked = _navigationBar.IsLayoutLocked;
            _navigationBar.IsLayoutLocked = true;

            // Remove previous buttons
            _navigationBar.DisposeChildren();

            // Spawn buttons
            var nodes = NavUpdateCache;
            nodes.Clear();
            ContentTreeNode node = SelectedNode;
            while (node != null)
            {
                nodes.Add(node);
                node = node.ParentNode;
            }
            float margin = 1;
            float x = NavigationBar.DefaultButtonsMargin;
            float h = _toolStrip.ItemsHeight - 2 * margin;
            for (int i = nodes.Count - 1; i >= 0; i--)
            {
                var button = new ContentNavigationButton(nodes[i], x, margin, h);
                button.PerformLayout();
                x += button.Width + NavigationBar.DefaultButtonsMargin;
                _navigationBar.AddChild(button);
                if (i > 0)
                {
                    var separator = new ContentNavigationSeparator(button, x, margin, h);
                    separator.PerformLayout();
                    x += separator.Width + NavigationBar.DefaultButtonsMargin;
                    _navigationBar.AddChild(separator);
                }
            }
            nodes.Clear();

            // Update
            _navigationBar.IsLayoutLocked = wasLayoutLocked;
            _navigationBar.PerformLayout();
            UpdateNavigationBarBounds();
        }

        /// <summary>
        /// Gets the selected tree node.
        /// </summary>
        public ContentTreeNode SelectedNode => _tree.SelectedNode as ContentTreeNode;

        /// <summary>
        /// Gets the current view folder.
        /// </summary>
        public ContentFolder CurrentViewFolder => SelectedNode?.Folder;

        /// <summary>
        /// Shows the root folder.
        /// </summary>
        public void ShowRoot()
        {
            _tree.Select(_root);
        }
    }
}
