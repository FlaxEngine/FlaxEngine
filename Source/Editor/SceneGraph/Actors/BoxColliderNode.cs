// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#if USE_LARGE_WORLDS
using Real = System.Double;
#else
using Real = System.Single;
#endif

using FlaxEngine;

namespace FlaxEditor.SceneGraph.Actors
{
    /// <summary>
    /// Scene tree node for <see cref="BoxCollider"/> actor type.
    /// </summary>
    /// <seealso cref="ColliderNode" />
    [HideInEditor]
    public sealed class BoxColliderNode : ColliderNode
    {
        /// <inheritdoc />
        public BoxColliderNode(Actor actor)
        : base(actor)
        {
        }

        /// <inheritdoc />
        public override bool RayCastSelf(ref RayCastData ray, out Real distance, out Vector3 normal)
        {
            // Pick wires
            var actor = (BoxCollider)_actor;
            var box = actor.OrientedBox;
            if (Utilities.Utils.RayCastWire(ref box, ref ray.Ray, out distance, ref ray.View.Position))
            {
                normal = Vector3.Up;
                return true;
            }

            return base.RayCastSelf(ref ray, out distance, out normal);
        }
    }
}
