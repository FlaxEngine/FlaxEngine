// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;

namespace FlaxEditor.SceneGraph
{
    /// <summary>
    /// Manages the manual local scene graph made of actors not added to gameplay but used in editor. Handles creating, updating and removing scene graph nodes for the given root actor that holds the hierarchy.
    /// </summary>
    [HideInEditor]
    public class LocalSceneGraph
    {
        private ActorNode _main;

        /// <summary>
        /// The root node.
        /// </summary>
        public readonly RootNode Root;

        /// <summary>
        /// Gets or sets the main actor used to track the hierarchy.
        /// </summary>
        public Actor MainActor
        {
            get => _main?.Actor;
            set
            {
                if (value != _main?.Actor)
                {
                    if (_main != null)
                    {
                        _main = null;
                        UnregisterEvents();
                    }
                    Root.DisposeChildNodes();
                    Root.TreeNode.DisposeChildren();

                    if (value != null)
                    {
                        var node = SceneGraphFactory.FindNode(value.ID) ?? SceneGraphFactory.BuildActorNode(value);
                        var actorNode = node as ActorNode;
                        if (actorNode == null)
                            throw new Exception("Failed to setup a scene graph for the local actor.");

                        actorNode.TreeNode.UnlockChildrenRecursive();
                        actorNode.ParentNode = Root;

                        _main = actorNode;
                        RegisterEvents();
                    }
                }
            }
        }

        /// <summary>
        /// Gets the main scene graph node (owns <see cref="MainActor"/>).
        /// </summary>
        public ActorNode Main => _main;

        /// <summary>
        /// Initializes a new instance of the <see cref="LocalSceneGraph"/> class.
        /// </summary>
        /// <param name="root">The root node of the graph. Allows to override some logic for the scene graph section.</param>
        public LocalSceneGraph(RootNode root)
        {
            Root = root;
            Root.TreeNode.ChildrenIndent = 0;
            Root.TreeNode.Expand();
        }

        /// <summary>
        /// Releases all created scene graph nodes and unlinks the <see cref="MainActor"/>.
        /// </summary>
        public void Dispose()
        {
            MainActor = null;
            Root.Dispose();
        }

        private void RegisterEvents()
        {
            Level.ActorSpawned += OnActorSpawned;
        }

        private void UnregisterEvents()
        {
            Level.ActorSpawned -= OnActorSpawned;
        }

        private void OnActorSpawned(Actor actor)
        {
            // Skip actors from game
            if (actor.HasScene)
                return;

            // Check if it has parent
            var parentNode = SceneGraphFactory.FindNode(actor.Parent.ID) as ActorNode;
            if (parentNode == null)
                return;

            // Check if actor is not from the local graph (use main actor to verify it)
            var a = actor.Parent;
            var main = MainActor;
            while (a != null && a != main)
            {
                a = a.Parent;
            }
            if (a == null)
                return;

            // Add node
            var actorNode = SceneGraphFactory.BuildActorNode(actor);
            if (actorNode == null)
                throw new Exception("Failed to setup a scene graph for the local actor.");
            actorNode.TreeNode.UnlockChildrenRecursive();
            actorNode.ParentNode = parentNode;
        }
    }
}
