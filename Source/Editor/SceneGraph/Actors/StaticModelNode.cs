// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Content;
using FlaxEditor.GUI.ContextMenu;
using FlaxEngine;

namespace FlaxEditor.SceneGraph.Actors
{
    /// <summary>
    /// Scene tree node for <see cref="StaticModel"/> actor type.
    /// </summary>
    /// <seealso cref="ActorNode" />
    [HideInEditor]
    public sealed class StaticModelNode : ActorNode
    {
        /// <inheritdoc />
        public StaticModelNode(Actor actor)
        : base(actor)
        {
        }

        /// <inheritdoc />
        public override void OnContextMenu(ContextMenu contextMenu)
        {
            base.OnContextMenu(contextMenu);

            contextMenu.AddButton("Add mesh collider", OnAddMeshCollider).Enabled = ((StaticModel)Actor).Model != null;
        }

        private void OnAddMeshCollider()
        {
            var model = ((StaticModel)Actor).Model;
            if (!model)
                return;
            Action<CollisionData> created = collisionData =>
            {
                var actor = new MeshCollider
                {
                    StaticFlags = Actor.StaticFlags,
                    Transform = Actor.Transform,
                    CollisionData = collisionData,
                };
                Editor.Instance.SceneEditing.Spawn(actor, Actor);
            };
            var collisionDataProxy = (CollisionDataProxy)Editor.Instance.ContentDatabase.GetProxy<CollisionData>();
            collisionDataProxy.CreateCollisionDataFromModel(model, created);
        }
    }
}
