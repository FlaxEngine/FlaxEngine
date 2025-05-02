// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEngine;

namespace FlaxEditor.SceneGraph
{
    /// <summary>
    /// Set of tools for <see cref="SceneGraphNode"/> processing.
    /// </summary>
    [HideInEditor]
    public static class SceneGraphTools
    {
        /// <summary>
        /// Delegate for scene graph action execution callback.
        /// </summary>
        /// <param name="node">The node.</param>
        /// <returns>True if call deeper, otherwise skip calling node children.</returns>
        public delegate bool GraphExecuteCallbackDelegate(SceneGraphNode node);

        /// <summary>
        /// Executes the custom action on the graph nodes.
        /// </summary>
        /// <param name="node">The node</param>
        /// <param name="callback">The callback.</param>
        public static void ExecuteOnGraph(this SceneGraphNode node, GraphExecuteCallbackDelegate callback)
        {
            for (int i = 0; i < node.ChildNodes.Count; i++)
            {
                if (callback(node.ChildNodes[i]))
                {
                    ExecuteOnGraph(node.ChildNodes[i], callback);
                }
            }
        }

        /// <summary>
        /// Builds the array made of input nodes that are at the top level. The result collection contains only nodes that don't have parent nodes in the given collection.
        /// </summary>
        /// <typeparam name="T">The scene graph node type.</typeparam>
        /// <param name="nodes">The nodes.</param>
        /// <returns>The result.</returns>
        public static List<T> BuildNodesParents<T>(this List<T> nodes)
        where T : SceneGraphNode
        {
            var list = new List<T>();
            BuildNodesParents(nodes, list);
            return list;
        }

        /// <summary>
        /// Builds the list made of input nodes that are at the top level. The result collection contains only nodes that don't have parent nodes in the given collection.
        /// </summary>
        /// <typeparam name="T">The scene graph node type.</typeparam>
        /// <param name="nodes">The nodes.</param>
        /// <param name="result">The result.</param>
        public static void BuildNodesParents<T>(this List<T> nodes, List<T> result)
        where T : SceneGraphNode
        {
            if (nodes == null || result == null)
                throw new ArgumentNullException();

            result.Clear();

            // Build solid part of the tree
            var fullTree = BuildAllNodes(nodes);

            for (var i = 0; i < nodes.Count; i++)
            {
                var target = nodes[i];

                // If there is no target node parent in the solid tree list,
                // then it means it's a local root node and can be added to the results.
                if (!fullTree.Contains(target.ParentNode))
                    result.Add(target);
            }
        }

        /// <summary>
        /// Builds the array made of all nodes in the input list and child tree. The result collection contains all nodes in the tree.
        /// </summary>
        /// <param name="nodes">The nodes.</param>
        /// <returns>The result.</returns>
        public static List<SceneGraphNode> BuildAllNodes<T>(this List<T> nodes)
        where T : SceneGraphNode
        {
            var list = new List<SceneGraphNode>();
            BuildAllNodes(nodes, list);
            return list;
        }

        private static void FillTree(SceneGraphNode node, List<SceneGraphNode> result)
        {
            if (result.Contains(node))
                return;
            result.Add(node);
            var children = node.ChildNodes;
            for (int i = 0; i < children.Count; i++)
                FillTree(children[i], result);
        }

        /// <summary>
        /// Builds the list made of all nodes in the input list and child tree. The result collection contains all nodes in the tree.
        /// </summary>
        /// <param name="nodes">The nodes.</param>
        /// <param name="result">The result.</param>
        public static void BuildAllNodes<T>(this List<T> nodes, List<SceneGraphNode> result)
        where T : SceneGraphNode
        {
            if (nodes == null || result == null)
                throw new ArgumentNullException();
            result.Clear();
            for (var i = 0; i < nodes.Count; i++)
                FillTree(nodes[i], result);
        }

        /// <summary>
        /// Builds the list made of all nodes in the input scene tree root and child tree. The result collection contains all nodes in the tree.
        /// </summary>
        /// <param name="node">The node.</param>
        /// <returns>The result.</returns>
        public static List<SceneGraphNode> BuildAllNodes<T>(this T node)
        where T : SceneGraphNode
        {
            var list = new List<SceneGraphNode>();
            BuildAllNodes(node, list);
            return list;
        }

        /// <summary>
        /// Builds the list made of all nodes in the input scene tree root and child tree. The result collection contains all nodes in the tree.
        /// </summary>
        /// <param name="node">The node.</param>
        /// <param name="result">The result.</param>
        public static void BuildAllNodes<T>(this T node, List<SceneGraphNode> result)
        where T : SceneGraphNode
        {
            if (node == null || result == null)
                throw new ArgumentNullException();
            result.Clear();
            FillTree(node, result);
        }
    }
}
