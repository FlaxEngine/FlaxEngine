// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Tree;
using FlaxEditor.SceneGraph;
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

            var contextMenu = new ContextMenu();
            contextMenu.MinimumWidth = 120;

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
            b.Enabled = isSingleActorSelected && !isRootSelected && hasPrefabLink;

            // Prefab options

            contextMenu.AddSeparator();

            b = contextMenu.AddButton("Create Prefab", Editor.Prefabs.CreatePrefab);
            b.Enabled = isSingleActorSelected &&
                        (Selection[0] as ActorNode).CanCreatePrefab &&
                        Editor.Windows.ContentWin.CurrentViewFolder.CanHaveAssets;

            b = contextMenu.AddButton("Select Prefab", Editor.Prefabs.SelectPrefab);
            b.Enabled = hasPrefabLink;

            // Spawning actors options

            contextMenu.AddSeparator();
            var spawnMenu = contextMenu.AddChildMenu("New");
            var newActorCm = spawnMenu.ContextMenu;
            for (int i = 0; i < SceneTreeWindow.SpawnActorsGroups.Length; i++)
            {
                var group = SceneTreeWindow.SpawnActorsGroups[i];

                if (group.Types.Length == 1)
                {
                    var type = group.Types[0].Value;
                    newActorCm.AddButton(group.Types[0].Key, () => Spawn(type));
                }
                else
                {
                    var groupCm = newActorCm.AddChildMenu(group.Name).ContextMenu;
                    for (int j = 0; j < group.Types.Length; j++)
                    {
                        var type = group.Types[j].Value;
                        groupCm.AddButton(group.Types[j].Key, () => Spawn(type));
                    }
                }
            }

            // Custom options

            ContextMenuShow?.Invoke(contextMenu);

            return contextMenu;
        }

        /// <summary>
        /// Shows the context menu on a given location (in the given control coordinates).
        /// </summary>
        /// <param name="parent">The parent control.</param>
        /// <param name="location">The location (within a given control).</param>
        private void ShowContextMenu(Control parent, ref Vector2 location)
        {
            var contextMenu = CreateContextMenu();

            contextMenu.Show(parent, location);
        }

        private void Rename()
        {
            ((ActorNode)Selection[0]).TreeNode.StartRenaming();
        }

        /// <summary>
        /// Spawns the specified actor to the prefab (adds actor to root).
        /// </summary>
        /// <param name="actor">The actor.</param>
        public void Spawn(Actor actor)
        {
            // Link to parent
            Actor parentActor = null;
            if (Selection.Count > 0 && Selection[0] is ActorNode actorNode)
            {
                parentActor = actorNode.Actor;
                actorNode.TreeNode.Expand();
            }
            if (parentActor == null)
            {
                parentActor = Graph.MainActor;
            }
            if (parentActor != null)
            {
                // Use the same location
                actor.Transform = parentActor.Transform;

                // Rename actor to identify it easily
                actor.Name = StringUtils.IncrementNameNumber(actor.GetType().Name, x => parentActor.GetChild(x) == null);
            }

            // Spawn it
            Spawn(actor, parentActor);
        }

        /// <summary>
        /// Spawns the actor of the specified type to the prefab (adds actor to root).
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
        /// Spawns the specified actor.
        /// </summary>
        /// <param name="actor">The actor.</param>
        /// <param name="parent">The parent.</param>
        public void Spawn(Actor actor, Actor parent)
        {
            if (actor == null)
                throw new ArgumentNullException(nameof(actor));
            if (parent == null)
                throw new ArgumentNullException(nameof(parent));

            // Link it
            actor.Parent = parent;

            // Peek spawned node
            var actorNode = SceneGraphFactory.FindNode(actor.ID) as ActorNode ?? SceneGraphFactory.BuildActorNode(actor);
            if (actorNode == null)
                throw new InvalidOperationException("Failed to create scene node for the spawned actor.");
            var parentNode = SceneGraphFactory.FindNode(parent.ID) as ActorNode;
            if (parentNode == null)
                throw new InvalidOperationException("Missing scene graph node for the spawned parent actor.");
            actorNode.ParentNode = parentNode;

            // Call post spawn action (can possibly setup custom default values)
            actorNode.PostSpawn();

            // Create undo action
            var action = new CustomDeleteActorsAction(new List<SceneGraphNode>(1) { actorNode }, true);
            Undo.AddAction(action);
        }

        private void OnTreeRightClick(TreeNode node, Vector2 location)
        {
            // Skip if it's loading data
            if (Graph.Main == null)
                return;

            ShowContextMenu(node, ref location);
        }

        private void Update(ActorNode actorNode)
        {
            actorNode.TreeNode.OnNameChanged();
            actorNode.TreeNode.OnOrderInParentChanged();

            for (int i = 0; i < actorNode.ChildNodes.Count; i++)
            {
                if (actorNode.ChildNodes[i] is ActorNode child)
                    Update(child);
            }
        }
    }
}
