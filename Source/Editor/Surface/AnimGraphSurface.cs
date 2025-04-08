// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.Content;
using FlaxEditor.Scripting;
using FlaxEditor.Surface.ContextMenu;
using FlaxEditor.Surface.Elements;
using FlaxEngine;
using FlaxEngine.GUI;
using Animation = FlaxEditor.Surface.Archetypes.Animation;

namespace FlaxEditor.Surface
{
    /// <summary>
    /// The Visject Surface implementation for the animation graph editor.
    /// </summary>
    /// <seealso cref="FlaxEditor.Surface.VisjectSurface" />
    [HideInEditor]
    public class AnimGraphSurface : VisjectSurface
    {
        private static readonly List<GroupArchetype> StateMachineGroupArchetypes = new List<GroupArchetype>(new[]
        {
            // Customized Animations group with special nodes to use here
            new GroupArchetype
            {
                GroupID = 9,
                Name = "State Machine",
                Color = new Color(105, 179, 160),
                Archetypes = new[]
                {
                    new NodeArchetype
                    {
                        TypeID = 20,
                        Create = (id, context, arch, groupArch) => new Animation.StateMachineState(id, context, arch, groupArch),
                        Title = "State",
                        Description = "The animation states machine state node",
                        Flags = NodeFlags.AnimGraph,
                        DefaultValues = new object[]
                        {
                            "State",
                            Utils.GetEmptyArray<byte>(),
                            Utils.GetEmptyArray<byte>(),
                        },
                        Size = new Float2(100, 0),
                    },
                    new NodeArchetype
                    {
                        TypeID = 34,
                        Create = (id, context, arch, groupArch) => new Animation.StateMachineAny(id, context, arch, groupArch),
                        Title = "Any",
                        Description = "The generic animation states machine state with source transitions from any other state",
                        Flags = NodeFlags.AnimGraph,
                        Size = new Float2(100, 0),
                        DefaultValues = new object[]
                        {
                            Utils.GetEmptyArray<byte>(),
                        },
                    },
                }
            }
        });

        private static readonly GroupArchetype StateMachineTransitionGroupArchetype = new GroupArchetype
        {
            GroupID = 9,
            Name = "Transition",
            Color = new Color(105, 179, 160),
            Archetypes = new[]
            {
                new NodeArchetype
                {
                    TypeID = 23,
                    Title = "Transition Source State Anim",
                    Description = "The animation state machine transition source state animation data information",
                    Flags = NodeFlags.AnimGraph,
                    Size = new Float2(270, 110),
                    Elements = new[]
                    {
                        NodeElementArchetype.Factory.Output(0, "Length", typeof(float), 0),
                        NodeElementArchetype.Factory.Output(1, "Time", typeof(float), 1),
                        NodeElementArchetype.Factory.Output(2, "Normalized Time", typeof(float), 2),
                        NodeElementArchetype.Factory.Output(3, "Remaining Time", typeof(float), 3),
                        NodeElementArchetype.Factory.Output(4, "Remaining Normalized Time", typeof(float), 4),
                    }
                },
            }
        };

        private static NodesCache _nodesCache = new NodesCache(IterateNodesCache);

        /// <inheritdoc />
        public override bool UseContextMenuDescriptionPanel => true;

        /// <summary>
        /// The state machine editing context menu.
        /// </summary>
        protected VisjectCM _cmStateMachineMenu;

        /// <summary>
        /// The state machine transition editing context menu.
        /// </summary>
        protected VisjectCM _cmStateMachineTransitionMenu;

        /// <inheritdoc />
        public AnimGraphSurface(IVisjectSurfaceOwner owner, Action onSave, FlaxEditor.Undo undo)
        : base(owner, onSave, undo, CreateStyle())
        {
            // Find custom nodes for Anim Graph
            var customNodes = Editor.Instance.CodeEditing.AnimGraphNodes.GetArchetypes();
            if (customNodes != null && customNodes.Count > 0)
            {
                AddCustomNodes(customNodes);
            }

            ScriptsBuilder.ScriptsReloadBegin += OnScriptsReloadBegin;
        }

        internal AnimGraphTraceEvent[] LastTraceEvents;

        internal unsafe bool TryGetTraceEvent(SurfaceNode node, out AnimGraphTraceEvent traceEvent)
        {
            if (LastTraceEvents != null)
            {
                foreach (var e in LastTraceEvents)
                {
                    // Node IDs must match
                    if (e.NodeId == node.ID)
                    {
                        uint* nodePath = e.NodePath0;

                        // Get size of the path
                        int nodePathSize = 0;
                        while (nodePathSize < 8 && nodePath[nodePathSize] != 0)
                            nodePathSize++;

                        // Follow input node contexts path to verify if it matches with the path in the event
                        var c = node.Context;
                        for (int i = nodePathSize - 1; i >= 0 && c != null; i--)
                            c = c.OwnerNodeID == nodePath[i] ? c.Parent : null;
                        if (c != null)
                        {
                            traceEvent = e;
                            return true;
                        }
                    }
                }
            }
            traceEvent = default;
            return false;
        }

        private static SurfaceStyle CreateStyle()
        {
            var editor = Editor.Instance;
            var style = SurfaceStyle.CreateStyleHandler(editor);
            style.Icons.ArrowOpen = editor.Icons.Bone32;
            style.Icons.ArrowClose = editor.Icons.BoneFull32;
            return style;
        }

        private void OnScriptsReloadBegin()
        {
            _nodesCache.Clear();

            // Check if any of the nodes comes from the game scripts - those can be reloaded at runtime so prevent crashes
            bool hasTypeFromGameScripts = Editor.Instance.CodeEditing.AnimGraphNodes.HasTypeFromGameScripts;

            // Check any surface parameter comes from Game scripts module to handle scripts reloads in Editor
            if (!hasTypeFromGameScripts && RootContext != null)
            {
                foreach (var param in Parameters)
                {
                    if (FlaxEngine.Scripting.IsTypeFromGameScripts(param.Type.Type))
                    {
                        hasTypeFromGameScripts = true;
                        break;
                    }
                }
            }

            if (!hasTypeFromGameScripts)
                return;

            Owner.OnSurfaceClose();
            // TODO: make reload soft: dispose default primary context menu, update existing custom nodes to new ones or remove if invalid
        }

        /// <inheritdoc />
        protected override void OnContextChanged()
        {
            base.OnContextChanged();

            VisjectCM menu = null;

            // Override surface primary context menu for state machine editing
            if (Context?.Context is Animation.StateMachine)
            {
                if (_cmStateMachineMenu == null)
                {
                    _cmStateMachineMenu = new VisjectCM(new VisjectCM.InitInfo
                    {
                        Groups = StateMachineGroupArchetypes,
                        CanSpawnNode = (_, _) => true,
                    });
                    _cmStateMachineMenu.ShowExpanded = true;
                }
                menu = _cmStateMachineMenu;
            }

            // Override surface primary context menu for state machine transition editing
            if (Context?.Context is Animation.StateMachineTransition)
            {
                if (_cmStateMachineTransitionMenu == null)
                {
                    _cmStateMachineTransitionMenu = new VisjectCM(new VisjectCM.InitInfo
                    {
                        Groups = NodeFactory.DefaultGroups,
                        CanSpawnNode = CanUseNodeType,
                        ParametersGetter = null,
                        CustomNodesGroup = GetCustomNodes(),
                    });
                    _cmStateMachineTransitionMenu.AddGroup(StateMachineTransitionGroupArchetype, false);
                }
                menu = _cmStateMachineTransitionMenu;
            }

            SetPrimaryMenu(menu);
        }

        /// <inheritdoc />
        protected override void OnShowPrimaryMenu(VisjectCM activeCM, Float2 location, Box startBox)
        {
            // Check if show additional nodes in the current surface context
            if (activeCM != _cmStateMachineMenu)
            {
                _nodesCache.Get(activeCM);

                base.OnShowPrimaryMenu(activeCM, location, startBox);

                activeCM.VisibleChanged += OnActiveContextMenuVisibleChanged;
            }
            else
            {
                base.OnShowPrimaryMenu(activeCM, location, startBox);
            }
        }

        private void OnActiveContextMenuVisibleChanged(Control activeCM)
        {
            _nodesCache.Wait();
        }

        private static void IterateNodesCache(ScriptType scriptType, Dictionary<KeyValuePair<string, ushort>, GroupArchetype> cache, int version)
        {
            // Skip Newtonsoft.Json stuff
            var scriptTypeTypeName = scriptType.TypeName;
            if (scriptTypeTypeName.StartsWith("Newtonsoft.Json."))
                return;
            var scriptTypeName = scriptType.Name;

            // Enum
            if (scriptType.IsEnum)
            {
                // Create node archetype
                var node = (NodeArchetype)Archetypes.Constants.Nodes[10].Clone();
                node.DefaultValues[0] = Activator.CreateInstance(scriptType.Type);
                node.Flags &= ~NodeFlags.NoSpawnViaGUI;
                node.Title = scriptTypeName;
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
                node.Description = tooltip;
                ((IList<NodeArchetype>)group.Archetypes).Add(node);

                // Create Unpack node archetype
                node = (NodeArchetype)Archetypes.Packing.Nodes[13].Clone();
                node.DefaultValues[0] = scriptTypeTypeName;
                node.Flags &= ~NodeFlags.NoSpawnViaGUI;
                node.Title = "Unpack " + scriptTypeName;
                node.Description = tooltip;
                ((IList<NodeArchetype>)group.Archetypes).Add(node);
            }
        }

        /// <inheritdoc />
        public override string GetTypeName(ScriptType type)
        {
            if (type.Type == typeof(void))
                return "Skeleton Pose (local space)";
            return base.GetTypeName(type);
        }

        /// <inheritdoc />
        public override bool CanUseNodeType(GroupArchetype groupArchetype, NodeArchetype nodeArchetype)
        {
            return (nodeArchetype.Flags & NodeFlags.AnimGraph) != 0 && base.CanUseNodeType(groupArchetype, nodeArchetype);
        }

        /// <inheritdoc />
        protected override bool ValidateDragItem(AssetItem assetItem)
        {
            if (assetItem.IsOfType<SkeletonMask>())
                return true;
            if (assetItem.IsOfType<FlaxEngine.Animation>())
                return true;
            if (assetItem.IsOfType<AnimationGraphFunction>())
                return true;
            if (assetItem.IsOfType<GameplayGlobals>())
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

                if (assetItem.IsOfType<FlaxEngine.Animation>())
                {
                    node = Context.SpawnNode(9, 2, args.SurfaceLocation, new object[]
                    {
                        assetItem.ID,
                        1.0f,
                        true,
                        0.0f,
                    });
                }
                else if (assetItem.IsOfType<SkeletonMask>())
                {
                    node = Context.SpawnNode(9, 11, args.SurfaceLocation, new object[]
                    {
                        0.0f,
                        assetItem.ID,
                    });
                }
                else if (assetItem.IsOfType<AnimationGraphFunction>())
                {
                    node = Context.SpawnNode(9, 24, args.SurfaceLocation, new object[] { assetItem.ID, });
                }
                else if (assetItem.IsOfType<GameplayGlobals>())
                {
                    node = Context.SpawnNode(7, 16, args.SurfaceLocation, new object[]
                    {
                        assetItem.ID,
                        string.Empty,
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
        public override void OnDestroy()
        {
            if (IsDisposing)
                return;
            if (_cmStateMachineMenu != null)
            {
                _cmStateMachineMenu.Dispose();
                _cmStateMachineMenu = null;
            }
            if (_cmStateMachineTransitionMenu != null)
            {
                _cmStateMachineTransitionMenu.Dispose();
                _cmStateMachineTransitionMenu = null;
            }
            ScriptsBuilder.ScriptsReloadBegin -= OnScriptsReloadBegin;
            _nodesCache.Wait();
            LastTraceEvents = null;

            base.OnDestroy();
        }
    }
}
