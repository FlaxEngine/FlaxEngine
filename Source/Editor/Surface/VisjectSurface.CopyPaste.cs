// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.Surface.Elements;
using FlaxEditor.Surface.Undo;
using FlaxEngine;
using FlaxEngine.Utilities;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

// ReSharper disable ClassNeverInstantiated.Local
#pragma warning disable 649

namespace FlaxEditor.Surface
{
    public partial class VisjectSurface
    {
        private struct DataModelValue
        {
            public string EnumTypeName;
            public object Value;
        }

        private class DataModelBox
        {
            public int ID;
            public uint[] NodeIDs;
            public int[] BoxIDs;
        }

        private class DataModelNode
        {
            public ushort GroupID;
            public ushort TypeID;
            public uint ID;
            public float X;
            public float Y;
            public DataModelValue[] Values;
            public DataModelBox[] Boxes;
        }

        private class DataModelComment
        {
            public string Title;
            public Color Color;
            public Rectangle Bounds;
        }

        private class DataModel
        {
            public DataModelNode[] Nodes;
            public DataModelComment[] Comments;
        }

        /// <summary>
        /// Copies the selected items.
        /// </summary>
        public void Copy()
        {
            var selection = SelectedControls;
            if (selection.Count == 0)
            {
                Clipboard.Text = string.Empty;
                return;
            }

            // Collect sealed nodes to be copied as well
            foreach (var control in selection.ToArray())
            {
                if (control is SurfaceNode node)
                {
                    var sealedNodes = node.SealedNodes;
                    if (sealedNodes != null)
                    {
                        foreach (var sealedNode in sealedNodes)
                        {
                            if (sealedNode != null && !selection.Contains(sealedNode))
                            {
                                selection.Add(sealedNode);
                            }
                        }
                    }
                }
            }

            var dataModel = new DataModel();
            var dataModelNodes = new List<DataModelNode>(selection.Count);
            var dataModelComments = new List<DataModelComment>();
            var dataModelBoxes = new List<DataModelBox>(32);

            for (int i = 0; i < selection.Count; i++)
            {
                var node = selection[i] as SurfaceNode;
                if (node == null)
                    continue;

                var dataModelNode = new DataModelNode
                {
                    GroupID = node.GroupArchetype.GroupID,
                    TypeID = node.Archetype.TypeID,
                    ID = node.ID,
                    X = node.Location.X,
                    Y = node.Location.Y,
                };
                if (node.Values != null)
                {
                    dataModelNode.Values = new DataModelValue[node.Values.Length];
                    for (int j = 0; j < node.Values.Length; j++)
                    {
                        var value = new DataModelValue
                        {
                            Value = node.Values[j],
                        };
                        if (value.Value != null && value.Value.GetType().IsEnum)
                            value.EnumTypeName = value.Value.GetType().FullName;
                        dataModelNode.Values[j] = value;
                    }
                }

                if (node.Elements != null && node.Elements.Count > 0)
                {
                    for (int j = 0; j < node.Elements.Count; j++)
                    {
                        if (node.Elements[j] is Box box && box.Connections.Count > 0)
                        {
                            var dataModelBox = new DataModelBox
                            {
                                ID = box.ID,
                                NodeIDs = new uint[box.Connections.Count],
                                BoxIDs = new int[box.Connections.Count],
                            };

                            for (int k = 0; k < box.Connections.Count; k++)
                            {
                                var target = box.Connections[k];
                                dataModelBox.NodeIDs[k] = target.ParentNode.ID;
                            }

                            for (int k = 0; k < box.Connections.Count; k++)
                            {
                                var target = box.Connections[k];
                                dataModelBox.BoxIDs[k] = target.ID;
                            }

                            dataModelBoxes.Add(dataModelBox);
                        }
                    }

                    if (dataModelBoxes.Count > 0)
                    {
                        dataModelNode.Boxes = dataModelBoxes.ToArray();
                        dataModelBoxes.Clear();
                    }
                }

                dataModelNodes.Add(dataModelNode);
            }

            dataModel.Nodes = dataModelNodes.ToArray();

            for (int i = 0; i < selection.Count; i++)
            {
                var comment = selection[i] as SurfaceComment;
                if (comment == null)
                    continue;

                var dataModelComment = new DataModelComment
                {
                    Title = comment.Title,
                    Color = comment.Color,
                    Bounds = comment.Bounds,
                };

                dataModelComments.Add(dataModelComment);
            }

            dataModel.Comments = dataModelComments.ToArray();

            Clipboard.Text = FlaxEngine.Json.JsonSerializer.Serialize(dataModel);
        }

        /// <summary>
        /// Checks if can paste the nodes data from the clipboard.
        /// </summary>
        /// <returns>True if can paste data, otherwise false.</returns>
        public bool CanPaste()
        {
            var data = Clipboard.Text;
            if (data == null || data.Length < 2)
                return false;

            return true;
        }

        /// <summary>
        /// Pastes the copied items.
        /// </summary>
        public void Paste()
        {
            if (!CanEdit)
                return;
            var data = Clipboard.Text;
            if (data == null || data.Length < 2)
                return;

            try
            {
                // Load Mr Json
                DataModel model;
                try
                {
                    model = FlaxEngine.Json.JsonSerializer.Deserialize<DataModel>(data);
                }
                catch
                {
                    return;
                }
                if (model.Nodes == null)
                    model.Nodes = new DataModelNode[0];

                // Build the nodes IDs mapping (need to generate new IDs for the pasted nodes and preserve the internal connections)
                var idsMapping = new Dictionary<uint, uint>();
                for (int i = 0; i < model.Nodes.Length; i++)
                {
                    uint result = 1;
                    while (true)
                    {
                        bool valid = true;
                        if (idsMapping.ContainsValue(result))
                        {
                            result++;
                            valid = false;
                        }
                        else
                        {
                            for (int j = 0; j < Nodes.Count; j++)
                            {
                                if (Nodes[j].ID == result)
                                {
                                    result++;
                                    valid = false;
                                    break;
                                }
                            }
                        }

                        if (valid)
                            break;
                    }

                    idsMapping.Add(model.Nodes[i].ID, result);
                }

                // Find controls upper left location
                var upperLeft = new Float2(model.Nodes[0].X, model.Nodes[0].Y);
                for (int i = 1; i < model.Nodes.Length; i++)
                {
                    upperLeft.X = Mathf.Min(upperLeft.X, model.Nodes[i].X);
                    upperLeft.Y = Mathf.Min(upperLeft.Y, model.Nodes[i].Y);
                }

                // Create nodes
                var nodes = new Dictionary<uint, SurfaceNode>();
                var nodesData = new Dictionary<uint, DataModelNode>();
                for (int i = 0; i < model.Nodes.Length; i++)
                {
                    var nodeData = model.Nodes[i];

                    // Peek type
                    if (!NodeFactory.GetArchetype(NodeArchetypes, nodeData.GroupID, nodeData.TypeID, out var groupArchetype, out var nodeArchetype))
                        throw new InvalidOperationException("Unknown node type.");

                    // Validate given node type
                    if (!CanUseNodeType(groupArchetype, nodeArchetype))
                        continue;

                    // Create
                    var node = NodeFactory.CreateNode(idsMapping[nodeData.ID], Context, groupArchetype, nodeArchetype);
                    if (node == null)
                        throw new InvalidOperationException("Failed to create node.");
                    Nodes.Add(node);
                    nodes.Add(nodeData.ID, node);
                    nodesData.Add(nodeData.ID, nodeData);

                    // Initialize
                    if (nodeData.Values != null && node.Values.Length > 0)
                    {
                        var nodeValues = (object[])node.Values?.Clone();
                        if (nodeValues != null && nodeValues.Length == nodeData.Values.Length)
                        {
                            // Copy and fix values (Json deserializes may output them in a different format)
                            for (int l = 0; l < nodeData.Values.Length; l++)
                            {
                                var src = nodeData.Values[l].Value;
                                var dst = nodeValues[l];

                                try
                                {
                                    if (nodeData.Values[l].EnumTypeName != null)
                                    {
                                        var enumType = TypeUtils.GetManagedType(nodeData.Values[l].EnumTypeName);
                                        if (enumType != null)
                                        {
                                            src = Enum.ToObject(enumType, src);
                                        }
                                    }
                                    else if (src is JToken token)
                                    {
                                        if (dst is Vector2)
                                        {
                                            src = new Vector2(token["X"].Value<float>(),
                                                              token["Y"].Value<float>());
                                        }
                                        else if (dst is Vector3)
                                        {
                                            src = new Vector3(token["X"].Value<float>(),
                                                              token["Y"].Value<float>(),
                                                              token["Z"].Value<float>());
                                        }
                                        else if (dst is Vector4)
                                        {
                                            src = new Vector4(token["X"].Value<float>(),
                                                              token["Y"].Value<float>(),
                                                              token["Z"].Value<float>(),
                                                              token["W"].Value<float>());
                                        }
                                        else if (dst is Color)
                                        {
                                            src = new Color(token["R"].Value<float>(),
                                                            token["G"].Value<float>(),
                                                            token["B"].Value<float>(),
                                                            token["A"].Value<float>());
                                        }
                                        else
                                        {
                                            Editor.LogWarning("Unknown pasted node value token: " + token);
                                            src = dst;
                                        }
                                    }
                                    else if (src is double asDouble)
                                    {
                                        src = (float)asDouble;
                                    }
                                    else if (dst is Guid)
                                    {
                                        src = Guid.Parse((string)src);
                                    }
                                    else if (dst is int)
                                    {
                                        src = Convert.ToInt32(src);
                                    }
                                    else if (dst is float)
                                    {
                                        src = Convert.ToSingle(src);
                                    }
                                    else if (dst is byte[] && src is string s)
                                    {
                                        src = Convert.FromBase64String(s);
                                    }
                                }
                                catch (Exception ex)
                                {
                                    Editor.LogWarning(string.Format("Failed to convert node data from {0} to {1}", src?.GetType().FullName ?? "null", dst?.GetType().FullName ?? "null"));
                                    Editor.LogWarning(ex);
                                }

                                nodeValues[l] = src;
                            }
                        }
                        else if (node.Archetype.Flags.HasFlag(NodeFlags.VariableValuesSize))
                        {
                            // Copy values
                            nodeValues = new object[nodeData.Values.Length];
                            for (int l = 0; l < nodeData.Values.Length; l++)
                            {
                                nodeValues[l] = nodeData.Values[l].Value;
                            }
                        }
                        else
                        {
                            Editor.LogWarning("Invalid node custom values.");
                        }
                        if (nodeValues != null)
                            node.SetValuesPaste(nodeValues);
                    }

                    Context.OnControlLoaded(node, SurfaceNodeActions.Paste);
                }

                // Setup connections
                foreach (var e in nodes)
                {
                    var node = e.Value;
                    var nodeData = nodesData[e.Key];
                    if (nodeData.Boxes != null)
                    {
                        foreach (var boxData in nodeData.Boxes)
                        {
                            var box = node.GetBox(boxData.ID);
                            if (box == null || boxData.BoxIDs == null || boxData.NodeIDs == null || boxData.BoxIDs.Length != boxData.NodeIDs.Length)
                                continue;

                            for (int i = 0; i < boxData.NodeIDs.Length; i++)
                            {
                                if (nodes.TryGetValue(boxData.NodeIDs[i], out var targetNode)
                                    && targetNode.TryGetBox(boxData.BoxIDs[i], out var targetBox))
                                {
                                    box.Connections.Add(targetBox);
                                }
                            }
                        }
                    }
                }

                // Arrange controls
                foreach (var e in nodes)
                {
                    var node = e.Value;
                    var nodeData = nodesData[e.Key];
                    var pos = new Float2(nodeData.X, nodeData.Y) - upperLeft;
                    node.Location = ViewPosition + pos + _mousePos / ViewScale;
                }

                // Post load
                foreach (var node in nodes)
                {
                    node.Value.OnSurfaceLoaded(SurfaceNodeActions.Paste);
                }
                foreach (var node in nodes)
                {
                    node.Value.OnSpawned(SurfaceNodeActions.Paste);
                }
                foreach (var node in nodes)
                {
                    node.Value.OnPasted(idsMapping);
                }

                // Add undo action
                if (Undo != null && nodes.Count > 0)
                {
                    var actions = new List<IUndoAction>();
                    foreach (var node in nodes)
                    {
                        var action = new AddRemoveNodeAction(node.Value, true);
                        actions.Add(action);
                    }
                    foreach (var node in nodes)
                    {
                        var action = new EditNodeConnections(Context, node.Value);
                        action.End();
                        actions.Add(action);
                    }
                    Undo.AddAction(new MultiUndoAction(actions, nodes.Count == 1 ? "Paste node" : "Paste nodes"));
                }

                // Select those nodes
                Select(nodes.Values);

                if (nodes.Count > 0)
                    MarkAsEdited();
            }
            catch (Exception ex)
            {
                Editor.LogWarning("Failed to paste Visject Surface nodes");
                Editor.LogWarning(ex);
                MessageBox.Show("Failed to paste Visject Surface nodes. " + ex.Message, "Paste failed", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        /// <summary>
        /// Cuts the selected items.
        /// </summary>
        public void Cut()
        {
            Copy();
            Delete();
        }

        /// <summary>
        /// Duplicates the selected items.
        /// </summary>
        public void Duplicate()
        {
            Copy();
            Paste();
        }
    }
}
