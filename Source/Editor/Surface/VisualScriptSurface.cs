// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

//#define DEBUG_INVOKE_METHODS_SEARCHING
//#define DEBUG_FIELDS_SEARCHING
//#define DEBUG_EVENTS_SEARCHING

#if DEBUG_INVOKE_METHODS_SEARCHING || DEBUG_FIELDS_SEARCHING || DEBUG_EVENTS_SEARCHING
#define DEBUG_SEARCH_TIME
#endif

using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Threading.Tasks;
using FlaxEditor.Content;
using FlaxEditor.GUI.Drag;
using FlaxEditor.SceneGraph;
using FlaxEditor.Scripting;
using FlaxEditor.Surface.ContextMenu;
using FlaxEditor.Surface.Elements;
using FlaxEngine;
using FlaxEngine.GUI;
using Object = FlaxEngine.Object;

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

        internal static class NodesCache
        {
            private static readonly object _locker = new object();
            private static int _version;
            private static Task _task;
            private static VisjectCM _taskContextMenu;
            private static Dictionary<KeyValuePair<string, ushort>, GroupArchetype> _cache;

            public static void Wait()
            {
                _task?.Wait();
            }

            public static void Clear()
            {
                Wait();

                if (_cache != null && _cache.Count != 0)
                {
                    OnCodeEditingTypesCleared();
                }
            }

            public static void Get(VisjectCM contextMenu)
            {
                Wait();

                lock (_locker)
                {
                    if (_cache == null)
                        _cache = new Dictionary<KeyValuePair<string, ushort>, GroupArchetype>();
                    contextMenu.LockChildrenRecursive();

                    // Check if has cached groups
                    if (_cache.Count != 0)
                    {
                        // Check if context menu doesn't have the recent cached groups
                        if (!contextMenu.Groups.Any(g => g.Archetype.Tag is int asInt && asInt == _version))
                        {
                            var groups = contextMenu.Groups.Where(g => g.Archetype.Tag is int).ToArray();
                            foreach (var g in groups)
                                contextMenu.RemoveGroup(g);
                            foreach (var g in _cache.Values)
                                contextMenu.AddGroup(g);
                        }
                    }
                    else
                    {
                        // Remove any old groups from context menu
                        var groups = contextMenu.Groups.Where(g => g.Archetype.Tag is int).ToArray();
                        foreach (var g in groups)
                            contextMenu.RemoveGroup(g);

                        // Register for scripting types reload
                        Editor.Instance.CodeEditing.TypesCleared += OnCodeEditingTypesCleared;

                        // Run caching on an async
                        _task = Task.Run(OnActiveContextMenuShowAsync);
                        _taskContextMenu = contextMenu;
                    }

                    contextMenu.UnlockChildrenRecursive();
                }
            }

            private static void OnActiveContextMenuShowAsync()
            {
                Profiler.BeginEvent("Setup Visual Script Context Menu (async)");
#if DEBUG_SEARCH_TIME
                var searchStartTime = DateTime.Now;
                var searchHitsCount = 0;
#endif

                foreach (var scriptType in Editor.Instance.CodeEditing.All.Get())
                {
                    if (!SurfaceUtils.IsValidVisualScriptType(scriptType))
                        continue;

                    // Skip Newtonsoft.Json stuff
                    var scriptTypeTypeName = scriptType.TypeName;
                    if (scriptTypeTypeName.StartsWith("Newtonsoft.Json."))
                        continue;
                    var scriptTypeName = scriptType.Name;

                    // Enum
                    if (scriptType.IsEnum)
                    {
                        // Create node archetype
                        var node = (NodeArchetype)Archetypes.Constants.Nodes[10].Clone();
                        node.DefaultValues[0] = Activator.CreateInstance(scriptType.Type);
                        node.Flags &= ~NodeFlags.NoSpawnViaGUI;
                        node.Title = scriptTypeName;
                        node.Description = scriptTypeTypeName;
                        var attributes = scriptType.GetAttributes(false);
                        var tooltipAttribute = (TooltipAttribute)attributes.FirstOrDefault(x => x is TooltipAttribute);
                        if (tooltipAttribute != null)
                            node.Description += "\n" + tooltipAttribute.Text;

                        // Create group archetype
                        var groupKey = new KeyValuePair<string, ushort>(scriptTypeName, 2);
                        if (!_cache.TryGetValue(groupKey, out var group))
                        {
                            group = new GroupArchetype
                            {
                                GroupID = groupKey.Value,
                                Name = groupKey.Key,
                                Color = new Color(243, 156, 18),
                                Tag = _version,
                                Archetypes = new List<NodeArchetype>(),
                            };
                            _cache.Add(groupKey, group);
                        }

                        // Add node to the group
                        ((IList<NodeArchetype>)group.Archetypes).Add(node);
                        continue;
                    }

                    // Structure
                    if (scriptType.IsValueType)
                    {
                        if (scriptType.IsVoid)
                            continue;

                        // Create group archetype
                        var groupKey = new KeyValuePair<string, ushort>(scriptTypeName, 4);
                        if (!_cache.TryGetValue(groupKey, out var group))
                        {
                            group = new GroupArchetype
                            {
                                GroupID = groupKey.Value,
                                Name = groupKey.Key,
                                Color = new Color(155, 89, 182),
                                Tag = _version,
                                Archetypes = new List<NodeArchetype>(),
                            };
                            _cache.Add(groupKey, group);
                        }

                        var attributes = scriptType.GetAttributes(false);
                        var tooltipAttribute = (TooltipAttribute)attributes.FirstOrDefault(x => x is TooltipAttribute);

                        // Create Pack node archetype
                        var node = (NodeArchetype)Archetypes.Packing.Nodes[6].Clone();
                        node.DefaultValues[0] = scriptTypeTypeName;
                        node.Flags &= ~NodeFlags.NoSpawnViaGUI;
                        node.Title = "Pack " + scriptTypeName;
                        node.Description = scriptTypeTypeName;
                        if (tooltipAttribute != null)
                            node.Description += "\n" + tooltipAttribute.Text;
                        ((IList<NodeArchetype>)group.Archetypes).Add(node);

                        // Create Unpack node archetype
                        node = (NodeArchetype)Archetypes.Packing.Nodes[13].Clone();
                        node.DefaultValues[0] = scriptTypeTypeName;
                        node.Flags &= ~NodeFlags.NoSpawnViaGUI;
                        node.Title = "Unpack " + scriptTypeName;
                        node.Description = scriptTypeTypeName;
                        if (tooltipAttribute != null)
                            node.Description += "\n" + tooltipAttribute.Text;
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
                                node.Description = SurfaceUtils.GetVisualScriptMemberInfoDescription(member);
                                node.SubTitle = string.Format(" (in {0})", scriptTypeName);
                                node.Tag = member;

                                // Create group archetype
                                var groupKey = new KeyValuePair<string, ushort>(scriptTypeName, 16);
                                if (!_cache.TryGetValue(groupKey, out var group))
                                {
                                    group = new GroupArchetype
                                    {
                                        GroupID = groupKey.Value,
                                        Name = groupKey.Key,
                                        Color = new Color(109, 160, 24),
                                        Tag = _version,
                                        Archetypes = new List<NodeArchetype>(),
                                    };
                                    _cache.Add(groupKey, group);
                                }

                                // Add node to the group
                                ((IList<NodeArchetype>)group.Archetypes).Add(node);
#if DEBUG_INVOKE_METHODS_SEARCHING
                                Editor.LogWarning(scriptTypeTypeName + " -> " + member.GetSignature());
                                searchHitsCount++;
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
                                    node.Description = SurfaceUtils.GetVisualScriptMemberInfoDescription(member);
                                    node.SubTitle = string.Format(" (in {0})", scriptTypeName);

                                    // Create group archetype
                                    var groupKey = new KeyValuePair<string, ushort>(scriptTypeName, 16);
                                    if (!_cache.TryGetValue(groupKey, out var group))
                                    {
                                        group = new GroupArchetype
                                        {
                                            GroupID = groupKey.Value,
                                            Name = groupKey.Key,
                                            Color = new Color(109, 160, 24),
                                            Tag = _version,
                                            Archetypes = new List<NodeArchetype>(),
                                        };
                                        _cache.Add(groupKey, group);
                                    }

                                    // Add node to the group
                                    ((IList<NodeArchetype>)group.Archetypes).Add(node);
#if DEBUG_FIELDS_SEARCHING
                                    Editor.LogWarning(scriptTypeTypeName + " -> Get " + member.GetSignature());
                                    searchHitsCount++;
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
                                    node.Description = SurfaceUtils.GetVisualScriptMemberInfoDescription(member);
                                    node.SubTitle = string.Format(" (in {0})", scriptTypeName);

                                    // Create group archetype
                                    var groupKey = new KeyValuePair<string, ushort>(scriptTypeName, 16);
                                    if (!_cache.TryGetValue(groupKey, out var group))
                                    {
                                        group = new GroupArchetype
                                        {
                                            GroupID = groupKey.Value,
                                            Name = groupKey.Key,
                                            Color = new Color(109, 160, 24),
                                            Tag = _version,
                                            Archetypes = new List<NodeArchetype>(),
                                        };
                                        _cache.Add(groupKey, group);
                                    }

                                    // Add node to the group
                                    ((IList<NodeArchetype>)group.Archetypes).Add(node);
#if DEBUG_FIELDS_SEARCHING
                                    Editor.LogWarning(scriptTypeTypeName + " -> Set " + member.GetSignature());
                                    searchHitsCount++;
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
                                if (!_cache.TryGetValue(groupKey, out var group))
                                {
                                    group = new GroupArchetype
                                    {
                                        GroupID = groupKey.Value,
                                        Name = groupKey.Key,
                                        Color = new Color(109, 160, 24),
                                        Tag = _version,
                                        Archetypes = new List<NodeArchetype>(),
                                    };
                                    _cache.Add(groupKey, group);
                                }

                                // Add Bind event node
                                var bindNode = (NodeArchetype)Archetypes.Function.Nodes[8].Clone();
                                bindNode.DefaultValues[0] = scriptTypeTypeName;
                                bindNode.DefaultValues[1] = name;
                                bindNode.Flags &= ~NodeFlags.NoSpawnViaGUI;
                                bindNode.Title = "Bind " + name;
                                bindNode.Description = SurfaceUtils.GetVisualScriptMemberInfoDescription(member);
                                bindNode.SubTitle = string.Format(" (in {0})", scriptTypeName);
                                ((IList<NodeArchetype>)group.Archetypes).Add(bindNode);

                                // Add Unbind event node
                                var unbindNode = (NodeArchetype)Archetypes.Function.Nodes[9].Clone();
                                unbindNode.DefaultValues[0] = scriptTypeTypeName;
                                unbindNode.DefaultValues[1] = name;
                                unbindNode.Flags &= ~NodeFlags.NoSpawnViaGUI;
                                unbindNode.Title = "Unbind " + name;
                                unbindNode.Description = bindNode.Description;
                                unbindNode.SubTitle = bindNode.SubTitle;
                                ((IList<NodeArchetype>)group.Archetypes).Add(unbindNode);

#if DEBUG_EVENTS_SEARCHING
                                Editor.LogWarning(scriptTypeTypeName + " -> " + member.GetSignature());
                                searchHitsCount++;
#endif
                            }
                        }
                    }
                }

                // Add group to context menu (on a main thread)
                FlaxEngine.Scripting.InvokeOnUpdate(() =>
                {
#if DEBUG_SEARCH_TIME
                    var addStartTime = DateTime.Now;
#endif
                    lock (_locker)
                    {
                        _taskContextMenu.AddGroups(_cache.Values);
                        _taskContextMenu = null;
                    }
#if DEBUG_SEARCH_TIME
                    Editor.LogError($"Added items to VisjectCM in: {(DateTime.Now - addStartTime).TotalMilliseconds} ms");
#endif
                });

#if DEBUG_SEARCH_TIME
                Editor.LogError($"Collected {searchHitsCount} items in: {(DateTime.Now - searchStartTime).TotalMilliseconds} ms");
#endif
                Profiler.EndEvent();

                lock (_locker)
                {
                    _task = null;
                }
            }

            private static void OnCodeEditingTypesCleared()
            {
                Wait();

                lock (_locker)
                {
                    _cache.Clear();
                    _version++;
                }

                Editor.Instance.CodeEditing.TypesCleared -= OnCodeEditingTypesCleared;
            }
        }

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
        }

        private bool ValidateDragActor(ActorNode actor)
        {
            return true;
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
                            text = $"{local.Value ?? string.Empty} ({local.ValueTypeName})";
                            return true;
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
                    if (Editor.Internal_EvaluateVisualScriptLocal(Object.GetUnmanagedPtr(script), ref local))
                    {
                        text = $"{local.Value ?? string.Empty} ({local.ValueTypeName})";
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
        public override bool CanUseNodeType(NodeArchetype nodeArchetype)
        {
            return (nodeArchetype.Flags & NodeFlags.VisualScriptGraph) != 0 && base.CanUseNodeType(nodeArchetype);
        }

        /// <inheritdoc />
        protected override NodeArchetype GetParameterGetterNodeArchetype(out ushort groupId)
        {
            groupId = 6;
            return Archetypes.Parameters.Nodes[2];
        }

        /// <inheritdoc />
        protected override void OnShowPrimaryMenu(VisjectCM activeCM, Vector2 location, Box startBox)
        {
            Profiler.BeginEvent("Setup Visual Script Context Menu");

            // Update nodes for method overrides
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
                        var attributes = member.GetAttributes(true);
                        var tooltipAttribute = (TooltipAttribute)attributes.FirstOrDefault(x => x is TooltipAttribute);
                        if (tooltipAttribute != null)
                            node.Description = tooltipAttribute.Text;
                        node.DefaultValues[0] = name;
                        node.DefaultValues[1] = parameters.Length;
                        node.Title = "Override " + name;
                        nodes.Add(node);
                    }
                }

                activeCM.AddGroup(_methodOverridesGroupArchetype);
            }

            // Update nodes for invoke methods (async)
            NodesCache.Get(activeCM);

            Profiler.EndEvent();

            base.OnShowPrimaryMenu(activeCM, location, startBox);

            activeCM.VisibleChanged += OnActiveContextMenuVisibleChanged;
        }

        private void OnActiveContextMenuVisibleChanged(Control activeCM)
        {
            NodesCache.Wait();
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
        public override DragDropEffect OnDragDrop(ref Vector2 location, DragData data)
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
            NodesCache.Wait();

            base.OnDestroy();
        }
    }
}
