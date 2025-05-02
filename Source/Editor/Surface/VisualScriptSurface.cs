// Copyright (c) Wojciech Figat. All rights reserved.

//#define DEBUG_INVOKE_METHODS_SEARCHING
//#define DEBUG_FIELDS_SEARCHING
//#define DEBUG_EVENTS_SEARCHING

using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using FlaxEditor.Content;
using FlaxEditor.GUI.Drag;
using FlaxEditor.SceneGraph;
using FlaxEditor.Scripting;
using FlaxEditor.Surface.ContextMenu;
using FlaxEditor.Surface.Elements;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;

namespace FlaxEditor.Surface
{
    /// <summary>
    /// The Visject Surface implementation for the Visual Script editor.
    /// </summary>
    /// <seealso cref="FlaxEditor.Surface.VisjectSurface" />
    [HideInEditor]
    public class VisualScriptSurface : VisjectSurface
    {
        private readonly GroupArchetype _methodOverridesGroupArchetype = new GroupArchetype
        {
            GroupID = 16,
            Name = "Method Overrides",
            Color = new Color(109, 160, 24),
            Archetypes = new List<NodeArchetype>(),
        };

        private static readonly string[] _blacklistedTypeNames =
        {
            "Newtonsoft.Json.",
            "System.Array",
            "System.Linq.Expressions.",
            "System.Reflection.",
        };

        private static NodesCache _nodesCache = new NodesCache(IterateNodesCache);
        private DragActors _dragActors;

        /// <summary>
        /// The script that is edited by the surface.
        /// </summary>
        public VisualScript Script;

        /// <summary>
        /// The list of nodes with breakpoints set.
        /// </summary>
        public readonly List<SurfaceNode> Breakpoints = new List<SurfaceNode>();

        /// <inheritdoc />
        public VisualScriptSurface(IVisjectSurfaceOwner owner, Action onSave, FlaxEditor.Undo undo)
        : base(owner, onSave, undo, null, null, true)
        {
            _supportsImplicitCastFromObjectToBoolean = true;
            DragHandlers.Add(_dragActors = new DragActors(ValidateDragActor));
            ScriptsBuilder.ScriptsReloadBegin += OnScriptsReloadBegin;
        }

        private void OnScriptsReloadBegin()
        {
            _nodesCache.Clear();
        }

        private bool ValidateDragActor(ActorNode actor)
        {
            return true;
        }

        private string GetBoxDebuggerTooltip(ref Editor.VisualScriptLocal local)
        {
            if (string.IsNullOrEmpty(local.ValueTypeName))
            {
                if (string.IsNullOrEmpty(local.Value))
                    return string.Empty;
                return local.Value;
            }
            if (string.IsNullOrEmpty(local.Value))
                return $"({local.ValueTypeName})";
            return $"{local.Value}\n({local.ValueTypeName})";
        }

        /// <inheritdoc />
        public override void OnNodeBreakpointEdited(SurfaceNode node)
        {
            base.OnNodeBreakpointEdited(node);

            if (node.Breakpoint.Set && node.Breakpoint.Enabled)
                Breakpoints.Add(node);
            else
                Breakpoints.Remove(node);
        }

        /// <inheritdoc />
        public override string GetTypeName(ScriptType type)
        {
            if (type.Type == typeof(void))
                return "Impulse";
            return base.GetTypeName(type);
        }

        /// <inheritdoc />
        public override bool GetBoxDebuggerTooltip(Box box, out string text)
        {
            if (Editor.Instance.Simulation.IsDuringBreakpointHang)
            {
                // Find any local variable from the current scope that matches this box
                var state = Windows.Assets.VisualScriptWindow.GetLocals();
                if (state.Locals != null)
                {
                    for (int i = 0; i < state.Locals.Length; i++)
                    {
                        ref var local = ref state.Locals[i];
                        if (local.BoxId == box.ID && local.NodeId == box.ParentNode.ID)
                        {
                            text = GetBoxDebuggerTooltip(ref local);
                            return true;
                        }
                    }
                    if (box.HasSingleConnection)
                    {
                        var connectedBox = box.Connections[0];
                        for (int i = 0; i < state.Locals.Length; i++)
                        {
                            ref var local = ref state.Locals[i];
                            if (local.BoxId == connectedBox.ID && local.NodeId == connectedBox.ParentNode.ID)
                            {
                                text = GetBoxDebuggerTooltip(ref local);
                                return true;
                            }
                        }
                    }
                }

                // Evaluate the value using the Visual Scripting backend
                if (box.CurrentType != typeof(void))
                {
                    var local = new Editor.VisualScriptLocal
                    {
                        NodeId = box.ParentNode.ID,
                        BoxId = box.ID,
                    };
                    var script = ((Windows.Assets.VisualScriptWindow)box.Surface.Owner).Asset;
                    if (Editor.EvaluateVisualScriptLocal(script, ref local))
                    {
                        // Check if got no value (null)
                        if (string.IsNullOrEmpty(local.ValueTypeName) && string.Equals(local.Value, "null", StringComparison.Ordinal))
                        {
                            var connections = box.Connections;
                            if (connections.Count == 0 && box.Archetype.ValueIndex >= 0 && box.ParentNode.Values != null && box.Archetype.ValueIndex < box.ParentNode.Values.Length)
                            {
                                // Special case when there is no value but the box has no connection and uses default value
                                var defaultValue = box.ParentNode.Values[box.Archetype.ValueIndex];
                                if (defaultValue != null)
                                {
                                    local.Value = defaultValue.ToString();
                                    local.ValueTypeName = defaultValue.GetType().FullName;
                                }
                            }
                            else if (connections.Count == 1)
                            {
                                // Special case when there is no value but the box has a connection with valid value to try to use it instead
                                box = connections[0];
                                local.NodeId = box.ParentNode.ID;
                                local.BoxId = box.ID;
                                Editor.EvaluateVisualScriptLocal(script, ref local);
                            }
                        }

                        text = GetBoxDebuggerTooltip(ref local);
                        return true;
                    }
                }
            }
            text = null;
            return false;
        }

        /// <inheritdoc />
        public override bool CanSetParameters => true;

        /// <inheritdoc />
        public override bool UseContextMenuDescriptionPanel => true;

        /// <inheritdoc />
        public override bool CanUseNodeType(GroupArchetype groupArchetype, NodeArchetype nodeArchetype)
        {
            return (nodeArchetype.Flags & NodeFlags.VisualScriptGraph) != 0 && base.CanUseNodeType(groupArchetype, nodeArchetype);
        }

        /// <inheritdoc />
        protected internal override NodeArchetype GetParameterGetterNodeArchetype(out ushort groupId)
        {
            groupId = 6;
            return Archetypes.Parameters.Nodes[2];
        }

        /// <inheritdoc />
        protected override bool TryGetParameterSetterNodeArchetype(out ushort groupId, out NodeArchetype archetype)
        {
            groupId = 6;
            archetype = Archetypes.Parameters.Nodes[3];
            return true;
        }

        /// <inheritdoc />
        protected override void OnShowPrimaryMenu(VisjectCM activeCM, Float2 location, Box startBox)
        {
            // Update nodes for method overrides
            Profiler.BeginEvent("Overrides");
            activeCM.RemoveGroup(_methodOverridesGroupArchetype);
            if (Script && !Script.WaitForLoaded(100))
            {
                var nodes = (List<NodeArchetype>)_methodOverridesGroupArchetype.Archetypes;
                nodes.Clear();

                // Find methods to override from the inherited types
                var scriptMeta = Script.Meta;
                var baseType = TypeUtils.GetType(scriptMeta.BaseTypename);
                var members = baseType.GetMembers(BindingFlags.Public | BindingFlags.Instance);
                for (var i = 0; i < members.Length; i++)
                {
                    ref var member = ref members[i];
                    if (SurfaceUtils.IsValidVisualScriptOverrideMethod(member, out var parameters))
                    {
                        var name = member.Name;

                        // Skip already added override method nodes
                        bool isAlreadyAdded = false;
                        foreach (var n in Nodes)
                        {
                            if (n.GroupArchetype.GroupID == 16 &&
                                n.Archetype.TypeID == 3 &&
                                (string)n.Values[0] == name &&
                                (int)n.Values[1] == parameters.Length)
                            {
                                isAlreadyAdded = true;
                                break;
                            }
                        }
                        if (isAlreadyAdded)
                            continue;

                        var node = (NodeArchetype)Archetypes.Function.Nodes[2].Clone();
                        node.Flags &= ~NodeFlags.NoSpawnViaGUI;
                        node.Signature = SurfaceUtils.GetVisualScriptMemberInfoSignature(member);
                        node.Description = SurfaceUtils.GetVisualScriptMemberShortDescription(member);
                        node.DefaultValues[0] = name;
                        node.DefaultValues[1] = parameters.Length;
                        node.Title = "Override " + name;
                        node.Tag = member;
                        nodes.Add(node);
                    }
                }

                activeCM.AddGroup(_methodOverridesGroupArchetype, false);
            }
            Profiler.EndEvent();

            // Update nodes for invoke methods (async)
            _nodesCache.Get(activeCM);

            base.OnShowPrimaryMenu(activeCM, location, startBox);

            activeCM.VisibleChanged += OnActiveContextMenuVisibleChanged;
        }

        private void OnActiveContextMenuVisibleChanged(Control activeCM)
        {
            _nodesCache.Wait();
        }

        private static void IterateNodesCache(ScriptType scriptType, Dictionary<KeyValuePair<string, ushort>, GroupArchetype> cache, int version)
        {
            // Skip Newtonsoft.Json stuff
            var scriptTypeTypeName = scriptType.TypeName;
            for (var i = 0; i < _blacklistedTypeNames.Length; i++)
            {
                if (scriptTypeTypeName.StartsWith(_blacklistedTypeNames[i]))
                    return;
            }
            var scriptTypeName = scriptType.Name;

            // Enum
            if (scriptType.IsEnum)
            {
                // Create node archetype
                var node = (NodeArchetype)Archetypes.Constants.Nodes[10].Clone();
                node.DefaultValues[0] = Activator.CreateInstance(scriptType.Type);
                node.Flags &= ~NodeFlags.NoSpawnViaGUI;
                node.Title = scriptTypeName;
                node.Signature = scriptTypeName;
                node.Description = Editor.Instance.CodeDocs.GetTooltip(scriptType);

                // Create group archetype
                var groupKey = new KeyValuePair<string, ushort>(scriptTypeName, 2);
                if (!cache.TryGetValue(groupKey, out var group))
                {
                    group = new GroupArchetype
                    {
                        GroupID = groupKey.Value,
                        Name = groupKey.Key,
                        Color = new Color(243, 156, 18),
                        Tag = version,
                        Archetypes = new List<NodeArchetype>(),
                    };
                    cache.Add(groupKey, group);
                }

                // Add node to the group
                ((IList<NodeArchetype>)group.Archetypes).Add(node);
                return;
            }

            // Structure
            if (scriptType.IsValueType)
            {
                if (scriptType.IsVoid)
                    return;

                // Create group archetype
                var groupKey = new KeyValuePair<string, ushort>(scriptTypeName, 4);
                if (!cache.TryGetValue(groupKey, out var group))
                {
                    group = new GroupArchetype
                    {
                        GroupID = groupKey.Value,
                        Name = groupKey.Key,
                        Color = new Color(155, 89, 182),
                        Tag = version,
                        Archetypes = new List<NodeArchetype>(),
                    };
                    cache.Add(groupKey, group);
                }

                var tooltip = Editor.Instance.CodeDocs.GetTooltip(scriptType);

                // Create Pack node archetype
                var node = (NodeArchetype)Archetypes.Packing.Nodes[6].Clone();
                node.DefaultValues[0] = scriptTypeTypeName;
                node.Flags &= ~NodeFlags.NoSpawnViaGUI;
                node.Title = "Pack " + scriptTypeName;
                node.Signature = "Pack " + scriptTypeName;
                node.Description = tooltip;
                ((IList<NodeArchetype>)group.Archetypes).Add(node);

                // Create Unpack node archetype
                node = (NodeArchetype)Archetypes.Packing.Nodes[13].Clone();
                node.DefaultValues[0] = scriptTypeTypeName;
                node.Flags &= ~NodeFlags.NoSpawnViaGUI;
                node.Title = "Unpack " + scriptTypeName;
                node.Signature = "Unpack " + scriptTypeName;
                node.Description = tooltip;
                ((IList<NodeArchetype>)group.Archetypes).Add(node);
            }

            foreach (var member in scriptType.GetMembers(BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static | BindingFlags.DeclaredOnly))
            {
                if (member.IsGeneric)
                    continue;

                if (member.IsMethod)
                {
                    // Skip methods not declared in this type
                    if (member.Type is MethodInfo m && m.GetBaseDefinition().DeclaringType != m.DeclaringType)
                        continue;
                    var name = member.Name;
                    if (name == "ToString")
                        continue;

                    // Skip if searching by name doesn't return a match
                    var members = scriptType.GetMembers(name, MemberTypes.Method, BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static | BindingFlags.DeclaredOnly);
                    if (!members.Contains(member))
                        continue;

                    // Check if method is valid for Visual Script usage
                    if (SurfaceUtils.IsValidVisualScriptInvokeMethod(member, out var parameters))
                    {
                        // Create node archetype
                        var node = (NodeArchetype)Archetypes.Function.Nodes[3].Clone();
                        node.DefaultValues[0] = scriptTypeTypeName;
                        node.DefaultValues[1] = name;
                        node.DefaultValues[2] = parameters.Length;
                        node.Flags &= ~NodeFlags.NoSpawnViaGUI;
                        node.Title = SurfaceUtils.GetMethodDisplayName((string)node.DefaultValues[1]);
                        node.Signature = SurfaceUtils.GetVisualScriptMemberInfoSignature(member);
                        node.Description = SurfaceUtils.GetVisualScriptMemberShortDescription(member);
                        node.SubTitle = string.Format(" (in {0})", scriptTypeName);
                        node.Tag = member;

                        // Create group archetype
                        var groupKey = new KeyValuePair<string, ushort>(scriptTypeName, 16);
                        if (!cache.TryGetValue(groupKey, out var group))
                        {
                            group = new GroupArchetype
                            {
                                GroupID = groupKey.Value,
                                Name = groupKey.Key,
                                Color = new Color(109, 160, 24),
                                Tag = version,
                                Archetypes = new List<NodeArchetype>(),
                            };
                            cache.Add(groupKey, group);
                        }

                        // Add node to the group
                        ((IList<NodeArchetype>)group.Archetypes).Add(node);
#if DEBUG_INVOKE_METHODS_SEARCHING
                        Editor.LogWarning(scriptTypeTypeName + " -> " + member.GetSignature());
#endif
                    }
                }
                else if (member.IsField)
                {
                    var name = member.Name;

                    // Skip if searching by name doesn't return a match
                    var members = scriptType.GetMembers(name, MemberTypes.Field, BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static | BindingFlags.DeclaredOnly);
                    if (!members.Contains(member))
                        continue;

                    // Check if field is valid for Visual Script usage
                    if (SurfaceUtils.IsValidVisualScriptField(member))
                    {
                        if (member.HasGet)
                        {
                            // Create node archetype
                            var node = (NodeArchetype)Archetypes.Function.Nodes[6].Clone();
                            node.DefaultValues[0] = scriptTypeTypeName;
                            node.DefaultValues[1] = name;
                            node.DefaultValues[2] = member.ValueType.TypeName;
                            node.DefaultValues[3] = member.IsStatic;
                            node.Flags &= ~NodeFlags.NoSpawnViaGUI;
                            node.Title = "Get " + name;
                            node.Signature = SurfaceUtils.GetVisualScriptMemberInfoSignature(member);
                            node.Description = SurfaceUtils.GetVisualScriptMemberShortDescription(member);
                            node.SubTitle = string.Format(" (in {0})", scriptTypeName);
                            node.Tag = member;

                            // Create group archetype
                            var groupKey = new KeyValuePair<string, ushort>(scriptTypeName, 16);
                            if (!cache.TryGetValue(groupKey, out var group))
                            {
                                group = new GroupArchetype
                                {
                                    GroupID = groupKey.Value,
                                    Name = groupKey.Key,
                                    Color = new Color(109, 160, 24),
                                    Tag = version,
                                    Archetypes = new List<NodeArchetype>(),
                                };
                                cache.Add(groupKey, group);
                            }

                            // Add node to the group
                            ((IList<NodeArchetype>)group.Archetypes).Add(node);
#if DEBUG_FIELDS_SEARCHING
                            Editor.LogWarning(scriptTypeTypeName + " -> Get " + member.GetSignature());
#endif
                        }
                        if (member.HasSet)
                        {
                            // Create node archetype
                            var node = (NodeArchetype)Archetypes.Function.Nodes[7].Clone();
                            node.DefaultValues[0] = scriptTypeTypeName;
                            node.DefaultValues[1] = name;
                            node.DefaultValues[2] = member.ValueType.TypeName;
                            node.DefaultValues[3] = member.IsStatic;
                            node.Flags &= ~NodeFlags.NoSpawnViaGUI;
                            node.Title = "Set " + name;
                            node.Signature = SurfaceUtils.GetVisualScriptMemberInfoSignature(member);
                            node.Description = SurfaceUtils.GetVisualScriptMemberShortDescription(member);
                            node.SubTitle = string.Format(" (in {0})", scriptTypeName);
                            node.Tag = member;

                            // Create group archetype
                            var groupKey = new KeyValuePair<string, ushort>(scriptTypeName, 16);
                            if (!cache.TryGetValue(groupKey, out var group))
                            {
                                group = new GroupArchetype
                                {
                                    GroupID = groupKey.Value,
                                    Name = groupKey.Key,
                                    Color = new Color(109, 160, 24),
                                    Tag = version,
                                    Archetypes = new List<NodeArchetype>(),
                                };
                                cache.Add(groupKey, group);
                            }

                            // Add node to the group
                            ((IList<NodeArchetype>)group.Archetypes).Add(node);
#if DEBUG_FIELDS_SEARCHING
                            Editor.LogWarning(scriptTypeTypeName + " -> Set " + member.GetSignature());
#endif
                        }
                    }
                }
                else if (member.IsEvent)
                {
                    var name = member.Name;

                    // Skip if searching by name doesn't return a match
                    var members = scriptType.GetMembers(name, MemberTypes.Event, BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static | BindingFlags.DeclaredOnly);
                    if (!members.Contains(member))
                        continue;

                    // Check if field is valid for Visual Script usage
                    if (SurfaceUtils.IsValidVisualScriptEvent(member))
                    {
                        var groupKey = new KeyValuePair<string, ushort>(scriptTypeName, 16);
                        if (!cache.TryGetValue(groupKey, out var group))
                        {
                            group = new GroupArchetype
                            {
                                GroupID = groupKey.Value,
                                Name = groupKey.Key,
                                Color = new Color(109, 160, 24),
                                Tag = version,
                                Archetypes = new List<NodeArchetype>(),
                            };
                            cache.Add(groupKey, group);
                        }

                        // Add Bind event node
                        var bindNode = (NodeArchetype)Archetypes.Function.Nodes[8].Clone();
                        bindNode.DefaultValues[0] = scriptTypeTypeName;
                        bindNode.DefaultValues[1] = name;
                        bindNode.Flags &= ~NodeFlags.NoSpawnViaGUI;
                        bindNode.Title = "Bind " + name;
                        bindNode.Signature = SurfaceUtils.GetVisualScriptMemberInfoSignature(member);
                        bindNode.Description = SurfaceUtils.GetVisualScriptMemberShortDescription(member);
                        bindNode.SubTitle = string.Format(" (in {0})", scriptTypeName);
                        bindNode.Tag = member;
                        ((IList<NodeArchetype>)group.Archetypes).Add(bindNode);

                        // Add Unbind event node
                        var unbindNode = (NodeArchetype)Archetypes.Function.Nodes[9].Clone();
                        unbindNode.DefaultValues[0] = scriptTypeTypeName;
                        unbindNode.DefaultValues[1] = name;
                        unbindNode.Flags &= ~NodeFlags.NoSpawnViaGUI;
                        unbindNode.Title = "Unbind " + name;
                        unbindNode.Signature = bindNode.Signature;
                        unbindNode.Description = bindNode.Description;
                        unbindNode.SubTitle = bindNode.SubTitle;
                        unbindNode.Tag = member;
                        ((IList<NodeArchetype>)group.Archetypes).Add(unbindNode);

#if DEBUG_EVENTS_SEARCHING
                        Editor.LogWarning(scriptTypeTypeName + " -> " + member.GetSignature());
#endif
                    }
                }
            }
        }

        /// <inheritdoc />
        protected override bool ValidateDragItem(AssetItem assetItem)
        {
            if (assetItem.IsOfType<GameplayGlobals>())
                return true;
            if (assetItem.IsOfType<Asset>())
                return true;
            return base.ValidateDragItem(assetItem);
        }

        /// <inheritdoc />
        protected override void HandleDragDropAssets(List<AssetItem> objects, DragDropEventArgs args)
        {
            for (int i = 0; i < objects.Count; i++)
            {
                var assetItem = objects[i];
                SurfaceNode node = null;

                if (assetItem.IsOfType<GameplayGlobals>())
                {
                    node = Context.SpawnNode(7, 16, args.SurfaceLocation, new object[]
                    {
                        assetItem.ID,
                        string.Empty,
                    });
                }
                else if (assetItem.IsOfType<Asset>())
                {
                    node = Context.SpawnNode(7, 18, args.SurfaceLocation, new object[]
                    {
                        assetItem.ID,
                    });
                }

                if (node != null)
                {
                    args.SurfaceLocation.X += node.Width + 10;
                }
            }

            base.HandleDragDropAssets(objects, args);
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragDrop(ref Float2 location, DragData data)
        {
            var args = new DragDropEventArgs
            {
                SurfaceLocation = _rootControl.PointFromParent(ref location)
            };

            // Drag actors
            if (_dragActors.HasValidDrag)
            {
                for (int i = 0; i < _dragActors.Objects.Count; i++)
                {
                    var actorNode = _dragActors.Objects[i];
                    var node = Context.SpawnNode(7, 21, args.SurfaceLocation, new object[]
                    {
                        actorNode.ID
                    });
                    args.SurfaceLocation.X += node.Width + 10;
                }
                DragHandlers.OnDragDrop(args);
                return _dragActors.Effect;
            }

            return base.OnDragDrop(ref location, data);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (IsDisposing)
                return;
            ScriptsBuilder.ScriptsReloadBegin -= OnScriptsReloadBegin;
            _nodesCache.Wait();

            base.OnDestroy();
        }
    }
}
