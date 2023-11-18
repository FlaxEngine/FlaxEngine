// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.Content;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.ContextMenu.Utils;
using FlaxEditor.GUI.Drag;
using FlaxEditor.GUI.Tree;
using FlaxEditor.SceneGraph;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows.Assets
{
    public sealed partial class PrefabWindow
    {
        /// <summary>
        /// The custom implementation of the root node for the scene graph.
        /// </summary>
        public class CustomRootNode : RootNode
        {
            private readonly PrefabWindow _window;

            /// <summary>
            /// Initializes a new instance of the <see cref="CustomRootNode"/> class.
            /// </summary>
            /// <param name="window">The window.</param>
            public CustomRootNode(PrefabWindow window)
            {
                _window = window;
            }

            /// <inheritdoc />
            public override void Spawn(Actor actor, Actor parent)
            {
                _window.Spawn(actor, parent);
            }

            /// <inheritdoc />
            public override Undo Undo => _window.Undo;

            /// <inheritdoc />
            public override List<SceneGraphNode> Selection => _window.Selection;
        }

        /// <summary>
        /// The prefab hierarchy tree control.
        /// </summary>
        /// <seealso cref="Tree" />
        public class PrefabTree : Tree
        {
            /// <summary>
            /// Initializes a new instance of the <see cref="PrefabTree"/> class.
            /// </summary>
            public PrefabTree()
            : base(true)
            {
            }
        }

        private class SceneTreePanel : Panel
        {
            private PrefabWindow _window;
            private DragAssets _dragAssets;
            private DragActorType _dragActorType;
            private DragHandlers _dragHandlers;

            public SceneTreePanel(PrefabWindow window)
            : base(ScrollBars.None)
            {
                _window = window;
                Offsets = Margin.Zero;
                AnchorPreset = AnchorPresets.StretchAll;
            }

            private bool ValidateDragAsset(AssetItem assetItem)
            {
                return assetItem.OnEditorDrag(this);
            }

            private static bool ValidateDragActorType(ScriptType actorType)
            {
                return true;
            }

            /// <inheritdoc />
            public override DragDropEffect OnDragEnter(ref Float2 location, DragData data)
            {
                var result = base.OnDragEnter(ref location, data);
                if (result == DragDropEffect.None)
                {
                    if (_dragHandlers == null)
                        _dragHandlers = new DragHandlers();
                    if (_dragAssets == null)
                    {
                        _dragAssets = new DragAssets(ValidateDragAsset);
                        _dragHandlers.Add(_dragAssets);
                    }
                    if (_dragAssets.OnDragEnter(data))
                        return _dragAssets.Effect;
                    if (_dragActorType == null)
                    {
                        _dragActorType = new DragActorType(ValidateDragActorType);
                        _dragHandlers.Add(_dragActorType);
                    }
                    if (_dragActorType.OnDragEnter(data))
                        return _dragActorType.Effect;
                }
                return result;
            }

            /// <inheritdoc />
            public override DragDropEffect OnDragMove(ref Float2 location, DragData data)
            {
                var result = base.OnDragMove(ref location, data);
                if (result == DragDropEffect.None && _dragHandlers != null)
                    result = _dragHandlers.Effect;
                return result;
            }

            /// <inheritdoc />
            public override void OnDragLeave()
            {
                base.OnDragLeave();

                _dragHandlers?.OnDragLeave();
            }

            /// <inheritdoc />
            public override DragDropEffect OnDragDrop(ref Float2 location, DragData data)
            {
                var result = base.OnDragDrop(ref location, data);
                if (result == DragDropEffect.None)
                {
                    // Drag assets
                    if (_dragAssets != null && _dragAssets.HasValidDrag)
                    {
                        for (int i = 0; i < _dragAssets.Objects.Count; i++)
                        {
                            var item = _dragAssets.Objects[i];
                            var actor = item.OnEditorDrop(this);
                            actor.Name = item.ShortName;
                            _window.Spawn(actor);
                        }
                        result = DragDropEffect.Move;
                    }
                    // Drag actor type
                    else if (_dragActorType != null && _dragActorType.HasValidDrag)
                    {
                        for (int i = 0; i < _dragActorType.Objects.Count; i++)
                        {
                            var item = _dragActorType.Objects[i];
                            var actor = item.CreateInstance() as Actor;
                            if (actor == null)
                            {
                                Editor.LogWarning("Failed to spawn actor of type " + item.TypeName);
                                continue;
                            }
                            actor.Name = item.Name;
                            _window.Spawn(actor);
                        }
                        result = DragDropEffect.Move;
                    }

                    _dragHandlers.OnDragDrop(null);
                }
                return result;
            }

            public override void OnDestroy()
            {
                _window = null;
                _dragAssets = null;
                _dragActorType = null;
                _dragHandlers?.Clear();
                _dragHandlers = null;

                base.OnDestroy();
            }
        }

        /// <summary>
        /// Occurs when prefab hierarchy panel wants to show the context menu. Allows to add custom options. Applies to all prefab windows.
        /// </summary>
        public static event Action<ContextMenu> ContextMenuShow;

        /// <summary>
        /// Creates the context menu for the current objects selection.
        /// </summary>
        /// <returns>The context menu.</returns>
        private ContextMenu CreateContextMenu()
        {
            // Prepare

            bool hasSthSelected = Selection.Count > 0;
            bool isSingleActorSelected = Selection.Count == 1 && Selection[0] is ActorNode;
            bool isRootSelected = isSingleActorSelected && Selection[0] == Graph.Main;
            bool hasPrefabLink = isSingleActorSelected && (Selection[0] as ActorNode).HasPrefabLink;

            // Create popup

            var contextMenu = new ContextMenu
            {
                MinimumWidth = 120
            };

            // Basic editing options

            var b = contextMenu.AddButton("Rename", Rename);
            b.Enabled = isSingleActorSelected;

            b = contextMenu.AddButton("Duplicate", Duplicate);
            b.Enabled = hasSthSelected && !isRootSelected;

            b = contextMenu.AddButton("Delete", Delete);
            b.Enabled = hasSthSelected && !isRootSelected;

            contextMenu.AddSeparator();
            b = contextMenu.AddButton("Copy", Copy);
            b.Enabled = hasSthSelected;

            b.Enabled = hasSthSelected;
            contextMenu.AddButton("Paste", Paste);

            b = contextMenu.AddButton("Cut", Cut);
            b.Enabled = hasSthSelected && !isRootSelected;

            b = contextMenu.AddButton("Set Root", SetRoot);
            b.Enabled = isSingleActorSelected && !isRootSelected && hasPrefabLink && Editor.Internal_CanSetToRoot(FlaxEngine.Object.GetUnmanagedPtr(Asset), FlaxEngine.Object.GetUnmanagedPtr(((ActorNode)Selection[0]).Actor));

            // Prefab options

            contextMenu.AddSeparator();

            b = contextMenu.AddButton("Create Prefab", () => Editor.Prefabs.CreatePrefab(Selection, this));
            b.Enabled = isSingleActorSelected &&
                        (Selection[0] as ActorNode).CanCreatePrefab &&
                        Editor.Windows.ContentWin.CurrentViewFolder.CanHaveAssets;

            b = contextMenu.AddButton("Select Prefab", Editor.Prefabs.SelectPrefab);
            b.Enabled = hasPrefabLink;

            // Spawning actors options

            contextMenu.AddSeparator();

            // Go through each actor and add it to the context menu if it has the ActorContextMenu attribute
            foreach (var actorType in Editor.CodeEditing.Actors.Get())
            {
                if (actorType.IsAbstract)
                    continue;
                ActorContextMenuAttribute attribute = null;
                foreach (var e in actorType.GetAttributes(false))
                {
                    if (e is ActorContextMenuAttribute actorContextMenuAttribute)
                    {
                        attribute = actorContextMenuAttribute;
                        break;
                    }
                }
                if (attribute == null)
                    continue;

                var basePath = attribute.Path.Split('/');
                var contextMenuPath = new List<string>(new string[] { "New" });
                contextMenuPath.AddRange(basePath);

                // create new actor cm
                ContextMenuUtils.CreateChildByPath(contextMenu, contextMenuPath, () => SpawnRootChild(actorType.Type));

                // create actor as child cm
                if (isSingleActorSelected)
                {
                    var newChildSplitPath = new List<string>(new string[] { "New Child" });
                    newChildSplitPath.AddRange(basePath);
                    ContextMenuUtils.CreateChildByPath(contextMenu, newChildSplitPath, () => Spawn(actorType.Type));
                }
            }

            // Custom options
            var showCustomNodeOptions = Selection.Count == 1;
            if (!showCustomNodeOptions && Selection.Count != 0)
            {
                showCustomNodeOptions = true;
                for (int i = 1; i < Selection.Count; i++)
                {
                    if (Selection[0].GetType() != Selection[i].GetType())
                    {
                        showCustomNodeOptions = false;
                        break;
                    }
                }
            }
            if (showCustomNodeOptions)
            {
                Selection[0].OnContextMenu(contextMenu, this);
            }
            ContextMenuShow?.Invoke(contextMenu);

            return contextMenu;
        }

        /// <summary>
        /// Shows the context menu on a given location (in the given control coordinates).
        /// </summary>
        /// <param name="parent">The parent control.</param>
        /// <param name="location">The location (within a given control).</param>
        private void ShowContextMenu(Control parent, ref Float2 location)
        {
            var contextMenu = CreateContextMenu();

            contextMenu.Show(parent, location);
        }

        private void Rename()
        {
            var selection = Selection;
            if (selection.Count != 0 && selection[0] is ActorNode actor)
            {
                if (selection.Count != 0)
                    Select(actor);
                actor.TreeNode.StartRenaming(this, _treePanel);
            }
        }

        /// <summary>
        /// Spawns the specified actor to the prefab (adds actor to child of selected actor).
        /// </summary>
        /// <param name="actor"></param>
        public void Spawn(Actor actor)
        {
            Actor parentActor = null;
            // Link to parent
            if (Selection.Count > 0 && Selection[0] is ActorNode actorNode)
            {
                parentActor = actorNode.Actor;
                actorNode.TreeNode.Expand();
            }

            Spawn(actor, parentActor);
        }

        /// <summary>
        /// Spawns the specified actor to the prefab (adds actor to root).
        /// </summary>
        /// <param name="actor">The actor.</param>
        public void SpawnRootChild(Actor actor)
        {
            Actor parentActor = Graph.MainActor;
            // Spawn it
            Spawn(actor, parentActor);
        }

        /// <summary>
        /// Spawns the actor of the specified type to the prefab (adds actor to child of selected actor).
        /// </summary>
        /// <param name="type">The actor type.</param>
        public void Spawn(Type type)
        {
            // Create actor
            Actor actor = (Actor)FlaxEngine.Object.New(type);

            // Spawn it
            Spawn(actor);
        }

        /// <summary>
        /// Spawns the actor of the specified type to the prefab (adds actor to root).
        /// </summary>
        /// <param name="type">The actor type.</param>
        public void SpawnRootChild(Type type)
        {
            // Create actor
            Actor actor = (Actor)FlaxEngine.Object.New(type);

            // Spawn it
            SpawnRootChild(actor);
        }

        /// <summary>
        /// Spawns the specified actor.
        /// </summary>
        /// <param name="actor">The actor.</param>
        /// <param name="parent">The parent.</param>
        public void Spawn(Actor actor, Actor parent)
        {
            if (actor == null)
                throw new ArgumentNullException(nameof(actor));

            // Link it
            actor.Parent = parent ?? throw new ArgumentNullException(nameof(parent));

            // Peek spawned node
            var actorNode = SceneGraphFactory.FindNode(actor.ID) as ActorNode ?? SceneGraphFactory.BuildActorNode(actor);
            if (actorNode == null)
                throw new InvalidOperationException("Failed to create scene node for the spawned actor.");

            var parentNode = SceneGraphFactory.FindNode(parent.ID) as ActorNode;
            actorNode.ParentNode = parentNode ?? throw new InvalidOperationException("Missing scene graph node for the spawned parent actor.");

            // Call post spawn action (can possibly setup custom default values)
            actorNode.PostSpawn();

            // Create undo action
            var action = new CustomDeleteActorsAction(new List<SceneGraphNode>(1) { actorNode }, true);
            Undo.AddAction(action);

            OnSpawnActor(actor);
        }

        private void OnSpawnActor(Actor actor)
        {
            if (actor.Parent != null)
            {
                // Match the parent
                actor.Transform = actor.Parent.Transform;
                actor.StaticFlags = actor.Parent.StaticFlags;
                actor.Layer = actor.Parent.Layer;

                // Rename actor to identify it easily
                actor.Name = Utilities.Utils.IncrementNameNumber(actor.GetType().Name, x => actor.Parent.GetChild(x) == null);
            }
        }

        private void OnTreeRightClick(TreeNode node, Float2 location)
        {
            // Skip if it's loading data
            if (Graph.Main == null)
                return;

            ShowContextMenu(node, ref location);
        }

        private void Update(ActorNode actorNode)
        {
            if (actorNode.Actor)
            {
                actorNode.TreeNode.UpdateText();
                actorNode.TreeNode.OnOrderInParentChanged();
            }

            for (int i = 0; i < actorNode.ChildNodes.Count; i++)
            {
                if (actorNode.ChildNodes[i] is ActorNode child)
                    Update(child);
            }
        }
    }
}
