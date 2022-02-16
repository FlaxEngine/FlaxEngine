// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
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
                        Size = new Vector2(100, 0),
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
                    Size = new Vector2(270, 110),
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
                Profiler.BeginEvent("Setup Anim Graph Context Menu (async)");

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
                        node.Description = Editor.Instance.CodeDocs.GetTooltip(scriptType);

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

                // Add group to context menu (on a main thread)
                FlaxEngine.Scripting.InvokeOnUpdate(() =>
                {
                    lock (_locker)
                    {
                        _taskContextMenu.AddGroups(_cache.Values);
                        _taskContextMenu = null;
                    }
                });

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

        /// <summary>
        /// The state machine editing context menu.
        /// </summary>
        protected VisjectCM _cmStateMachineMenu;

        /// <summary>
        /// The state machine transition editing context menu.
        /// </summary>
        protected VisjectCM _cmStateMachineTransitionMenu;

        private bool _isRegisteredForScriptsReload;

        /// <inheritdoc />
        public AnimGraphSurface(IVisjectSurfaceOwner owner, Action onSave, FlaxEditor.Undo undo)
        : base(owner, onSave, undo, CreateStyle())
        {
            // Find custom nodes for Anim Graph
            var customNodes = Editor.Instance.CodeEditing.AnimGraphNodes.GetArchetypes();
            if (customNodes != null && customNodes.Count > 0)
            {
                AddCustomNodes(customNodes);

                // Check if any of the nodes comes from the game scripts - those can be reloaded at runtime so prevent crashes
                if (Editor.Instance.CodeEditing.AnimGraphNodes.HasTypeFromGameScripts)
                {
                    _isRegisteredForScriptsReload = true;
                    ScriptsBuilder.ScriptsReloadBegin += OnScriptsReloadBegin;
                }
            }
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
                        CanSpawnNode = arch => true,
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
                    _cmStateMachineTransitionMenu.AddGroup(StateMachineTransitionGroupArchetype);
                }
                menu = _cmStateMachineTransitionMenu;
            }

            SetPrimaryMenu(menu);
        }

        /// <inheritdoc />
        protected override void OnShowPrimaryMenu(VisjectCM activeCM, Vector2 location, Box startBox)
        {
            // Check if show additional nodes in the current surface context
            if (activeCM != _cmStateMachineMenu)
            {
                Profiler.BeginEvent("Setup Anim Graph Context Menu");
                NodesCache.Get(activeCM);
                Profiler.EndEvent();

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
            NodesCache.Wait();
        }

        /// <inheritdoc />
        public override string GetTypeName(ScriptType type)
        {
            if (type.Type == typeof(void))
                return "Skeleton Pose (local space)";
            return base.GetTypeName(type);
        }

        /// <inheritdoc />
        public override bool CanUseNodeType(NodeArchetype nodeArchetype)
        {
            return (nodeArchetype.Flags & NodeFlags.AnimGraph) != 0 && base.CanUseNodeType(nodeArchetype);
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
            if (_isRegisteredForScriptsReload)
            {
                _isRegisteredForScriptsReload = false;
                ScriptsBuilder.ScriptsReloadBegin -= OnScriptsReloadBegin;
            }
            NodesCache.Wait();

            base.OnDestroy();
        }
    }
}
