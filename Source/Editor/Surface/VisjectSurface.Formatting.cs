using FlaxEditor.Surface.Elements;
using FlaxEditor.Surface.Undo;
using FlaxEngine;
using System;
using System.Collections.Generic;
using System.Linq;

namespace FlaxEditor.Surface
{
    public partial class VisjectSurface
    {
        // Reference https://github.com/stefnotch/xnode-graph-formatter/blob/812e08e71c7b9b7eb0810dbdfb0a9a1034da6941/Assets/Examples/MathGraph/Editor/MathGraphEditor.cs

        private class NodeFormattingData
        {
            /// <summary>
            /// Starting from 0 at the main nodes.
            /// </summary>
            public int Layer;

            /// <summary>
            /// Position in the layer.
            /// </summary>
            public int Offset;

            /// <summary>
            /// How far the subtree needs to be moved additionally.
            /// </summary>
            public int SubtreeOffset;
        }

        /// <summary>
        /// Formats a graph where the nodes can be disjointed.
        /// Uses the Sugiyama method.
        /// </summary>
        /// <param name="nodes">List of nodes.</param>
        public void FormatGraph(List<SurfaceNode> nodes)
        {
            if (nodes.Count <= 1)
                return;

            var nodesToVisit = new HashSet<SurfaceNode>(nodes);

            // While we haven't formatted every node
            while (nodesToVisit.Count > 0)
            {
                // Run a search in both directions
                var connectedNodes = new List<SurfaceNode>();
                var queue = new Queue<SurfaceNode>();

                var startNode = nodesToVisit.First();
                nodesToVisit.Remove(startNode);
                queue.Enqueue(startNode);

                while (queue.Count > 0)
                {
                    var node = queue.Dequeue();
                    connectedNodes.Add(node);

                    for (int i = 0; i < node.Elements.Count; i++)
                    {
                        if (node.Elements[i] is Box box)
                        {
                            for (int j = 0; j < box.Connections.Count; j++)
                            {
                                if (nodesToVisit.Contains(box.Connections[j].ParentNode))
                                {
                                    nodesToVisit.Remove(box.Connections[j].ParentNode);
                                    queue.Enqueue(box.Connections[j].ParentNode);
                                }
                            }
                        }
                    }
                }

                FormatConnectedGraph(connectedNodes);
            }
        }

        /// <summary>
        /// Formats a graph where all nodes are connected.
        /// </summary>
        /// <param name="nodes">List of connected nodes.</param>
        protected void FormatConnectedGraph(List<SurfaceNode> nodes)
        {
            if (nodes.Count <= 1)
                return;

            var boundingBox = GetNodesBounds(nodes);

            var nodeData = nodes.ToDictionary(n => n, n => new NodeFormattingData { });

            // Rightmost nodes with none of our nodes to the right of them
            var endNodes = nodes
                           .Where(n => !n.GetBoxes().Any(b => b.IsOutput && b.Connections.Any(c => nodeData.ContainsKey(c.ParentNode))))
                           .OrderBy(n => n.Top) // Keep their relative order
                           .ToList();

            // Longest path layering
            int maxLayer = SetLayers(nodeData, endNodes);

            // Set the vertical offsets
            int maxOffset = SetOffsets(nodeData, endNodes, maxLayer);

            // Layout the nodes

            // Get the largest nodes in the Y and X direction
            float[] widths = new float[maxLayer + 1];
            float[] heights = new float[maxOffset + 1];
            for (int i = 0; i < nodes.Count; i++)
            {
                if (nodeData.TryGetValue(nodes[i], out var data))
                {
                    if (nodes[i].Width > widths[data.Layer])
                    {
                        widths[data.Layer] = nodes[i].Width;
                    }
                    if (nodes[i].Height > heights[data.Offset])
                    {
                        heights[data.Offset] = nodes[i].Height;
                    }
                }
            }

            var minDistanceBetweenNodes = new Float2(30, 30);

            // Figure out the node positions (aligned to a grid)
            float[] nodeXPositions = new float[widths.Length];
            for (int i = 1; i < widths.Length; i++)
            {
                // Go from right to left (backwards) through the nodes
                nodeXPositions[i] = nodeXPositions[i - 1] + minDistanceBetweenNodes.X + widths[i];
            }

            float[] nodeYPositions = new float[heights.Length];
            for (int i = 1; i < heights.Length; i++)
            {
                // Go from top to bottom through the nodes
                nodeYPositions[i] = nodeYPositions[i - 1] + heights[i - 1] + minDistanceBetweenNodes.Y;
            }

            // Set the node positions
            var undoActions = new List<MoveNodesAction>();
            var topRightPosition = endNodes[0].Location;
            for (int i = 0; i < nodes.Count; i++)
            {
                if (nodeData.TryGetValue(nodes[i], out var data))
                {
                    var newLocation = new Float2(-nodeXPositions[data.Layer], nodeYPositions[data.Offset]) + topRightPosition;
                    var locationDelta = newLocation - nodes[i].Location;
                    nodes[i].Location = newLocation;

                    if (Undo != null)
                        undoActions.Add(new MoveNodesAction(Context, new[] { nodes[i].ID }, locationDelta));
                }
            }

            MarkAsEdited(false);
            Undo?.AddAction(new MultiUndoAction(undoActions, "Format nodes"));
        }

        /// <summary>
        /// Straightens every connection between nodes in <paramref name="nodes"/>.
        /// </summary>
        /// <param name="nodes">List of nodes.</param>
        public void StraightenGraphConnections(List<SurfaceNode> nodes)
        {
            if (nodes.Count <= 1)
                return;

            List<MoveNodesAction> undoActions = new List<MoveNodesAction>();

            // Only process nodes that have any connection
            List<SurfaceNode> connectedNodes = nodes.Where(n => n.GetBoxes().Any(b => b.HasAnyConnection)).ToList();

            if (connectedNodes.Count == 0)
                return;

            for (int i = 0; i < connectedNodes.Count - 1; i++)
            {
                SurfaceNode nodeA = connectedNodes[i];
                List<Box> connectedOutputBoxes = nodeA.GetBoxes().Where(b => b.IsOutput && b.HasAnyConnection).ToList();

                for (int j = 0; j < connectedOutputBoxes.Count; j++)
                {
                    Box boxA = connectedOutputBoxes[j];

                    for (int b = 0; b < boxA.Connections.Count; b++)
                    {
                        Box boxB = boxA.Connections[b];

                        // Ensure the other node is selected
                        if (!connectedNodes.Contains(boxB.ParentNode))
                            continue;

                        // Node with no outgoing connections reached. Advance to next node in list
                        if (boxA == null || boxB == null)
                            continue;

                        SurfaceNode nodeB = boxB.ParentNode;

                        // Calculate the Y offset needed for nodeB to align boxB's Y to boxA's Y
                        float boxASurfaceY = boxA.PointToParent(this, Float2.Zero).Y;
                        float boxBSurfaceY = boxB.PointToParent(this, Float2.Zero).Y;
                        float deltaY = (boxASurfaceY - boxBSurfaceY) / ViewScale;
                        Float2 delta = new Float2(0f, deltaY);

                        nodeB.Location += delta;

                        if (Undo != null)
                            undoActions.Add(new MoveNodesAction(Context, new[] { nodeB.ID }, delta));
                    }
                }
            }

            if (undoActions.Count > 0)
                Undo?.AddAction(new MultiUndoAction(undoActions, "Straightned "));

            MarkAsEdited(false);
        }

        /// <summary>
        /// Assigns a layer to every node.
        /// </summary>
        /// <param name="nodeData">The exta node data.</param>
        /// <param name="endNodes">The end nodes.</param>
        /// <returns>The number of the maximum layer.</returns>
        private int SetLayers(Dictionary<SurfaceNode, NodeFormattingData> nodeData, List<SurfaceNode> endNodes)
        {
            // Longest path layering
            int maxLayer = 0;
            var stack = new Stack<SurfaceNode>(endNodes);

            while (stack.Count > 0)
            {
                var node = stack.Pop();
                int layer = nodeData[node].Layer;

                for (int i = 0; i < node.Elements.Count; i++)
                {
                    if (node.Elements[i] is InputBox box && box.HasAnyConnection)
                    {
                        var childNode = box.Connections[0].ParentNode;

                        if (nodeData.TryGetValue(childNode, out var data))
                        {
                            int nodeLayer = Math.Max(data.Layer, layer + 1);
                            data.Layer = nodeLayer;
                            if (nodeLayer > maxLayer)
                            {
                                maxLayer = nodeLayer;
                            }

                            stack.Push(childNode);
                        }
                    }
                }
            }
            return maxLayer;
        }


        /// <summary>
        /// Sets the node offsets.
        /// </summary>
        /// <param name="nodeData">The exta node data.</param>
        /// <param name="endNodes">The end nodes.</param>
        /// <param name="maxLayer">The number of the maximum layer.</param>
        /// <returns>The number of the maximum offset.</returns>
        private int SetOffsets(Dictionary<SurfaceNode, NodeFormattingData> nodeData, List<SurfaceNode> endNodes, int maxLayer)
        {
            int maxOffset = 0;

            // Keeps track of the largest offset (Y axis) for every layer
            int[] offsets = new int[maxLayer + 1];

            var visitedNodes = new HashSet<SurfaceNode>();

            void SetOffsets(SurfaceNode node, NodeFormattingData straightParentData)
            {
                if (!nodeData.TryGetValue(node, out var data))
                    return;

                // If we realize that the current node would collide with an already existing node in this layer
                if (data.Layer >= 0 && offsets[data.Layer] > data.Offset)
                {
                    // Move the entire sub-tree down
                    straightParentData.SubtreeOffset = Math.Max(straightParentData.SubtreeOffset, offsets[data.Layer] - data.Offset);
                }

                // Keeps track of the offset of the last direct child we visited
                int childOffset = data.Offset;
                bool straightChild = true;

                // Run the algorithm for every child
                for (int i = 0; i < node.Elements.Count; i++)
                {
                    if (node.Elements[i] is InputBox box && box.HasAnyConnection)
                    {
                        var childNode = box.Connections[0].ParentNode;
                        if (!visitedNodes.Contains(childNode) && nodeData.TryGetValue(childNode, out var childData))
                        {
                            visitedNodes.Add(childNode);
                            childData.Offset = childOffset;
                            SetOffsets(childNode, straightChild ? straightParentData : childData);
                            childOffset = childData.Offset + 1;
                            straightChild = false;
                        }
                    }
                }

                if (data.Layer >= 0)
                {
                    // When coming out of the recursion, apply the extra subtree offsets
                    data.Offset += straightParentData.SubtreeOffset;
                    if (data.Offset > maxOffset)
                    {
                        maxOffset = data.Offset;
                    }
                    offsets[data.Layer] = data.Offset + 1;
                }
            }

            {
                // An imaginary final node
                var endNodeData = new NodeFormattingData { Layer = -1 };
                int childOffset = 0;
                bool straightChild = true;

                for (int i = 0; i < endNodes.Count; i++)
                {
                    if (nodeData.TryGetValue(endNodes[i], out var childData))
                    {
                        visitedNodes.Add(endNodes[i]);
                        childData.Offset = childOffset;
                        SetOffsets(endNodes[i], straightChild ? endNodeData : childData);
                        childOffset = childData.Offset + 1;
                        straightChild = false;
                    }
                }
            }

            return maxOffset;
        }

        /// <summary>
        /// Align given nodes on a graph using the given alignment type.
        /// Ignores any potential overlap.
        /// </summary>
        /// <param name="nodes">List of nodes.</param>
        /// <param name="alignmentType">Alignemnt type.</param>
        public void AlignNodes(List<SurfaceNode> nodes, NodeAlignmentType alignmentType)
        {  
            if(nodes.Count <= 1)
                return;

            var undoActions = new List<MoveNodesAction>();
            var boundingBox = GetNodesBounds(nodes);
            for(int i = 0; i < nodes.Count; i++)
            {
                var centerY = boundingBox.Center.Y - (nodes[i].Height / 2);
                var centerX = boundingBox.Center.X - (nodes[i].Width / 2);
                
                var newLocation = alignmentType switch
                {
                    NodeAlignmentType.Top    => new Float2(nodes[i].Location.X, boundingBox.Top),
                    NodeAlignmentType.Middle => new Float2(nodes[i].Location.X, centerY),
                    NodeAlignmentType.Bottom => new Float2(nodes[i].Location.X, boundingBox.Bottom - nodes[i].Height),

                    NodeAlignmentType.Left   => new Float2(boundingBox.Left, nodes[i].Location.Y),
                    NodeAlignmentType.Center => new Float2(centerX, nodes[i].Location.Y),
                    NodeAlignmentType.Right  => new Float2(boundingBox.Right - nodes[i].Width, nodes[i].Location.Y),

                    _ => throw new NotImplementedException($"Unsupported node alignment type: {alignmentType}"),
                };

                var locationDelta = newLocation - nodes[i].Location;
                nodes[i].Location = newLocation;

                if(Undo != null)
                    undoActions.Add(new MoveNodesAction(Context, new[] { nodes[i].ID }, locationDelta));
            }

            MarkAsEdited(false);
            Undo?.AddAction(new MultiUndoAction(undoActions, $"Align nodes ({alignmentType})"));
        }

        /// <summary>
        /// Distribute the given nodes as equally as possible inside the bounding box, if no fit can be done it will use a default pad of 10 pixels between nodes.
        /// </summary>
        /// <param name="nodes">List of nodes.</param>
        /// <param name="vertically">If false will be done horizontally, if true will be done vertically.</param>
        public void DistributeNodes(List<SurfaceNode> nodes, bool vertically)
        {
            if(nodes.Count <= 1)
                return;

            var undoActions = new List<MoveNodesAction>();
            var boundingBox = GetNodesBounds(nodes);
            float padding = 10;
            float totalSize = 0;
            for (int i = 0; i < nodes.Count; i++)
            {
                if (vertically)
                {
                    totalSize += nodes[i].Height;
                }
                else
                {
                    totalSize += nodes[i].Width;
                }
            }

            if(vertically)
            {
                nodes.Sort((leftValue, rightValue) => { return leftValue.Y.CompareTo(rightValue.Y); });

                float position = boundingBox.Top; 
                if(totalSize < boundingBox.Height)
                {
                    padding = (boundingBox.Height - totalSize) / nodes.Count;
                }

                for(int i = 0; i < nodes.Count; i++)
                {
                    var newLocation = new Float2(nodes[i].X, position);
                    var locationDelta = newLocation - nodes[i].Location;
                    nodes[i].Location = newLocation;

                    position += nodes[i].Height + padding;

                    if (Undo != null)
                        undoActions.Add(new MoveNodesAction(Context, new[] { nodes[i].ID }, locationDelta));
                }
            }
            else
            {
                nodes.Sort((leftValue, rightValue) => { return leftValue.X.CompareTo(rightValue.X); });

                float position = boundingBox.Left; 
                if(totalSize < boundingBox.Width)
                {
                    padding = (boundingBox.Width - totalSize) / nodes.Count;
                }

                for(int i = 0; i < nodes.Count; i++)
                {
                    var newLocation = new Float2(position, nodes[i].Y);
                    var locationDelta = newLocation - nodes[i].Location;
                    nodes[i].Location = newLocation;

                    position += nodes[i].Width + padding;

                    if (Undo != null)
                        undoActions.Add(new MoveNodesAction(Context, new[] { nodes[i].ID }, locationDelta));
                }
            }

            MarkAsEdited(false);
            Undo?.AddAction(new MultiUndoAction(undoActions, vertically ? "Distribute nodes vertically" : "Distribute nodes horizontally"));
        }
    }
}
