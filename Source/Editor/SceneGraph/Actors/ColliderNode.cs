#if USE_LARGE_WORLDS
using Real = System.Double;
#else
using Real = System.Single;
#endif

// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.SceneGraph
{
    /// <summary>
    /// Scene Graph node type used for the collider shapes.
    /// </summary>
    /// <seealso cref="ActorNode" />
    /// <seealso cref="Collider" />
    [HideInEditor]
    public class ColliderNode : ActorNode
    {
        /// <inheritdoc />
        public ColliderNode(Actor actor)
        : base(actor)
        {
        }

        /// <inheritdoc />
        public override bool AffectsNavigation => true;

        /// <inheritdoc />
        public override bool RayCastSelf(ref RayCastData ray, out Real distance, out Vector3 normal)
        {
            // Check if skip raycasts
            if ((ray.Flags & RayCastData.FlagTypes.SkipColliders) == RayCastData.FlagTypes.SkipColliders)
            {
                distance = 0;
                normal = Vector3.Up;
                return false;
            }

            return base.RayCastSelf(ref ray, out distance, out normal);
        }
    }
}
