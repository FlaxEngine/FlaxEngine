// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using FlaxEditor.GUI.Tree;
using FlaxEditor.SceneGraph;
using FlaxEditor.SceneGraph.GUI;
using FlaxEngine;

namespace FlaxEditor.Windows.Assets
{
    public sealed partial class PrefabWindow
    {
        private readonly List<SceneGraphNode> _selection = new List<SceneGraphNode>();

        /// <summary>
        /// Occurs when selection gets changed.
        /// </summary>
        public event Action SelectionChanged;

        private void OnTreeSelectedChanged(List<TreeNode> before, List<TreeNode> after)
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
                Select(actors);
            }
            else
            {
                // Deselect
                Deselect();
            }
        }

        /// <summary>
        /// Called when selection gets changed.
        /// </summary>
        /// <param name="before">The selection before the change.</param>
        public void OnSelectionChanged(SceneGraphNode[] before)
        {
            if (LockSelection)
                return;

            Undo.AddAction(new SelectionChangeAction(before, Selection.ToArray(), OnSelectionUndo));

            OnSelectionChanges();
        }

        private void OnSelectionUndo(SceneGraphNode[] toSelect)
        {
            Selection.Clear();
            foreach (var e in toSelect)
            {
                if (e != null)
                    Selection.Add(e);
            }
            OnSelectionChanges();
        }

        private void OnSelectionChanges()
        {
            _isUpdatingSelection = true;

            // Update tree
            var selection = Selection;
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
                    ScrollToSelectedNode();
            }

            // Update properties editor
            var objects = Selection.ConvertAll(x => x.EditableObject).Distinct();
            _propertiesEditor.Select(objects);

            _isUpdatingSelection = false;

            // Send event
            SelectionChanged?.Invoke();
        }

        /// <summary>
        /// Selects the specified nodes collection.
        /// </summary>
        /// <param name="nodes">The nodes.</param>
        public void Select(List<SceneGraphNode> nodes)
        {
            nodes?.RemoveAll(x => x == null);
            if (nodes == null || nodes.Count == 0)
            {
                Deselect();
                return;
            }

            if (Utils.ArraysEqual(Selection, nodes))
                return;

            var before = Selection.ToArray();
            Selection.Clear();
            Selection.AddRange(nodes);

            OnSelectionChanged(before);
        }

        /// <summary>
        /// Selects the specified node.
        /// </summary>
        /// <param name="node">The node.</param>
        /// <param name="additive">if set to <c>true</c> will use additive mode, otherwise will clear previous selection.</param>
        public void Select(SceneGraphNode node, bool additive = false)
        {
            if (node == null)
            {
                Deselect();
                return;
            }

            // Check if won't change
            if (!additive && Selection.Count == 1 && Selection[0] == node)
                return;
            if (additive && Selection.Contains(node))
                return;

            var before = Selection.ToArray();
            if (!additive)
                Selection.Clear();
            Selection.Add(node);

            OnSelectionChanged(before);
        }

        /// <summary>
        /// Deselects the specified node.
        /// </summary>
        public void Deselect(SceneGraphNode node)
        {
            if (!Selection.Contains(node))
                return;

            var before = Selection.ToArray();
            Selection.Remove(node);

            OnSelectionChanged(before);
        }

        /// <summary>
        /// Clears the selection.
        /// </summary>
        public void Deselect()
        {
            if (Selection.Count == 0)
                return;

            var before = Selection.ToArray();
            Selection.Clear();

            OnSelectionChanged(before);
        }
    }
}
